#include "py68k_builtin.h"
#include "py68k_native.h"

#include <stddef.h>
#include <string.h>

static int py68_builtin_name_equal(const Py68GlobalEntry *entry,
                                   const Py68U8 *name, Py68U16 name_length)
{
    return entry->name_length == name_length &&
           memcmp(entry->name, name, name_length) == 0;
}

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

Py68Status py68_builtin_set_copy(Py68Runtime *runtime, const Py68U8 *name,
                                 Py68U16 name_length, Py68Value value)
{
    Py68U16 index;
    if (runtime == NULL || name == NULL) return PY68_STATUS_INTERNAL_ERROR;
    for (index = 0; index < runtime->builtin_count; ++index) {
        if (py68_builtin_name_equal(&runtime->builtins[index], name,
                                    name_length)) {
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
    runtime->builtins[runtime->builtin_count].name = name;
    runtime->builtins[runtime->builtin_count].name_length = name_length;
    runtime->builtins[runtime->builtin_count].value = value;
    runtime->builtins[runtime->builtin_count].occupied = 1;
    py68_value_retain(value);
    ++runtime->builtin_count;
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_get_copy(Py68Runtime *runtime, const Py68U8 *name,
                                 Py68U16 name_length, Py68Value *result)
{
    Py68U16 index;
    if (runtime == NULL || name == NULL || result == NULL)
        return PY68_STATUS_INTERNAL_ERROR;
    for (index = 0; index < runtime->builtin_count; ++index) {
        if (py68_builtin_name_equal(&runtime->builtins[index], name,
                                    name_length)) {
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

typedef struct Py68BuiltinDefinition {
    const char *name;
    Py68U16 minimum_arguments;
    Py68U16 maximum_arguments;
    Py68NativeCallback callback;
} Py68BuiltinDefinition;

static Py68U16 py68_static_strlen(const char *text)
{
    Py68U16 length = 0;
    while (text[length] != '\0') ++length;
    return length;
}

Py68Status py68_builtins_install(Py68Runtime *runtime)
{
    static const Py68BuiltinDefinition definitions[] = {
        { "print", 0, 65535, py68_builtin_print },
        { "len", 1, 1, py68_builtin_len },
        { "range", 1, 3, py68_builtin_range },
        { "list_pop", 1, 1, py68_builtin_list_pop },
        { "list_append", 2, 2, py68_builtin_list_append }
    };
    Py68U16 index;
    Py68Status status;
    Py68NativeFunction *function;

    for (index = 0; index < sizeof(definitions) / sizeof(definitions[0]);
         ++index) {
        status = py68_native_new(runtime, definitions[index].name,
                                 definitions[index].minimum_arguments,
                                 definitions[index].maximum_arguments,
                                 definitions[index].callback, &function);
        if (status != PY68_STATUS_OK) return status;
        status = py68_builtin_set_copy(
            runtime, (const Py68U8 *)definitions[index].name,
            py68_static_strlen(definitions[index].name),
            py68_value_from_object(&function->base));
        py68_object_release(runtime, &function->base);
        if (status != PY68_STATUS_OK) return status;
    }
    return PY68_STATUS_OK;
}
