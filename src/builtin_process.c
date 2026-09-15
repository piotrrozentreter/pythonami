/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_builtin.h"
#include "py68k_platform.h"
#include "py68k_string.h"

#include <string.h>

static void py68_process_error(Py68Runtime *runtime, Py68ErrorKind kind,
                               const char *message)
{
    Py68Location location;
    location.offset = 0;
    location.line = 0;
    location.column = 0;
    location.length = 0;
    py68_error_set(&runtime->error, kind, location, NULL, message);
}

Py68Status py68_builtin_system(Py68Runtime *runtime, Py68U16 argument_count,
                               Py68Value *arguments, Py68Value *result)
{
    Py68String *command;
    char *command_copy;
    Py68I32 return_code;
    Py68Status status;
    Py68U32 index;

    if (argument_count != 1 || arguments[0].type != PY68_VALUE_OBJECT ||
        arguments[0].as.object == NULL ||
        arguments[0].as.object->type != PY68_OBJECT_STRING) {
        py68_process_error(runtime, PY68_ERROR_TYPE,
                           "os.system expects a command string");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    command = (Py68String *)arguments[0].as.object;
    for (index = 0; index < command->length; ++index) {
        if (command->data[index] == '\0') {
            py68_process_error(runtime, PY68_ERROR_VALUE,
                               "os.system command contains NUL");
            return PY68_STATUS_RUNTIME_ERROR;
        }
    }
    if (command->length == 0) {
        py68_process_error(runtime, PY68_ERROR_VALUE,
                           "os.system command must not be empty");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    command_copy = (char *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP,
                                      command->length + 1);
    if (command_copy == NULL) return PY68_STATUS_MEMORY_ERROR;
    memcpy(command_copy, command->data, command->length);
    command_copy[command->length] = '\0';
    status = py68_platform_system(runtime, command_copy, &return_code);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, command_copy,
              command->length + 1);
    if (status != PY68_STATUS_OK) {
        py68_process_error(runtime, PY68_ERROR_IO,
                           "os.system could not start the command");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    *result = py68_value_int(return_code);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_popen(Py68Runtime *runtime, Py68U16 argument_count,
                              Py68Value *arguments, Py68Value *result)
{
    Py68String *command;
    char *command_copy;
    Py68I32 return_code;
    Py68U8 *captured;
    Py68U32 captured_length;
    Py68String *string;
    Py68Status status;
    Py68U32 index;

    if (argument_count != 1 || arguments[0].type != PY68_VALUE_OBJECT ||
        arguments[0].as.object == NULL ||
        arguments[0].as.object->type != PY68_OBJECT_STRING) {
        py68_process_error(runtime, PY68_ERROR_TYPE,
                           "os.popen expects a command string");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    command = (Py68String *)arguments[0].as.object;
    for (index = 0; index < command->length; ++index) {
        if (command->data[index] == '\0') {
            py68_process_error(runtime, PY68_ERROR_VALUE,
                               "os.popen command contains NUL");
            return PY68_STATUS_RUNTIME_ERROR;
        }
    }
    if (command->length == 0) {
        py68_process_error(runtime, PY68_ERROR_VALUE,
                           "os.popen command must not be empty");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    command_copy = (char *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP,
                                      command->length + 1);
    if (command_copy == NULL) return PY68_STATUS_MEMORY_ERROR;
    memcpy(command_copy, command->data, command->length);
    command_copy[command->length] = '\0';
    status = py68_platform_system_capture(runtime, command_copy, &return_code,
                                          &captured, &captured_length);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, command_copy,
              command->length + 1);
    if (status != PY68_STATUS_OK) {
        py68_process_error(runtime, PY68_ERROR_IO,
                           "os.popen could not capture command output");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    status = py68_string_new_copy(runtime, (const char *)captured,
                                  captured_length, &string);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, captured,
              captured_length + 1);
    if (status != PY68_STATUS_OK) return status;
    *result = py68_value_from_object(&string->base);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_check_break(Py68Runtime *runtime,
                                    Py68U16 argument_count,
                                    Py68Value *arguments, Py68Value *result)
{
    (void)argument_count;
    (void)arguments;
    /* Consumes the break: the script takes responsibility for stopping. */
    *result = py68_value_bool(
        py68_platform_poll(runtime, PY68_POLL_BREAK) != PY68_STATUS_OK);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_yield_cpu(Py68Runtime *runtime,
                                  Py68U16 argument_count,
                                  Py68Value *arguments, Py68Value *result)
{
    (void)argument_count;
    (void)arguments;
    py68_platform_poll(runtime, PY68_POLL_YIELD);
    *result = py68_value_none();
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_set_poll_interval(Py68Runtime *runtime,
                                          Py68U16 argument_count,
                                          Py68Value *arguments,
                                          Py68Value *result)
{
    (void)argument_count;
    if (arguments[0].type != PY68_VALUE_INT) {
        py68_process_error(runtime, PY68_ERROR_TYPE,
                           "set_poll_interval expects an int");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (arguments[0].as.integer < 0) {
        py68_process_error(runtime, PY68_ERROR_VALUE,
                           "set_poll_interval expects a non-negative int");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    runtime->poll_interval = (Py68U32)arguments[0].as.integer;
    runtime->poll_counter = runtime->poll_interval;
    *result = py68_value_none();
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_get_poll_interval(Py68Runtime *runtime,
                                          Py68U16 argument_count,
                                          Py68Value *arguments,
                                          Py68Value *result)
{
    (void)argument_count;
    (void)arguments;
    *result = py68_value_int((Py68I32)runtime->poll_interval);
    return PY68_STATUS_OK;
}