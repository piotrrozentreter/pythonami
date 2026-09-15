/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_range.h"
#include "py68k_runtime.h"
#include "py68k_string.h"
#include "py68k_tuple.h"
#include "py68k_dict.h"
#include "py68k_set.h"

#include <stddef.h>

Py68Status py68_range_new(Py68Runtime *runtime,
                         Py68I32 start, Py68I32 stop, Py68I32 step,
                         Py68Range **result)
{
    Py68Range *range;
    if (step == 0) return PY68_STATUS_RUNTIME_ERROR;
    range = (Py68Range *)py68_alloc(&runtime->allocator, PY68_MEM_RUNTIME,
                                   sizeof(Py68Range));
    if (range == NULL) return PY68_STATUS_MEMORY_ERROR;
    range->base.type = PY68_OBJECT_RANGE;
    range->base.flags = 0;
    range->base.reference_count = 1;
    range->base.next_object = runtime->live_objects;
    runtime->live_objects = &range->base;
    range->list = NULL;
    range->current = start;
    range->stop = stop;
    range->step = step;
    *result = range;
    return PY68_STATUS_OK;
}

Py68Status py68_range_new_list(Py68Runtime *runtime,
                              Py68List *list,
                              Py68Range **result)
{
    Py68Range *range;
    if (list == NULL) return PY68_STATUS_INTERNAL_ERROR;
    range = (Py68Range *)py68_alloc(&runtime->allocator, PY68_MEM_RUNTIME,
                                   sizeof(Py68Range));
    if (range == NULL) return PY68_STATUS_MEMORY_ERROR;
    range->base.type = PY68_OBJECT_RANGE;
    range->base.flags = 0;
    range->base.reference_count = 1;
    range->base.next_object = runtime->live_objects;
    runtime->live_objects = &range->base;
    range->list = list;
    py68_object_retain(&list->base);
    range->current = 0;
    range->stop = (Py68I32)list->count;
    range->step = 1;
    *result = range;
    return PY68_STATUS_OK;
}

Py68Status py68_range_next_value(Py68Runtime *runtime,
                                 Py68Range *range,
                                 Py68Value *result,
                                 int *has_next)
{
    if (range == NULL || result == NULL || has_next == NULL)
        return PY68_STATUS_INTERNAL_ERROR;
    if (range->step > 0) {
        if (range->current >= range->stop) {
            *has_next = 0;
            return PY68_STATUS_OK;
        }
    } else {
        if (range->current <= range->stop) {
            *has_next = 0;
            return PY68_STATUS_OK;
        }
    }

    *has_next = 1;
    if (range->list != NULL) {
        Py68Status status = py68_list_get_copy(runtime, range->list,
                                               range->current, result);
        if (status != PY68_STATUS_OK) return status;
    } else {
        *result = py68_value_int(range->current);
    }
    range->current += range->step;
    return PY68_STATUS_OK;
}

Py68Status py68_iterable_get_iter(Py68Runtime *runtime, Py68Value value,
                                  Py68Range **result)
{
    if (value.type == PY68_VALUE_OBJECT && value.as.object != NULL &&
        value.as.object->type == PY68_OBJECT_RANGE) {
        py68_value_retain(value);
        *result = (Py68Range *)value.as.object;
        return PY68_STATUS_OK;
    }
    if (value.type == PY68_VALUE_OBJECT && value.as.object != NULL &&
        value.as.object->type == PY68_OBJECT_LIST)
        return py68_range_new_list(runtime, (Py68List *)value.as.object,
                                   result);
    if (value.type == PY68_VALUE_OBJECT && value.as.object != NULL &&
        value.as.object->type == PY68_OBJECT_TUPLE) {
        Py68Tuple *tuple = (Py68Tuple *)value.as.object;
        Py68List *list;
        Py68U32 index;
        Py68Status status = py68_list_new(runtime, &list);
        if (status != PY68_STATUS_OK) return status;
        for (index = 0; index < tuple->count; ++index) {
            status = py68_list_append_copy(runtime, list, tuple->items[index]);
            if (status != PY68_STATUS_OK) {
                py68_object_release(runtime, &list->base);
                return status;
            }
        }
        status = py68_range_new_list(runtime, list, result);
        py68_object_release(runtime, &list->base);
        return status;
    }
    if (value.type == PY68_VALUE_OBJECT && value.as.object != NULL &&
        value.as.object->type == PY68_OBJECT_DICT) {
        Py68List *list;
        Py68Status status = py68_dict_keys(
            runtime, (Py68Dict *)value.as.object, &list);
        if (status != PY68_STATUS_OK) return status;
        status = py68_range_new_list(runtime, list, result);
        py68_object_release(runtime, &list->base);
        return status;
    }
    if (value.type == PY68_VALUE_OBJECT && value.as.object != NULL &&
        value.as.object->type == PY68_OBJECT_SET) {
        Py68List *list;
        Py68Status status = py68_set_values(
            runtime, (Py68Set *)value.as.object, &list);
        if (status != PY68_STATUS_OK) return status;
        status = py68_range_new_list(runtime, list, result);
        py68_object_release(runtime, &list->base);
        return status;
    }
    if (value.type == PY68_VALUE_OBJECT && value.as.object != NULL &&
        value.as.object->type == PY68_OBJECT_STRING) {
        Py68String *string = (Py68String *)value.as.object;
        Py68List *list;
        Py68U32 index;
        Py68Status status = py68_list_new(runtime, &list);
        if (status != PY68_STATUS_OK) return status;
        for (index = 0; index < string->length; ++index) {
            Py68String *ch;
            status = py68_string_new_copy(runtime, string->data + index, 1, &ch);
            if (status != PY68_STATUS_OK) {
                py68_object_release(runtime, &list->base);
                return status;
            }
            status = py68_list_append_copy(
                runtime, list, py68_value_from_object(&ch->base));
            py68_object_release(runtime, &ch->base);
            if (status != PY68_STATUS_OK) {
                py68_object_release(runtime, &list->base);
                return status;
            }
        }
        status = py68_range_new_list(runtime, list, result);
        py68_object_release(runtime, &list->base);
        return status;
    }
    return PY68_STATUS_SOURCE_ERROR;
}
