#include "py68k_runtime.h"
#include "py68k_vm.h"

#include <stdio.h>

static int add_constant(Py68Allocator *allocator, Py68Code *code,
                        Py68I32 integer)
{
    Py68Constant constant;
    Py68U16 index;
    constant.kind = PY68_CONSTANT_INTEGER;
    constant.flags = 0;
    constant.integer = integer;
    constant.offset = 0;
    constant.length = 0;
    return py68_code_add_constant(allocator, code, constant, &index) ==
           PY68_STATUS_OK;
}

static int emit(Py68Allocator *allocator, Py68Code *code, Py68U8 byte)
{
    return py68_code_emit_u8(allocator, code, byte) == PY68_STATUS_OK;
}

int main(void)
{
    Py68Runtime runtime;
    Py68Code code;
    int passed = 1;

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    py68_code_initialize(&code);
    passed &= add_constant(&runtime.allocator, &code, 7);
    passed &= add_constant(&runtime.allocator, &code, 5);
    passed &= emit(&runtime.allocator, &code, OP_LOAD_CONST);
    passed &= py68_code_emit_u16_be(&runtime.allocator, &code, 0) == PY68_STATUS_OK;
    passed &= emit(&runtime.allocator, &code, OP_LOAD_CONST);
    passed &= py68_code_emit_u16_be(&runtime.allocator, &code, 1) == PY68_STATUS_OK;
    passed &= emit(&runtime.allocator, &code, OP_ADD);
    passed &= emit(&runtime.allocator, &code, OP_POP);
    passed &= emit(&runtime.allocator, &code, OP_HALT);
    passed &= py68_vm_execute(&runtime, &code) == PY68_STATUS_OK;
    passed &= runtime.value_stack_count == 0 && runtime.frame_count == 0;
    py68_code_destroy(&runtime.allocator, &code);
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;

    py68_runtime_initialize(&runtime);
    py68_code_initialize(&code);
    passed &= add_constant(&runtime.allocator, &code, 1);
    passed &= add_constant(&runtime.allocator, &code, 0);
    passed &= emit(&runtime.allocator, &code, OP_LOAD_CONST);
    passed &= py68_code_emit_u16_be(&runtime.allocator, &code, 0) == PY68_STATUS_OK;
    passed &= emit(&runtime.allocator, &code, OP_LOAD_CONST);
    passed &= py68_code_emit_u16_be(&runtime.allocator, &code, 1) == PY68_STATUS_OK;
    passed &= emit(&runtime.allocator, &code, OP_FLOOR_DIVIDE);
    passed &= emit(&runtime.allocator, &code, OP_HALT);
    passed &= py68_vm_execute(&runtime, &code) == PY68_STATUS_RUNTIME_ERROR;
    passed &= runtime.value_stack_count == 0 && runtime.frame_count == 0;
    py68_code_destroy(&runtime.allocator, &code);
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;

    if (passed) { puts("PASS: VM tests"); return 0; }
    return 1;
}
