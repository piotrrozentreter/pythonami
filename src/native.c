/* 2026 by Piotr Rozentreter (Rozsoft) */

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
    function->consumes_iterable = 0;
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

static void py68_native_append(char *buffer, Py68U32 capacity, Py68U32 *length,
                               const char *text)
{
    Py68U32 index;
    for (index = 0; text[index] != '\0' && *length < capacity - 1; ++index)
        buffer[(*length)++] = text[index];
    buffer[*length] = '\0';
}

static void py68_native_append_u16(char *buffer, Py68U32 capacity,
                                   Py68U32 *length, Py68U16 value)
{
    char digits[6];
    Py68U32 count = 0;
    do {
        digits[count++] = (char)('0' + (value % 10));
        value = (Py68U16)(value / 10);
    } while (value != 0 && count < sizeof(digits));
    while (count != 0 && *length < capacity - 1)
        buffer[(*length)++] = digits[--count];
    buffer[*length] = '\0';
}

static void py68_native_error(Py68Runtime *runtime, Py68ErrorKind kind,
                              const char *message)
{
    Py68Location location;
    location.offset = 0;
    location.line = 0;
    location.column = 0;
    location.length = 0;
    py68_error_set(&runtime->error, kind, location, NULL, message);
}

/* A builtin called with the wrong number of arguments used to fail without a
   diagnostic, which printed a bare "Exception:" line. */
static void py68_native_arity_error(Py68Runtime *runtime,
                                    Py68NativeFunction *function,
                                    Py68U16 argument_count)
{
    char message[192];
    Py68U32 length = 0;
    message[0] = '\0';
    if (function == NULL) {
        py68_native_error(runtime, PY68_ERROR_TYPE, "callable required");
        return;
    }
    py68_native_append(message, sizeof(message), &length,
                       function->name != NULL ? function->name : "builtin");
    py68_native_append(message, sizeof(message), &length, "() takes ");
    if (function->minimum_arguments == function->maximum_arguments) {
        py68_native_append(message, sizeof(message), &length, "exactly ");
        py68_native_append_u16(message, sizeof(message), &length,
                               function->minimum_arguments);
    } else {
        py68_native_append_u16(message, sizeof(message), &length,
                               function->minimum_arguments);
        py68_native_append(message, sizeof(message), &length, " to ");
        py68_native_append_u16(message, sizeof(message), &length,
                               function->maximum_arguments);
    }
    py68_native_append(message, sizeof(message), &length,
                       function->maximum_arguments == 1 ? " argument, got "
                                                        : " arguments, got ");
    py68_native_append_u16(message, sizeof(message), &length, argument_count);
    py68_native_error(runtime, PY68_ERROR_TYPE, message);
}

Py68Status py68_native_call(Py68NativeFunction *function,
                            Py68Runtime *runtime, Py68U16 argument_count,
                            Py68Value *arguments, Py68Value *result)
{
    Py68Status status;
    if (py68_native_check_arguments(function, argument_count) !=
        PY68_STATUS_OK) {
        py68_native_arity_error(runtime, function, argument_count);
        return PY68_STATUS_RUNTIME_ERROR;
    }
    runtime->active_native = function;
    status = function->callback(runtime, argument_count, arguments, result);
    runtime->active_native = NULL;
    if (status != PY68_STATUS_OK && runtime->error.active == 0) {
        /* The callback failed without recording why; name it rather than
           reporting a blank exception. */
        char message[192];
        Py68U32 length = 0;
        message[0] = '\0';
        py68_native_append(message, sizeof(message), &length,
                           function->name != NULL ? function->name : "builtin");
        py68_native_append(message, sizeof(message), &length,
                           "() rejected its arguments");
        py68_native_error(runtime, PY68_ERROR_TYPE, message);
    }
    return status;
}
