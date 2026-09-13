#include "py68k_intern.h"
#include "py68k_memory.h"
#include "py68k_object.h"
#include "py68k_runtime.h"

#include <string.h>

static Py68Status py68_intern_grow(struct Py68Runtime *runtime,
                                   Py68InternTable *table)
{
    Py68InternEntry *replacement;
    Py68U16 capacity;
    Py68U16 index;
    Py68U16 slot;

    capacity = table->capacity == 0 ? 16 :
               (Py68U16)(table->capacity * 2);
    if (capacity < table->capacity || capacity > 32768U)
        return PY68_STATUS_MEMORY_ERROR;
    replacement = (Py68InternEntry *)py68_alloc(
        &runtime->allocator, PY68_MEM_SYMBOL,
        (Py68U32)capacity * (Py68U32)sizeof(Py68InternEntry));
    if (replacement == NULL) return PY68_STATUS_MEMORY_ERROR;
    memset(replacement, 0,
           (Py68U32)capacity * (Py68U32)sizeof(Py68InternEntry));
    for (index = 0; index < table->capacity; ++index) {
        if (table->entries[index].string != NULL) {
            slot = (Py68U16)(table->entries[index].hash &
                             (Py68U32)(capacity - 1));
            while (replacement[slot].string != NULL)
                slot = (Py68U16)((slot + 1) & (capacity - 1));
            replacement[slot] = table->entries[index];
        }
    }
    py68_free(&runtime->allocator, PY68_MEM_SYMBOL, table->entries,
              (Py68U32)table->capacity * (Py68U32)sizeof(Py68InternEntry));
    table->entries = replacement;
    table->capacity = capacity;
    return PY68_STATUS_OK;
}

void py68_intern_initialize(Py68InternTable *table)
{
    table->entries = NULL;
    table->count = 0;
    table->capacity = 0;
}

void py68_intern_destroy(struct Py68Runtime *runtime,
                         Py68InternTable *table)
{
    Py68U16 index;
    for (index = 0; index < table->capacity; ++index) {
        if (table->entries[index].string != NULL)
            py68_object_release(runtime,
                                &table->entries[index].string->base);
    }
    py68_free(&runtime->allocator, PY68_MEM_SYMBOL, table->entries,
              (Py68U32)table->capacity * (Py68U32)sizeof(Py68InternEntry));
    py68_intern_initialize(table);
}

Py68Status py68_intern_get_copy(struct Py68Runtime *runtime,
                                Py68InternTable *table,
                                const char *data, Py68U32 length,
                                Py68String **result)
{
    Py68U32 hash;
    Py68U16 slot;
    Py68String *string;
    Py68Status status;

    if (runtime == NULL || table == NULL || result == NULL)
        return PY68_STATUS_INTERNAL_ERROR;
    hash = py68_string_hash_bytes(data, length);
    if (table->capacity == 0 ||
        ((Py68U32)(table->count + 1) * 10U >
         (Py68U32)table->capacity * 7U)) {
        status = py68_intern_grow(runtime, table);
        if (status != PY68_STATUS_OK) return status;
    }
    slot = (Py68U16)(hash & (Py68U32)(table->capacity - 1));
    for (;;) {
        if (table->entries[slot].string == NULL) break;
        if (table->entries[slot].hash == hash &&
            table->entries[slot].string->length == length &&
            (length == 0 || memcmp(table->entries[slot].string->data,
                                   data, length) == 0)) {
            string = table->entries[slot].string;
            py68_object_retain(&string->base);
            *result = string;
            return PY68_STATUS_OK;
        }
        slot = (Py68U16)((slot + 1) & (table->capacity - 1));
    }
    status = py68_string_new_copy(runtime, data, length, &string);
    if (status != PY68_STATUS_OK) return status;
    table->entries[slot].string = string;
    table->entries[slot].hash = hash;
    ++table->count;
    py68_object_retain(&string->base);
    *result = string;
    return PY68_STATUS_OK;
}