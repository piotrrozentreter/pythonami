/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_attr.h"
#include "py68k_list.h"
#include "py68k_runtime.h"

#include <stdio.h>

int main(void)
{
    Py68Runtime runtime;
    Py68List *list;
    Py68Value method;
    int passed = 1;

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    passed &= py68_list_new(&runtime, &list) == PY68_STATUS_OK;
    passed &= py68_attr_load(&runtime, py68_value_from_object(&list->base),
                             (const Py68U8 *)"append", 6, &method) ==
              PY68_STATUS_OK;
    passed &= method.type == PY68_VALUE_OBJECT &&
              method.as.object->type == PY68_OBJECT_BOUND_METHOD;
    py68_value_release(&runtime, method);
    py68_object_release(&runtime, &list->base);
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    if (passed) { puts("PASS: attr tests"); return 0; }
    return 1;
}
