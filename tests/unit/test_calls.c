/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_native.h"
#include "py68k_global.h"
#include "py68k_code.h"
#include "py68k_function.h"
#include "py68k_runtime.h"
#include "py68k_vm.h"

#include <stdio.h>

static Py68Status native_sum(struct Py68Runtime *runtime, Py68U16 count,
                             Py68Value *arguments, Py68Value *result)
{
    (void)runtime;
    if (count != 2 || arguments[0].type != PY68_VALUE_INT ||
        arguments[1].type != PY68_VALUE_INT)
        return PY68_STATUS_RUNTIME_ERROR;
    *result = py68_value_int(arguments[0].as.integer + arguments[1].as.integer);
    return PY68_STATUS_OK;
}

int main(void)
{
    Py68Runtime runtime;
    Py68NativeFunction *native;
    Py68Code code;
    Py68Code function_code;
    Py68Function *user_function;
    Py68U16 value_index;
    Py68U16 constant_index;
    Py68Value arguments[2];
    Py68Value result;
    int passed = 1;

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    passed &= py68_native_new(&runtime, "sum", 2, 2, native_sum, &native) ==
              PY68_STATUS_OK;
    py68_code_initialize(&code);
    code.source_data = (const Py68U8 *)"sum";
    code.source_length = 3;
    passed &= py68_code_add_name(&runtime.allocator, &code, 0, 3, &value_index) ==
              PY68_STATUS_OK;
    passed &= py68_global_set_copy(&runtime, (const Py68U8 *)"sum", 3,
                                   py68_value_from_object(&native->base)) ==
              PY68_STATUS_OK;
    py68_object_release(&runtime, &native->base);
    arguments[0] = py68_value_int(2);
    arguments[1] = py68_value_int(3);
    passed &= py68_native_call(native, &runtime, 2, arguments, &result) ==
              PY68_STATUS_OK;
    passed &= result.type == PY68_VALUE_INT && result.as.integer == 5;
    passed &= py68_code_emit_u8(&runtime.allocator, &code, OP_LOAD_GLOBAL) ==
              PY68_STATUS_OK;
    passed &= py68_code_emit_u16_be(&runtime.allocator, &code, value_index) ==
              PY68_STATUS_OK;
    {
        Py68Constant constant;
        constant.kind = PY68_CONSTANT_INTEGER;
        constant.flags = 0;
        constant.offset = 0;
        constant.length = 0;
        constant.integer = 2;
        passed &= py68_code_add_constant(&runtime.allocator, &code,
                                          constant, &constant_index) == PY68_STATUS_OK;
        passed &= py68_code_emit_u8(&runtime.allocator, &code, OP_LOAD_CONST) ==
                  PY68_STATUS_OK;
        passed &= py68_code_emit_u16_be(&runtime.allocator, &code,
                                        constant_index) == PY68_STATUS_OK;
        constant.integer = 3;
        passed &= py68_code_add_constant(&runtime.allocator, &code,
                                          constant, &constant_index) == PY68_STATUS_OK;
        passed &= py68_code_emit_u8(&runtime.allocator, &code, OP_LOAD_CONST) ==
                  PY68_STATUS_OK;
        passed &= py68_code_emit_u16_be(&runtime.allocator, &code,
                                        constant_index) == PY68_STATUS_OK;
    }
    passed &= py68_code_emit_u8(&runtime.allocator, &code, OP_CALL) ==
              PY68_STATUS_OK;
    passed &= py68_code_emit_u8(&runtime.allocator, &code, 2) == PY68_STATUS_OK;
    passed &= py68_code_emit_u8(&runtime.allocator, &code, OP_POP) ==
              PY68_STATUS_OK;
    passed &= py68_code_emit_u8(&runtime.allocator, &code, OP_HALT) ==
              PY68_STATUS_OK;
    passed &= py68_vm_execute(&runtime, &code) == PY68_STATUS_OK;
    passed &= runtime.value_stack_count == 0 && runtime.frame_count == 0;
    py68_code_destroy(&runtime.allocator, &code);
    py68_code_initialize(&function_code);
    {
        Py68Constant constant;
        constant.kind = PY68_CONSTANT_INTEGER;
        constant.flags = 0;
        constant.integer = 9;
        constant.offset = 0;
        constant.length = 0;
        passed &= py68_code_add_constant(&runtime.allocator, &function_code,
                                         constant, &constant_index) ==
                  PY68_STATUS_OK;
    }
    passed &= py68_code_emit_u8(&runtime.allocator, &function_code,
                                OP_LOAD_CONST) == PY68_STATUS_OK;
    passed &= py68_code_emit_u16_be(&runtime.allocator, &function_code,
                                    constant_index) == PY68_STATUS_OK;
    passed &= py68_code_emit_u8(&runtime.allocator, &function_code,
                                OP_RETURN_VALUE) == PY68_STATUS_OK;
    passed &= py68_function_new(&runtime, &function_code, 0, 0,
                                &user_function) == PY68_STATUS_OK;
    py68_code_initialize(&code);
    code.source_data = (const Py68U8 *)"myfun";
    code.source_length = 5;
    passed &= py68_code_add_name(&runtime.allocator, &code, 0, 5,
                                 &value_index) == PY68_STATUS_OK;
    passed &= py68_global_set_copy(&runtime, (const Py68U8 *)"myfun", 5,
                                   py68_value_from_object(&user_function->base)) ==
              PY68_STATUS_OK;
    py68_object_release(&runtime, &user_function->base);
    passed &= py68_code_emit_u8(&runtime.allocator, &code,
                                OP_LOAD_GLOBAL) == PY68_STATUS_OK;
    passed &= py68_code_emit_u16_be(&runtime.allocator, &code,
                                    value_index) == PY68_STATUS_OK;
    passed &= py68_code_emit_u8(&runtime.allocator, &code, OP_CALL) ==
              PY68_STATUS_OK;
    passed &= py68_code_emit_u8(&runtime.allocator, &code, 0) ==
              PY68_STATUS_OK;
    passed &= py68_code_emit_u8(&runtime.allocator, &code, OP_POP) ==
              PY68_STATUS_OK;
    passed &= py68_code_emit_u8(&runtime.allocator, &code, OP_HALT) ==
              PY68_STATUS_OK;
    passed &= py68_vm_execute(&runtime, &code) == PY68_STATUS_OK;
    passed &= runtime.value_stack_count == 0 && runtime.frame_count == 0;
    py68_code_destroy(&runtime.allocator, &code);
    py68_code_destroy(&runtime.allocator, &function_code);

    runtime.recursion_limit = 4;
    py68_code_initialize(&function_code);
    function_code.source_data = (const Py68U8 *)"loop";
    function_code.source_length = 4;
    passed &= py68_code_add_name(&runtime.allocator, &function_code, 0, 4,
                                 &value_index) == PY68_STATUS_OK;
    passed &= py68_code_emit_u8(&runtime.allocator, &function_code,
                                OP_LOAD_GLOBAL) == PY68_STATUS_OK;
    passed &= py68_code_emit_u16_be(&runtime.allocator, &function_code,
                                    value_index) == PY68_STATUS_OK;
    passed &= py68_code_emit_u8(&runtime.allocator, &function_code,
                                OP_CALL) == PY68_STATUS_OK;
    passed &= py68_code_emit_u8(&runtime.allocator, &function_code, 0) ==
              PY68_STATUS_OK;
    passed &= py68_code_emit_u8(&runtime.allocator, &function_code,
                                OP_RETURN_VALUE) == PY68_STATUS_OK;
    passed &= py68_function_new(&runtime, &function_code, 0, 0,
                                &user_function) == PY68_STATUS_OK;
    py68_code_initialize(&code);
    code.source_data = (const Py68U8 *)"loop";
    code.source_length = 4;
    passed &= py68_code_add_name(&runtime.allocator, &code, 0, 4,
                                 &value_index) == PY68_STATUS_OK;
    passed &= py68_global_set_copy(&runtime, (const Py68U8 *)"loop", 4,
                                   py68_value_from_object(&user_function->base)) ==
              PY68_STATUS_OK;
    py68_object_release(&runtime, &user_function->base);
    passed &= py68_code_emit_u8(&runtime.allocator, &code,
                                OP_LOAD_GLOBAL) == PY68_STATUS_OK;
    passed &= py68_code_emit_u16_be(&runtime.allocator, &code,
                                    value_index) == PY68_STATUS_OK;
    passed &= py68_code_emit_u8(&runtime.allocator, &code, OP_CALL) ==
              PY68_STATUS_OK;
    passed &= py68_code_emit_u8(&runtime.allocator, &code, 0) ==
              PY68_STATUS_OK;
    passed &= py68_code_emit_u8(&runtime.allocator, &code, OP_POP) ==
              PY68_STATUS_OK;
    passed &= py68_code_emit_u8(&runtime.allocator, &code, OP_HALT) ==
              PY68_STATUS_OK;
    passed &= py68_vm_execute(&runtime, &code) == PY68_STATUS_RUNTIME_ERROR;
    passed &= runtime.error.kind == PY68_ERROR_RECURSION;
    passed &= runtime.value_stack_count == 0 && runtime.frame_count == 0;
    py68_code_destroy(&runtime.allocator, &code);
    py68_code_destroy(&runtime.allocator, &function_code);

    py68_code_initialize(&function_code);
    passed &= py68_code_emit_u8(&runtime.allocator, &function_code,
                                OP_HALT) == PY68_STATUS_OK;
    passed &= py68_function_new(&runtime, &function_code, 0, 0,
                                &user_function) == PY68_STATUS_OK;
    py68_code_initialize(&code);
    code.source_data = (const Py68U8 *)"haltfun";
    code.source_length = 7;
    passed &= py68_code_add_name(&runtime.allocator, &code, 0, 7,
                                 &value_index) == PY68_STATUS_OK;
    passed &= py68_global_set_copy(&runtime, (const Py68U8 *)"haltfun", 7,
                                   py68_value_from_object(&user_function->base)) ==
              PY68_STATUS_OK;
    py68_object_release(&runtime, &user_function->base);
    passed &= py68_code_emit_u8(&runtime.allocator, &code,
                                OP_LOAD_GLOBAL) == PY68_STATUS_OK;
    passed &= py68_code_emit_u16_be(&runtime.allocator, &code,
                                    value_index) == PY68_STATUS_OK;
    passed &= py68_code_emit_u8(&runtime.allocator, &code, OP_CALL) ==
              PY68_STATUS_OK;
    passed &= py68_code_emit_u8(&runtime.allocator, &code, 0) ==
              PY68_STATUS_OK;
    passed &= py68_code_emit_u8(&runtime.allocator, &code, OP_POP) ==
              PY68_STATUS_OK;
    passed &= py68_code_emit_u8(&runtime.allocator, &code, OP_HALT) ==
              PY68_STATUS_OK;
    passed &= py68_vm_execute(&runtime, &code) == PY68_STATUS_OK;
    passed &= runtime.value_stack_count == 0 && runtime.frame_count == 0;
    py68_code_destroy(&runtime.allocator, &code);
    py68_code_destroy(&runtime.allocator, &function_code);
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    if (passed) { puts("PASS: native call tests"); return 0; }
    return 1;
}
