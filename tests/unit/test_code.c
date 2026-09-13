#include "py68k_code.h"
#include "py68k_opcode.h"

#include <stdio.h>

int main(void)
{
    Py68Allocator allocator;
    Py68Code code;
    Py68Constant constant;
    Py68U16 index;
    int passed = 1;

    py68_allocator_initialize(&allocator);
    py68_code_initialize(&code);
    passed &= py68_opcode_info(OP_LOAD_CONST) != NULL;
    passed &= py68_opcode_info(0x0E) == NULL;
    passed &= py68_code_emit_u8(&allocator, &code, OP_JUMP) == PY68_STATUS_OK;
    passed &= py68_code_emit_u16_be(&allocator, &code, 0) == PY68_STATUS_OK;
    passed &= py68_code_patch_i16_be(&code, 1, -7) == PY68_STATUS_OK;
    passed &= code.bytecode[1] == 0xFF && code.bytecode[2] == 0xF9;
    constant.kind = PY68_CONSTANT_INTEGER;
    constant.flags = 0;
    constant.integer = 42;
    constant.offset = 0;
    constant.length = 0;
    constant.code = NULL;
    passed &= py68_code_add_constant(&allocator, &code, constant, &index) ==
              PY68_STATUS_OK && index == 0;
    passed &= py68_code_add_constant(&allocator, &code, constant, &index) ==
              PY68_STATUS_OK && index == 0 && code.constant_count == 1;
    passed &= py68_code_add_name(&allocator, &code, 4, 3, &index) ==
              PY68_STATUS_OK && index == 0;
    passed &= py68_code_add_name(&allocator, &code, 4, 3, &index) ==
              PY68_STATUS_OK && index == 0 && code.name_count == 1;
    py68_code_destroy(&allocator, &code);
    passed &= allocator.stats.current_bytes == 0;
    if (passed) { puts("PASS: code and opcode tests"); return 0; }
    return 1;
}
