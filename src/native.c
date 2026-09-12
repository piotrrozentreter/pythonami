#include "py68k_native.h"
#include "py68k_runtime.h"

#include <stddef.h>

Py68Status py68_native_new(Py68Runtime *runtime, const char *name,
                           Py68U16 minimum_arguments,
                           Py68U16 maximum_arguments,
                           Py68NativeCallback callback,
                           Py68NativeFunction **result)
{
    Py68NativeFunction *function = (Py68NativeFunction *)py68_alloc(
        &runtime->allocator, PY68_MEM_FUNCTION, sizeof(Py68NativeFunction));
    if (function == NULL) return PY68_STATUS_MEMORY_ERROR;
    function->base.type = PY68_OBJECT_NATIVE_FUNCTION;
    function->base.flags = 0;
    function->base.reference_count = 1;
    function->base.next_object = runtime->live_objects;
    runtime->live_objects = &function->base;
    function->name = name;
    function->minimum_arguments = minimum_arguments;
    function->maximum_arguments = maximum_arguments;
    function->callback = callback;
    *result = function;
    return PY68_STATUS_OK;
}

Py68Status py68_native_check_arguments(Py68NativeFunction *function,
                                       Py68U16 argument_count)
{
    return function != NULL && argument_count >= function->minimum_arguments &&
           argument_count <= function->maximum_arguments
           ? PY68_STATUS_OK : PY68_STATUS_RUNTIME_ERROR;
}

Py68Status py68_native_call(Py68NativeFunction *function,
                            Py68Runtime *runtime, Py68U16 argument_count,
                            Py68Value *arguments, Py68Value *result)
{
    if (py68_native_check_arguments(function, argument_count) != PY68_STATUS_OK)
        return PY68_STATUS_RUNTIME_ERROR;
    return function->callback(runtime, argument_count, arguments, result);
}
