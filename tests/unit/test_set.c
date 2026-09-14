/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_runtime.h"
#include "py68k_set.h"

#include <stdio.h>

int main(void)
{
    Py68Runtime runtime;
    Py68Set *set;
    int passed = 1;

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    passed &= py68_set_new(&runtime, &set) == PY68_STATUS_OK;
    passed &= py68_set_add(&runtime, set, py68_value_int(3)) == PY68_STATUS_OK;
    passed &= py68_set_add(&runtime, set, py68_value_int(3)) == PY68_STATUS_OK;
    passed &= set->count == 1;
    passed &= py68_set_discard(&runtime, set, py68_value_int(4)) == PY68_STATUS_OK;
    passed &= py68_set_remove(&runtime, set, py68_value_int(3)) == PY68_STATUS_OK;
    passed &= set->count == 0;
    py68_object_release(&runtime, &set->base);
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    if (passed) { puts("PASS: set tests"); return 0; }
    return 1;
}
