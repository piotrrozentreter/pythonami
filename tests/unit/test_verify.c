/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_code.h"
#include "py68k_verify.h"

#include <stdio.h>

static int emit(Py68Allocator *allocator, Py68Code *code, Py68U8 value)
{
    return py68_code_emit_u8(allocator, code, value) == PY68_STATUS_OK;
}

int main(void)
{
    Py68Allocator allocator;
    Py68Code code;
    Py68Error error;
    Py68Constant constant;
    Py68U16 index;
    int passed = 1;

    py68_allocator_initialize(&allocator);
    py68_code_initialize(&code);
    constant.kind = PY68_CONSTANT_INTEGER;
    constant.flags = 0;
    constant.integer = 1;
    constant.offset = 0;
    constant.length = 0;
    passed &= py68_code_add_constant(&allocator, &code, constant, &index) ==
              PY68_STATUS_OK;
    passed &= emit(&allocator, &code, OP_LOAD_CONST);
    passed &= py68_code_emit_u16_be(&allocator, &code, 0) == PY68_STATUS_OK;
    passed &= emit(&allocator, &code, OP_POP);
    passed &= emit(&allocator, &code, OP_HALT);
    passed &= py68_verify_code(&code, &error) == PY68_STATUS_OK;
    passed &= code.maximum_stack == 1;
    py68_code_destroy(&allocator, &code);

    py68_code_initialize(&code);
    passed &= emit(&allocator, &code, OP_LOAD_CONST);
    passed &= py68_code_emit_u16_be(&allocator, &code, 4) == PY68_STATUS_OK;
    passed &= emit(&allocator, &code, OP_HALT);
    passed &= py68_verify_code(&code, &error) == PY68_STATUS_SOURCE_ERROR;
    py68_code_destroy(&allocator, &code);

    py68_code_initialize(&code);
    passed &= emit(&allocator, &code, OP_POP);
    passed &= emit(&allocator, &code, OP_HALT);
    passed &= py68_verify_code(&code, &error) == PY68_STATUS_SOURCE_ERROR;
    py68_code_destroy(&allocator, &code);

    py68_code_initialize(&code);
    passed &= emit(&allocator, &code, OP_JUMP);
    passed &= py68_code_emit_u16_be(&allocator, &code, 1) == PY68_STATUS_OK;
    passed &= emit(&allocator, &code, OP_HALT);
    passed &= py68_verify_code(&code, &error) == PY68_STATUS_SOURCE_ERROR;
    py68_code_destroy(&allocator, &code);

    passed &= allocator.stats.current_bytes == 0;
    if (passed) { puts("PASS: verifier tests"); return 0; }
    return 1;
}
