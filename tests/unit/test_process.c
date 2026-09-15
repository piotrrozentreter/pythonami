/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_builtin.h"
#include "py68k_runtime.h"
#include "py68k_string.h"

#include <stdio.h>
#include <string.h>
#if !defined(_WIN32)
#include <sys/wait.h>
#endif

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
    /* D-0032 / compatibility: return native command status (POSIX wait
     * encoding on Unix hosts; plain exit code on Windows). */
    passed &= result.type == PY68_VALUE_INT;
#if defined(_WIN32)
    passed &= result.as.integer == 7;
#else
    passed &= WIFEXITED(result.as.integer) &&
              WEXITSTATUS(result.as.integer) == 7;
#endif
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
    py68_error_clear(&runtime.error);

    passed &= py68_string_new_copy(&runtime, "echo PY68K_POPEN_OK", 19,
                                   &command) == PY68_STATUS_OK;
    argument = py68_value_from_object(&command->base);
    result = py68_value_none();
    passed &= py68_builtin_popen(&runtime, 1, &argument, &result) ==
              PY68_STATUS_OK;
    passed &= result.type == PY68_VALUE_OBJECT &&
              result.as.object->type == PY68_OBJECT_STRING;
    if (result.type == PY68_VALUE_OBJECT) {
        Py68String *captured = (Py68String *)result.as.object;
        passed &= captured->length >= 15 &&
                  memcmp(captured->data, "PY68K_POPEN_OK", 14) == 0;
        py68_object_release(&runtime, result.as.object);
    }
    py68_object_release(&runtime, &command->base);

    argument = py68_value_int(7);
    passed &= py68_builtin_popen(&runtime, 1, &argument, &result) !=
              PY68_STATUS_OK;
    passed &= runtime.error.kind == PY68_ERROR_TYPE;
    py68_error_clear(&runtime.error);

    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    if (passed) {
        puts("PASS: process tests");
        return 0;
    }
    return 1;
}