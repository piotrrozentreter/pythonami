/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_runtime.h"
#include "py68k_string.h"
#include "py68k_value.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    Py68Runtime runtime;
    Py68String *string;
    Py68Value value;
    int passed = 1;

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    passed &= py68_string_new_copy(&runtime, "hello", 5, &string) == PY68_STATUS_OK;
    passed &= string != NULL && string->length == 5 &&
              memcmp(string->data, "hello", 5) == 0;
    passed &= string->base.reference_count == 1;
    value = py68_value_from_object(&string->base);
    py68_value_retain(value);
    passed &= string->base.reference_count == 2;
    py68_value_release(&runtime, value);
    passed &= string->base.reference_count == 1;
    py68_object_release(&runtime, &string->base);
    passed &= runtime.live_objects == NULL;
    passed &= runtime.allocator.stats.current_bytes == 0;
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    if (passed) { puts("PASS: object and string tests"); return 0; }
    return 1;
}
