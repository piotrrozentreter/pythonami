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
    PY68_CONSTANT_STRING,
    /* `integer` holds an index into the owning Py68Code's `nested` array. */
    PY68_CONSTANT_CODE
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
    /* Display name for tracebacks: offset/length into source_data.
       name_length == 0 means the module top-level ("<module>"). */
    Py68U32 name_offset;
    Py68U16 name_length;
    Py68U8 *bytecode;
    Py68U32 bytecode_length;
    Py68U32 bytecode_capacity;
    /* Parallel to bytecode: source line for each emitted byte. */
    Py68U16 *line_map;
    Py68U16 emit_line;
    Py68Constant *constants;
    Py68U16 constant_count;
    Py68U16 constant_capacity;
    Py68U32 *name_offsets;
    Py68U16 *name_lengths;
    Py68U16 name_count;
    Py68U16 name_capacity;
    Py68U16 maximum_stack;
    /* Function-body metadata: total addressable local slots (parameters
       plus other locals) and how many of those are parameters. Zero for
       module-level (top) code objects. */
    Py68U16 local_count;
    Py68U16 argument_count;
    /* Bodies of functions defined directly in this code object. Owned;
       destroyed recursively by py68_code_destroy. Nested defs are rejected
       by symbol analysis, so in practice this is only populated on the
       module's top-level code object. */
    struct Py68Code *nested;
    Py68U16 nested_count;
    Py68U16 nested_capacity;
} Py68Code;

void py68_code_initialize(Py68Code *code);
void py68_code_destroy(Py68Allocator *allocator, Py68Code *code);
Py68Status py68_code_reserve_nested(Py68Allocator *allocator, Py68Code *code,
                                    Py68Code **child_out,
                                    Py68U16 *index_out);
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