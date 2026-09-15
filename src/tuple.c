/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_tuple.h"
#include "py68k_runtime.h"
#include "py68k_value.h"

#include <stddef.h>

static Py68Status py68_tuple_index(Py68Tuple *tuple, Py68I32 index,
                                   Py68U32 *normalized)
{
    Py68I32 result = index;
    if (result < 0) result += (Py68I32)tuple->count;
    if (result < 0 || (Py68U32)result >= tuple->count)
        return PY68_STATUS_SOURCE_ERROR;
    *normalized = (Py68U32)result;
    return PY68_STATUS_OK;
}

Py68Status py68_tuple_new(Py68Runtime *runtime, Py68U32 count,
                          Py68Tuple **result)
{
    Py68Tuple *tuple;
    Py68U32 index;
    tuple = (Py68Tuple *)py68_alloc(&runtime->allocator, PY68_MEM_TUPLE,
                                    sizeof(Py68Tuple));
    if (tuple == NULL) return PY68_STATUS_MEMORY_ERROR;
    tuple->base.type = PY68_OBJECT_TUPLE;
    tuple->base.flags = 0;
    tuple->base.reference_count = 1;
    tuple->base.next_object = runtime->live_objects;
    runtime->live_objects = &tuple->base;
    tuple->count = count;
    tuple->items = NULL;
    if (count != 0) {
        tuple->items = (Py68Value *)py68_alloc(&runtime->allocator,
                                               PY68_MEM_TUPLE,
                                               count * sizeof(Py68Value));
        if (tuple->items == NULL) {
            py68_object_release(runtime, &tuple->base);
            return PY68_STATUS_MEMORY_ERROR;
        }
        for (index = 0; index < count; ++index)
            tuple->items[index] = py68_value_none();
    }
    *result = tuple;
    return PY68_STATUS_OK;
}

Py68Status py68_tuple_from_values(Py68Runtime *runtime, Py68Value *items,
                                  Py68U32 count, Py68Tuple **result)
{
    Py68Tuple *tuple;
    Py68U32 index;
    Py68Status status = py68_tuple_new(runtime, count, &tuple);
    if (status != PY68_STATUS_OK) return status;
    for (index = 0; index < count; ++index) {
        tuple->items[index] = items[index];
        py68_value_retain(items[index]);
    }
    *result = tuple;
    return PY68_STATUS_OK;
}

Py68Status py68_tuple_get_copy(Py68Runtime *runtime, Py68Tuple *tuple,
                               Py68I32 index, Py68Value *result)
{
    Py68U32 normalized;
    if (runtime == NULL || tuple == NULL || result == NULL)
        return PY68_STATUS_INTERNAL_ERROR;
    if (py68_tuple_index(tuple, index, &normalized) != PY68_STATUS_OK)
        return PY68_STATUS_SOURCE_ERROR;
    *result = tuple->items[normalized];
    py68_value_retain(*result);
    return PY68_STATUS_OK;
}

Py68Status py68_tuple_concat(Py68Runtime *runtime, Py68Tuple *left,
                             Py68Tuple *right, Py68Tuple **result)
{
    Py68Tuple *tuple;
    Py68U32 index;
    Py68U32 total;
    Py68Status status;
    if (left->count > (Py68U32)~(Py68U32)0 - right->count)
        return PY68_STATUS_MEMORY_ERROR;
    total = left->count + right->count;
    status = py68_tuple_new(runtime, total, &tuple);
    if (status != PY68_STATUS_OK) return status;
    for (index = 0; index < left->count; ++index) {
        tuple->items[index] = left->items[index];
        py68_value_retain(tuple->items[index]);
    }
    for (index = 0; index < right->count; ++index) {
        tuple->items[left->count + index] = right->items[index];
        py68_value_retain(tuple->items[left->count + index]);
    }
    *result = tuple;
    return PY68_STATUS_OK;
}

Py68Status py68_tuple_slice(Py68Runtime *runtime, Py68Tuple *tuple,
                            Py68I32 start, Py68I32 end, int start_omitted,
                            int end_omitted, Py68Tuple **result)
{
    Py68I32 length = (Py68I32)tuple->count;
    Py68I32 slice_start = start_omitted ? 0 : start;
    Py68I32 slice_end = end_omitted ? length : end;
    Py68I32 index;
    Py68U32 count;
    Py68Tuple *sliced;
    Py68Status status;
    if (slice_start < 0) slice_start += length;
    if (slice_end < 0) slice_end += length;
    if (slice_start < 0) slice_start = 0;
    if (slice_end < 0) slice_end = 0;
    if (slice_start > length) slice_start = length;
    if (slice_end > length) slice_end = length;
    if (slice_end < slice_start) slice_end = slice_start;
    count = (Py68U32)(slice_end - slice_start);
    status = py68_tuple_new(runtime, count, &sliced);
    if (status != PY68_STATUS_OK) return status;
    for (index = 0; index < (Py68I32)count; ++index) {
        sliced->items[index] = tuple->items[slice_start + index];
        py68_value_retain(sliced->items[index]);
    }
    *result = sliced;
    return PY68_STATUS_OK;
}

int py68_tuple_equal(Py68Runtime *runtime, Py68Tuple *left, Py68Tuple *right)
{
    Py68U32 index;
    if (left == right) return 1;
    if (left->count != right->count) return 0;
    for (index = 0; index < left->count; ++index) {
        if (!py68_value_equal(runtime, left->items[index], right->items[index]))
            return 0;
    }
    return 1;
}

int py68_tuple_hash(Py68Runtime *runtime, Py68Tuple *tuple, Py68U32 *hash_out)
{
    Py68U32 hash = 0x345678UL;
    Py68U32 index;
    Py68U32 item_hash;
    for (index = 0; index < tuple->count; ++index) {
        if (!py68_value_hash(runtime, tuple->items[index], &item_hash))
            return 0;
        hash = (hash * 1000003UL) ^ item_hash;
    }
    *hash_out = hash;
    return 1;
}

int py68_tuple_has_item(Py68Runtime *runtime, Py68Tuple *tuple, Py68Value value)
{
    Py68U32 index;
    for (index = 0; index < tuple->count; ++index) {
        if (py68_value_equal(runtime, tuple->items[index], value)) return 1;
    }
    return 0;
}
