/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_module.h"

#include <stddef.h>
#include <string.h>

static int py68_module_name_equal(const Py68GlobalEntry *entry,
                                  const Py68U8 *name, Py68U16 name_length)
{
    return entry->name_length == name_length &&
           memcmp(entry->name, name, name_length) == 0;
}

static Py68Status py68_module_grow(Py68Runtime *runtime, Py68Module *module)
{
    Py68U16 capacity = module->global_capacity == 0 ? 8 :
                       (Py68U16)(module->global_capacity * 2);
    Py68GlobalEntry *entries;
    if (capacity < module->global_capacity) return PY68_STATUS_MEMORY_ERROR;
    entries = (Py68GlobalEntry *)py68_realloc(
        &runtime->allocator, PY68_MEM_MODULE, module->globals,
        (Py68U32)module->global_capacity * sizeof(Py68GlobalEntry),
        (Py68U32)capacity * sizeof(Py68GlobalEntry));
    if (entries == NULL) return PY68_STATUS_MEMORY_ERROR;
    module->globals = entries;
    module->global_capacity = capacity;
    return PY68_STATUS_OK;
}

Py68Status py68_module_new(Py68Runtime *runtime, const char *name,
                           const char *path, Py68Module **result)
{
    Py68Module *module = (Py68Module *)py68_alloc(&runtime->allocator,
                                                  PY68_MEM_MODULE,
                                                  sizeof(Py68Module));
    if (module == NULL) return PY68_STATUS_MEMORY_ERROR;
    module->base.type = PY68_OBJECT_MODULE;
    module->base.flags = 0;
    module->base.reference_count = 1;
    module->base.next_object = runtime->live_objects;
    runtime->live_objects = &module->base;
    module->name[0] = '\0';
    module->path[0] = '\0';
    if (name != NULL) {
        strncpy(module->name, name, sizeof(module->name) - 1);
        module->name[sizeof(module->name) - 1] = '\0';
    }
    if (path != NULL) {
        strncpy(module->path, path, sizeof(module->path) - 1);
        module->path[sizeof(module->path) - 1] = '\0';
    }
    module->owned_source = NULL;
    module->owned_source_length = 0;
    py68_code_initialize(&module->owned_code);
    module->globals = NULL;
    module->global_count = 0;
    module->global_capacity = 0;
    module->native_seg = NULL;
    *result = module;
    return PY68_STATUS_OK;
}

Py68Status py68_module_set(Py68Runtime *runtime, Py68Module *module,
                           const Py68U8 *name, Py68U16 name_length,
                           Py68Value value)
{
    Py68U16 index;
    for (index = 0; index < module->global_count; ++index) {
        if (module->globals[index].occupied &&
            py68_module_name_equal(&module->globals[index], name,
                                   name_length)) {
            Py68Value old = module->globals[index].value;
            py68_value_retain(value);
            module->globals[index].value = value;
            py68_value_release(runtime, old);
            return PY68_STATUS_OK;
        }
    }
    if (module->global_count == module->global_capacity &&
        py68_module_grow(runtime, module) != PY68_STATUS_OK)
        return PY68_STATUS_MEMORY_ERROR;
    module->globals[module->global_count].name = name;
    module->globals[module->global_count].name_length = name_length;
    module->globals[module->global_count].value = value;
    module->globals[module->global_count].occupied = 1;
    py68_value_retain(value);
    ++module->global_count;
    return PY68_STATUS_OK;
}

Py68Status py68_module_get(Py68Runtime *runtime, Py68Module *module,
                           const Py68U8 *name, Py68U16 name_length,
                           Py68Value *result)
{
    Py68U16 index;
    if (runtime == NULL || module == NULL || result == NULL)
        return PY68_STATUS_INTERNAL_ERROR;
    for (index = 0; index < module->global_count; ++index) {
        if (module->globals[index].occupied &&
            py68_module_name_equal(&module->globals[index], name,
                                   name_length)) {
            *result = module->globals[index].value;
            py68_value_retain(*result);
            return PY68_STATUS_OK;
        }
    }
    return PY68_STATUS_SOURCE_ERROR;
}

void py68_module_clear(Py68Runtime *runtime, Py68Module *module)
{
    Py68U16 index;
    if (module == NULL) return;
    for (index = 0; index < module->global_count; ++index)
        py68_value_release(runtime, module->globals[index].value);
    py68_free(&runtime->allocator, PY68_MEM_MODULE, module->globals,
              (Py68U32)module->global_capacity * sizeof(Py68GlobalEntry));
    /* Drop function objects before freeing the bytecode they point into. */
    py68_code_destroy(&runtime->allocator, &module->owned_code);
    py68_free(&runtime->allocator, PY68_MEM_SOURCE, module->owned_source,
              module->owned_source_length + 1);
    module->owned_source = NULL;
    module->owned_source_length = 0;
    module->globals = NULL;
    module->global_count = 0;
    module->global_capacity = 0;
}
