/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_time.h"
#include "py68k_runtime.h"
#include "py68k_builtin.h"
#include "py68k_exception.h"
#include "py68k_float.h"
#include "py68k_string.h"

#include <stddef.h>
#include <string.h>
#include <time.h>

static Py68Status py68_time_error(Py68Runtime *runtime, Py68ErrorKind kind,
                                   const char *message)
{
    Py68Location location;
    location.offset = 0;
    location.line = 0;
    location.column = 0;
    location.length = 0;
    py68_error_set(&runtime->error, kind, location, NULL, message);
    return PY68_STATUS_RUNTIME_ERROR;
}

static Py68U32 py68_time_float(Py68U32 seconds, Py68U32 microseconds)
{
    Py68U32 value = py68_f32_from_i32((Py68I32)seconds);
    Py68U32 fraction;
    if (microseconds != 0 && py68_f32_div(
            py68_f32_from_i32((Py68I32)microseconds),
            py68_f32_from_i32(1000000), &fraction) == 0)
        value = py68_f32_add(value, fraction);
    return value;
}

static Py68Status py68_time_seconds(Py68Value value, Py68U32 *seconds)
{
    Py68I32 integer;
    if (value.type != PY68_VALUE_INT && value.type != PY68_VALUE_BOOL &&
        value.type != PY68_VALUE_FLOAT) return PY68_STATUS_RUNTIME_ERROR;
    if (value.type == PY68_VALUE_FLOAT) {
        if (!py68_f32_to_i32_trunc(py68_value_float_bits_get(value), &integer))
            return PY68_STATUS_RUNTIME_ERROR;
    } else integer = value.as.integer;
    if (integer < 0) return PY68_STATUS_RUNTIME_ERROR;
    *seconds = (Py68U32)integer;
    return PY68_STATUS_OK;
}

static Py68Status py68_time_string(Py68Runtime *runtime, const char *text,
                                   Py68U32 length, Py68Value *result)
{
    Py68String *string;
    Py68Status status = py68_string_new_copy(runtime, text, length, &string);
    if (status != PY68_STATUS_OK) return status;
    *result = py68_value_from_object(&string->base);
    return PY68_STATUS_OK;
}

Py68Status py68_struct_time_new(Py68Runtime *runtime, const Py68I32 fields[9],
                                Py68StructTime **result)
{
    Py68StructTime *value;
    Py68U16 index;
    value = (Py68StructTime *)py68_alloc(&runtime->allocator, PY68_MEM_RUNTIME,
                                         sizeof(Py68StructTime));
    if (value == NULL) return PY68_STATUS_MEMORY_ERROR;
    value->base.type = PY68_OBJECT_STRUCT_TIME;
    value->base.flags = 0;
    value->base.reference_count = 1;
    value->base.next_object = runtime->live_objects;
    runtime->live_objects = &value->base;
    for (index = 0; index < 9; ++index) value->fields[index] = fields[index];
    *result = value;
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_time(Py68Runtime *runtime, Py68U16 count,
                             Py68Value *arguments, Py68Value *result)
{
    Py68U32 seconds, microseconds;
    (void)arguments;
    if (count != 0) return py68_time_error(runtime, PY68_ERROR_TYPE,
                                           "time expects no arguments");
    if (py68_platform_time_epoch(runtime, &seconds, &microseconds) !=
        PY68_STATUS_OK) return py68_time_error(runtime, PY68_ERROR_IO,
                                                "clock unavailable");
    *result = py68_value_float_bits(py68_time_float(seconds, microseconds));
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_perf_counter(Py68Runtime *runtime, Py68U16 count,
                                     Py68Value *arguments, Py68Value *result)
{
    Py68U32 seconds, microseconds;
    (void)arguments;
    if (count != 0) return py68_time_error(runtime, PY68_ERROR_TYPE,
                                           "perf_counter expects no arguments");
    if (py68_platform_time_monotonic(runtime, &seconds, &microseconds) !=
        PY68_STATUS_OK) return py68_time_error(runtime, PY68_ERROR_IO,
                                                "monotonic clock unavailable");
    *result = py68_value_float_bits(py68_time_float(seconds, microseconds));
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_time_tick(Py68Runtime *runtime, Py68U16 count,
                                  Py68Value *arguments, Py68Value *result)
{
    Py68U32 milliseconds;
    (void)arguments;
    if (count != 0) return py68_time_error(runtime, PY68_ERROR_TYPE,
                                           "time_tick expects no arguments");
    if (py68_platform_time_tick(runtime, &milliseconds) != PY68_STATUS_OK)
        return py68_time_error(runtime, PY68_ERROR_IO, "clock unavailable");
    *result = py68_value_int((Py68I32)milliseconds);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_sleep(Py68Runtime *runtime, Py68U16 count,
                              Py68Value *arguments, Py68Value *result)
{
    Py68U32 micros;
    Py68U32 seconds;
    Py68U32 fraction;
    Py68U32 scaled;
    Py68I32 scaled_integer;
    if (count != 1) return py68_time_error(runtime, PY68_ERROR_TYPE,
                                           "sleep expects one number");
    if (arguments[0].type != PY68_VALUE_INT &&
        arguments[0].type != PY68_VALUE_BOOL &&
        arguments[0].type != PY68_VALUE_FLOAT)
        return py68_time_error(runtime, PY68_ERROR_TYPE,
                               "sleep argument must be a number");
    if (arguments[0].type == PY68_VALUE_FLOAT) {
        scaled = py68_f32_mul(py68_value_float_bits_get(arguments[0]),
                              py68_f32_from_i32(1000000));
        if (!py68_f32_to_i32_trunc(scaled, &scaled_integer) ||
            scaled_integer < 0)
            return py68_time_error(runtime, PY68_ERROR_VALUE,
                                   "sleep duration is out of range");
        micros = (Py68U32)scaled_integer;
    } else {
        if (arguments[0].as.integer < 0)
            return py68_time_error(runtime, PY68_ERROR_VALUE,
                                   "sleep length must be non-negative");
        if ((Py68U32)arguments[0].as.integer > 4294UL)
            return py68_time_error(runtime, PY68_ERROR_VALUE,
                                   "sleep duration is out of range");
        micros = (Py68U32)arguments[0].as.integer * 1000000UL;
    }
    seconds = micros / 1000000UL;
    fraction = micros % 1000000UL;
    if (py68_platform_sleep(runtime, seconds, fraction) != PY68_STATUS_OK)
        return py68_time_error(runtime, PY68_ERROR_IO, "sleep failed");
    *result = py68_value_none();
    return PY68_STATUS_OK;
}

static Py68Status py68_time_parts(Py68Runtime *runtime, Py68U32 seconds,
                                   int utc, Py68Value *result)
{
    time_t stamp = (time_t)seconds;
    struct tm *parts = utc ? gmtime(&stamp) : localtime(&stamp);
    Py68I32 fields[9];
    Py68StructTime *value;
    if (parts == NULL) return py68_time_error(runtime, PY68_ERROR_VALUE,
                                              "timestamp out of range");
    fields[0] = (Py68I32)parts->tm_year + 1900;
    fields[1] = (Py68I32)parts->tm_mon + 1;
    fields[2] = (Py68I32)parts->tm_mday;
    fields[3] = (Py68I32)parts->tm_hour;
    fields[4] = (Py68I32)parts->tm_min;
    fields[5] = (Py68I32)parts->tm_sec;
    fields[6] = (Py68I32)parts->tm_wday;
    fields[7] = (Py68I32)parts->tm_yday + 1;
    fields[8] = (Py68I32)parts->tm_isdst;
    if (py68_struct_time_new(runtime, fields, &value) != PY68_STATUS_OK)
        return PY68_STATUS_MEMORY_ERROR;
    *result = py68_value_from_object(&value->base);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_localtime(Py68Runtime *runtime, Py68U16 count,
                                  Py68Value *arguments, Py68Value *result)
{
    Py68U32 seconds;
    if (count > 1) return py68_time_error(runtime, PY68_ERROR_TYPE,
                                          "localtime expects zero or one argument");
    if (count == 0) {
        Py68U32 micros;
        if (py68_platform_time_epoch(runtime, &seconds, &micros) != PY68_STATUS_OK)
            return py68_time_error(runtime, PY68_ERROR_IO, "clock unavailable");
    } else if (py68_time_seconds(arguments[0], &seconds) != PY68_STATUS_OK)
        return py68_time_error(runtime, PY68_ERROR_VALUE,
                               "timestamp must be a non-negative number");
    return py68_time_parts(runtime, seconds, 0, result);
}

Py68Status py68_builtin_ctime(Py68Runtime *runtime, Py68U16 count,
                              Py68Value *arguments, Py68Value *result)
{
    Py68U32 seconds;
    time_t stamp;
    char *text;
    Py68U32 length;
    if (count > 1) return py68_time_error(runtime, PY68_ERROR_TYPE,
                                          "ctime expects zero or one argument");
    if (count == 0) {
        Py68U32 micros;
        if (py68_platform_time_epoch(runtime, &seconds, &micros) != PY68_STATUS_OK)
            return py68_time_error(runtime, PY68_ERROR_IO, "clock unavailable");
    } else if (py68_time_seconds(arguments[0], &seconds) != PY68_STATUS_OK)
        return py68_time_error(runtime, PY68_ERROR_VALUE,
                               "timestamp must be a non-negative number");
    stamp = (time_t)seconds;
    text = ctime(&stamp);
    if (text == NULL) return py68_time_error(runtime, PY68_ERROR_VALUE,
                                             "timestamp out of range");
    length = 0;
    while (text[length] != '\0' && text[length] != '\n') ++length;
    return py68_time_string(runtime, text, length, result);
}

Py68Status py68_builtin_strftime(Py68Runtime *runtime, Py68U16 count,
                                 Py68Value *arguments, Py68Value *result)
{
    Py68String *format;
    Py68StructTime *value;
    char buffer[256];
    struct tm parts;
    Py68U32 index;
    if (count < 1 || count > 2 || arguments[0].type != PY68_VALUE_OBJECT ||
        arguments[0].as.object == NULL ||
        arguments[0].as.object->type != PY68_OBJECT_STRING)
        return py68_time_error(runtime, PY68_ERROR_TYPE,
                               "strftime expects a format string");
    format = (Py68String *)arguments[0].as.object;
    if (count == 1) {
        Py68Value current;
        if (py68_builtin_localtime(runtime, 0, arguments, &current) != PY68_STATUS_OK)
            return PY68_STATUS_RUNTIME_ERROR;
        value = (Py68StructTime *)current.as.object;
        for (index = 0; index < sizeof(parts); ++index)
            ((unsigned char *)&parts)[index] = 0;
        parts.tm_year = value->fields[0] - 1900;
        parts.tm_mon = value->fields[1] - 1;
        parts.tm_mday = value->fields[2]; parts.tm_hour = value->fields[3];
        parts.tm_min = value->fields[4]; parts.tm_sec = value->fields[5];
        parts.tm_wday = value->fields[6]; parts.tm_yday = value->fields[7] - 1;
        parts.tm_isdst = value->fields[8];
        py68_value_release(runtime, current);
    } else {
        if (arguments[1].type != PY68_VALUE_OBJECT ||
            arguments[1].as.object == NULL ||
            arguments[1].as.object->type != PY68_OBJECT_STRUCT_TIME)
            return py68_time_error(runtime, PY68_ERROR_TYPE,
                                   "strftime argument must be localtime result");
        value = (Py68StructTime *)arguments[1].as.object;
        parts.tm_year = value->fields[0] - 1900; parts.tm_mon = value->fields[1] - 1;
        parts.tm_mday = value->fields[2]; parts.tm_hour = value->fields[3];
        parts.tm_min = value->fields[4]; parts.tm_sec = value->fields[5];
        parts.tm_wday = value->fields[6]; parts.tm_yday = value->fields[7] - 1;
        parts.tm_isdst = value->fields[8];
    }
    if (strftime(buffer, sizeof(buffer), format->data, &parts) == 0)
        return py68_time_error(runtime, PY68_ERROR_VALUE,
                               "strftime result is too long");
    return py68_time_string(runtime, buffer, (Py68U32)strlen(buffer), result);
}