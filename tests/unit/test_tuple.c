/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_runtime.h"
#include "py68k_tuple.h"

#include <stdio.h>

int main(void)
{
    Py68Runtime runtime;
    Py68Tuple *tuple;
    Py68Tuple *other;
    Py68Tuple *concat;
    Py68Value items[2];
    Py68Value read;
    Py68U32 hash;
    int passed = 1;

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    items[0] = py68_value_int(1);
    items[1] = py68_value_int(2);
    passed &= py68_tuple_from_values(&runtime, items, 2, &tuple) == PY68_STATUS_OK;
    passed &= py68_tuple_get_copy(&runtime, tuple, 0, &read) == PY68_STATUS_OK;
    passed &= read.type == PY68_VALUE_INT && read.as.integer == 1;
    py68_value_release(&runtime, read);
    passed &= py68_tuple_get_copy(&runtime, tuple, 2, &read) == PY68_STATUS_SOURCE_ERROR;
    passed &= py68_value_equal(&runtime, py68_value_from_object(&tuple->base),
                               py68_value_from_object(&tuple->base));
    passed &= py68_tuple_from_values(&runtime, items, 2, &other) == PY68_STATUS_OK;
    passed &= py68_tuple_equal(&runtime, tuple, other);
    passed &= py68_tuple_hash(&runtime, tuple, &hash);
    passed &= py68_tuple_concat(&runtime, tuple, other, &concat) == PY68_STATUS_OK;
    passed &= concat->count == 4;
    py68_object_release(&runtime, &concat->base);
    py68_object_release(&runtime, &other->base);
    py68_object_release(&runtime, &tuple->base);
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    if (passed) { puts("PASS: tuple tests"); return 0; }
    return 1;
}
