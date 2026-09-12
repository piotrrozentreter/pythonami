#include "py68k_list.h"
#include "py68k_runtime.h"

#include <stdio.h>

int main(void)
{
    Py68Runtime runtime;
    Py68List *outer;
    Py68List *inner;
    Py68Value value;
    Py68Value read;
    int passed = 1;

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    passed &= py68_list_new(&runtime, &outer) == PY68_STATUS_OK;
    passed &= py68_list_append_copy(&runtime, outer, py68_value_int(7)) == PY68_STATUS_OK;
    passed &= py68_list_get_copy(&runtime, outer, -1, &read) == PY68_STATUS_OK;
    passed &= read.type == PY68_VALUE_INT && read.as.integer == 7;
    py68_value_release(&runtime, read);
    value = py68_value_int(9);
    passed &= py68_list_set_move(&runtime, outer, 0, &value) == PY68_STATUS_OK;
    passed &= value.type == PY68_VALUE_NONE;
    passed &= py68_list_get_copy(&runtime, outer, 4, &read) == PY68_STATUS_SOURCE_ERROR;
    passed &= py68_list_new(&runtime, &inner) == PY68_STATUS_OK;
    value = py68_value_from_object(&outer->base);
    passed &= py68_list_append_copy(&runtime, inner, value) == PY68_STATUS_OK;
    passed &= py68_list_append_copy(&runtime, outer,
                                    py68_value_from_object(&inner->base)) ==
              PY68_STATUS_SOURCE_ERROR;
    py68_object_release(&runtime, &inner->base);
    py68_object_release(&runtime, &outer->base);
    passed &= runtime.live_objects == NULL;
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    if (passed) { puts("PASS: list tests"); return 0; }
    return 1;
}
