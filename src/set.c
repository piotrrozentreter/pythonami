/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_set.h"
#include "py68k_list.h"
#include "py68k_runtime.h"
#include "py68k_value.h"

#include <stddef.h>

static Py68Status py68_set_grow(Py68Runtime *runtime, Py68Set *set)
{
    Py68U32 capacity = set->capacity == 0 ? 8 : set->capacity * 2;
    Py68SetEntry *entries;
    Py68SetEntry *old;
    Py68U32 old_capacity;
    Py68U32 index;
    Py68U32 mask;
    if (capacity < set->capacity) return PY68_STATUS_MEMORY_ERROR;
    entries = (Py68SetEntry *)py68_alloc(&runtime->allocator, PY68_MEM_SET,
                                         capacity * sizeof(Py68SetEntry));
    if (entries == NULL) return PY68_STATUS_MEMORY_ERROR;
    for (index = 0; index < capacity; ++index) {
        entries[index].used = 0;
        entries[index].hash = 0;
        entries[index].value = py68_value_none();
    }
    old = set->entries;
    old_capacity = set->capacity;
    mask = capacity - 1;
    for (index = 0; index < old_capacity; ++index) {
        if (old[index].used == 1) {
            Py68U32 slot = old[index].hash & mask;
            while (entries[slot].used == 1)
                slot = (slot + 1) & mask;
            entries[slot] = old[index];
        }
    }
    set->entries = entries;
    set->capacity = capacity;
    py68_free(&runtime->allocator, PY68_MEM_SET, old,
              old_capacity * sizeof(Py68SetEntry));
    return PY68_STATUS_OK;
}

Py68Status py68_set_new(Py68Runtime *runtime, Py68Set **result)
{
    Py68Set *set = (Py68Set *)py68_alloc(&runtime->allocator, PY68_MEM_SET,
                                         sizeof(Py68Set));
    if (set == NULL) return PY68_STATUS_MEMORY_ERROR;
    set->base.type = PY68_OBJECT_SET;
    set->base.flags = 0;
    set->base.reference_count = 1;
    set->base.next_object = runtime->live_objects;
    runtime->live_objects = &set->base;
    set->count = 0;
    set->capacity = 0;
    set->entries = NULL;
    *result = set;
    return PY68_STATUS_OK;
}

static Py68Status py68_set_lookup(Py68Runtime *runtime, Py68Set *set,
                                  Py68Value value, Py68U32 hash,
                                  Py68U32 *slot_out, int *found)
{
    Py68U32 mask;
    Py68U32 slot;
    Py68U32 start;
    Py68U32 deleted = (Py68U32)~(Py68U32)0;
    if (set->capacity == 0) {
        *found = 0;
        *slot_out = 0;
        return PY68_STATUS_OK;
    }
    mask = set->capacity - 1;
    slot = hash & mask;
    start = slot;
    *found = 0;
    for (;;) {
        if (set->entries[slot].used == 0) {
            *slot_out = deleted != (Py68U32)~(Py68U32)0 ? deleted : slot;
            return PY68_STATUS_OK;
        }
        if (set->entries[slot].used == 2) {
            if (deleted == (Py68U32)~(Py68U32)0) deleted = slot;
        } else if (set->entries[slot].hash == hash &&
                   py68_value_equal(runtime, set->entries[slot].value, value)) {
            *slot_out = slot;
            *found = 1;
            return PY68_STATUS_OK;
        }
        slot = (slot + 1) & mask;
        if (slot == start) {
            *slot_out = deleted != (Py68U32)~(Py68U32)0 ? deleted : slot;
            return PY68_STATUS_OK;
        }
    }
}

Py68Status py68_set_add(Py68Runtime *runtime, Py68Set *set, Py68Value value)
{
    Py68U32 hash;
    Py68U32 slot;
    int found;
    if (!py68_value_hashable(value) || !py68_value_hash(runtime, value, &hash))
        return PY68_STATUS_SOURCE_ERROR;
    if (set->capacity == 0 || (set->count + 1) * 3 > set->capacity * 2) {
        if (py68_set_grow(runtime, set) != PY68_STATUS_OK)
            return PY68_STATUS_MEMORY_ERROR;
    }
    if (py68_set_lookup(runtime, set, value, hash, &slot, &found) !=
        PY68_STATUS_OK)
        return PY68_STATUS_INTERNAL_ERROR;
    if (found) return PY68_STATUS_OK;
    py68_value_retain(value);
    set->entries[slot].used = 1;
    set->entries[slot].hash = hash;
    set->entries[slot].value = value;
    ++set->count;
    return PY68_STATUS_OK;
}

Py68Status py68_set_remove(Py68Runtime *runtime, Py68Set *set, Py68Value value)
{
    Py68U32 hash;
    Py68U32 slot;
    int found;
    if (!py68_value_hash(runtime, value, &hash))
        return PY68_STATUS_SOURCE_ERROR;
    if (py68_set_lookup(runtime, set, value, hash, &slot, &found) !=
        PY68_STATUS_OK)
        return PY68_STATUS_INTERNAL_ERROR;
    if (!found) return PY68_STATUS_SOURCE_ERROR;
    py68_value_release(runtime, set->entries[slot].value);
    set->entries[slot].used = 2;
    set->entries[slot].value = py68_value_none();
    --set->count;
    return PY68_STATUS_OK;
}

Py68Status py68_set_discard(Py68Runtime *runtime, Py68Set *set, Py68Value value)
{
    Py68Status status = py68_set_remove(runtime, set, value);
    if (status == PY68_STATUS_SOURCE_ERROR) return PY68_STATUS_OK;
    return status;
}

Py68Status py68_set_values(Py68Runtime *runtime, Py68Set *set, Py68List **result)
{
    Py68List *list;
    Py68U32 index;
    Py68Status status = py68_list_new(runtime, &list);
    if (status != PY68_STATUS_OK) return status;
    for (index = 0; index < set->capacity; ++index) {
        if (set->entries[index].used == 1) {
            status = py68_list_append_copy(runtime, list,
                                           set->entries[index].value);
            if (status != PY68_STATUS_OK) {
                py68_object_release(runtime, &list->base);
                return status;
            }
        }
    }
    *result = list;
    return PY68_STATUS_OK;
}

int py68_set_equal(Py68Runtime *runtime, Py68Set *left, Py68Set *right)
{
    Py68U32 index;
    Py68U32 slot;
    int found;
    Py68U32 hash;
    if (left == right) return 1;
    if (left->count != right->count) return 0;
    for (index = 0; index < left->capacity; ++index) {
        if (left->entries[index].used != 1) continue;
        if (!py68_value_hash(runtime, left->entries[index].value, &hash))
            return 0;
        if (py68_set_lookup(runtime, right, left->entries[index].value, hash,
                            &slot, &found) != PY68_STATUS_OK || !found)
            return 0;
    }
    return 1;
}

Py68Status py68_set_contains(Py68Runtime *runtime, Py68Set *set,
                             Py68Value value, int *found)
{
    Py68U32 hash;
    Py68U32 slot;
    if (!py68_value_hashable(value) || !py68_value_hash(runtime, value, &hash))
        return PY68_STATUS_SOURCE_ERROR;
    if (py68_set_lookup(runtime, set, value, hash, &slot, found) !=
        PY68_STATUS_OK)
        return PY68_STATUS_INTERNAL_ERROR;
    return PY68_STATUS_OK;
}
