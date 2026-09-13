#include "py68k_code.h"
#include "py68k_intern.h"
#include "py68k_runtime.h"

#include <stddef.h>
#include <string.h>

void py68_code_initialize(Py68Code *code)
{
    memset(code, 0, sizeof(*code));
}

void py68_code_destroy(Py68Allocator *allocator, Py68Code *code)
{
    Py68U16 index;
    for (index = 0; index < code->constant_count; ++index) {
        if (code->constants[index].kind == PY68_CONSTANT_CODE &&
            code->constants[index].code != NULL) {
            py68_code_destroy(allocator, code->constants[index].code);
            py68_free(allocator, PY68_MEM_CODE,
                      code->constants[index].code, sizeof(Py68Code));
        }
    }
    py68_free(allocator, PY68_MEM_CODE, code->bytecode,
              code->bytecode_capacity);
    py68_free(allocator, PY68_MEM_CONSTANT, code->constants,
              (Py68U32)code->constant_capacity * sizeof(Py68Constant));
    py68_free(allocator, PY68_MEM_CODE, code->name_offsets,
              (Py68U32)code->name_capacity * sizeof(Py68U32));
    py68_free(allocator, PY68_MEM_CODE, code->name_lengths,
              (Py68U32)code->name_capacity * sizeof(Py68U16));
    py68_free(allocator, PY68_MEM_CODE, code->name_strings,
              (Py68U32)code->name_capacity * sizeof(Py68String *));
    py68_code_initialize(code);
}

static Py68Status py68_grow_bytes(Py68Allocator *allocator, Py68Code *code)
{
    Py68U32 capacity = code->bytecode_capacity == 0 ? 64 :
                       code->bytecode_capacity * 2;
    Py68U8 *replacement;
    if (capacity < code->bytecode_capacity) return PY68_STATUS_MEMORY_ERROR;
    replacement = (Py68U8 *)py68_realloc(allocator, PY68_MEM_CODE,
        code->bytecode, code->bytecode_capacity, capacity);
    if (replacement == NULL) return PY68_STATUS_MEMORY_ERROR;
    code->bytecode = replacement;
    code->bytecode_capacity = capacity;
    return PY68_STATUS_OK;
}

Py68Status py68_code_emit_u8(Py68Allocator *allocator, Py68Code *code,
                             Py68U8 value)
{
    Py68Status status;
    if (code->bytecode_length == code->bytecode_capacity) {
        status = py68_grow_bytes(allocator, code);
        if (status != PY68_STATUS_OK) return status;
    }
    code->bytecode[code->bytecode_length++] = value;
    return PY68_STATUS_OK;
}

Py68Status py68_code_emit_u16_be(Py68Allocator *allocator, Py68Code *code,
                                 Py68U16 value)
{
    Py68Status status = py68_code_emit_u8(allocator, code,
                                          (Py68U8)(value >> 8));
    if (status != PY68_STATUS_OK) return status;
    return py68_code_emit_u8(allocator, code, (Py68U8)value);
}

Py68Status py68_code_patch_i16_be(Py68Code *code, Py68U32 operand_offset,
                                  Py68I32 displacement)
{
    if (operand_offset + 1 >= code->bytecode_length || displacement < -32768L ||
        displacement > 32767L) return PY68_STATUS_SOURCE_ERROR;
    code->bytecode[operand_offset] = (Py68U8)((Py68U32)displacement >> 8);
    code->bytecode[operand_offset + 1] = (Py68U8)displacement;
    return PY68_STATUS_OK;
}

Py68Status py68_code_add_constant(Py68Allocator *allocator, Py68Code *code,
                                  Py68Constant constant, Py68U16 *index_out)
{
    Py68Constant *replacement;
    Py68U16 capacity;
    Py68U32 old_size;
    Py68U32 new_size;
    Py68U16 index;
    for (index = 0; index < code->constant_count; ++index) {
        if (memcmp(&code->constants[index], &constant,
                   sizeof(Py68Constant)) == 0) {
            *index_out = index;
            return PY68_STATUS_OK;
        }
    }
    if (code->constant_count == code->constant_capacity) {
        capacity = code->constant_capacity == 0 ? 8 :
                   (Py68U16)(code->constant_capacity * 2);
        old_size = (Py68U32)code->constant_capacity * sizeof(Py68Constant);
        new_size = (Py68U32)capacity * sizeof(Py68Constant);
        replacement = (Py68Constant *)py68_realloc(
            allocator, PY68_MEM_CONSTANT, code->constants, old_size, new_size);
        if (replacement == NULL) return PY68_STATUS_MEMORY_ERROR;
        code->constants = replacement;
        code->constant_capacity = capacity;
    }
    *index_out = code->constant_count;
    code->constants[code->constant_count++] = constant;
    return PY68_STATUS_OK;
}

Py68Status py68_code_add_code_move(Py68Allocator *allocator, Py68Code *code,
                                   Py68Code *nested, Py68U16 *index_out)
{
    Py68Constant constant;
    if (nested == NULL || index_out == NULL)
        return PY68_STATUS_INTERNAL_ERROR;
    memset(&constant, 0, sizeof(constant));
    constant.kind = PY68_CONSTANT_CODE;
    constant.code = nested;
    if (code->constant_count == 65535U)
        return PY68_STATUS_MEMORY_ERROR;
    return py68_code_add_constant(allocator, code, constant, index_out);
}

Py68Status py68_code_add_name(Py68Allocator *allocator, Py68Code *code,
                              Py68U32 offset, Py68U16 length,
                              Py68U16 *index_out)
{
    Py68U16 index;
    Py68U16 capacity;
    Py68U32 *offsets;
    Py68U16 *lengths;
    for (index = 0; index < code->name_count; ++index) {
        if (code->name_lengths[index] == length) {
            if (code->name_offsets[index] == offset) {
                *index_out = index;
                return PY68_STATUS_OK;
            }
            if (code->source_data != NULL &&
                memcmp(code->source_data + code->name_offsets[index],
                       code->source_data + offset, length) == 0) {
                *index_out = index;
                return PY68_STATUS_OK;
            }
        }
    }
    if (code->name_count == code->name_capacity) {
        capacity = code->name_capacity == 0 ? 8 :
                   (Py68U16)(code->name_capacity * 2);
        offsets = (Py68U32 *)py68_realloc(
            allocator, PY68_MEM_CODE, code->name_offsets,
            (Py68U32)code->name_capacity * sizeof(Py68U32),
            (Py68U32)capacity * sizeof(Py68U32));
        if (offsets == NULL) return PY68_STATUS_MEMORY_ERROR;
        lengths = (Py68U16 *)py68_realloc(
            allocator, PY68_MEM_CODE, code->name_lengths,
            (Py68U32)code->name_capacity * sizeof(Py68U16),
            (Py68U32)capacity * sizeof(Py68U16));
        if (lengths == NULL) return PY68_STATUS_MEMORY_ERROR;
        code->name_offsets = offsets;
        code->name_lengths = lengths;
        code->name_capacity = capacity;
    }
    *index_out = code->name_count;
    code->name_offsets[code->name_count] = offset;
    code->name_lengths[code->name_count++] = length;
    return PY68_STATUS_OK;
}

Py68Status py68_code_intern_names(struct Py68Runtime *runtime,
                                  Py68Code *code)
{
    Py68String **strings;
    Py68U16 index;
    Py68Status status;

    if (runtime == NULL || code == NULL) return PY68_STATUS_INTERNAL_ERROR;
    if (code->name_count == 0 || code->name_strings != NULL ||
        code->source_data == NULL)
        return PY68_STATUS_OK;
    strings = (Py68String **)py68_alloc(
        &runtime->allocator, PY68_MEM_CODE,
        (Py68U32)code->name_capacity * (Py68U32)sizeof(Py68String *));
    if (strings == NULL) return PY68_STATUS_MEMORY_ERROR;
    for (index = 0; index < code->name_count; ++index) {
        status = py68_intern_get_copy(
            runtime, &runtime->interned_names,
            (const char *)code->source_data + code->name_offsets[index],
            code->name_lengths[index], &strings[index]);
        if (status != PY68_STATUS_OK) {
            py68_free(&runtime->allocator, PY68_MEM_CODE, strings,
                      (Py68U32)code->name_capacity * sizeof(Py68String *));
            return status;
        }
        py68_object_release(runtime, &strings[index]->base);
    }
    code->name_strings = strings;
    return PY68_STATUS_OK;
}