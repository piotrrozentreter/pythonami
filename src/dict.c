/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_dict.h"
#include "py68k_list.h"
#include "py68k_tuple.h"
#include "py68k_runtime.h"
#include "py68k_value.h"

#include <stddef.h>

static int py68_dict_contains_dict(Py68Value value, Py68Dict *target,
                                   Py68U32 depth)
{
    Py68U32 index;
    if (depth > 32) return 0;
    if (value.type != PY68_VALUE_OBJECT || value.as.object == NULL) return 0;
    if (value.as.object == &target->base) return 1;
    if (value.as.object->type == PY68_OBJECT_LIST) {
        Py68List *list = (Py68List *)value.as.object;
        for (index = 0; index < list->count; ++index)
            if (py68_dict_contains_dict(list->items[index], target, depth + 1))
                return 1;
    } else if (value.as.object->type == PY68_OBJECT_DICT) {
        Py68Dict *dict = (Py68Dict *)value.as.object;
        for (index = 0; index < dict->capacity; ++index) {
            if (dict->entries[index].used == 1 &&
                py68_dict_contains_dict(dict->entries[index].value, target,
                                        depth + 1))
                return 1;
        }
    }
    return 0;
}

static Py68Status py68_dict_grow(Py68Runtime *runtime, Py68Dict *dict)
{
    Py68U32 capacity = dict->capacity == 0 ? 8 : dict->capacity * 2;
    Py68DictEntry *entries;
    Py68DictEntry *old;
    Py68U32 old_capacity;
    Py68U32 index;
    Py68U32 mask;
    if (capacity < dict->capacity) return PY68_STATUS_MEMORY_ERROR;
    entries = (Py68DictEntry *)py68_alloc(&runtime->allocator, PY68_MEM_DICT,
                                          capacity * sizeof(Py68DictEntry));
    if (entries == NULL) return PY68_STATUS_MEMORY_ERROR;
    for (index = 0; index < capacity; ++index) {
        entries[index].used = 0;
        entries[index].hash = 0;
        entries[index].key = py68_value_none();
        entries[index].value = py68_value_none();
    }
    old = dict->entries;
    old_capacity = dict->capacity;
    mask = capacity - 1;
    for (index = 0; index < old_capacity; ++index) {
        if (old[index].used == 1) {
            Py68U32 slot = old[index].hash & mask;
            while (entries[slot].used == 1)
                slot = (slot + 1) & mask;
            entries[slot] = old[index];
        }
    }
    dict->entries = entries;
    dict->capacity = capacity;
    py68_free(&runtime->allocator, PY68_MEM_DICT, old,
              old_capacity * sizeof(Py68DictEntry));
    return PY68_STATUS_OK;
}

Py68Status py68_dict_new(Py68Runtime *runtime, Py68Dict **result)
{
    Py68Dict *dict = (Py68Dict *)py68_alloc(&runtime->allocator, PY68_MEM_DICT,
                                            sizeof(Py68Dict));
    if (dict == NULL) return PY68_STATUS_MEMORY_ERROR;
    dict->base.type = PY68_OBJECT_DICT;
    dict->base.flags = 0;
    dict->base.reference_count = 1;
    dict->base.next_object = runtime->live_objects;
    runtime->live_objects = &dict->base;
    dict->count = 0;
    dict->capacity = 0;
    dict->entries = NULL;
    *result = dict;
    return PY68_STATUS_OK;
}

static Py68Status py68_dict_lookup(Py68Runtime *runtime, Py68Dict *dict,
                                   Py68Value key, Py68U32 hash,
                                   Py68U32 *slot_out, int *found)
{
    Py68U32 mask;
    Py68U32 slot;
    Py68U32 start;
    if (dict->capacity == 0) {
        *found = 0;
        *slot_out = 0;
        return PY68_STATUS_OK;
    }
    mask = dict->capacity - 1;
    slot = hash & mask;
    start = slot;
    *found = 0;
    for (;;) {
        if (dict->entries[slot].used == 0) {
            *slot_out = slot;
            return PY68_STATUS_OK;
        }
        if (dict->entries[slot].used == 1 &&
            dict->entries[slot].hash == hash &&
            py68_value_equal(runtime, dict->entries[slot].key, key)) {
            *slot_out = slot;
            *found = 1;
            return PY68_STATUS_OK;
        }
        slot = (slot + 1) & mask;
        if (slot == start) {
            *slot_out = slot;
            return PY68_STATUS_OK;
        }
    }
}

Py68Status py68_dict_get_copy(Py68Runtime *runtime, Py68Dict *dict,
                              Py68Value key, Py68Value *result)
{
    Py68U32 hash;
    Py68U32 slot;
    int found;
    if (!py68_value_hash(runtime, key, &hash))
        return PY68_STATUS_SOURCE_ERROR;
    if (py68_dict_lookup(runtime, dict, key, hash, &slot, &found) !=
        PY68_STATUS_OK)
        return PY68_STATUS_INTERNAL_ERROR;
    if (!found) return PY68_STATUS_SOURCE_ERROR;
    *result = dict->entries[slot].value;
    py68_value_retain(*result);
    return PY68_STATUS_OK;
}

Py68Status py68_dict_set_copy(Py68Runtime *runtime, Py68Dict *dict,
                              Py68Value key, Py68Value value)
{
    Py68U32 hash;
    Py68U32 slot;
    int found;
    if (!py68_value_hashable(key) || !py68_value_hash(runtime, key, &hash))
        return PY68_STATUS_SOURCE_ERROR;
    if (py68_dict_contains_dict(value, dict, 0))
        return PY68_STATUS_RUNTIME_ERROR;
    if (dict->capacity == 0 ||
        (dict->count + 1) * 3 > dict->capacity * 2) {
        if (py68_dict_grow(runtime, dict) != PY68_STATUS_OK)
            return PY68_STATUS_MEMORY_ERROR;
    }
    if (py68_dict_lookup(runtime, dict, key, hash, &slot, &found) !=
        PY68_STATUS_OK)
        return PY68_STATUS_INTERNAL_ERROR;
    py68_value_retain(value);
    if (found) {
        Py68Value old = dict->entries[slot].value;
        dict->entries[slot].value = value;
        py68_value_release(runtime, old);
        return PY68_STATUS_OK;
    }
    py68_value_retain(key);
    dict->entries[slot].used = 1;
    dict->entries[slot].hash = hash;
    dict->entries[slot].key = key;
    dict->entries[slot].value = value;
    ++dict->count;
    return PY68_STATUS_OK;
}

Py68Status py68_dict_pop(Py68Runtime *runtime, Py68Dict *dict, Py68Value key,
                         Py68Value *result)
{
    Py68U32 hash;
    Py68U32 slot;
    int found;
    if (!py68_value_hash(runtime, key, &hash))
        return PY68_STATUS_SOURCE_ERROR;
    if (py68_dict_lookup(runtime, dict, key, hash, &slot, &found) !=
        PY68_STATUS_OK)
        return PY68_STATUS_INTERNAL_ERROR;
    if (!found) return PY68_STATUS_SOURCE_ERROR;
    *result = dict->entries[slot].value;
    py68_value_release(runtime, dict->entries[slot].key);
    dict->entries[slot].used = 2;
    dict->entries[slot].key = py68_value_none();
    dict->entries[slot].value = py68_value_none();
    --dict->count;
    return PY68_STATUS_OK;
}

Py68Status py68_dict_keys(Py68Runtime *runtime, Py68Dict *dict,
                          Py68List **result)
{
    Py68List *list;
    Py68U32 index;
    Py68Status status = py68_list_new(runtime, &list);
    if (status != PY68_STATUS_OK) return status;
    for (index = 0; index < dict->capacity; ++index) {
        if (dict->entries[index].used == 1) {
            status = py68_list_append_copy(runtime, list,
                                           dict->entries[index].key);
            if (status != PY68_STATUS_OK) {
                py68_object_release(runtime, &list->base);
                return status;
            }
        }
    }
    *result = list;
    return PY68_STATUS_OK;
}

Py68Status py68_dict_values(Py68Runtime *runtime, Py68Dict *dict,
                            Py68List **result)
{
    Py68List *list;
    Py68U32 index;
    Py68Status status = py68_list_new(runtime, &list);
    if (status != PY68_STATUS_OK) return status;
    for (index = 0; index < dict->capacity; ++index) {
        if (dict->entries[index].used == 1) {
            status = py68_list_append_copy(runtime, list,
                                           dict->entries[index].value);
            if (status != PY68_STATUS_OK) {
                py68_object_release(runtime, &list->base);
                return status;
            }
        }
    }
    *result = list;
    return PY68_STATUS_OK;
}

Py68Status py68_dict_items(Py68Runtime *runtime, Py68Dict *dict,
                           Py68List **result)
{
    Py68List *list;
    Py68U32 index;
    Py68Status status = py68_list_new(runtime, &list);
    if (status != PY68_STATUS_OK) return status;
    for (index = 0; index < dict->capacity; ++index) {
        Py68Value pair_items[2];
        Py68Tuple *pair;
        if (dict->entries[index].used != 1) continue;
        pair_items[0] = dict->entries[index].key;
        pair_items[1] = dict->entries[index].value;
        status = py68_tuple_from_values(runtime, pair_items, 2, &pair);
        if (status != PY68_STATUS_OK) {
            py68_object_release(runtime, &list->base);
            return status;
        }
        status = py68_list_append_copy(runtime, list,
                                       py68_value_from_object(&pair->base));
        py68_object_release(runtime, &pair->base);
        if (status != PY68_STATUS_OK) {
            py68_object_release(runtime, &list->base);
            return status;
        }
    }
    *result = list;
    return PY68_STATUS_OK;
}

int py68_dict_equal(Py68Runtime *runtime, Py68Dict *left, Py68Dict *right)
{
    Py68U32 index;
    Py68Value other;
    if (left == right) return 1;
    if (left->count != right->count) return 0;
    for (index = 0; index < left->capacity; ++index) {
        if (left->entries[index].used != 1) continue;
        if (py68_dict_get_copy(runtime, right, left->entries[index].key,
                               &other) != PY68_STATUS_OK)
            return 0;
        if (!py68_value_equal(runtime, left->entries[index].value, other)) {
            py68_value_release(runtime, other);
            return 0;
        }
        py68_value_release(runtime, other);
    }
    return 1;
}
