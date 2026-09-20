/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_builtin.h"
#include "py68k_string.h"
#include "py68k_list.h"
#include "py68k_tuple.h"
#include "py68k_dict.h"
#include "py68k_set.h"
#include "py68k_range.h"
#include "py68k_exception.h"
#include "py68k_float.h"
#include "py68k_native.h"
#include "py68k_value.h"

#include <stddef.h>
#include <string.h>

static Py68Status py68_print_int(Py68Runtime *runtime, Py68I32 integer)
{
    char buffer[12];
    Py68U32 position = sizeof(buffer) - 1;
    Py68U32 magnitude;
    int negative = integer < 0;
    if (negative) {
        magnitude = (Py68U32)(-(integer + 1));
        ++magnitude;
    } else {
        magnitude = (Py68U32)integer;
    }
    buffer[position] = '\0';
    do {
        buffer[--position] = (char)('0' + (magnitude % 10));
        magnitude /= 10;
    } while (magnitude != 0);
    if (negative) buffer[--position] = '-';
    return py68_platform_write_stdout(runtime, buffer + position,
                                      (Py68U32)(sizeof(buffer) - 1 - position));
}

static Py68Status py68_print_float(Py68Runtime *runtime, Py68Value value)
{
    char buffer[32];
    Py68I32 whole;
    Py68U32 written = py68_f32_format(py68_value_float_bits_get(value),
                                      buffer, (Py68U32)sizeof(buffer));
    if (written == 0) {
        buffer[0] = '\0';
        if (py68_f32_to_i32_trunc(py68_value_float_bits_get(value), &whole)) {
            Py68Status status = py68_print_int(runtime, whole);
            if (status != PY68_STATUS_OK) return status;
            return py68_platform_write_stdout(runtime, ".0", 2);
        }
        return PY68_STATUS_RUNTIME_ERROR;
    }
    return py68_platform_write_stdout(runtime, buffer, written);
}

static Py68Status py68_builtin_string_length(Py68Runtime *runtime,
                                             Py68Value value,
                                             Py68Value *result)
{
    Py68String *string;
    Py68Location location;
    location.offset = 0;
    location.line = 0;
    location.column = 0;
    location.length = 0;
    if (value.type != PY68_VALUE_OBJECT || value.as.object == NULL ||
        value.as.object->type != PY68_OBJECT_STRING) {
        py68_error_set(&runtime->error, PY68_ERROR_TYPE,
                       location, NULL,
                       "len argument must be a string or list");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    string = (Py68String *)value.as.object;
    *result = py68_value_int((Py68I32)string->length);
    return PY68_STATUS_OK;
}

static Py68Status py68_builtin_list_length(Py68Runtime *runtime,
                                           Py68Value value,
                                           Py68Value *result)
{
    Py68List *list;
    Py68Location location;
    location.offset = 0;
    location.line = 0;
    location.column = 0;
    location.length = 0;
    if (value.type != PY68_VALUE_OBJECT || value.as.object == NULL ||
        value.as.object->type != PY68_OBJECT_LIST) {
        py68_error_set(&runtime->error, PY68_ERROR_TYPE,
                       location, NULL,
                       "len argument must be a string or list");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    list = (Py68List *)value.as.object;
    *result = py68_value_int((Py68I32)list->count);
    return PY68_STATUS_OK;
}

static Py68Status py68_builtin_range_list(Py68Runtime *runtime,
                                          Py68I32 start,
                                          Py68I32 stop,
                                          Py68I32 step,
                                          Py68Value *result)
{
    Py68List *list;
    Py68I32 value;
    Py68Status status;
    Py68Location location;
    location.offset = 0;
    location.line = 0;
    location.column = 0;
    location.length = 0;
    if (step == 0) {
        py68_error_set(&runtime->error, PY68_ERROR_VALUE,
                       location, NULL,
                       "range step cannot be zero");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    status = py68_list_new(runtime, &list);
    if (status != PY68_STATUS_OK) return status;
    for (value = start;
         (step > 0 && value < stop) || (step < 0 && value > stop);
         value += step) {
        status = py68_list_append_copy(runtime, list, py68_value_int(value));
        if (status != PY68_STATUS_OK) {
            py68_object_release(runtime, &list->base);
            return status;
        }
    }
    *result = py68_value_from_object(&list->base);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_len(Py68Runtime *runtime, Py68U16 argument_count,
                            Py68Value *arguments, Py68Value *result)
{
    Py68Location location;
    location.offset = 0;
    location.line = 0;
    location.column = 0;
    location.length = 0;
    if (argument_count != 1) {
        py68_error_set(&runtime->error, PY68_ERROR_TYPE,
                       location, NULL,
                       "len expects exactly one argument");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (arguments[0].type == PY68_VALUE_OBJECT && arguments[0].as.object != NULL &&
        arguments[0].as.object->type == PY68_OBJECT_STRING)
        return py68_builtin_string_length(runtime, arguments[0], result);
    if (arguments[0].type == PY68_VALUE_OBJECT && arguments[0].as.object != NULL &&
        arguments[0].as.object->type == PY68_OBJECT_LIST)
        return py68_builtin_list_length(runtime, arguments[0], result);
    if (arguments[0].type == PY68_VALUE_OBJECT && arguments[0].as.object != NULL &&
        arguments[0].as.object->type == PY68_OBJECT_TUPLE) {
        *result = py68_value_int((Py68I32)((Py68Tuple *)arguments[0].as.object)->count);
        return PY68_STATUS_OK;
    }
    if (arguments[0].type == PY68_VALUE_OBJECT && arguments[0].as.object != NULL &&
        arguments[0].as.object->type == PY68_OBJECT_DICT) {
        *result = py68_value_int((Py68I32)((Py68Dict *)arguments[0].as.object)->count);
        return PY68_STATUS_OK;
    }
    if (arguments[0].type == PY68_VALUE_OBJECT && arguments[0].as.object != NULL &&
        arguments[0].as.object->type == PY68_OBJECT_SET) {
        *result = py68_value_int((Py68I32)((Py68Set *)arguments[0].as.object)->count);
        return PY68_STATUS_OK;
    }
    py68_error_set(&runtime->error, PY68_ERROR_TYPE,
                   location, NULL,
                   "len argument must be a string, list, tuple, dict, or set");
    return PY68_STATUS_RUNTIME_ERROR;
}

Py68Status py68_builtin_range(Py68Runtime *runtime, Py68U16 argument_count,
                              Py68Value *arguments, Py68Value *result)
{
    Py68I32 start = 0;
    Py68I32 stop;
    Py68I32 step = 1;
    Py68Location location;
    location.offset = 0;
    location.line = 0;
    location.column = 0;
    location.length = 0;
    if (argument_count < 1 || argument_count > 3) {
        py68_error_set(&runtime->error, PY68_ERROR_TYPE,
                       location, NULL,
                       "range expects 1 to 3 integer arguments");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (arguments[0].type != PY68_VALUE_INT ||
        (argument_count >= 2 && arguments[1].type != PY68_VALUE_INT) ||
        (argument_count == 3 && arguments[2].type != PY68_VALUE_INT)) {
        py68_error_set(&runtime->error, PY68_ERROR_TYPE,
                       location, NULL,
                       "range arguments must be integers");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (argument_count == 1) {
        stop = arguments[0].as.integer;
    } else {
        start = arguments[0].as.integer;
        stop = arguments[1].as.integer;
        if (argument_count == 3) step = arguments[2].as.integer;
    }
    return py68_builtin_range_list(runtime, start, stop, step, result);
}

Py68Status py68_builtin_list_pop(Py68Runtime *runtime, Py68U16 argument_count,
                                Py68Value *arguments, Py68Value *result)
{
    Py68List *list;
    Py68Value value;
    Py68Location location;
    location.offset = 0;
    location.line = 0;
    location.column = 0;
    location.length = 0;
    if (argument_count != 1 || arguments[0].type != PY68_VALUE_OBJECT ||
        arguments[0].as.object == NULL ||
        arguments[0].as.object->type != PY68_OBJECT_LIST) {
        py68_error_set(&runtime->error, PY68_ERROR_TYPE,
                       location, NULL,
                       "list_pop expects a list");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    list = (Py68List *)arguments[0].as.object;
    if (list->count == 0) {
        py68_error_set(&runtime->error, PY68_ERROR_INDEX,
                       location, NULL,
                       "list_pop from empty list");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    value = list->items[list->count - 1];
    py68_value_retain(value);
    list->count -= 1;
    *result = value;
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_list_append(Py68Runtime *runtime,
                                    Py68U16 argument_count,
                                    Py68Value *arguments, Py68Value *result)
{
    Py68List *list;
    Py68Location location;
    Py68Status status;
    location.offset = 0;
    location.line = 0;
    location.column = 0;
    location.length = 0;
    if (argument_count != 2 || arguments[0].type != PY68_VALUE_OBJECT ||
        arguments[0].as.object == NULL ||
        arguments[0].as.object->type != PY68_OBJECT_LIST) {
        py68_error_set(&runtime->error, PY68_ERROR_TYPE,
                       location, NULL,
                       "list_append expects a list and a value");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    list = (Py68List *)arguments[0].as.object;
    status = py68_list_append_copy(runtime, list, arguments[1]);
    if (status != PY68_STATUS_OK) return status;
    *result = py68_value_none();
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_print(Py68Runtime *runtime, Py68U16 argument_count,
                              Py68Value *arguments, Py68Value *result)
{
    Py68U16 index;
    Py68Status status;
    const char *text;
    Py68U32 length;
    for (index = 0; index < argument_count; ++index) {
        if (index != 0) {
            status = py68_platform_write_stdout(runtime, " ", 1);
            if (status != PY68_STATUS_OK) return status;
        }
        if (arguments[index].type == PY68_VALUE_INT) {
            status = py68_print_int(runtime, arguments[index].as.integer);
        } else if (arguments[index].type == PY68_VALUE_BOOL) {
            text = arguments[index].as.integer ? "True" : "False";
            status = py68_platform_write_stdout(runtime, text,
                                                (Py68U32)(arguments[index].as.integer ? 4 : 5));
        } else if (arguments[index].type == PY68_VALUE_NONE) {
            status = py68_platform_write_stdout(runtime, "None", 4);
        } else if (arguments[index].type == PY68_VALUE_FLOAT) {
            status = py68_print_float(runtime, arguments[index]);
        } else if (arguments[index].type == PY68_VALUE_OBJECT &&
                   arguments[index].as.object != NULL &&
                   arguments[index].as.object->type == PY68_OBJECT_STRING) {
            Py68String *string = (Py68String *)arguments[index].as.object;
            length = string->length;
            status = py68_platform_write_stdout(runtime, string->data, length);
        } else if (arguments[index].type == PY68_VALUE_OBJECT &&
                   arguments[index].as.object != NULL &&
                   arguments[index].as.object->type == PY68_OBJECT_LIST) {
            Py68List *list = (Py68List *)arguments[index].as.object;
            Py68U32 item;
            status = py68_platform_write_stdout(runtime, "[", 1);
            for (item = 0; status == PY68_STATUS_OK && item < list->count; ++item) {
                if (item != 0) status = py68_platform_write_stdout(runtime, ", ", 2);
                if (status == PY68_STATUS_OK && list->items[item].type == PY68_VALUE_INT)
                    status = py68_print_int(runtime, list->items[item].as.integer);
                else if (status == PY68_STATUS_OK && list->items[item].type == PY68_VALUE_BOOL)
                    status = py68_platform_write_stdout(runtime,
                        list->items[item].as.integer ? "True" : "False",
                        list->items[item].as.integer ? 4 : 5);
                else if (status == PY68_STATUS_OK && list->items[item].type == PY68_VALUE_NONE)
                    status = py68_platform_write_stdout(runtime, "None", 4);
                else if (status == PY68_STATUS_OK &&
                         list->items[item].type == PY68_VALUE_OBJECT &&
                         list->items[item].as.object != NULL &&
                         list->items[item].as.object->type == PY68_OBJECT_STRING) {
                    Py68String *string =
                        (Py68String *)list->items[item].as.object;
                    status = py68_platform_write_stdout(runtime, "\"", 1);
                    if (status == PY68_STATUS_OK)
                        status = py68_platform_write_stdout(
                            runtime, string->data, string->length);
                    if (status == PY68_STATUS_OK)
                        status = py68_platform_write_stdout(runtime, "\"", 1);
                }
                else status = PY68_STATUS_RUNTIME_ERROR;
            }
            if (status == PY68_STATUS_OK) status = py68_platform_write_stdout(runtime, "]", 1);
        } else if (arguments[index].type == PY68_VALUE_OBJECT &&
                   arguments[index].as.object != NULL &&
                   arguments[index].as.object->type == PY68_OBJECT_TUPLE) {
            Py68Tuple *tuple = (Py68Tuple *)arguments[index].as.object;
            Py68U32 item;
            status = py68_platform_write_stdout(runtime, "(", 1);
            for (item = 0; status == PY68_STATUS_OK && item < tuple->count; ++item) {
                if (item != 0) status = py68_platform_write_stdout(runtime, ", ", 2);
                if (status == PY68_STATUS_OK && tuple->items[item].type == PY68_VALUE_INT)
                    status = py68_print_int(runtime, tuple->items[item].as.integer);
                else if (status == PY68_STATUS_OK && tuple->items[item].type == PY68_VALUE_BOOL)
                    status = py68_platform_write_stdout(runtime,
                        tuple->items[item].as.integer ? "True" : "False",
                        tuple->items[item].as.integer ? 4 : 5);
                else if (status == PY68_STATUS_OK &&
                         tuple->items[item].type == PY68_VALUE_FLOAT)
                    status = py68_print_float(runtime, tuple->items[item]);
                else if (status == PY68_STATUS_OK &&
                         tuple->items[item].type == PY68_VALUE_NONE)
                    status = py68_platform_write_stdout(runtime, "None", 4);
                else if (status == PY68_STATUS_OK &&
                         tuple->items[item].type == PY68_VALUE_OBJECT &&
                         tuple->items[item].as.object != NULL &&
                         tuple->items[item].as.object->type ==
                             PY68_OBJECT_STRING) {
                    Py68String *string =
                        (Py68String *)tuple->items[item].as.object;
                    status = py68_platform_write_stdout(runtime, "\"", 1);
                    if (status == PY68_STATUS_OK)
                        status = py68_platform_write_stdout(
                            runtime, string->data, string->length);
                    if (status == PY68_STATUS_OK)
                        status = py68_platform_write_stdout(runtime, "\"", 1);
                }
                else status = PY68_STATUS_RUNTIME_ERROR;
            }
            if (status == PY68_STATUS_OK && tuple->count == 1)
                status = py68_platform_write_stdout(runtime, ",", 1);
            if (status == PY68_STATUS_OK)
                status = py68_platform_write_stdout(runtime, ")", 1);
        } else if (arguments[index].type == PY68_VALUE_OBJECT &&
                   arguments[index].as.object != NULL &&
                   arguments[index].as.object->type == PY68_OBJECT_EXCEPTION) {
            Py68Exception *exception =
                (Py68Exception *)arguments[index].as.object;
            const char *kind = py68_error_kind_name(exception->kind);
            status = py68_platform_write_stdout(runtime, kind,
                                                (Py68U32)strlen(kind));
            if (status == PY68_STATUS_OK)
                status = py68_platform_write_stdout(runtime, ": ", 2);
            if (status == PY68_STATUS_OK)
                status = py68_platform_write_stdout(
                    runtime, exception->message,
                    (Py68U32)strlen(exception->message));
        } else {
            Py68Location location;
            location.offset = 0;
            location.line = 0;
            location.column = 0;
            location.length = 0;
            py68_error_set(&runtime->error, PY68_ERROR_TYPE,
                           location, NULL,
                           "print value is not supported");
            return PY68_STATUS_RUNTIME_ERROR;
        }
        if (status != PY68_STATUS_OK) return status;
    }
    status = py68_platform_write_stdout(runtime, "\n", 1);
    if (status == PY68_STATUS_OK) *result = py68_value_none();
    return status;
}

Py68Status py68_builtin_input(Py68Runtime *runtime, Py68U16 argument_count,
                              Py68Value *arguments, Py68Value *result)
{
    Py68U8 *data;
    Py68U32 length;
    Py68String *string;
    Py68Status status;
    Py68Location location;

    location.offset = 0;
    location.line = 0;
    location.column = 0;
    location.length = 0;

    if (argument_count > 1) {
        py68_error_set(&runtime->error, PY68_ERROR_TYPE, location, NULL,
                       "input expects at most one prompt string");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (argument_count == 1) {
        if (arguments[0].type != PY68_VALUE_OBJECT ||
            arguments[0].as.object == NULL ||
            arguments[0].as.object->type != PY68_OBJECT_STRING) {
            py68_error_set(&runtime->error, PY68_ERROR_TYPE, location, NULL,
                           "input prompt must be a string");
            return PY68_STATUS_RUNTIME_ERROR;
        }
        {
            Py68String *prompt = (Py68String *)arguments[0].as.object;
            status = py68_platform_write_stdout(runtime, prompt->data,
                                                prompt->length);
            if (status != PY68_STATUS_OK) return status;
            py68_platform_flush_stdout();
        }
    }
    status = py68_platform_read_stdin_line(runtime, &data, &length);
    if (status == PY68_STATUS_SOURCE_ERROR) {
        py68_error_set(&runtime->error, PY68_ERROR_IO, location, NULL,
                       "EOF when reading a line");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (status != PY68_STATUS_OK) {
        py68_error_set(&runtime->error, PY68_ERROR_IO, location, NULL,
                       "input failed");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    status = py68_string_new_copy(runtime, (const char *)data, length, &string);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, data, length + 1);
    if (status != PY68_STATUS_OK) return status;
    *result = py68_value_from_object(&string->base);
    return PY68_STATUS_OK;
}

static void py68_builtin_error(Py68Runtime *runtime, Py68ErrorKind kind,
                               const char *message)
{
    Py68Location location;
    location.offset = 0;
    location.line = 0;
    location.column = 0;
    location.length = 0;
    py68_error_set(&runtime->error, kind, location, NULL, message);
}

static int py68_builtin_truth(Py68Value value)
{
    if (value.type == PY68_VALUE_NONE) return 0;
    if (value.type == PY68_VALUE_BOOL || value.type == PY68_VALUE_INT)
        return value.as.integer != 0;
    if (value.type == PY68_VALUE_OBJECT && value.as.object != NULL) {
        if (value.as.object->type == PY68_OBJECT_STRING)
            return ((Py68String *)value.as.object)->length != 0;
        if (value.as.object->type == PY68_OBJECT_LIST)
            return ((Py68List *)value.as.object)->count != 0;
    }
    return 1;
}

Py68Status py68_builtin_int(Py68Runtime *runtime, Py68U16 argument_count,
                            Py68Value *arguments, Py68Value *result)
{
    Py68Value value;
    if (argument_count != 1) {
        py68_builtin_error(runtime, PY68_ERROR_TYPE, "int expects one argument");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    value = arguments[0];
    if (value.type == PY68_VALUE_INT || value.type == PY68_VALUE_BOOL) {
        *result = py68_value_int(value.as.integer);
        return PY68_STATUS_OK;
    }
    if (value.type == PY68_VALUE_FLOAT) {
        Py68I32 truncated;
        if (!py68_f32_to_i32_trunc((Py68U32)value.as.integer, &truncated)) {
            py68_builtin_error(runtime, PY68_ERROR_OVERFLOW,
                               "integer overflow");
            return PY68_STATUS_RUNTIME_ERROR;
        }
        *result = py68_value_int(truncated);
        return PY68_STATUS_OK;
    }
    if (value.type == PY68_VALUE_OBJECT && value.as.object != NULL &&
        value.as.object->type == PY68_OBJECT_STRING) {
        Py68String *string = (Py68String *)value.as.object;
        Py68U32 index = 0;
        Py68I32 integer = 0;
        int negative = 0;
        if (string->length == 0) {
            py68_builtin_error(runtime, PY68_ERROR_VALUE,
                               "invalid literal for int()");
            return PY68_STATUS_RUNTIME_ERROR;
        }
        if (string->data[0] == '-') {
            negative = 1;
            index = 1;
            if (index == string->length) {
                py68_builtin_error(runtime, PY68_ERROR_VALUE,
                                   "invalid literal for int()");
                return PY68_STATUS_RUNTIME_ERROR;
            }
        }
        for (; index < string->length; ++index) {
            char digit = string->data[index];
            if (digit < '0' || digit > '9') {
                py68_builtin_error(runtime, PY68_ERROR_VALUE,
                                   "invalid literal for int()");
                return PY68_STATUS_RUNTIME_ERROR;
            }
            if (integer > 214748364 ||
                (integer == 214748364 &&
                 (digit - '0') > (negative ? 8 : 7))) {
                py68_builtin_error(runtime, PY68_ERROR_OVERFLOW,
                                   "integer overflow");
                return PY68_STATUS_RUNTIME_ERROR;
            }
            integer = integer * 10 + (digit - '0');
        }
        *result = py68_value_int(negative ? -integer : integer);
        return PY68_STATUS_OK;
    }
    py68_builtin_error(runtime, PY68_ERROR_TYPE,
                       "int argument must be int, bool, or string");
    return PY68_STATUS_RUNTIME_ERROR;
}

Py68Status py68_builtin_str(Py68Runtime *runtime, Py68U16 argument_count,
                            Py68Value *arguments, Py68Value *result)
{
    Py68String *string;
    Py68Status status;
    char buffer[12];
    Py68U32 position;
    Py68U32 magnitude;
    int negative;
    Py68Value value;
    if (argument_count != 1) {
        py68_builtin_error(runtime, PY68_ERROR_TYPE, "str expects one argument");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    value = arguments[0];
    if (value.type == PY68_VALUE_OBJECT && value.as.object != NULL &&
        value.as.object->type == PY68_OBJECT_STRING) {
        status = py68_string_new_copy(
            runtime, ((Py68String *)value.as.object)->data,
            ((Py68String *)value.as.object)->length, &string);
        if (status != PY68_STATUS_OK) return status;
        *result = py68_value_from_object(&string->base);
        return PY68_STATUS_OK;
    }
    if (value.type == PY68_VALUE_NONE) {
        status = py68_string_new_copy(runtime, "None", 4, &string);
    } else if (value.type == PY68_VALUE_BOOL) {
        status = py68_string_new_copy(
            runtime, value.as.integer ? "True" : "False",
            value.as.integer ? 4 : 5, &string);
    } else if (value.type == PY68_VALUE_INT) {
        position = sizeof(buffer);
        negative = value.as.integer < 0;
        if (negative) {
            magnitude = (Py68U32)(-(value.as.integer + 1));
            ++magnitude;
        } else {
            magnitude = (Py68U32)value.as.integer;
        }
        do {
            buffer[--position] = (char)('0' + (magnitude % 10));
            magnitude /= 10;
        } while (magnitude != 0);
        if (negative) buffer[--position] = '-';
        status = py68_string_new_copy(runtime, buffer + position,
                                      (Py68U32)(sizeof(buffer) - position),
                                      &string);
    } else {
        py68_builtin_error(runtime, PY68_ERROR_TYPE,
                           "str argument type is not supported");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (status != PY68_STATUS_OK) return status;
    *result = py68_value_from_object(&string->base);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_bool(Py68Runtime *runtime, Py68U16 argument_count,
                             Py68Value *arguments, Py68Value *result)
{
    if (argument_count != 1) {
        py68_builtin_error(runtime, PY68_ERROR_TYPE, "bool expects one argument");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    *result = py68_value_bool(py68_builtin_truth(arguments[0]));
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_abs(Py68Runtime *runtime, Py68U16 argument_count,
                            Py68Value *arguments, Py68Value *result)
{
    Py68I32 value;
    if (argument_count != 1 ||
        (arguments[0].type != PY68_VALUE_INT &&
         arguments[0].type != PY68_VALUE_BOOL)) {
        py68_builtin_error(runtime, PY68_ERROR_TYPE, "abs expects an integer");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    value = arguments[0].as.integer;
    if (value == (Py68I32)-2147483647 - 1) {
        py68_builtin_error(runtime, PY68_ERROR_OVERFLOW, "integer overflow");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    *result = py68_value_int(value < 0 ? -value : value);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_min(Py68Runtime *runtime, Py68U16 argument_count,
                            Py68Value *arguments, Py68Value *result)
{
    if (argument_count != 2 ||
        arguments[0].type != PY68_VALUE_INT ||
        arguments[1].type != PY68_VALUE_INT) {
        py68_builtin_error(runtime, PY68_ERROR_TYPE,
                           "min expects two integers");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    *result = py68_value_int(arguments[0].as.integer < arguments[1].as.integer
                                 ? arguments[0].as.integer
                                 : arguments[1].as.integer);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_max(Py68Runtime *runtime, Py68U16 argument_count,
                            Py68Value *arguments, Py68Value *result)
{
    if (argument_count != 2 ||
        arguments[0].type != PY68_VALUE_INT ||
        arguments[1].type != PY68_VALUE_INT) {
        py68_builtin_error(runtime, PY68_ERROR_TYPE,
                           "max expects two integers");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    *result = py68_value_int(arguments[0].as.integer > arguments[1].as.integer
                                 ? arguments[0].as.integer
                                 : arguments[1].as.integer);
    return PY68_STATUS_OK;
}

static int py68_sum_add_int(Py68I32 left, Py68I32 right, Py68I32 *out)
{
    if ((right > 0 && left > (Py68I32)0x7fffffff - right) ||
        (right < 0 && left < (Py68I32)-2147483647 - 1 - right))
        return 0;
    *out = left + right;
    return 1;
}

Py68Status py68_builtin_sum(Py68Runtime *runtime, Py68U16 argument_count,
                            Py68Value *arguments, Py68Value *result)
{
    Py68Value *items = NULL;
    Py68U32 count = 0;
    Py68U32 index;
    Py68Value start;
    Py68I32 int_total = 0;
    Py68U32 float_bits = PY68_F32_ZERO;
    int use_float = 0;

    if (argument_count < 1 || argument_count > 2) {
        py68_builtin_error(runtime, PY68_ERROR_TYPE,
                           "sum expects one or two arguments");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (arguments[0].type != PY68_VALUE_OBJECT || arguments[0].as.object == NULL) {
        py68_builtin_error(runtime, PY68_ERROR_TYPE,
                           "sum() argument must be a list or tuple");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (arguments[0].as.object->type == PY68_OBJECT_LIST) {
        Py68List *list = (Py68List *)arguments[0].as.object;
        items = list->items;
        count = list->count;
    } else if (arguments[0].as.object->type == PY68_OBJECT_TUPLE) {
        Py68Tuple *tuple = (Py68Tuple *)arguments[0].as.object;
        items = tuple->items;
        count = tuple->count;
    } else {
        py68_builtin_error(runtime, PY68_ERROR_TYPE,
                           "sum() argument must be a list or tuple");
        return PY68_STATUS_RUNTIME_ERROR;
    }

    if (argument_count == 2)
        start = arguments[1];
    else
        start = py68_value_int(0);
    if (!py68_value_is_number(start)) {
        py68_builtin_error(runtime, PY68_ERROR_TYPE,
                           "sum() start must be a number");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (start.type == PY68_VALUE_FLOAT) {
        use_float = 1;
        float_bits = py68_value_float_bits_get(start);
    } else {
        int_total = start.as.integer;
    }

    for (index = 0; index < count; ++index) {
        Py68Value item = items[index];
        if (!py68_value_is_number(item)) {
            py68_builtin_error(runtime, PY68_ERROR_TYPE,
                               "sum() items must be numbers");
            return PY68_STATUS_RUNTIME_ERROR;
        }
        if (!use_float && item.type == PY68_VALUE_FLOAT) {
            float_bits = py68_f32_from_i32(int_total);
            use_float = 1;
        }
        if (use_float) {
            float_bits = py68_f32_add(float_bits,
                                     py68_value_float_bits_get(item));
            if (!py68_f32_is_finite(float_bits)) {
                py68_builtin_error(runtime, PY68_ERROR_VALUE,
                                   "non-finite float result");
                return PY68_STATUS_RUNTIME_ERROR;
            }
        } else if (!py68_sum_add_int(int_total, item.as.integer, &int_total)) {
            py68_builtin_error(runtime, PY68_ERROR_OVERFLOW,
                               "integer overflow");
            return PY68_STATUS_RUNTIME_ERROR;
        }
    }

    if (use_float)
        *result = py68_value_float_bits(float_bits);
    else
        *result = py68_value_int(int_total);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_iter(Py68Runtime *runtime, Py68U16 argument_count,
                             Py68Value *arguments, Py68Value *result)
{
    Py68Range *range_obj;
    Py68Status status;
    if (argument_count != 1) {
        py68_builtin_error(runtime, PY68_ERROR_TYPE,
                           "iter expects one argument");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    /* A generator is its own iterator (D-0045). */
    if (arguments[0].type == PY68_VALUE_OBJECT &&
        arguments[0].as.object != NULL &&
        arguments[0].as.object->type == PY68_OBJECT_GENERATOR) {
        *result = arguments[0];
        py68_value_retain(*result);
        return PY68_STATUS_OK;
    }
    status = py68_iterable_get_iter(runtime, arguments[0], &range_obj);
    if (status == PY68_STATUS_SOURCE_ERROR) {
        py68_builtin_error(runtime, PY68_ERROR_TYPE,
                           "iter() argument must be iterable");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (status != PY68_STATUS_OK) return status;
    *result = py68_value_from_object(&range_obj->base);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_next(Py68Runtime *runtime, Py68U16 argument_count,
                             Py68Value *arguments, Py68Value *result)
{
    Py68Range *range_obj;
    Py68Status status;
    int has_next = 0;
    if (argument_count < 1 || argument_count > 2) {
        py68_builtin_error(runtime, PY68_ERROR_TYPE,
                           "next expects one or two arguments");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (arguments[0].type == PY68_VALUE_OBJECT &&
        arguments[0].as.object != NULL &&
        arguments[0].as.object->type == PY68_OBJECT_GENERATOR) {
        /* OP_CALL resumes generators inline, so reaching the callback with a
           generator means it was invoked outside a script call (D-0045). */
        py68_builtin_error(runtime, PY68_ERROR_TYPE,
                           "next() on a generator requires a direct call");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (arguments[0].type != PY68_VALUE_OBJECT ||
        arguments[0].as.object == NULL ||
        arguments[0].as.object->type != PY68_OBJECT_RANGE) {
        py68_builtin_error(runtime, PY68_ERROR_TYPE,
                           "next() argument must be an iterator");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    range_obj = (Py68Range *)arguments[0].as.object;
    status = py68_range_next_value(runtime, range_obj, result, &has_next);
    if (status != PY68_STATUS_OK) return status;
    if (has_next) return PY68_STATUS_OK;
    if (argument_count == 2) {
        *result = arguments[1];
        py68_value_retain(*result);
        return PY68_STATUS_OK;
    }
    py68_builtin_error(runtime, PY68_ERROR_STOP_ITERATION, "StopIteration");
    return PY68_STATUS_RUNTIME_ERROR;
}

Py68Status py68_builtin_exit(Py68Runtime *runtime, Py68U16 argument_count,
                             Py68Value *arguments, Py68Value *result)
{
    Py68I32 code = 0;
    if (argument_count > 1) {
        py68_builtin_error(runtime, PY68_ERROR_TYPE,
                           "exit expects at most one integer");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (argument_count == 1) {
        if (arguments[0].type != PY68_VALUE_INT &&
            arguments[0].type != PY68_VALUE_BOOL) {
            py68_builtin_error(runtime, PY68_ERROR_TYPE,
                               "exit argument must be an integer");
            return PY68_STATUS_RUNTIME_ERROR;
        }
        code = arguments[0].as.integer;
    }
    runtime->requested_exit_code = code;
    *result = py68_value_none();
    return PY68_STATUS_EXIT;
}

static Py68Status py68_copy_sequence(Py68Runtime *runtime, Py68Value value,
                                     Py68List **list_out)
{
    Py68List *list;
    Py68U32 index;
    Py68Status status;
    if (value.type != PY68_VALUE_OBJECT || value.as.object == NULL)
        return PY68_STATUS_SOURCE_ERROR;
    status = py68_list_new(runtime, &list);
    if (status != PY68_STATUS_OK) return status;
    if (value.as.object->type == PY68_OBJECT_LIST) {
        Py68List *source = (Py68List *)value.as.object;
        for (index = 0; index < source->count; ++index) {
            status = py68_list_append_copy(runtime, list, source->items[index]);
            if (status != PY68_STATUS_OK) {
                py68_object_release(runtime, &list->base);
                return status;
            }
        }
    } else if (value.as.object->type == PY68_OBJECT_TUPLE) {
        Py68Tuple *source = (Py68Tuple *)value.as.object;
        for (index = 0; index < source->count; ++index) {
            status = py68_list_append_copy(runtime, list, source->items[index]);
            if (status != PY68_STATUS_OK) {
                py68_object_release(runtime, &list->base);
                return status;
            }
        }
    } else {
        py68_object_release(runtime, &list->base);
        return PY68_STATUS_SOURCE_ERROR;
    }
    *list_out = list;
    return PY68_STATUS_OK;
}

static Py68Status py68_sorted_materialize(Py68Runtime *runtime, Py68Value value,
                                          Py68List **list_out)
{
    Py68List *list;
    Py68Status status;
    Py68U32 index;
    if (value.type != PY68_VALUE_OBJECT || value.as.object == NULL) {
        py68_builtin_error(runtime, PY68_ERROR_TYPE,
                           "sorted() argument must be iterable");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (value.as.object->type == PY68_OBJECT_LIST ||
        value.as.object->type == PY68_OBJECT_TUPLE) {
        status = py68_copy_sequence(runtime, value, &list);
        if (status != PY68_STATUS_OK) {
            py68_builtin_error(runtime, PY68_ERROR_TYPE,
                               "sorted() argument must be iterable");
            return PY68_STATUS_RUNTIME_ERROR;
        }
        *list_out = list;
        return PY68_STATUS_OK;
    }
    if (value.as.object->type == PY68_OBJECT_DICT)
        return py68_dict_keys(runtime, (Py68Dict *)value.as.object, list_out);
    if (value.as.object->type == PY68_OBJECT_SET)
        return py68_set_values(runtime, (Py68Set *)value.as.object, list_out);
    if (value.as.object->type == PY68_OBJECT_STRING) {
        Py68String *string = (Py68String *)value.as.object;
        status = py68_list_new(runtime, &list);
        if (status != PY68_STATUS_OK) return status;
        for (index = 0; index < string->length; ++index) {
            Py68String *character;
            Py68Value character_value;
            status = py68_string_get_char(runtime, string, (Py68I32)index,
                                          &character);
            if (status != PY68_STATUS_OK) {
                py68_object_release(runtime, &list->base);
                return status;
            }
            character_value = py68_value_from_object(&character->base);
            status = py68_list_append_move(runtime, list, &character_value);
            if (status != PY68_STATUS_OK) {
                py68_value_release(runtime, character_value);
                py68_object_release(runtime, &list->base);
                return status;
            }
        }
        *list_out = list;
        return PY68_STATUS_OK;
    }
    py68_builtin_error(runtime, PY68_ERROR_TYPE,
                       "sorted() argument must be iterable");
    return PY68_STATUS_RUNTIME_ERROR;
}

Py68Status py68_builtin_sorted(Py68Runtime *runtime, Py68U16 argument_count,
                               Py68Value *arguments, Py68Value *result)
{
    Py68List *list;
    Py68U32 index;
    Py68Status status;
    if (argument_count != 1) {
        py68_builtin_error(runtime, PY68_ERROR_TYPE,
                           "sorted() takes exactly one argument");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    status = py68_sorted_materialize(runtime, arguments[0], &list);
    if (status != PY68_STATUS_OK) return status;
    for (index = 1; index < list->count; ++index) {
        Py68Value key = list->items[index];
        Py68U32 insert = index;
        while (insert > 0) {
            int cmp = 0;
            if (!py68_value_compare(runtime, list->items[insert - 1], key,
                                    &cmp)) {
                py68_object_release(runtime, &list->base);
                py68_builtin_error(runtime, PY68_ERROR_TYPE,
                    "unsupported operand types for ordering comparison");
                return PY68_STATUS_RUNTIME_ERROR;
            }
            if (cmp <= 0) break;
            list->items[insert] = list->items[insert - 1];
            --insert;
        }
        list->items[insert] = key;
    }
    *result = py68_value_from_object(&list->base);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_list(Py68Runtime *runtime, Py68U16 argument_count,
                             Py68Value *arguments, Py68Value *result)
{
    Py68List *list;
    Py68Status status;
    if (argument_count == 0) {
        status = py68_list_new(runtime, &list);
        if (status != PY68_STATUS_OK) return status;
        *result = py68_value_from_object(&list->base);
        return PY68_STATUS_OK;
    }
    status = py68_copy_sequence(runtime, arguments[0], &list);
    if (status != PY68_STATUS_OK) {
        py68_builtin_error(runtime, PY68_ERROR_TYPE,
                           "list() argument must be a list or tuple");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    *result = py68_value_from_object(&list->base);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_tuple(Py68Runtime *runtime, Py68U16 argument_count,
                              Py68Value *arguments, Py68Value *result)
{
    Py68Tuple *tuple;
    Py68List *list;
    Py68Status status;
    if (argument_count == 0) {
        status = py68_tuple_new(runtime, 0, &tuple);
        if (status != PY68_STATUS_OK) return status;
        *result = py68_value_from_object(&tuple->base);
        return PY68_STATUS_OK;
    }
    if (arguments[0].type == PY68_VALUE_OBJECT &&
        arguments[0].as.object != NULL &&
        arguments[0].as.object->type == PY68_OBJECT_TUPLE) {
        *result = arguments[0];
        py68_value_retain(*result);
        return PY68_STATUS_OK;
    }
    status = py68_copy_sequence(runtime, arguments[0], &list);
    if (status != PY68_STATUS_OK) {
        py68_builtin_error(runtime, PY68_ERROR_TYPE,
                           "tuple() argument must be a list or tuple");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    status = py68_tuple_from_values(runtime, list->items, list->count, &tuple);
    py68_object_release(runtime, &list->base);
    if (status != PY68_STATUS_OK) return status;
    *result = py68_value_from_object(&tuple->base);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_dict(Py68Runtime *runtime, Py68U16 argument_count,
                             Py68Value *arguments, Py68Value *result)
{
    Py68Dict *dict;
    Py68Status status;
    (void)arguments;
    if (argument_count != 0) {
        py68_builtin_error(runtime, PY68_ERROR_TYPE,
                           "dict() takes no arguments");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    status = py68_dict_new(runtime, &dict);
    if (status != PY68_STATUS_OK) return status;
    *result = py68_value_from_object(&dict->base);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_set(Py68Runtime *runtime, Py68U16 argument_count,
                            Py68Value *arguments, Py68Value *result)
{
    Py68Set *set;
    Py68List *list;
    Py68U32 index;
    Py68Status status;
    status = py68_set_new(runtime, &set);
    if (status != PY68_STATUS_OK) return status;
    if (argument_count == 0) {
        *result = py68_value_from_object(&set->base);
        return PY68_STATUS_OK;
    }
    status = py68_copy_sequence(runtime, arguments[0], &list);
    if (status != PY68_STATUS_OK) {
        py68_object_release(runtime, &set->base);
        py68_builtin_error(runtime, PY68_ERROR_TYPE,
                           "set() argument must be a list or tuple");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    for (index = 0; index < list->count; ++index) {
        status = py68_set_add(runtime, set, list->items[index]);
        if (status != PY68_STATUS_OK) {
            py68_object_release(runtime, &list->base);
            py68_object_release(runtime, &set->base);
            py68_builtin_error(runtime, PY68_ERROR_TYPE, "unhashable type");
            return PY68_STATUS_RUNTIME_ERROR;
        }
    }
    py68_object_release(runtime, &list->base);
    *result = py68_value_from_object(&set->base);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_float(Py68Runtime *runtime, Py68U16 argument_count,
                              Py68Value *arguments, Py68Value *result)
{
    if (argument_count != 1) {
        py68_builtin_error(runtime, PY68_ERROR_TYPE, "float expects one argument");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (py68_value_is_number(arguments[0])) {
        *result = py68_value_float_bits(py68_value_float_bits_get(arguments[0]));
        return PY68_STATUS_OK;
    }
    py68_builtin_error(runtime, PY68_ERROR_TYPE, "float argument must be numeric");
    return PY68_STATUS_RUNTIME_ERROR;
}

Py68Status py68_builtin_exception(Py68Runtime *runtime,
                                  Py68U16 argument_count,
                                  Py68Value *arguments, Py68Value *result)
{
    Py68Exception *exception;
    Py68U16 kind = PY68_ERROR_TYPE;
    const char *message = "";
    Py68Status status;
    if (runtime->active_native != NULL)
        kind = runtime->active_native->base.flags;
    if (argument_count == 1) {
        if (arguments[0].type == PY68_VALUE_OBJECT &&
            arguments[0].as.object != NULL &&
            arguments[0].as.object->type == PY68_OBJECT_STRING)
            message = ((Py68String *)arguments[0].as.object)->data;
        else if (arguments[0].type == PY68_VALUE_INT) {
            py68_builtin_error(runtime, PY68_ERROR_TYPE,
                               "exception message must be a string");
            return PY68_STATUS_RUNTIME_ERROR;
        }
    }
    status = py68_exception_new(runtime, kind, message, &exception);
    if (status != PY68_STATUS_OK) return status;
    *result = py68_value_from_object(&exception->base);
    return PY68_STATUS_OK;
}