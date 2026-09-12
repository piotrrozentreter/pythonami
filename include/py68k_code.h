#ifndef PY68K_CODE_H
#define PY68K_CODE_H

#include "py68k_memory.h"
#include "py68k_opcode.h"
#include "py68k_source.h"
#include "py68k_status.h"

typedef enum Py68ConstantKind {
    PY68_CONSTANT_NONE = 0,
    PY68_CONSTANT_BOOL,
    PY68_CONSTANT_INTEGER,
    PY68_CONSTANT_STRING
} Py68ConstantKind;

typedef struct Py68Constant {
    Py68U16 kind;
    Py68U16 flags;
    Py68I32 integer;
    Py68U32 offset;
    Py68U16 length;
} Py68Constant;

typedef struct Py68Code {
    const Py68U8 *source_data;
    Py68U32 source_length;
    Py68U8 *bytecode;
    Py68U32 bytecode_length;
    Py68U32 bytecode_capacity;
    Py68Constant *constants;
    Py68U16 constant_count;
    Py68U16 constant_capacity;
    Py68U32 *name_offsets;
    Py68U16 *name_lengths;
    Py68U16 name_count;
    Py68U16 name_capacity;
    Py68U16 maximum_stack;
} Py68Code;

void py68_code_initialize(Py68Code *code);
void py68_code_destroy(Py68Allocator *allocator, Py68Code *code);
Py68Status py68_code_emit_u8(Py68Allocator *allocator, Py68Code *code,
                             Py68U8 value);
Py68Status py68_code_emit_u16_be(Py68Allocator *allocator, Py68Code *code,
                                 Py68U16 value);
Py68Status py68_code_patch_i16_be(Py68Code *code, Py68U32 operand_offset,
                                  Py68I32 displacement);
Py68Status py68_code_add_constant(Py68Allocator *allocator, Py68Code *code,
                                  Py68Constant constant, Py68U16 *index_out);
Py68Status py68_code_add_name(Py68Allocator *allocator, Py68Code *code,
                              Py68U32 offset, Py68U16 length,
                              Py68U16 *index_out);

#endif