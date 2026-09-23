/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_runtime.h"
#define PY68K_EXT_OMIT_HELPERS
#include "py68k_ext.h"
#include "py68k_frame.h"
#include "py68k_global.h"
#include "py68k_builtin.h"
#include "py68k_import.h"
#include "py68k_list.h"
#include "py68k_module.h"
#include "py68k_value.h"

#include <string.h>

void py68_runtime_initialize_struct(Py68Runtime *runtime)
{
    memset(runtime, 0, sizeof(*runtime));
    py68_allocator_initialize(&runtime->allocator);
}

Py68Status py68_runtime_initialize(Py68Runtime *runtime)
{
    py68_runtime_initialize_struct(runtime);
    py68_ext_services_install(runtime);
    runtime->recursion_limit = 64;
    runtime->poll_interval = PY68_POLL_INTERVAL_DEFAULT;
    runtime->poll_counter = PY68_POLL_INTERVAL_DEFAULT;
    return py68_platform_initialize(runtime);
}

void py68_runtime_shutdown(Py68Runtime *runtime)
{
    Py68U16 index;
    py68_frame_unwind(runtime);
    py68_value_release(runtime, runtime->current_exception);
    runtime->current_exception = py68_value_none();
    for (index = 0; index < runtime->import_count; ++index)
        py68_object_release(runtime, runtime->import_modules[index]);
    py68_free(&runtime->allocator, PY68_MEM_MODULE, runtime->import_modules,
              (Py68U32)runtime->import_capacity * sizeof(Py68Object *));
    runtime->import_modules = NULL;
    runtime->import_count = 0;
    runtime->import_capacity = 0;
    if (runtime->sys_module != NULL) {
        py68_object_release(runtime, runtime->sys_module);
        runtime->sys_module = NULL;
    }
    runtime->sys_path = NULL;
    if (runtime->sys_argv != NULL) {
        py68_object_release(runtime, &runtime->sys_argv->base);
        runtime->sys_argv = NULL;
    }
    py68_global_clear(runtime);
    py68_builtin_clear(runtime);
    /* Oldest first: containers were allocated before their items, so a
       leftover list/module can release children before those children are
       force-freed as independent live objects. */
    while (runtime->live_objects != NULL) {
        Py68Object *object = runtime->live_objects;
        while (object->next_object != NULL)
            object = object->next_object;
        object->reference_count = 1;
        py68_object_release(runtime, object);
    }
    py68_platform_shutdown(runtime);
    py68_free(&runtime->allocator, PY68_MEM_STACK,
              runtime->value_stack,
              (Py68U32)(runtime->value_stack_capacity * sizeof(Py68Value)));
    py68_free(&runtime->allocator, PY68_MEM_STACK,
              runtime->frames,
              (Py68U32)(runtime->frame_capacity * sizeof(Py68Frame)));
    runtime->value_stack = NULL;
    runtime->frames = NULL;
    runtime->value_stack_count = 0;
    runtime->frame_count = 0;
    runtime->live_objects = NULL;
}
