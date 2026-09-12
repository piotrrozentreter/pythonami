#include "py68k_frame.h"
#include "py68k_runtime.h"
#include "py68k_string.h"

#include <stdio.h>

int main(void)
{
    Py68Runtime runtime;
    Py68Code code;
    Py68String *string;
    Py68Value arguments[2];
    Py68Value local;
    int passed = 1;

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    py68_code_initialize(&code);
    passed &= py68_string_new_copy(&runtime, "arg", 3, &string) == PY68_STATUS_OK;
    arguments[0] = py68_value_from_object(&string->base);
    arguments[1] = py68_value_int(42);
    passed &= py68_frame_push(&runtime, &code, NULL, 3, 2, arguments, 19) ==
              PY68_STATUS_OK;
    passed &= runtime.frame_count == 1;
    passed &= py68_frame_get_local(&runtime, 1, &local) == PY68_STATUS_OK;
    passed &= local.type == PY68_VALUE_INT && local.as.integer == 42;
    py68_value_release(&runtime, local);
    passed &= py68_frame_set_local_copy(&runtime, 2,
                                        py68_value_from_object(&string->base)) ==
              PY68_STATUS_OK;
    py68_frame_pop(&runtime);
    py68_object_release(&runtime, &string->base);
    passed &= runtime.frame_count == 0 && runtime.live_objects == NULL;
    runtime.recursion_limit = 3;
    passed &= py68_frame_push(&runtime, &code, NULL, 0, 0, NULL, 0) ==
              PY68_STATUS_OK;
    passed &= py68_frame_push(&runtime, &code, NULL, 0, 0, NULL, 0) ==
              PY68_STATUS_OK;
    passed &= py68_frame_push(&runtime, &code, NULL, 0, 0, NULL, 0) ==
              PY68_STATUS_OK;
    passed &= py68_frame_push(&runtime, &code, NULL, 0, 0, NULL, 0) ==
              PY68_STATUS_RUNTIME_ERROR;
    passed &= runtime.error.kind == PY68_ERROR_RECURSION;
    py68_frame_unwind(&runtime);
    py68_error_clear(&runtime.error);
    py68_runtime_shutdown(&runtime);
    py68_code_destroy(&runtime.allocator, &code);
    passed &= runtime.allocator.stats.current_bytes == 0;
    if (passed) { puts("PASS: frame tests"); return 0; }
    return 1;
}