#include "py68k_global.h"

#include <stddef.h>
#include <string.h>

static int py68_global_name_equal(const Py68GlobalEntry *entry,
                                  const Py68U8 *name, Py68U16 name_length)
{
    return entry->name_length == name_length &&
           memcmp(entry->name, name, name_length) == 0;
}

static Py68Status py68_global_grow(Py68Runtime *runtime)
{
    Py68U16 capacity = runtime->global_capacity == 0 ? 8 :
                       (Py68U16)(runtime->global_capacity * 2);
    Py68GlobalEntry *entries;
    if (capacity < runtime->global_capacity) return PY68_STATUS_MEMORY_ERROR;
    entries = (Py68GlobalEntry *)py68_realloc(
        &runtime->allocator, PY68_MEM_RUNTIME, runtime->globals,
        (Py68U32)runtime->global_capacity * sizeof(Py68GlobalEntry),
        (Py68U32)capacity * sizeof(Py68GlobalEntry));
    if (entries == NULL) return PY68_STATUS_MEMORY_ERROR;
    runtime->globals = entries;
    runtime->global_capacity = capacity;
    return PY68_STATUS_OK;
}

Py68Status py68_global_set_copy(Py68Runtime *runtime, const Py68U8 *name,
                                Py68U16 name_length, Py68Value value)
{
    Py68U16 index;
    if (runtime == NULL || name == NULL) return PY68_STATUS_INTERNAL_ERROR;
    for (index = 0; index < runtime->global_count; ++index) {
        if (runtime->globals[index].occupied &&
            py68_global_name_equal(&runtime->globals[index], name,
                                   name_length)) {
            Py68Value old = runtime->globals[index].value;
            py68_value_retain(value);
            runtime->globals[index].value = value;
            py68_value_release(runtime, old);
            return PY68_STATUS_OK;
        }
    }
    if (runtime->global_count == runtime->global_capacity &&
        py68_global_grow(runtime) != PY68_STATUS_OK)
        return PY68_STATUS_MEMORY_ERROR;
    runtime->globals[runtime->global_count].name = name;
    runtime->globals[runtime->global_count].name_length = name_length;
    runtime->globals[runtime->global_count].value = value;
    runtime->globals[runtime->global_count].occupied = 1;
    py68_value_retain(value);
    ++runtime->global_count;
    return PY68_STATUS_OK;
}

Py68Status py68_global_get_copy(Py68Runtime *runtime, const Py68U8 *name,
                                Py68U16 name_length, Py68Value *result)
{
    Py68U16 index;
    if (runtime == NULL || name == NULL || result == NULL)
        return PY68_STATUS_INTERNAL_ERROR;
    for (index = 0; index < runtime->global_count; ++index) {
        if (runtime->globals[index].occupied &&
            py68_global_name_equal(&runtime->globals[index], name,
                                   name_length)) {
            *result = runtime->globals[index].value;
            py68_value_retain(*result);
            return PY68_STATUS_OK;
        }
    }
    return PY68_STATUS_SOURCE_ERROR;
}

void py68_global_clear(Py68Runtime *runtime)
{
    Py68U16 index;
    if (runtime == NULL) return;
    for (index = 0; index < runtime->global_count; ++index)
        py68_value_release(runtime, runtime->globals[index].value);
    py68_free(&runtime->allocator, PY68_MEM_RUNTIME, runtime->globals,
              (Py68U32)runtime->global_capacity * sizeof(Py68GlobalEntry));
    runtime->globals = NULL;
    runtime->global_count = 0;
    runtime->global_capacity = 0;
}
