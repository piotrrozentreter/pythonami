/* 2026 by Piotr Rozentreter (Rozsoft) */

#define PY68K_EXT_OMIT_HELPERS
#include "py68k_ext.h"
#include "py68k_runtime.h"
#include "py68k_value.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    Py68Runtime runtime;
    const Py68ExtServices *svc;
    Py68Value str_value;
    Py68Value list_value;
    Py68Value item;
    const char *data;
    Py68U32 length;
    int passed = 1;

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    svc = ((const Py68ExtRuntimeHead *)&runtime)->ext_services;
    passed &= svc != 0;
    passed &= svc->string_new_copy != 0;
    passed &= svc->string_borrow != 0;
    passed &= svc->list_new != 0;
    passed &= svc->list_append_copy != 0;
    passed &= svc->list_count != 0;
    passed &= svc->list_get_copy != 0;
    passed &= svc->value_release != 0;

    passed &= svc->string_new_copy(&runtime, "hi", 2, &str_value) ==
              PY68_STATUS_OK;
    passed &= svc->string_borrow(&runtime, str_value, &data, &length) == 1;
    passed &= length == 2 && data[0] == 'h' && data[1] == 'i';

    passed &= svc->list_new(&runtime, &list_value) == PY68_STATUS_OK;
    passed &= svc->list_append_copy(&runtime, list_value, str_value) ==
              PY68_STATUS_OK;
    passed &= svc->list_count(&runtime, list_value) == 1;
    passed &= svc->list_get_copy(&runtime, list_value, 0, &item) ==
              PY68_STATUS_OK;
    passed &= svc->string_borrow(&runtime, item, &data, &length) == 1;
    passed &= length == 2;

    svc->value_release(&runtime, item);
    svc->value_release(&runtime, list_value);
    svc->value_release(&runtime, str_value);

    py68_runtime_shutdown(&runtime);

    if (!passed) {
        fprintf(stderr, "test_ext_services FAILED\n");
        return 1;
    }
    printf("test_ext_services OK\n");
    return 0;
}
