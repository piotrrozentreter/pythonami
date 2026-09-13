/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_builtin.h"
#include "py68k_list.h"
#include "py68k_runtime.h"
#include "py68k_string.h"

#include <stdio.h>

int main(void)
{
    Py68Runtime runtime;
    Py68Value arguments[3];
    Py68Value result;
    Py68Value range_value;
    Py68List *list;
    Py68List *pop_list;
    Py68String *string;
    int passed = 1;

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;

    arguments[0] = py68_value_int(7);
    arguments[1] = py68_value_bool(1);
    arguments[2] = py68_value_none();
    passed &= py68_builtin_print(&runtime, 3, arguments, &result) ==
              PY68_STATUS_OK;
    passed &= result.type == PY68_VALUE_NONE;

    arguments[0] = py68_value_int(7);
    passed &= py68_builtin_len(&runtime, 1, arguments, &result) ==
              PY68_STATUS_RUNTIME_ERROR;

    passed &= py68_string_new_copy(&runtime, "abcd", 4, &string) ==
              PY68_STATUS_OK;
    arguments[0] = py68_value_from_object(&string->base);
    passed &= py68_builtin_len(&runtime, 1, arguments, &result) ==
              PY68_STATUS_OK;
    passed &= result.type == PY68_VALUE_INT && result.as.integer == 4;
    py68_value_release(&runtime, result);
    py68_object_release(&runtime, &string->base);

    passed &= py68_list_new(&runtime, &list) == PY68_STATUS_OK;
    passed &= py68_list_append_copy(&runtime, list, py68_value_int(1)) ==
              PY68_STATUS_OK;
    passed &= py68_list_append_copy(&runtime, list, py68_value_int(2)) ==
              PY68_STATUS_OK;
    passed &= py68_list_append_copy(&runtime, list, py68_value_int(3)) ==
              PY68_STATUS_OK;
    arguments[0] = py68_value_from_object(&list->base);
    passed &= py68_builtin_len(&runtime, 1, arguments, &result) ==
              PY68_STATUS_OK;
    passed &= result.type == PY68_VALUE_INT && result.as.integer == 3;
    py68_value_release(&runtime, result);

    arguments[0] = py68_value_int(3);
    passed &= py68_builtin_range(&runtime, 1, arguments, &range_value) ==
              PY68_STATUS_OK;
    passed &= range_value.type == PY68_VALUE_OBJECT &&
              range_value.as.object != NULL &&
              range_value.as.object->type == PY68_OBJECT_LIST;
    if (range_value.type == PY68_VALUE_OBJECT) {
        Py68List *range_list = (Py68List *)range_value.as.object;
        passed &= range_list->count == 3;
        passed &= range_list->items[0].type == PY68_VALUE_INT &&
                  range_list->items[0].as.integer == 0;
        passed &= range_list->items[1].type == PY68_VALUE_INT &&
                  range_list->items[1].as.integer == 1;
        passed &= range_list->items[2].type == PY68_VALUE_INT &&
                  range_list->items[2].as.integer == 2;
    }
    py68_value_release(&runtime, range_value);

    passed &= py68_list_new(&runtime, &pop_list) == PY68_STATUS_OK;
    passed &= py68_list_append_copy(&runtime, pop_list, py68_value_int(4)) ==
              PY68_STATUS_OK;
    passed &= py68_list_append_copy(&runtime, pop_list, py68_value_int(6)) ==
              PY68_STATUS_OK;
    arguments[0] = py68_value_from_object(&pop_list->base);
    passed &= py68_builtin_list_pop(&runtime, 1, arguments, &result) ==
              PY68_STATUS_OK;
    passed &= result.type == PY68_VALUE_INT && result.as.integer == 6;
    py68_value_release(&runtime, result);
    passed &= pop_list->count == 1;
    passed &= pop_list->items[0].type == PY68_VALUE_INT &&
              pop_list->items[0].as.integer == 4;

    py68_object_release(&runtime, &pop_list->base);
    py68_object_release(&runtime, &list->base);
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    if (passed) { puts("PASS: builtin IO tests"); return 0; }
    return 1;
}
