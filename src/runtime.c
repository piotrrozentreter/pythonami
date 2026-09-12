#include "py68k_runtime.h"
#include "py68k_frame.h"
#include "py68k_global.h"
#include "py68k_builtin.h"

#include <string.h>

void py68_runtime_initialize_struct(Py68Runtime *runtime)
{
    memset(runtime, 0, sizeof(*runtime));
    py68_allocator_initialize(&runtime->allocator);
}

Py68Status py68_runtime_initialize(Py68Runtime *runtime)
{
    py68_runtime_initialize_struct(runtime);
    runtime->recursion_limit = 64;
    return py68_platform_initialize(runtime);
}

void py68_runtime_shutdown(Py68Runtime *runtime)
{
    py68_frame_unwind(runtime);
    py68_global_clear(runtime);
    py68_builtin_clear(runtime);
    while (runtime->live_objects != NULL) {
        Py68Object *object = runtime->live_objects;
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
