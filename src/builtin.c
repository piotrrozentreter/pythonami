#include "py68k_builtin.h"

#include <stddef.h>

static Py68Status py68_builtin_grow(Py68Runtime *runtime)
{
    Py68U16 capacity = runtime->builtin_capacity == 0 ? 8 :
                       (Py68U16)(runtime->builtin_capacity * 2);
    Py68GlobalEntry *entries;
    if (capacity < runtime->builtin_capacity) return PY68_STATUS_MEMORY_ERROR;
    entries = (Py68GlobalEntry *)py68_realloc(
        &runtime->allocator, PY68_MEM_RUNTIME, runtime->builtins,
        (Py68U32)runtime->builtin_capacity * sizeof(Py68GlobalEntry),
        (Py68U32)capacity * sizeof(Py68GlobalEntry));
    if (entries == NULL) return PY68_STATUS_MEMORY_ERROR;
    runtime->builtins = entries;
    runtime->builtin_capacity = capacity;
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_set_copy(Py68Runtime *runtime, Py68U16 name_index,
                                 Py68Value value)
{
    Py68U16 index;
    for (index = 0; index < runtime->builtin_count; ++index) {
        if (runtime->builtins[index].name_index == name_index) {
            Py68Value old = runtime->builtins[index].value;
            py68_value_retain(value);
            runtime->builtins[index].value = value;
            py68_value_release(runtime, old);
            return PY68_STATUS_OK;
        }
    }
    if (runtime->builtin_count == runtime->builtin_capacity &&
        py68_builtin_grow(runtime) != PY68_STATUS_OK)
        return PY68_STATUS_MEMORY_ERROR;
    runtime->builtins[runtime->builtin_count].name_index = name_index;
    runtime->builtins[runtime->builtin_count].value = value;
    runtime->builtins[runtime->builtin_count].occupied = 1;
    py68_value_retain(value);
    ++runtime->builtin_count;
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_get_copy(Py68Runtime *runtime, Py68U16 name_index,
                                 Py68Value *result)
{
    Py68U16 index;
    for (index = 0; index < runtime->builtin_count; ++index) {
        if (runtime->builtins[index].name_index == name_index) {
            *result = runtime->builtins[index].value;
            py68_value_retain(*result);
            return PY68_STATUS_OK;
        }
    }
    return PY68_STATUS_SOURCE_ERROR;
}

void py68_builtin_clear(Py68Runtime *runtime)
{
    Py68U16 index;
    for (index = 0; index < runtime->builtin_count; ++index)
        py68_value_release(runtime, runtime->builtins[index].value);
    py68_free(&runtime->allocator, PY68_MEM_RUNTIME, runtime->builtins,
              (Py68U32)runtime->builtin_capacity * sizeof(Py68GlobalEntry));
    runtime->builtins = NULL;
    runtime->builtin_count = 0;
    runtime->builtin_capacity = 0;
}