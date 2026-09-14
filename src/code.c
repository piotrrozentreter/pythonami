/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_code.h"

#include <stddef.h>
#include <string.h>

void py68_code_initialize(Py68Code *code)
{
    memset(code, 0, sizeof(*code));
}

void py68_code_destroy(Py68Allocator *allocator, Py68Code *code)
{
    Py68U16 index;
    for (index = 0; index < code->nested_count; ++index)
        py68_code_destroy(allocator, &code->nested[index]);
    py68_free(allocator, PY68_MEM_CODE, code->nested,
              (Py68U32)code->nested_capacity * sizeof(Py68Code));
    py68_free(allocator, PY68_MEM_CODE, code->bytecode,
              code->bytecode_capacity);
    py68_free(allocator, PY68_MEM_CODE, code->line_map,
              code->bytecode_capacity * sizeof(Py68U16));
    py68_free(allocator, PY68_MEM_CONSTANT, code->constants,
              (Py68U32)code->constant_capacity * sizeof(Py68Constant));
    py68_free(allocator, PY68_MEM_CODE, code->name_offsets,
              (Py68U32)code->name_capacity * sizeof(Py68U32));
    py68_free(allocator, PY68_MEM_CODE, code->name_lengths,
              (Py68U32)code->name_capacity * sizeof(Py68U16));
    py68_free(allocator, PY68_MEM_CODE, code->interned, code->interned_capacity);
    py68_code_initialize(code);
}

Py68Status py68_code_reserve_nested(Py68Allocator *allocator, Py68Code *code,
                                    Py68Code **child_out, Py68U16 *index_out)
{
    Py68Code *replacement;
    Py68U16 capacity;
    Py68U32 old_size;
    Py68U32 new_size;
    if (code->nested_count == code->nested_capacity) {
        capacity = code->nested_capacity == 0 ? 4 :
                   (Py68U16)(code->nested_capacity * 2);
        old_size = (Py68U32)code->nested_capacity * sizeof(Py68Code);
        new_size = (Py68U32)capacity * sizeof(Py68Code);
        replacement = (Py68Code *)py68_realloc(
            allocator, PY68_MEM_CODE, code->nested, old_size, new_size);
        if (replacement == NULL) return PY68_STATUS_MEMORY_ERROR;
        code->nested = replacement;
        code->nested_capacity = capacity;
    }
    *index_out = code->nested_count;
    py68_code_initialize(&code->nested[code->nested_count]);
    *child_out = &code->nested[code->nested_count];
    ++code->nested_count;
    return PY68_STATUS_OK;
}

static Py68Status py68_grow_bytes(Py68Allocator *allocator, Py68Code *code)
{
    Py68U32 capacity = code->bytecode_capacity == 0 ? 64 :
                       code->bytecode_capacity * 2;
    Py68U8 *replacement;
    Py68U16 *line_replacement;
    if (capacity < code->bytecode_capacity) return PY68_STATUS_MEMORY_ERROR;
    replacement = (Py68U8 *)py68_alloc(allocator, PY68_MEM_CODE, capacity);
    if (replacement == NULL) return PY68_STATUS_MEMORY_ERROR;
    line_replacement = (Py68U16 *)py68_alloc(
        allocator, PY68_MEM_CODE, capacity * sizeof(Py68U16));
    if (line_replacement == NULL) {
        py68_free(allocator, PY68_MEM_CODE, replacement, capacity);
        return PY68_STATUS_MEMORY_ERROR;
    }
    if (code->bytecode_length != 0) {
        memcpy(replacement, code->bytecode, code->bytecode_length);
        if (code->line_map != NULL)
            memcpy(line_replacement, code->line_map,
                   code->bytecode_length * sizeof(Py68U16));
    }
    py68_free(allocator, PY68_MEM_CODE, code->bytecode,
              code->bytecode_capacity);
    py68_free(allocator, PY68_MEM_CODE, code->line_map,
              code->bytecode_capacity * sizeof(Py68U16));
    code->bytecode = replacement;
    code->line_map = line_replacement;
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
    if (code->line_map != NULL)
        code->line_map[code->bytecode_length] = code->emit_line;
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

#define PY68_NAME_INTERNED ((Py68U32)0x80000000u)

const Py68U8 *py68_code_name_bytes(const Py68Code *code, Py68U16 index)
{
    Py68U32 offset;
    if (code == NULL || index >= code->name_count) return NULL;
    offset = code->name_offsets[index];
    if ((offset & PY68_NAME_INTERNED) != 0)
        return code->interned + (offset & ~PY68_NAME_INTERNED);
    if (code->source_data == NULL) return NULL;
    return code->source_data + offset;
}

Py68Status py68_code_add_name(Py68Allocator *allocator, Py68Code *code,
                              Py68U32 offset, Py68U16 length,
                              Py68U16 *index_out)
{
    Py68U16 index;
    Py68U16 capacity;
    Py68U32 *offsets;
    Py68U16 *lengths;
    const Py68U8 *incoming;
    incoming = ((offset & PY68_NAME_INTERNED) != 0)
                   ? code->interned + (offset & ~PY68_NAME_INTERNED)
                   : (code->source_data != NULL ? code->source_data + offset
                                                : NULL);
    for (index = 0; index < code->name_count; ++index) {
        if (code->name_lengths[index] == length) {
            if (code->name_offsets[index] == offset) {
                *index_out = index;
                return PY68_STATUS_OK;
            }
            if (incoming != NULL) {
                const Py68U8 *existing = py68_code_name_bytes(code, index);
                if (existing != NULL && memcmp(existing, incoming, length) == 0) {
                    *index_out = index;
                    return PY68_STATUS_OK;
                }
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

Py68Status py68_code_add_interned_name(Py68Allocator *allocator, Py68Code *code,
                                       const char *bytes, Py68U16 length,
                                       Py68U16 *index_out)
{
    Py68U8 *replacement;
    Py68U32 capacity;
    Py68U32 offset;
    if (bytes == NULL || length == 0) return PY68_STATUS_INTERNAL_ERROR;
    if (code->interned_length > (Py68U32)~(Py68U32)0 - length)
        return PY68_STATUS_MEMORY_ERROR;
    if (code->interned_length + length > code->interned_capacity) {
        capacity = code->interned_capacity == 0 ? 32 : code->interned_capacity * 2;
        while (capacity < code->interned_length + length) {
            if (capacity > ((Py68U32)~(Py68U32)0) / 2)
                return PY68_STATUS_MEMORY_ERROR;
            capacity *= 2;
        }
        replacement = (Py68U8 *)py68_realloc(
            allocator, PY68_MEM_CODE, code->interned, code->interned_capacity,
            capacity);
        if (replacement == NULL) return PY68_STATUS_MEMORY_ERROR;
        code->interned = replacement;
        code->interned_capacity = capacity;
    }
    offset = code->interned_length;
    memcpy(code->interned + offset, bytes, length);
    code->interned_length += length;
    return py68_code_add_name(allocator, code, PY68_NAME_INTERNED | offset,
                              length, index_out);
}