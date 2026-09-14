/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_function.h"
#include "py68k_native.h"
#include "py68k_runtime.h"

#include <stdio.h>

static Py68Status callback(struct Py68Runtime *runtime, Py68U16 count,
                           Py68Value *arguments, Py68Value *result)
{
    (void)runtime;
    (void)arguments;
    *result = py68_value_int((Py68I32)count);
    return PY68_STATUS_OK;
}

int main(void)
{
    Py68Runtime runtime;
    Py68Code code;
    Py68Function *function;
    Py68NativeFunction *native;
    Py68Value result;
    Py68Value arguments[2];
    int passed = 1;

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    py68_code_initialize(&code);
    passed &= py68_function_new(&runtime, &code, 2, 2, NULL, &function) ==
              PY68_STATUS_OK;
    passed &= py68_function_check_arguments(function, 2) == PY68_STATUS_OK;
    passed &= py68_function_check_arguments(function, 1) == PY68_STATUS_RUNTIME_ERROR;
    passed &= py68_native_new(&runtime, "count", 1, 2, callback, &native) ==
              PY68_STATUS_OK;
    arguments[0] = py68_value_int(1);
    arguments[1] = py68_value_int(2);
    passed &= py68_native_call(native, &runtime, 2, arguments, &result) ==
              PY68_STATUS_OK;
    passed &= result.type == PY68_VALUE_INT && result.as.integer == 2;
    passed &= py68_native_call(native, &runtime, 0, arguments, &result) ==
              PY68_STATUS_RUNTIME_ERROR;
    py68_object_release(&runtime, &native->base);
    py68_object_release(&runtime, &function->base);
    py68_code_destroy(&runtime.allocator, &code);
    passed &= runtime.live_objects == NULL;
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    if (passed) { puts("PASS: function/native tests"); return 0; }
    return 1;
}
