#include "py68k_range.h"
#include "py68k_runtime.h"

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
