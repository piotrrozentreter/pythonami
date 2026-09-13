#include "py68k_verify.h"

#include <stddef.h>
#include <string.h>

static Py68U16 py68_read_u16(const Py68U8 *bytes, Py68U32 offset)
{
    return (Py68U16)(((Py68U16)bytes[offset] << 8) |
                     (Py68U16)bytes[offset + 1]);
}

static Py68I32 py68_read_i16(const Py68U8 *bytes, Py68U32 offset)
{
    return (Py68I32)(Py68I16)py68_read_u16(bytes, offset);
}

static void py68_verify_error(Py68Error *error, const char *message,
                              Py68U32 offset)
{
    Py68Location location;
    location.offset = offset;
    location.line = 0;
    location.column = 0;
    location.length = 1;
    py68_error_clear(error);
    py68_error_set(error, PY68_ERROR_BYTECODE, location, NULL, message);
}

static Py68I32 py68_effect(const Py68U8 *bytes, Py68U32 offset,
                           Py68U8 opcode)
{
    const Py68OpcodeInfo *info = py68_opcode_info(opcode);
    Py68U16 operand;
    if (opcode == OP_BUILD_LIST) {
        operand = py68_read_u16(bytes, offset + 1);
        return 1 - (Py68I32)operand;
    }
    if (opcode == OP_RANGE_INIT) {
        return 1 - (Py68I32)bytes[offset + 1];
    }
    if (opcode == OP_CALL) {
        return -(Py68I32)bytes[offset + 1];
    }
    return info->stack_effect;
}

static Py68Status py68_successor(Py68Code *code, Py68U8 *boundaries,
                                 Py68I16 *depths, Py68U32 *worklist,
                                 Py68U32 *work_count, Py68U32 offset,
                                 Py68I32 depth, Py68U32 target,
                                 Py68Error *error)
{
    if (target >= code->bytecode_length || boundaries[target] == 0) {
        py68_verify_error(error, "branch target is not an instruction boundary",
                          offset);
        return PY68_STATUS_SOURCE_ERROR;
    }
    if (depth > 32767L) {
        py68_verify_error(error, "value stack depth is too large", offset);
        return PY68_STATUS_SOURCE_ERROR;
    }
    if (depths[target] == -1) {
        depths[target] = (Py68I16)depth;
        worklist[(*work_count)++] = target;
    } else if (depths[target] != depth) {
        py68_verify_error(error, "inconsistent stack depth at control-flow join",
                          target);
        return PY68_STATUS_SOURCE_ERROR;
    }
    return PY68_STATUS_OK;
}

Py68Status py68_verify_code(Py68Code *code, Py68Error *error)
{
    Py68Allocator allocator;
    Py68U8 *boundaries;
    Py68I16 *depths;
    Py68U32 *worklist;
    Py68U32 offset;
    Py68U32 work_count = 0;
    Py68U32 current;
    Py68U32 next;
    Py68U32 target;
    Py68U8 opcode;
    const Py68OpcodeInfo *info;
    Py68I32 depth;
    Py68I32 effect;
    Py68I32 maximum = 0;
    Py68Status status = PY68_STATUS_OK;

    py68_allocator_initialize(&allocator);
    py68_error_clear(error);
    if (code->bytecode_length == 0) {
        py68_verify_error(error, "empty bytecode", 0);
        return PY68_STATUS_SOURCE_ERROR;
    }
    boundaries = (Py68U8 *)py68_alloc(&allocator, PY68_MEM_TEMP,
                                      code->bytecode_length);
    depths = (Py68I16 *)py68_alloc(&allocator, PY68_MEM_TEMP,
                                   code->bytecode_length * sizeof(Py68I16));
    worklist = (Py68U32 *)py68_alloc(&allocator, PY68_MEM_TEMP,
                                     code->bytecode_length * sizeof(Py68U32));
    if (boundaries == NULL || depths == NULL || worklist == NULL) {
        status = PY68_STATUS_MEMORY_ERROR;
        goto cleanup;
    }
    memset(boundaries, 0, code->bytecode_length);
    for (offset = 0; offset < code->bytecode_length; ++offset) {
        depths[offset] = -1;
    }
    offset = 0;
    while (offset < code->bytecode_length) {
        opcode = code->bytecode[offset];
        info = py68_opcode_info(opcode);
        if (info == NULL || info->width == 0 ||
            info->width > code->bytecode_length - offset) {
            py68_verify_error(error, "unknown or truncated opcode", offset);
            status = PY68_STATUS_SOURCE_ERROR;
            goto cleanup;
        }
        boundaries[offset] = 1;
        if (((opcode == OP_LOAD_CONST || opcode == OP_MAKE_FUNCTION) &&
             py68_read_u16(code->bytecode, offset + 1) >= code->constant_count) ||
            ((opcode == OP_LOAD_GLOBAL || opcode == OP_STORE_GLOBAL) &&
             py68_read_u16(code->bytecode, offset + 1) >= code->name_count) ||
            ((opcode == OP_LOAD_LOCAL || opcode == OP_STORE_LOCAL) &&
             py68_read_u16(code->bytecode, offset + 1) >= code->local_count)) {
            py68_verify_error(error, "bytecode table index is out of range", offset);
            status = PY68_STATUS_SOURCE_ERROR;
            goto cleanup;
        }
        if (opcode == OP_MAKE_FUNCTION) {
            Py68U16 constant_index = py68_read_u16(code->bytecode, offset + 1);
            if (code->constants[constant_index].kind != PY68_CONSTANT_CODE ||
                code->constants[constant_index].integer >= code->nested_count) {
                py68_verify_error(error, "function constant does not reference code",
                                  offset);
                status = PY68_STATUS_SOURCE_ERROR;
                goto cleanup;
            }
        }
        offset += info->width;
    }
    if (code->bytecode[code->bytecode_length - 1] != OP_HALT &&
        code->bytecode[code->bytecode_length - 1] != OP_RETURN_VALUE &&
        code->bytecode[code->bytecode_length - 1] != OP_RETURN_NONE) {
        py68_verify_error(error, "bytecode has no valid terminal instruction",
                          code->bytecode_length - 1);
        status = PY68_STATUS_SOURCE_ERROR;
        goto cleanup;
    }
    depths[0] = 0;
    worklist[work_count++] = 0;
    while (work_count != 0) {
        current = worklist[--work_count];
        opcode = code->bytecode[current];
        info = py68_opcode_info(opcode);
        depth = depths[current];
        effect = py68_effect(code->bytecode, current, opcode);
        if (depth + effect < 0) {
            py68_verify_error(error, "value stack underflow", current);
            status = PY68_STATUS_SOURCE_ERROR;
            goto cleanup;
        }
        if (depth + effect > maximum) maximum = depth + effect;
        next = current + info->width;
        if (opcode == OP_HALT || opcode == OP_RETURN_VALUE ||
            opcode == OP_RETURN_NONE) continue;
        if (opcode == OP_JUMP || opcode == OP_JUMP_IF_FALSE ||
            opcode == OP_JUMP_IF_TRUE || opcode == OP_JUMP_IF_FALSE_OR_POP ||
            opcode == OP_JUMP_IF_TRUE_OR_POP || opcode == OP_RANGE_NEXT) {
            Py68I32 target_depth = (opcode == OP_RANGE_NEXT) ? (depth - 1) : (depth + effect);
            target = (Py68U32)((Py68I32)(current + info->width) +
                              py68_read_i16(code->bytecode, current + 1));
            status = py68_successor(code, boundaries, depths, worklist,
                                    &work_count, current,
                                    target_depth, target, error);
            if (status != PY68_STATUS_OK) goto cleanup;
            if (opcode == OP_JUMP) continue;
        }
        if (next >= code->bytecode_length || boundaries[next] == 0) {
            py68_verify_error(error, "reachable fall-through beyond bytecode",
                              current);
            status = PY68_STATUS_SOURCE_ERROR;
            goto cleanup;
        }
        status = py68_successor(code, boundaries, depths, worklist,
                                &work_count, current, depth + effect,
                                next, error);
        if (status != PY68_STATUS_OK) goto cleanup;
    }
    if (maximum > 65535L) {
        py68_verify_error(error, "maximum stack exceeds code metadata", 0);
        status = PY68_STATUS_SOURCE_ERROR;
        goto cleanup;
    }
    code->maximum_stack = (Py68U16)maximum;
cleanup:
    py68_free(&allocator, PY68_MEM_TEMP, boundaries, code->bytecode_length);
    py68_free(&allocator, PY68_MEM_TEMP, depths,
              code->bytecode_length * sizeof(Py68I16));
    py68_free(&allocator, PY68_MEM_TEMP, worklist,
              code->bytecode_length * sizeof(Py68U32));
    return status;
}