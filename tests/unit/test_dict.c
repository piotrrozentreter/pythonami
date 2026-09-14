/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_dict.h"
#include "py68k_list.h"
#include "py68k_runtime.h"

#include <stdio.h>

int main(void)
{
    Py68Runtime runtime;
    Py68Dict *dict;
    Py68List *list;
    Py68Value value;
    int passed = 1;

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    passed &= py68_dict_new(&runtime, &dict) == PY68_STATUS_OK;
    passed &= py68_dict_set_copy(&runtime, dict, py68_value_int(1),
                                 py68_value_int(2)) == PY68_STATUS_OK;
    passed &= py68_dict_get_copy(&runtime, dict, py68_value_int(1),
                                 &value) == PY68_STATUS_OK;
    passed &= value.type == PY68_VALUE_INT && value.as.integer == 2;
    py68_value_release(&runtime, value);
    passed &= py68_dict_keys(&runtime, dict, &list) == PY68_STATUS_OK;
    passed &= list->count == 1;
    py68_object_release(&runtime, &list->base);
    passed &= py68_dict_pop(&runtime, dict, py68_value_int(1),
                            &value) == PY68_STATUS_OK;
    passed &= value.as.integer == 2;
    py68_value_release(&runtime, value);
    passed &= dict->count == 0;
    py68_object_release(&runtime, &dict->base);
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    if (passed) { puts("PASS: dict tests"); return 0; }
    return 1;
}
