/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_builtin.h"
#include "py68k_exception.h"
#include "py68k_list.h"
#include "py68k_string.h"
#include "py68k_string_methods.h"
#include "py68k_tuple.h"

#include <stddef.h>
#include <string.h>

static void py68_text_error(Py68Runtime *runtime, Py68ErrorKind kind,
                            const char *message)
{
    Py68Location location;
    location.offset = 0;
    location.line = 0;
    location.column = 0;
    location.length = 0;
    py68_error_set(&runtime->error, kind, location, NULL, message);
}

static Py68Status py68_text_str_from_int(Py68Runtime *runtime, Py68I32 integer,
                                         Py68String **result)
{
    char buffer[12];
    Py68U32 position = sizeof(buffer);
    Py68U32 magnitude;
    int negative = integer < 0;
    if (negative) {
        magnitude = (Py68U32)(-(integer + 1));
        ++magnitude;
    } else {
        magnitude = (Py68U32)integer;
    }
    do {
        buffer[--position] = (char)('0' + (magnitude % 10));
        magnitude /= 10;
    } while (magnitude != 0);
    if (negative) buffer[--position] = '-';
    return py68_string_new_copy(runtime, buffer + position,
                                (Py68U32)(sizeof(buffer) - position), result);
}

Py68Status py68_builtin_ord(Py68Runtime *runtime, Py68U16 argument_count,
                            Py68Value *arguments, Py68Value *result)
{
    Py68String *string;
    if (argument_count != 1 || arguments[0].type != PY68_VALUE_OBJECT ||
        arguments[0].as.object == NULL ||
        arguments[0].as.object->type != PY68_OBJECT_STRING) {
        py68_text_error(runtime, PY68_ERROR_TYPE, "ord() expects a string");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    string = (Py68String *)arguments[0].as.object;
    if (string->length != 1) {
        py68_text_error(runtime, PY68_ERROR_TYPE,
                        "ord() expects a string of length 1");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    *result = py68_value_int((Py68I32)(unsigned char)string->data[0]);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_chr(Py68Runtime *runtime, Py68U16 argument_count,
                            Py68Value *arguments, Py68Value *result)
{
    Py68I32 code;
    char byte;
    Py68String *string;
    Py68Status status;
    if (argument_count != 1 ||
        (arguments[0].type != PY68_VALUE_INT &&
         arguments[0].type != PY68_VALUE_BOOL)) {
        py68_text_error(runtime, PY68_ERROR_TYPE, "chr() expects an int");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    code = arguments[0].as.integer;
    if (code < 0 || code > 255) {
        py68_text_error(runtime, PY68_ERROR_VALUE,
                        "chr() arg not in range(256)");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    byte = (char)code;
    status = py68_string_new_copy(runtime, &byte, 1, &string);
    if (status != PY68_STATUS_OK) return status;
    *result = py68_value_from_object(&string->base);
    return PY68_STATUS_OK;
}

static Py68Status py68_text_repr_value(Py68Runtime *runtime, Py68Value value,
                                       int escape_high, Py68Value *result)
{
    Py68String *string;
    Py68Status status;
    if (value.type == PY68_VALUE_OBJECT && value.as.object != NULL &&
        value.as.object->type == PY68_OBJECT_STRING) {
        status = py68_string_repr_escape(
            runtime, (Py68String *)value.as.object, escape_high, &string);
        if (status != PY68_STATUS_OK) return status;
        *result = py68_value_from_object(&string->base);
        return PY68_STATUS_OK;
    }
    if (value.type == PY68_VALUE_NONE) {
        status = py68_string_new_copy(runtime, "None", 4, &string);
    } else if (value.type == PY68_VALUE_BOOL) {
        status = py68_string_new_copy(runtime, value.as.integer ? "True" : "False",
                                      value.as.integer ? 4 : 5, &string);
    } else if (value.type == PY68_VALUE_INT) {
        status = py68_text_str_from_int(runtime, value.as.integer, &string);
    } else {
        py68_text_error(runtime, PY68_ERROR_TYPE,
                        "repr/ascii type is not supported");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (status != PY68_STATUS_OK) return status;
    *result = py68_value_from_object(&string->base);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_repr(Py68Runtime *runtime, Py68U16 argument_count,
                             Py68Value *arguments, Py68Value *result)
{
    if (argument_count != 1) {
        py68_text_error(runtime, PY68_ERROR_TYPE, "repr expects one argument");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    return py68_text_repr_value(runtime, arguments[0], 0, result);
}

Py68Status py68_builtin_ascii(Py68Runtime *runtime, Py68U16 argument_count,
                              Py68Value *arguments, Py68Value *result)
{
    if (argument_count != 1) {
        py68_text_error(runtime, PY68_ERROR_TYPE, "ascii expects one argument");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    return py68_text_repr_value(runtime, arguments[0], 1, result);
}

static Py68Status py68_text_iter_all_any(Py68Runtime *runtime, Py68Value value,
                                         int want_all, Py68Value *result)
{
    Py68Value *items = NULL;
    Py68U32 count = 0;
    Py68U32 index;
    if (value.type != PY68_VALUE_OBJECT || value.as.object == NULL) {
        py68_text_error(runtime, PY68_ERROR_TYPE,
                        "all/any expects list, tuple, or string");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (value.as.object->type == PY68_OBJECT_LIST) {
        Py68List *list = (Py68List *)value.as.object;
        items = list->items;
        count = list->count;
    } else if (value.as.object->type == PY68_OBJECT_TUPLE) {
        Py68Tuple *tuple = (Py68Tuple *)value.as.object;
        items = tuple->items;
        count = tuple->count;
    } else if (value.as.object->type == PY68_OBJECT_STRING) {
        Py68String *string = (Py68String *)value.as.object;
        if (want_all) {
            *result = py68_value_bool(1);
            for (index = 0; index < string->length; ++index) {
                /* every one-char string is truthy */
            }
            return PY68_STATUS_OK;
        }
        *result = py68_value_bool(string->length != 0);
        return PY68_STATUS_OK;
    } else {
        py68_text_error(runtime, PY68_ERROR_TYPE,
                        "all/any expects list, tuple, or string");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (want_all) {
        for (index = 0; index < count; ++index) {
            if (!py68_value_truthy(items[index])) {
                *result = py68_value_bool(0);
                return PY68_STATUS_OK;
            }
        }
        *result = py68_value_bool(1);
        return PY68_STATUS_OK;
    }
    for (index = 0; index < count; ++index) {
        if (py68_value_truthy(items[index])) {
            *result = py68_value_bool(1);
            return PY68_STATUS_OK;
        }
    }
    *result = py68_value_bool(0);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_all(Py68Runtime *runtime, Py68U16 argument_count,
                            Py68Value *arguments, Py68Value *result)
{
    if (argument_count != 1) {
        py68_text_error(runtime, PY68_ERROR_TYPE, "all expects one argument");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    return py68_text_iter_all_any(runtime, arguments[0], 1, result);
}

Py68Status py68_builtin_any(Py68Runtime *runtime, Py68U16 argument_count,
                            Py68Value *arguments, Py68Value *result)
{
    if (argument_count != 1) {
        py68_text_error(runtime, PY68_ERROR_TYPE, "any expects one argument");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    return py68_text_iter_all_any(runtime, arguments[0], 0, result);
}

static Py68Status py68_text_format_int(Py68Runtime *runtime, Py68I32 integer,
                                       const char *spec, Py68U32 spec_len,
                                       Py68Value *result)
{
    Py68String *digits;
    Py68String *out;
    Py68Status status;
    Py68U32 index = 0;
    char fill = ' ';
    char align = '>';
    Py68U32 width = 0;
    int have_width = 0;
    char type = 'd';
    Py68U32 pad;
    Py68U32 left;
    Py68U32 right;
    char *buffer;
    Py68U32 target;
    if (spec_len == 0) {
        status = py68_text_str_from_int(runtime, integer, &out);
        if (status != PY68_STATUS_OK) return status;
        *result = py68_value_from_object(&out->base);
        return PY68_STATUS_OK;
    }
    /* Optional fill+align */
    if (spec_len >= 2 &&
        (spec[1] == '<' || spec[1] == '>' || spec[1] == '^' ||
         spec[1] == '=')) {
        fill = spec[0];
        align = spec[1];
        index = 2;
    } else if (spec[0] == '<' || spec[0] == '>' || spec[0] == '^') {
        align = spec[0];
        index = 1;
    }
    if (index < spec_len && spec[index] == '0' &&
        (index + 1 >= spec_len ||
         (spec[index + 1] >= '0' && spec[index + 1] <= '9'))) {
        /* zero-pad shorthand: 0[width][type] */
        if (align == '>' && fill == ' ') {
            fill = '0';
            align = '=';
        }
        ++index;
    }
    while (index < spec_len && spec[index] >= '0' && spec[index] <= '9') {
        have_width = 1;
        if (width > 214748364 ||
            (width == 214748364 && spec[index] > '7')) {
            py68_text_error(runtime, PY68_ERROR_OVERFLOW, "format width overflow");
            return PY68_STATUS_RUNTIME_ERROR;
        }
        width = width * 10 + (Py68U32)(spec[index] - '0');
        ++index;
    }
    if (index < spec_len) {
        type = spec[index];
        ++index;
    }
    if (index != spec_len || type != 'd') {
        py68_text_error(runtime, PY68_ERROR_VALUE,
                        "unsupported format type (only d)");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    status = py68_text_str_from_int(runtime, integer, &digits);
    if (status != PY68_STATUS_OK) return status;
    if (!have_width || width <= digits->length) {
        *result = py68_value_from_object(&digits->base);
        return PY68_STATUS_OK;
    }
    target = width;
    pad = target - digits->length;
    if (align == '<') {
        left = 0;
        right = pad;
    } else if (align == '^') {
        left = pad / 2;
        right = pad - left;
    } else if (align == '=') {
        /* sign then zeros then digits */
        Py68U32 sign = 0;
        buffer =
            (char *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP, target + 1);
        if (buffer == NULL) {
            py68_object_release(runtime, &digits->base);
            return PY68_STATUS_MEMORY_ERROR;
        }
        if (digits->length > 0 &&
            (digits->data[0] == '-' || digits->data[0] == '+'))
            sign = 1;
        if (sign) buffer[0] = digits->data[0];
        for (index = 0; index < pad; ++index) buffer[sign + index] = fill;
        memcpy(buffer + sign + pad, digits->data + sign, digits->length - sign);
        buffer[target] = '\0';
        status = py68_string_new_copy(runtime, buffer, target, &out);
        py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer, target + 1);
        py68_object_release(runtime, &digits->base);
        if (status != PY68_STATUS_OK) return status;
        *result = py68_value_from_object(&out->base);
        return PY68_STATUS_OK;
    } else {
        left = pad;
        right = 0;
    }
    buffer = (char *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP, target + 1);
    if (buffer == NULL) {
        py68_object_release(runtime, &digits->base);
        return PY68_STATUS_MEMORY_ERROR;
    }
    for (index = 0; index < left; ++index) buffer[index] = fill;
    memcpy(buffer + left, digits->data, digits->length);
    for (index = 0; index < right; ++index)
        buffer[left + digits->length + index] = fill;
    buffer[target] = '\0';
    status = py68_string_new_copy(runtime, buffer, target, &out);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer, target + 1);
    py68_object_release(runtime, &digits->base);
    if (status != PY68_STATUS_OK) return status;
    *result = py68_value_from_object(&out->base);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_format(Py68Runtime *runtime, Py68U16 argument_count,
                               Py68Value *arguments, Py68Value *result)
{
    const char *spec = "";
    Py68U32 spec_len = 0;
    if (argument_count < 1 || argument_count > 2) {
        py68_text_error(runtime, PY68_ERROR_TYPE, "format expects 1 or 2 arguments");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (argument_count == 2) {
        Py68String *spec_string;
        if (arguments[1].type != PY68_VALUE_OBJECT ||
            arguments[1].as.object == NULL ||
            arguments[1].as.object->type != PY68_OBJECT_STRING) {
            py68_text_error(runtime, PY68_ERROR_TYPE,
                            "format_spec must be a string");
            return PY68_STATUS_RUNTIME_ERROR;
        }
        spec_string = (Py68String *)arguments[1].as.object;
        spec = spec_string->data;
        spec_len = spec_string->length;
    }
    if (arguments[0].type == PY68_VALUE_INT ||
        arguments[0].type == PY68_VALUE_BOOL)
        return py68_text_format_int(runtime, arguments[0].as.integer, spec,
                                    spec_len, result);
    if (spec_len == 0) {
        /* empty format: reuse str() */
        return py68_builtin_str(runtime, 1, arguments, result);
    }
    py68_text_error(runtime, PY68_ERROR_VALUE,
                    "format specs for non-int not implemented");
    return PY68_STATUS_RUNTIME_ERROR;
}
