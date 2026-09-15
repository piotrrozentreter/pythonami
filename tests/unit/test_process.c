/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_builtin.h"
#include "py68k_runtime.h"
#include "py68k_string.h"

#include <stdio.h>

int main(void)
{
    Py68Runtime runtime;
    Py68String *command;
    Py68Value argument;
    Py68Value result;
    int passed = 1;

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    passed &= py68_string_new_copy(&runtime, "exit 7", 6, &command) ==
              PY68_STATUS_OK;
    argument = py68_value_from_object(&command->base);
    result = py68_value_none();
    passed &= py68_builtin_system(&runtime, 1, &argument, &result) ==
              PY68_STATUS_OK;
    passed &= result.type == PY68_VALUE_INT && result.as.integer == 7;
    py68_object_release(&runtime, &command->base);
    py68_error_clear(&runtime.error);

    argument = py68_value_int(7);
    passed &= py68_builtin_system(&runtime, 1, &argument, &result) !=
              PY68_STATUS_OK;
    passed &= runtime.error.kind == PY68_ERROR_TYPE;
    py68_error_clear(&runtime.error);

    passed &= py68_string_new_copy(&runtime, "exit 7\0x", 8, &command) ==
              PY68_STATUS_OK;
    argument = py68_value_from_object(&command->base);
    passed &= py68_builtin_system(&runtime, 1, &argument, &result) !=
              PY68_STATUS_OK;
    passed &= runtime.error.kind == PY68_ERROR_VALUE;
    py68_object_release(&runtime, &command->base);
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    if (passed) {
        puts("PASS: process tests");
        return 0;
    }
    return 1;
}