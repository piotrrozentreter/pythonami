/* 2026 by Piotr Rozentreter (Rozsoft) */

#define PY68K_EXT_OMIT_HELPERS
#include "py68k_ext.h"
#include "py68k_list.h"
#include "py68k_object.h"
#include "py68k_runtime.h"
#include "py68k_string.h"
#include "py68k_value.h"

static Py68Status py68_ext_svc_string_new_copy(Py68Runtime *runtime,
                                               const char *data,
                                               Py68U32 length,
                                               Py68Value *out)
{
    Py68String *string;
    Py68Status status;
    if (runtime == 0 || out == 0)
        return PY68_STATUS_INTERNAL_ERROR;
    status = py68_string_new_copy(runtime, data == 0 ? "" : data, length,
                                  &string);
    if (status != PY68_STATUS_OK)
        return status;
    *out = py68_value_from_object(&string->base);
    return PY68_STATUS_OK;
}

static int py68_ext_svc_string_borrow(Py68Runtime *runtime, Py68Value value,
                                      const char **data, Py68U32 *length)
{
    Py68String *string;
    (void)runtime;
    if (data == 0 || length == 0)
        return 0;
    if (value.type != PY68_VALUE_OBJECT || value.as.object == 0 ||
        value.as.object->type != PY68_OBJECT_STRING)
        return 0;
    string = (Py68String *)value.as.object;
    *data = string->data;
    *length = string->length;
    return 1;
}

static Py68Status py68_ext_svc_list_new(Py68Runtime *runtime, Py68Value *out)
{
    Py68List *list;
    Py68Status status;
    if (runtime == 0 || out == 0)
        return PY68_STATUS_INTERNAL_ERROR;
    status = py68_list_new(runtime, &list);
    if (status != PY68_STATUS_OK)
        return status;
    *out = py68_value_from_object(&list->base);
    return PY68_STATUS_OK;
}

static Py68Status py68_ext_svc_list_append_copy(Py68Runtime *runtime,
                                                Py68Value list_value,
                                                Py68Value item)
{
    Py68List *list;
    if (runtime == 0)
        return PY68_STATUS_INTERNAL_ERROR;
    if (list_value.type != PY68_VALUE_OBJECT || list_value.as.object == 0 ||
        list_value.as.object->type != PY68_OBJECT_LIST)
        return PY68_STATUS_RUNTIME_ERROR;
    list = (Py68List *)list_value.as.object;
    return py68_list_append_copy(runtime, list, item);
}

static Py68U32 py68_ext_svc_list_count(Py68Runtime *runtime, Py68Value list_value)
{
    Py68List *list;
    (void)runtime;
    if (list_value.type != PY68_VALUE_OBJECT || list_value.as.object == 0 ||
        list_value.as.object->type != PY68_OBJECT_LIST)
        return 0;
    list = (Py68List *)list_value.as.object;
    return list->count;
}

static Py68Status py68_ext_svc_list_get_copy(Py68Runtime *runtime,
                                             Py68Value list_value,
                                             Py68I32 index, Py68Value *out)
{
    Py68List *list;
    if (runtime == 0 || out == 0)
        return PY68_STATUS_INTERNAL_ERROR;
    if (list_value.type != PY68_VALUE_OBJECT || list_value.as.object == 0 ||
        list_value.as.object->type != PY68_OBJECT_LIST)
        return PY68_STATUS_RUNTIME_ERROR;
    list = (Py68List *)list_value.as.object;
    return py68_list_get_copy(runtime, list, index, out);
}

static void py68_ext_svc_value_release(Py68Runtime *runtime, Py68Value value)
{
    if (runtime == 0)
        return;
    py68_value_release(runtime, value);
}

static const Py68ExtServices py68_ext_services_table = {
    py68_ext_svc_string_new_copy,
    py68_ext_svc_string_borrow,
    py68_ext_svc_list_new,
    py68_ext_svc_list_append_copy,
    py68_ext_svc_list_count,
    py68_ext_svc_list_get_copy,
    py68_ext_svc_value_release
};

void py68_ext_services_install(Py68Runtime *runtime)
{
    if (runtime == 0)
        return;
    runtime->ext_services = &py68_ext_services_table;
}
