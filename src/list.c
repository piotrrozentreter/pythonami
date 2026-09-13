#include "py68k_list.h"
#include "py68k_runtime.h"

#include <stddef.h>

static Py68Status py68_list_index(Py68List *list, Py68I32 index,
                                   Py68U32 *normalized)
{
    Py68I32 result = index;
    if (result < 0) result += (Py68I32)list->count;
    if (result < 0 || (Py68U32)result >= list->count)
        return PY68_STATUS_SOURCE_ERROR;
    *normalized = (Py68U32)result;
    return PY68_STATUS_OK;
}

static int py68_list_contains(Py68List *root, Py68List *target,
                              Py68Allocator *allocator)
{
    Py68List **pending;
    Py68List **visited;
    Py68U32 pending_count = 0;
    Py68U32 visited_count = 0;
    Py68U32 capacity = 8;
    Py68U32 index;
    Py68U32 item_index;
    int found = 0;

    pending = (Py68List **)py68_alloc(allocator, PY68_MEM_TEMP,
                                      capacity * sizeof(Py68List *));
    visited = (Py68List **)py68_alloc(allocator, PY68_MEM_TEMP,
                                      capacity * sizeof(Py68List *));
    if (pending == NULL || visited == NULL) goto cleanup;
    pending[pending_count++] = root;
    while (pending_count != 0) {
        Py68List *current = pending[--pending_count];
        if (current == target) { found = 1; break; }
        for (index = 0; index < visited_count; ++index)
            if (visited[index] == current) break;
        if (index != visited_count) continue;
        if (visited_count == capacity) break;
        visited[visited_count++] = current;
        for (item_index = 0; item_index < current->count; ++item_index) {
            Py68Value item = current->items[item_index];
            if (item.type == PY68_VALUE_OBJECT &&
                item.as.object != NULL &&
                item.as.object->type == PY68_OBJECT_LIST) {
                if (pending_count == capacity) break;
                pending[pending_count++] = (Py68List *)item.as.object;
            }
        }
    }
cleanup:
    py68_free(allocator, PY68_MEM_TEMP, pending,
              capacity * sizeof(Py68List *));
    py68_free(allocator, PY68_MEM_TEMP, visited,
              capacity * sizeof(Py68List *));
    return found;
}

static Py68Status py68_list_grow(Py68Runtime *runtime, Py68List *list)
{
    Py68U32 capacity = list->capacity == 0 ? 4 : list->capacity * 2;
    Py68Value *items;
    if (capacity < list->capacity ||
        capacity > (Py68U32)~(Py68U32)0 / sizeof(Py68Value))
        return PY68_STATUS_MEMORY_ERROR;
    items = (Py68Value *)py68_realloc(&runtime->allocator, PY68_MEM_LIST,
        list->items, list->capacity * sizeof(Py68Value),
        capacity * sizeof(Py68Value));
    if (items == NULL) return PY68_STATUS_MEMORY_ERROR;
    list->items = items;
    list->capacity = capacity;
    return PY68_STATUS_OK;
}

Py68Status py68_list_new(Py68Runtime *runtime, Py68List **result)
{
    Py68List *list = (Py68List *)py68_alloc(&runtime->allocator, PY68_MEM_LIST,
                                            sizeof(Py68List));
    if (list == NULL) return PY68_STATUS_MEMORY_ERROR;
    list->base.type = PY68_OBJECT_LIST;
    list->base.flags = 0;
    list->base.reference_count = 1;
    list->base.next_object = runtime->live_objects;
    runtime->live_objects = &list->base;
    list->count = 0;
    list->capacity = 0;
    list->items = NULL;
    *result = list;
    return PY68_STATUS_OK;
}

Py68Status py68_list_append_copy(Py68Runtime *runtime, Py68List *list,
                                 Py68Value value)
{
    if (value.type == PY68_VALUE_OBJECT && value.as.object != NULL &&
        value.as.object->type == PY68_OBJECT_LIST &&
        py68_list_contains((Py68List *)value.as.object, list,
                           &runtime->allocator))
        return PY68_STATUS_SOURCE_ERROR;
    if (list->count == list->capacity &&
        py68_list_grow(runtime, list) != PY68_STATUS_OK)
        return PY68_STATUS_MEMORY_ERROR;
    py68_value_retain(value);
    list->items[list->count++] = value;
    return PY68_STATUS_OK;
}

Py68Status py68_list_append_move(Py68Runtime *runtime, Py68List *list,
                                 Py68Value *value)
{
    Py68Status status = py68_list_append_copy(runtime, list, *value);
    if (status == PY68_STATUS_OK) {
        py68_value_release(runtime, *value);
        *value = py68_value_none();
    }
    return status;
}

Py68Status py68_list_get_copy(Py68Runtime *runtime, Py68List *list,
                              Py68I32 index, Py68Value *result)
{
    Py68U32 normalized;
    if (runtime == NULL || list == NULL || result == NULL)
        return PY68_STATUS_INTERNAL_ERROR;
    if (py68_list_index(list, index, &normalized) != PY68_STATUS_OK)
        return PY68_STATUS_SOURCE_ERROR;
    *result = list->items[normalized];
    py68_value_retain(*result);
    return PY68_STATUS_OK;
}

Py68Status py68_list_set_copy(Py68Runtime *runtime, Py68List *list,
                              Py68I32 index, Py68Value value)
{
    Py68U32 normalized;
    Py68Value old;
    if (value.type == PY68_VALUE_OBJECT && value.as.object != NULL &&
        value.as.object->type == PY68_OBJECT_LIST &&
        py68_list_contains((Py68List *)value.as.object, list,
                           &runtime->allocator))
        return PY68_STATUS_SOURCE_ERROR;
    if (py68_list_index(list, index, &normalized) != PY68_STATUS_OK)
        return PY68_STATUS_SOURCE_ERROR;
    py68_value_retain(value);
    old = list->items[normalized];
    list->items[normalized] = value;
    py68_value_release(runtime, old);
    return PY68_STATUS_OK;
}

Py68Status py68_list_set_move(Py68Runtime *runtime, Py68List *list,
                              Py68I32 index, Py68Value *value)
{
    Py68Status status = py68_list_set_copy(runtime, list, index, *value);
    if (status == PY68_STATUS_OK) {
        py68_value_release(runtime, *value);
        *value = py68_value_none();
    }
    return status;
}

Py68Status py68_list_concat(Py68Runtime *runtime, Py68List *left,
                            Py68List *right, Py68List **result)
{
    Py68List *list;
    Py68U32 index;
    Py68Status status = py68_list_new(runtime, &list);
    if (status != PY68_STATUS_OK) return status;
    for (index = 0; index < left->count; ++index) {
        status = py68_list_append_copy(runtime, list, left->items[index]);
        if (status != PY68_STATUS_OK) {
            py68_object_release(runtime, &list->base);
            return status;
        }
    }
    for (index = 0; index < right->count; ++index) {
        status = py68_list_append_copy(runtime, list, right->items[index]);
        if (status != PY68_STATUS_OK) {
            py68_object_release(runtime, &list->base);
            return status;
        }
    }
    *result = list;
    return PY68_STATUS_OK;
}

Py68Status py68_list_slice(Py68Runtime *runtime, Py68List *list,
                           Py68I32 start, Py68I32 end, int start_omitted,
                           int end_omitted, Py68List **result)
{
    Py68List *sliced;
    Py68I32 length = (Py68I32)list->count;
    Py68I32 slice_start = start_omitted ? 0 : start;
    Py68I32 slice_end = end_omitted ? length : end;
    Py68I32 index;
    Py68Status status;
    if (slice_start < 0) slice_start += length;
    if (slice_end < 0) slice_end += length;
    if (slice_start < 0) slice_start = 0;
    if (slice_end < 0) slice_end = 0;
    if (slice_start > length) slice_start = length;
    if (slice_end > length) slice_end = length;
    if (slice_end < slice_start) slice_end = slice_start;
    status = py68_list_new(runtime, &sliced);
    if (status != PY68_STATUS_OK) return status;
    for (index = slice_start; index < slice_end; ++index) {
        status = py68_list_append_copy(runtime, sliced, list->items[index]);
        if (status != PY68_STATUS_OK) {
            py68_object_release(runtime, &sliced->base);
            return status;
        }
    }
    *result = sliced;
    return PY68_STATUS_OK;
}
