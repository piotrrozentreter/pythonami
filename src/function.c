/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_function.h"
#include "py68k_runtime.h"

#include <stddef.h>

Py68Status py68_function_new(Py68Runtime *runtime, Py68Code *code,
                             Py68U16 argument_count, Py68U16 local_count,
                             struct Py68Module *globals_owner,
                             Py68Function **result)
{
    Py68Function *function = (Py68Function *)py68_alloc(
        &runtime->allocator, PY68_MEM_FUNCTION, sizeof(Py68Function));
    if (function == NULL) return PY68_STATUS_MEMORY_ERROR;
    function->base.type = PY68_OBJECT_FUNCTION;
    function->base.flags = 0;
    function->base.reference_count = 1;
    function->base.next_object = runtime->live_objects;
    runtime->live_objects = &function->base;
    function->code = code;
    function->globals_owner = globals_owner;
    function->argument_count = argument_count;
    function->local_count = local_count;
    *result = function;
    return PY68_STATUS_OK;
}

Py68Status py68_function_check_arguments(Py68Function *function,
                                         Py68U16 argument_count)
{
    return function != NULL && function->argument_count == argument_count
           ? PY68_STATUS_OK : PY68_STATUS_RUNTIME_ERROR;
}
