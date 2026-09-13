/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_builtin.h"
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
    Py68NativeFunction *native;
    Py68Value value;
    int passed = 1;

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    passed &= py68_native_new(&runtime, "builtin", 0, 2, callback, &native) ==
              PY68_STATUS_OK;
    passed &= py68_builtin_set_copy(&runtime, (const Py68U8 *)"widget", 6,
                                    py68_value_from_object(&native->base)) ==
              PY68_STATUS_OK;
    py68_object_release(&runtime, &native->base);
    passed &= py68_builtin_get_copy(&runtime, (const Py68U8 *)"widget", 6,
                                    &value) == PY68_STATUS_OK;
    passed &= value.type == PY68_VALUE_OBJECT;
    py68_value_release(&runtime, value);
    passed &= py68_builtin_get_copy(&runtime, (const Py68U8 *)"missing", 7,
                                    &value) == PY68_STATUS_SOURCE_ERROR;
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    if (passed) { puts("PASS: builtin registry tests"); return 0; }
    return 1;
}
