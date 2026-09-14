/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_string_methods.h"
#include "py68k_dict.h"
#include "py68k_exception.h"
#include "py68k_list.h"
#include "py68k_runtime.h"
#include "py68k_string.h"
#include "py68k_tuple.h"

#include <stddef.h>
#include <string.h>

static void py68_sm_error(Py68Runtime *runtime, Py68ErrorKind kind,
                          const char *message)
{
    Py68Location location;
    location.offset = 0;
    location.line = 0;
    location.column = 0;
    location.length = 0;
    py68_error_set(&runtime->error, kind, location, NULL, message);
}

static int py68_sm_is_upper(unsigned char c)
{
    return c >= (unsigned char)'A' && c <= (unsigned char)'Z';
}

static int py68_sm_is_lower(unsigned char c)
{
    return c >= (unsigned char)'a' && c <= (unsigned char)'z';
}

static int py68_sm_is_alpha(unsigned char c)
{
    return py68_sm_is_upper(c) || py68_sm_is_lower(c);
}

static int py68_sm_is_digit(unsigned char c)
{
    return c >= (unsigned char)'0' && c <= (unsigned char)'9';
}

static int py68_sm_is_alnum(unsigned char c)
{
    return py68_sm_is_alpha(c) || py68_sm_is_digit(c);
}

static int py68_sm_is_space(unsigned char c)
{
    return c == (unsigned char)' ' || c == (unsigned char)'\t' ||
           c == (unsigned char)'\n' || c == (unsigned char)'\r' ||
           c == (unsigned char)'\f' || c == (unsigned char)'\v';
}

static int py68_sm_is_printable(unsigned char c)
{
    return c >= (unsigned char)0x20 && c <= (unsigned char)0x7e;
}

static unsigned char py68_sm_to_upper(unsigned char c)
{
    if (py68_sm_is_lower(c)) return (unsigned char)(c - 32);
    return c;
}

static unsigned char py68_sm_to_lower(unsigned char c)
{
    if (py68_sm_is_upper(c)) return (unsigned char)(c + 32);
    return c;
}

static int py68_sm_require_string(Py68Runtime *runtime, Py68Value value,
                                  Py68String **out, const char *message)
{
    if (value.type != PY68_VALUE_OBJECT || value.as.object == NULL ||
        value.as.object->type != PY68_OBJECT_STRING) {
        py68_sm_error(runtime, PY68_ERROR_TYPE, message);
        return 0;
    }
    *out = (Py68String *)value.as.object;
    return 1;
}

static Py68Status py68_sm_require_self(Py68Runtime *runtime,
                                       Py68U16 argument_count,
                                       Py68Value *arguments, Py68U16 minimum,
                                       Py68U16 maximum, Py68String **self)
{
    if (argument_count < minimum || argument_count > maximum ||
        !py68_sm_require_string(runtime, arguments[0], self,
                                "string method requires a string"))
        return PY68_STATUS_RUNTIME_ERROR;
    return PY68_STATUS_OK;
}

static Py68I32 py68_sm_clamp_index(Py68I32 index, Py68U32 length)
{
    if (index < 0) {
        index += (Py68I32)length;
        if (index < 0) index = 0;
    } else if ((Py68U32)index > length) {
        index = (Py68I32)length;
    }
    return index;
}

static void py68_sm_normalize_range(Py68U32 length, int has_start,
                                    Py68I32 start, int has_end, Py68I32 end,
                                    Py68U32 *out_start, Py68U32 *out_end)
{
    Py68I32 s = has_start ? start : 0;
    Py68I32 e = has_end ? end : (Py68I32)length;
    s = py68_sm_clamp_index(s, length);
    e = py68_sm_clamp_index(e, length);
    if (e < s) e = s;
    *out_start = (Py68U32)s;
    *out_end = (Py68U32)e;
}

static int py68_sm_bytes_equal(const char *a, const char *b, Py68U32 length)
{
    return length == 0 || memcmp(a, b, length) == 0;
}

static Py68I32 py68_sm_find_bytes(const char *hay, const char *needle,
                                  Py68U32 needle_len, Py68U32 start,
                                  Py68U32 end)
{
    Py68U32 pos;
    if (needle_len == 0) return (Py68I32)start;
    if (start > end || needle_len > end - start) return -1;
    for (pos = start; pos + needle_len <= end; ++pos) {
        if (py68_sm_bytes_equal(hay + pos, needle, needle_len))
            return (Py68I32)pos;
    }
    return -1;
}

static Py68I32 py68_sm_rfind_bytes(const char *hay, const char *needle,
                                   Py68U32 needle_len, Py68U32 start,
                                   Py68U32 end)
{
    Py68U32 pos;
    if (needle_len == 0) return (Py68I32)end;
    if (start > end || needle_len > end - start) return -1;
    pos = end - needle_len;
    for (;;) {
        if (pos >= start &&
            py68_sm_bytes_equal(hay + pos, needle, needle_len))
            return (Py68I32)pos;
        if (pos == start) break;
        --pos;
    }
    return -1;
}

static Py68Status py68_sm_map_case(Py68Runtime *runtime, Py68String *self,
                                   int mode, Py68Value *result)
{
    char *buffer;
    Py68U32 index;
    Py68String *out;
    Py68Status status;
    int previous_cased = 0;
    buffer = (char *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP,
                                self->length + 1);
    if (buffer == NULL) return PY68_STATUS_MEMORY_ERROR;
    for (index = 0; index < self->length; ++index) {
        unsigned char c = (unsigned char)self->data[index];
        if (mode == 0) {
            buffer[index] = (char)py68_sm_to_upper(c);
        } else if (mode == 1) {
            buffer[index] = (char)py68_sm_to_lower(c);
        } else if (mode == 2) {
            if (index == 0)
                buffer[index] = (char)py68_sm_to_upper(c);
            else
                buffer[index] = (char)py68_sm_to_lower(c);
        } else if (mode == 3) {
            if (py68_sm_is_upper(c))
                buffer[index] = (char)py68_sm_to_lower(c);
            else if (py68_sm_is_lower(c))
                buffer[index] = (char)py68_sm_to_upper(c);
            else
                buffer[index] = (char)c;
        } else {
            if (py68_sm_is_alpha(c)) {
                if (!previous_cased)
                    buffer[index] = (char)py68_sm_to_upper(c);
                else
                    buffer[index] = (char)py68_sm_to_lower(c);
                previous_cased = 1;
            } else {
                buffer[index] = (char)c;
                previous_cased = 0;
            }
        }
    }
    buffer[self->length] = '\0';
    status = py68_string_new_copy(runtime, buffer, self->length, &out);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer, self->length + 1);
    if (status != PY68_STATUS_OK) return status;
    *result = py68_value_from_object(&out->base);
    return PY68_STATUS_OK;
}

Py68Status py68_attr_string_upper(Py68Runtime *runtime, Py68U16 argument_count,
                                  Py68Value *arguments, Py68Value *result)
{
    Py68String *self;
    if (py68_sm_require_self(runtime, argument_count, arguments, 1, 1, &self) !=
        PY68_STATUS_OK)
        return PY68_STATUS_RUNTIME_ERROR;
    return py68_sm_map_case(runtime, self, 0, result);
}

Py68Status py68_attr_string_lower(Py68Runtime *runtime, Py68U16 argument_count,
                                  Py68Value *arguments, Py68Value *result)
{
    Py68String *self;
    if (py68_sm_require_self(runtime, argument_count, arguments, 1, 1, &self) !=
        PY68_STATUS_OK)
        return PY68_STATUS_RUNTIME_ERROR;
    return py68_sm_map_case(runtime, self, 1, result);
}

Py68Status py68_attr_string_casefold(Py68Runtime *runtime,
                                     Py68U16 argument_count,
                                     Py68Value *arguments, Py68Value *result)
{
    return py68_attr_string_lower(runtime, argument_count, arguments, result);
}

Py68Status py68_attr_string_capitalize(Py68Runtime *runtime,
                                       Py68U16 argument_count,
                                       Py68Value *arguments, Py68Value *result)
{
    Py68String *self;
    if (py68_sm_require_self(runtime, argument_count, arguments, 1, 1, &self) !=
        PY68_STATUS_OK)
        return PY68_STATUS_RUNTIME_ERROR;
    return py68_sm_map_case(runtime, self, 2, result);
}

Py68Status py68_attr_string_swapcase(Py68Runtime *runtime,
                                     Py68U16 argument_count,
                                     Py68Value *arguments, Py68Value *result)
{
    Py68String *self;
    if (py68_sm_require_self(runtime, argument_count, arguments, 1, 1, &self) !=
        PY68_STATUS_OK)
        return PY68_STATUS_RUNTIME_ERROR;
    return py68_sm_map_case(runtime, self, 3, result);
}

Py68Status py68_attr_string_title(Py68Runtime *runtime, Py68U16 argument_count,
                                  Py68Value *arguments, Py68Value *result)
{
    Py68String *self;
    if (py68_sm_require_self(runtime, argument_count, arguments, 1, 1, &self) !=
        PY68_STATUS_OK)
        return PY68_STATUS_RUNTIME_ERROR;
    return py68_sm_map_case(runtime, self, 4, result);
}

static Py68Status py68_sm_parse_sub_range(Py68Runtime *runtime,
                                          Py68U16 argument_count,
                                          Py68Value *arguments,
                                          Py68String **self, Py68String **sub,
                                          Py68U32 *start, Py68U32 *end)
{
    int has_start = 0;
    int has_end = 0;
    Py68I32 start_i = 0;
    Py68I32 end_i = 0;
    if (py68_sm_require_self(runtime, argument_count, arguments, 2, 4, self) !=
        PY68_STATUS_OK)
        return PY68_STATUS_RUNTIME_ERROR;
    if (!py68_sm_require_string(runtime, arguments[1], sub,
                                "substring must be a string"))
        return PY68_STATUS_RUNTIME_ERROR;
    if (argument_count >= 3) {
        if (arguments[2].type != PY68_VALUE_INT &&
            arguments[2].type != PY68_VALUE_BOOL) {
            py68_sm_error(runtime, PY68_ERROR_TYPE, "slice indices must be int");
            return PY68_STATUS_RUNTIME_ERROR;
        }
        has_start = 1;
        start_i = arguments[2].as.integer;
    }
    if (argument_count == 4) {
        if (arguments[3].type != PY68_VALUE_INT &&
            arguments[3].type != PY68_VALUE_BOOL) {
            py68_sm_error(runtime, PY68_ERROR_TYPE, "slice indices must be int");
            return PY68_STATUS_RUNTIME_ERROR;
        }
        has_end = 1;
        end_i = arguments[3].as.integer;
    }
    py68_sm_normalize_range((*self)->length, has_start, start_i, has_end, end_i,
                            start, end);
    return PY68_STATUS_OK;
}

Py68Status py68_attr_string_find(Py68Runtime *runtime, Py68U16 argument_count,
                                 Py68Value *arguments, Py68Value *result)
{
    Py68String *self;
    Py68String *sub;
    Py68U32 start;
    Py68U32 end;
    if (py68_sm_parse_sub_range(runtime, argument_count, arguments, &self, &sub,
                                &start, &end) != PY68_STATUS_OK)
        return PY68_STATUS_RUNTIME_ERROR;
    *result = py68_value_int(py68_sm_find_bytes(self->data,
                                                sub->data, sub->length, start,
                                                end));
    return PY68_STATUS_OK;
}

Py68Status py68_attr_string_rfind(Py68Runtime *runtime, Py68U16 argument_count,
                                  Py68Value *arguments, Py68Value *result)
{
    Py68String *self;
    Py68String *sub;
    Py68U32 start;
    Py68U32 end;
    if (py68_sm_parse_sub_range(runtime, argument_count, arguments, &self, &sub,
                                &start, &end) != PY68_STATUS_OK)
        return PY68_STATUS_RUNTIME_ERROR;
    *result = py68_value_int(py68_sm_rfind_bytes(self->data,
                                                 sub->data, sub->length, start,
                                                 end));
    return PY68_STATUS_OK;
}

Py68Status py68_attr_string_index(Py68Runtime *runtime, Py68U16 argument_count,
                                  Py68Value *arguments, Py68Value *result)
{
    Py68Status status =
        py68_attr_string_find(runtime, argument_count, arguments, result);
    if (status != PY68_STATUS_OK) return status;
    if (result->as.integer < 0) {
        py68_sm_error(runtime, PY68_ERROR_VALUE, "substring not found");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    return PY68_STATUS_OK;
}

Py68Status py68_attr_string_rindex(Py68Runtime *runtime, Py68U16 argument_count,
                                   Py68Value *arguments, Py68Value *result)
{
    Py68Status status =
        py68_attr_string_rfind(runtime, argument_count, arguments, result);
    if (status != PY68_STATUS_OK) return status;
    if (result->as.integer < 0) {
        py68_sm_error(runtime, PY68_ERROR_VALUE, "substring not found");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    return PY68_STATUS_OK;
}

Py68Status py68_attr_string_count(Py68Runtime *runtime, Py68U16 argument_count,
                                  Py68Value *arguments, Py68Value *result)
{
    Py68String *self;
    Py68String *sub;
    Py68U32 start;
    Py68U32 end;
    Py68U32 count = 0;
    Py68U32 pos;
    if (py68_sm_parse_sub_range(runtime, argument_count, arguments, &self, &sub,
                                &start, &end) != PY68_STATUS_OK)
        return PY68_STATUS_RUNTIME_ERROR;
    if (sub->length == 0) {
        *result = py68_value_int((Py68I32)(end - start + 1));
        return PY68_STATUS_OK;
    }
    pos = start;
    while (pos + sub->length <= end) {
        Py68I32 found =
            py68_sm_find_bytes(self->data, sub->data, sub->length,
                               pos, end);
        if (found < 0) break;
        ++count;
        pos = (Py68U32)found + sub->length;
    }
    *result = py68_value_int((Py68I32)count);
    return PY68_STATUS_OK;
}

static int py68_sm_prefix_at(Py68String *self, Py68String *prefix, Py68U32 start,
                             Py68U32 end)
{
    if (prefix->length > end - start) return 0;
    return py68_sm_bytes_equal(self->data + start, prefix->data, prefix->length);
}

static int py68_sm_suffix_at(Py68String *self, Py68String *suffix, Py68U32 start,
                             Py68U32 end)
{
    if (suffix->length > end - start) return 0;
    return py68_sm_bytes_equal(self->data + end - suffix->length, suffix->data,
                               suffix->length);
}

static Py68Status py68_sm_startswith_endswith(Py68Runtime *runtime,
                                              Py68U16 argument_count,
                                              Py68Value *arguments,
                                              Py68Value *result, int is_start)
{
    Py68String *self;
    Py68U32 start;
    Py68U32 end;
    int has_start = 0;
    int has_end = 0;
    Py68I32 start_i = 0;
    Py68I32 end_i = 0;
    Py68Value prefix;
    if (py68_sm_require_self(runtime, argument_count, arguments, 2, 4, &self) !=
        PY68_STATUS_OK)
        return PY68_STATUS_RUNTIME_ERROR;
    prefix = arguments[1];
    if (argument_count >= 3) {
        if (arguments[2].type != PY68_VALUE_INT &&
            arguments[2].type != PY68_VALUE_BOOL) {
            py68_sm_error(runtime, PY68_ERROR_TYPE, "slice indices must be int");
            return PY68_STATUS_RUNTIME_ERROR;
        }
        has_start = 1;
        start_i = arguments[2].as.integer;
    }
    if (argument_count == 4) {
        if (arguments[3].type != PY68_VALUE_INT &&
            arguments[3].type != PY68_VALUE_BOOL) {
            py68_sm_error(runtime, PY68_ERROR_TYPE, "slice indices must be int");
            return PY68_STATUS_RUNTIME_ERROR;
        }
        has_end = 1;
        end_i = arguments[3].as.integer;
    }
    py68_sm_normalize_range(self->length, has_start, start_i, has_end, end_i,
                            &start, &end);
    if (prefix.type == PY68_VALUE_OBJECT && prefix.as.object != NULL &&
        prefix.as.object->type == PY68_OBJECT_STRING) {
        Py68String *p = (Py68String *)prefix.as.object;
        *result = py68_value_bool(is_start ? py68_sm_prefix_at(self, p, start, end)
                                           : py68_sm_suffix_at(self, p, start, end));
        return PY68_STATUS_OK;
    }
    if (prefix.type == PY68_VALUE_OBJECT && prefix.as.object != NULL &&
        prefix.as.object->type == PY68_OBJECT_TUPLE) {
        Py68Tuple *tuple = (Py68Tuple *)prefix.as.object;
        Py68U32 index;
        for (index = 0; index < tuple->count; ++index) {
            Py68String *p;
            if (!py68_sm_require_string(runtime, tuple->items[index], &p,
                                        "tuple item must be a string"))
                return PY68_STATUS_RUNTIME_ERROR;
            if (is_start ? py68_sm_prefix_at(self, p, start, end)
                         : py68_sm_suffix_at(self, p, start, end)) {
                *result = py68_value_bool(1);
                return PY68_STATUS_OK;
            }
        }
        *result = py68_value_bool(0);
        return PY68_STATUS_OK;
    }
    py68_sm_error(runtime, PY68_ERROR_TYPE,
                  "prefix/suffix must be string or tuple of strings");
    return PY68_STATUS_RUNTIME_ERROR;
}

Py68Status py68_attr_string_startswith(Py68Runtime *runtime,
                                       Py68U16 argument_count,
                                       Py68Value *arguments, Py68Value *result)
{
    return py68_sm_startswith_endswith(runtime, argument_count, arguments,
                                       result, 1);
}

Py68Status py68_attr_string_endswith(Py68Runtime *runtime,
                                     Py68U16 argument_count,
                                     Py68Value *arguments, Py68Value *result)
{
    return py68_sm_startswith_endswith(runtime, argument_count, arguments,
                                       result, 0);
}

static int py68_sm_char_in_set(unsigned char c, const char *chars,
                               Py68U32 length, int default_space)
{
    Py68U32 index;
    if (default_space) return py68_sm_is_space(c);
    for (index = 0; index < length; ++index)
        if ((unsigned char)chars[index] == c) return 1;
    return 0;
}

static Py68Status py68_sm_strip_impl(Py68Runtime *runtime, Py68String *self,
                                     const char *chars, Py68U32 chars_len,
                                     int default_space, int left, int right,
                                     Py68Value *result)
{
    Py68U32 start = 0;
    Py68U32 end = self->length;
    Py68String *out;
    Py68Status status;
    if (left) {
        while (start < end &&
               py68_sm_char_in_set((unsigned char)self->data[start], chars,
                                   chars_len, default_space))
            ++start;
    }
    if (right) {
        while (end > start &&
               py68_sm_char_in_set((unsigned char)self->data[end - 1], chars,
                                   chars_len, default_space))
            --end;
    }
    status = py68_string_new_copy(runtime, self->data + start, end - start, &out);
    if (status != PY68_STATUS_OK) return status;
    *result = py68_value_from_object(&out->base);
    return PY68_STATUS_OK;
}

static Py68Status py68_sm_strip_args(Py68Runtime *runtime,
                                     Py68U16 argument_count,
                                     Py68Value *arguments, int left, int right,
                                     Py68Value *result)
{
    Py68String *self;
    const char *chars = NULL;
    Py68U32 chars_len = 0;
    int default_space = 1;
    if (py68_sm_require_self(runtime, argument_count, arguments, 1, 2, &self) !=
        PY68_STATUS_OK)
        return PY68_STATUS_RUNTIME_ERROR;
    if (argument_count == 2) {
        Py68String *set;
        if (arguments[1].type == PY68_VALUE_NONE) {
            default_space = 1;
        } else if (py68_sm_require_string(runtime, arguments[1], &set,
                                          "strip chars must be a string")) {
            chars = set->data;
            chars_len = set->length;
            default_space = 0;
        } else {
            return PY68_STATUS_RUNTIME_ERROR;
        }
    }
    return py68_sm_strip_impl(runtime, self, chars, chars_len, default_space,
                              left, right, result);
}

Py68Status py68_attr_string_strip(Py68Runtime *runtime, Py68U16 argument_count,
                                  Py68Value *arguments, Py68Value *result)
{
    return py68_sm_strip_args(runtime, argument_count, arguments, 1, 1, result);
}

Py68Status py68_attr_string_lstrip(Py68Runtime *runtime, Py68U16 argument_count,
                                   Py68Value *arguments, Py68Value *result)
{
    return py68_sm_strip_args(runtime, argument_count, arguments, 1, 0, result);
}

Py68Status py68_attr_string_rstrip(Py68Runtime *runtime, Py68U16 argument_count,
                                   Py68Value *arguments, Py68Value *result)
{
    return py68_sm_strip_args(runtime, argument_count, arguments, 0, 1, result);
}

Py68Status py68_attr_string_removeprefix(Py68Runtime *runtime,
                                         Py68U16 argument_count,
                                         Py68Value *arguments,
                                         Py68Value *result)
{
    Py68String *self;
    Py68String *prefix;
    Py68String *out;
    Py68Status status;
    if (py68_sm_require_self(runtime, argument_count, arguments, 2, 2, &self) !=
            PY68_STATUS_OK ||
        !py68_sm_require_string(runtime, arguments[1], &prefix,
                                "prefix must be a string"))
        return PY68_STATUS_RUNTIME_ERROR;
    if (prefix->length <= self->length &&
        py68_sm_bytes_equal(self->data, prefix->data, prefix->length))
        status = py68_string_new_copy(runtime, self->data + prefix->length,
                                      self->length - prefix->length, &out);
    else
        status = py68_string_new_copy(runtime, self->data, self->length, &out);
    if (status != PY68_STATUS_OK) return status;
    *result = py68_value_from_object(&out->base);
    return PY68_STATUS_OK;
}

Py68Status py68_attr_string_removesuffix(Py68Runtime *runtime,
                                         Py68U16 argument_count,
                                         Py68Value *arguments,
                                         Py68Value *result)
{
    Py68String *self;
    Py68String *suffix;
    Py68String *out;
    Py68Status status;
    if (py68_sm_require_self(runtime, argument_count, arguments, 2, 2, &self) !=
            PY68_STATUS_OK ||
        !py68_sm_require_string(runtime, arguments[1], &suffix,
                                "suffix must be a string"))
        return PY68_STATUS_RUNTIME_ERROR;
    if (suffix->length <= self->length &&
        py68_sm_bytes_equal(self->data + self->length - suffix->length,
                            suffix->data, suffix->length))
        status = py68_string_new_copy(runtime, self->data,
                                      self->length - suffix->length, &out);
    else
        status = py68_string_new_copy(runtime, self->data, self->length, &out);
    if (status != PY68_STATUS_OK) return status;
    *result = py68_value_from_object(&out->base);
    return PY68_STATUS_OK;
}

static Py68Status py68_sm_append_slice(Py68Runtime *runtime, Py68List *list,
                                       Py68String *self, Py68U32 start,
                                       Py68U32 end)
{
    Py68String *part;
    Py68Status status =
        py68_string_new_copy(runtime, self->data + start, end - start, &part);
    if (status != PY68_STATUS_OK) return status;
    status = py68_list_append_copy(runtime, list, py68_value_from_object(&part->base));
    py68_object_release(runtime, &part->base);
    return status;
}

static Py68Status py68_sm_split_ws(Py68Runtime *runtime, Py68String *self,
                                   Py68I32 maxsplit, Py68Value *result)
{
    Py68List *list;
    Py68U32 index = 0;
    Py68I32 splits = 0;
    Py68Status status = py68_list_new(runtime, &list);
    if (status != PY68_STATUS_OK) return status;
    while (index < self->length) {
        Py68U32 start;
        while (index < self->length &&
               py68_sm_is_space((unsigned char)self->data[index]))
            ++index;
        if (index >= self->length) break;
        if (maxsplit >= 0 && splits >= maxsplit) {
            status = py68_sm_append_slice(runtime, list, self, index,
                                          self->length);
            if (status != PY68_STATUS_OK) {
                py68_object_release(runtime, &list->base);
                return status;
            }
            break;
        }
        start = index;
        while (index < self->length &&
               !py68_sm_is_space((unsigned char)self->data[index]))
            ++index;
        status = py68_sm_append_slice(runtime, list, self, start, index);
        if (status != PY68_STATUS_OK) {
            py68_object_release(runtime, &list->base);
            return status;
        }
        ++splits;
    }
    *result = py68_value_from_object(&list->base);
    return PY68_STATUS_OK;
}

static Py68Status py68_sm_split_sep(Py68Runtime *runtime, Py68String *self,
                                    Py68String *sep, Py68I32 maxsplit,
                                    int from_right, Py68Value *result)
{
    Py68List *list;
    Py68Status status;
    Py68U32 start = 0;
    Py68I32 splits = 0;
    if (sep->length == 0) {
        py68_sm_error(runtime, PY68_ERROR_VALUE, "empty separator");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    status = py68_list_new(runtime, &list);
    if (status != PY68_STATUS_OK) return status;
    if (!from_right) {
        while (start <= self->length) {
            Py68I32 found;
            if (maxsplit >= 0 && splits >= maxsplit) {
                status = py68_sm_append_slice(runtime, list, self, start,
                                              self->length);
                if (status != PY68_STATUS_OK) {
                    py68_object_release(runtime, &list->base);
                    return status;
                }
                break;
            }
            found = py68_sm_find_bytes(self->data, sep->data,
                                       sep->length, start, self->length);
            if (found < 0) {
                status = py68_sm_append_slice(runtime, list, self, start,
                                              self->length);
                if (status != PY68_STATUS_OK) {
                    py68_object_release(runtime, &list->base);
                    return status;
                }
                break;
            }
            status = py68_sm_append_slice(runtime, list, self, start,
                                          (Py68U32)found);
            if (status != PY68_STATUS_OK) {
                py68_object_release(runtime, &list->base);
                return status;
            }
            start = (Py68U32)found + sep->length;
            ++splits;
        }
    } else {
        /* Collect from the right then reverse. */
        Py68U32 end = self->length;
        Py68List *temp;
        Py68U32 index;
        status = py68_list_new(runtime, &temp);
        if (status != PY68_STATUS_OK) {
            py68_object_release(runtime, &list->base);
            return status;
        }
        while (end > 0) {
            Py68I32 found;
            if (maxsplit >= 0 && splits >= maxsplit) {
                status = py68_sm_append_slice(runtime, temp, self, 0, end);
                if (status != PY68_STATUS_OK) {
                    py68_object_release(runtime, &temp->base);
                    py68_object_release(runtime, &list->base);
                    return status;
                }
                break;
            }
            found = py68_sm_rfind_bytes(self->data, sep->data,
                                        sep->length, 0, end);
            if (found < 0) {
                status = py68_sm_append_slice(runtime, temp, self, 0, end);
                if (status != PY68_STATUS_OK) {
                    py68_object_release(runtime, &temp->base);
                    py68_object_release(runtime, &list->base);
                    return status;
                }
                break;
            }
            status = py68_sm_append_slice(runtime, temp, self,
                                          (Py68U32)found + sep->length, end);
            if (status != PY68_STATUS_OK) {
                py68_object_release(runtime, &temp->base);
                py68_object_release(runtime, &list->base);
                return status;
            }
            end = (Py68U32)found;
            ++splits;
            if (end == 0) {
                status = py68_sm_append_slice(runtime, temp, self, 0, 0);
                if (status != PY68_STATUS_OK) {
                    py68_object_release(runtime, &temp->base);
                    py68_object_release(runtime, &list->base);
                    return status;
                }
                break;
            }
        }
        if (temp->count == 0) {
            status = py68_sm_append_slice(runtime, temp, self, 0, self->length);
            if (status != PY68_STATUS_OK) {
                py68_object_release(runtime, &temp->base);
                py68_object_release(runtime, &list->base);
                return status;
            }
        }
        for (index = temp->count; index > 0; --index) {
            status = py68_list_append_copy(runtime, list,
                                           temp->items[index - 1]);
            if (status != PY68_STATUS_OK) {
                py68_object_release(runtime, &temp->base);
                py68_object_release(runtime, &list->base);
                return status;
            }
        }
        py68_object_release(runtime, &temp->base);
    }
    *result = py68_value_from_object(&list->base);
    return PY68_STATUS_OK;
}

static Py68Status py68_sm_split_common(Py68Runtime *runtime,
                                       Py68U16 argument_count,
                                       Py68Value *arguments, int from_right,
                                       Py68Value *result)
{
    Py68String *self;
    Py68I32 maxsplit = -1;
    if (py68_sm_require_self(runtime, argument_count, arguments, 1, 3, &self) !=
        PY68_STATUS_OK)
        return PY68_STATUS_RUNTIME_ERROR;
    if (argument_count >= 3) {
        if (arguments[2].type != PY68_VALUE_INT &&
            arguments[2].type != PY68_VALUE_BOOL) {
            py68_sm_error(runtime, PY68_ERROR_TYPE, "maxsplit must be int");
            return PY68_STATUS_RUNTIME_ERROR;
        }
        maxsplit = arguments[2].as.integer;
    }
    if (argument_count == 1 ||
        (argument_count >= 2 && arguments[1].type == PY68_VALUE_NONE)) {
        if (from_right) {
            /* rsplit with None uses same whitespace algorithm as split. */
            return py68_sm_split_ws(runtime, self, maxsplit, result);
        }
        return py68_sm_split_ws(runtime, self, maxsplit, result);
    }
    {
        Py68String *sep;
        if (!py68_sm_require_string(runtime, arguments[1], &sep,
                                    "separator must be a string"))
            return PY68_STATUS_RUNTIME_ERROR;
        return py68_sm_split_sep(runtime, self, sep, maxsplit, from_right,
                                 result);
    }
}

Py68Status py68_attr_string_split(Py68Runtime *runtime, Py68U16 argument_count,
                                  Py68Value *arguments, Py68Value *result)
{
    return py68_sm_split_common(runtime, argument_count, arguments, 0, result);
}

Py68Status py68_attr_string_rsplit(Py68Runtime *runtime, Py68U16 argument_count,
                                   Py68Value *arguments, Py68Value *result)
{
    return py68_sm_split_common(runtime, argument_count, arguments, 1, result);
}

Py68Status py68_attr_string_splitlines(Py68Runtime *runtime,
                                       Py68U16 argument_count,
                                       Py68Value *arguments, Py68Value *result)
{
    Py68String *self;
    Py68List *list;
    Py68U32 index = 0;
    int keepends = 0;
    Py68Status status;
    if (py68_sm_require_self(runtime, argument_count, arguments, 1, 2, &self) !=
        PY68_STATUS_OK)
        return PY68_STATUS_RUNTIME_ERROR;
    if (argument_count == 2) {
        if (arguments[1].type != PY68_VALUE_BOOL &&
            arguments[1].type != PY68_VALUE_INT) {
            py68_sm_error(runtime, PY68_ERROR_TYPE, "keepends must be bool/int");
            return PY68_STATUS_RUNTIME_ERROR;
        }
        keepends = arguments[1].as.integer != 0;
    }
    status = py68_list_new(runtime, &list);
    if (status != PY68_STATUS_OK) return status;
    while (index < self->length) {
        Py68U32 start = index;
        Py68U32 end;
        while (index < self->length) {
            unsigned char c = (unsigned char)self->data[index];
            if (c == (unsigned char)'\n' || c == (unsigned char)'\r' ||
                c == (unsigned char)'\v' || c == (unsigned char)'\f')
                break;
            ++index;
        }
        end = index;
        if (index < self->length) {
            unsigned char c = (unsigned char)self->data[index];
            ++index;
            if (c == (unsigned char)'\r' && index < self->length &&
                (unsigned char)self->data[index] == (unsigned char)'\n')
                ++index;
            if (keepends) end = index;
        }
        status = py68_sm_append_slice(runtime, list, self, start, end);
        if (status != PY68_STATUS_OK) {
            py68_object_release(runtime, &list->base);
            return status;
        }
    }
    *result = py68_value_from_object(&list->base);
    return PY68_STATUS_OK;
}

static Py68Status py68_sm_partition(Py68Runtime *runtime, Py68String *self,
                                    Py68String *sep, int from_right,
                                    Py68Value *result)
{
    Py68Tuple *tuple;
    Py68String *before;
    Py68String *middle;
    Py68String *after;
    Py68I32 found;
    Py68Status status;
    if (sep->length == 0) {
        py68_sm_error(runtime, PY68_ERROR_VALUE, "empty separator");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    found = from_right
                ? py68_sm_rfind_bytes(self->data, sep->data,
                                      sep->length, 0, self->length)
                : py68_sm_find_bytes(self->data, sep->data,
                                     sep->length, 0, self->length);
    status = py68_tuple_new(runtime, 3, &tuple);
    if (status != PY68_STATUS_OK) return status;
    if (found < 0) {
        if (from_right) {
            status = py68_string_new_copy(runtime, "", 0, &before);
            if (status == PY68_STATUS_OK)
                status = py68_string_new_copy(runtime, "", 0, &middle);
            if (status == PY68_STATUS_OK)
                status = py68_string_new_copy(runtime, self->data, self->length,
                                              &after);
        } else {
            status = py68_string_new_copy(runtime, self->data, self->length,
                                          &before);
            if (status == PY68_STATUS_OK)
                status = py68_string_new_copy(runtime, "", 0, &middle);
            if (status == PY68_STATUS_OK)
                status = py68_string_new_copy(runtime, "", 0, &after);
        }
    } else {
        status = py68_string_new_copy(runtime, self->data, (Py68U32)found,
                                      &before);
        if (status == PY68_STATUS_OK)
            status = py68_string_new_copy(runtime, sep->data, sep->length,
                                          &middle);
        if (status == PY68_STATUS_OK)
            status = py68_string_new_copy(
                runtime, self->data + (Py68U32)found + sep->length,
                self->length - (Py68U32)found - sep->length, &after);
    }
    if (status != PY68_STATUS_OK) {
        py68_object_release(runtime, &tuple->base);
        return status;
    }
    tuple->items[0] = py68_value_from_object(&before->base);
    tuple->items[1] = py68_value_from_object(&middle->base);
    tuple->items[2] = py68_value_from_object(&after->base);
    *result = py68_value_from_object(&tuple->base);
    return PY68_STATUS_OK;
}

Py68Status py68_attr_string_partition(Py68Runtime *runtime,
                                      Py68U16 argument_count,
                                      Py68Value *arguments, Py68Value *result)
{
    Py68String *self;
    Py68String *sep;
    if (py68_sm_require_self(runtime, argument_count, arguments, 2, 2, &self) !=
            PY68_STATUS_OK ||
        !py68_sm_require_string(runtime, arguments[1], &sep,
                                "separator must be a string"))
        return PY68_STATUS_RUNTIME_ERROR;
    return py68_sm_partition(runtime, self, sep, 0, result);
}

Py68Status py68_attr_string_rpartition(Py68Runtime *runtime,
                                       Py68U16 argument_count,
                                       Py68Value *arguments, Py68Value *result)
{
    Py68String *self;
    Py68String *sep;
    if (py68_sm_require_self(runtime, argument_count, arguments, 2, 2, &self) !=
            PY68_STATUS_OK ||
        !py68_sm_require_string(runtime, arguments[1], &sep,
                                "separator must be a string"))
        return PY68_STATUS_RUNTIME_ERROR;
    return py68_sm_partition(runtime, self, sep, 1, result);
}

Py68Status py68_attr_string_join(Py68Runtime *runtime, Py68U16 argument_count,
                                 Py68Value *arguments, Py68Value *result)
{
    Py68String *self;
    Py68Value *items = NULL;
    Py68U32 count = 0;
    Py68U32 index;
    Py68U32 total = 0;
    char *buffer;
    Py68U32 offset = 0;
    Py68String *out;
    Py68Status status;
    if (py68_sm_require_self(runtime, argument_count, arguments, 2, 2, &self) !=
        PY68_STATUS_OK)
        return PY68_STATUS_RUNTIME_ERROR;
    if (arguments[1].type != PY68_VALUE_OBJECT || arguments[1].as.object == NULL) {
        py68_sm_error(runtime, PY68_ERROR_TYPE, "join argument must be list or tuple");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (arguments[1].as.object->type == PY68_OBJECT_LIST) {
        Py68List *list = (Py68List *)arguments[1].as.object;
        items = list->items;
        count = list->count;
    } else if (arguments[1].as.object->type == PY68_OBJECT_TUPLE) {
        Py68Tuple *tuple = (Py68Tuple *)arguments[1].as.object;
        items = tuple->items;
        count = tuple->count;
    } else {
        py68_sm_error(runtime, PY68_ERROR_TYPE, "join argument must be list or tuple");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    for (index = 0; index < count; ++index) {
        Py68String *part;
        if (!py68_sm_require_string(runtime, items[index], &part,
                                    "sequence item must be a string"))
            return PY68_STATUS_RUNTIME_ERROR;
        if (index > 0) {
            if (total > (Py68U32)~(Py68U32)0 - self->length) {
                py68_sm_error(runtime, PY68_ERROR_OVERFLOW, "join result too large");
                return PY68_STATUS_RUNTIME_ERROR;
            }
            total += self->length;
        }
        if (total > (Py68U32)~(Py68U32)0 - part->length) {
            py68_sm_error(runtime, PY68_ERROR_OVERFLOW, "join result too large");
            return PY68_STATUS_RUNTIME_ERROR;
        }
        total += part->length;
    }
    buffer = (char *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP, total + 1);
    if (buffer == NULL) return PY68_STATUS_MEMORY_ERROR;
    for (index = 0; index < count; ++index) {
        Py68String *part = (Py68String *)items[index].as.object;
        if (index > 0 && self->length != 0) {
            memcpy(buffer + offset, self->data, self->length);
            offset += self->length;
        }
        if (part->length != 0) {
            memcpy(buffer + offset, part->data, part->length);
            offset += part->length;
        }
    }
    buffer[total] = '\0';
    status = py68_string_new_copy(runtime, buffer, total, &out);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer, total + 1);
    if (status != PY68_STATUS_OK) return status;
    *result = py68_value_from_object(&out->base);
    return PY68_STATUS_OK;
}

Py68Status py68_attr_string_replace(Py68Runtime *runtime,
                                    Py68U16 argument_count,
                                    Py68Value *arguments, Py68Value *result)
{
    Py68String *self;
    Py68String *old_s;
    Py68String *new_s;
    Py68I32 count = -1;
    Py68U32 capacity;
    Py68U32 length = 0;
    char *buffer;
    Py68U32 pos = 0;
    Py68I32 replacements = 0;
    Py68String *out;
    Py68Status status;
    if (py68_sm_require_self(runtime, argument_count, arguments, 3, 4, &self) !=
            PY68_STATUS_OK ||
        !py68_sm_require_string(runtime, arguments[1], &old_s,
                                "old must be a string") ||
        !py68_sm_require_string(runtime, arguments[2], &new_s,
                                "new must be a string"))
        return PY68_STATUS_RUNTIME_ERROR;
    if (argument_count == 4) {
        if (arguments[3].type != PY68_VALUE_INT &&
            arguments[3].type != PY68_VALUE_BOOL) {
            py68_sm_error(runtime, PY68_ERROR_TYPE, "count must be int");
            return PY68_STATUS_RUNTIME_ERROR;
        }
        count = arguments[3].as.integer;
    }
    if (count == 0) {
        status = py68_string_new_copy(runtime, self->data, self->length, &out);
        if (status != PY68_STATUS_OK) return status;
        *result = py68_value_from_object(&out->base);
        return PY68_STATUS_OK;
    }
    capacity = self->length + 1;
    if (new_s->length > old_s->length && old_s->length == 0)
        capacity = self->length * (new_s->length + 1) + new_s->length + 1;
    else if (new_s->length > old_s->length)
        capacity = self->length +
                   (self->length / (old_s->length == 0 ? 1 : old_s->length) + 1) *
                       (new_s->length - old_s->length) +
                   1;
    buffer = (char *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP, capacity);
    if (buffer == NULL) return PY68_STATUS_MEMORY_ERROR;
    if (old_s->length == 0) {
        /* Insert new around every character, including ends: 'abc'+'-' -> -a-b-c- */
        while (pos <= self->length) {
            if (count >= 0 && replacements >= count) {
                if (pos < self->length) {
                    memcpy(buffer + length, self->data + pos, self->length - pos);
                    length += self->length - pos;
                }
                break;
            }
            if (new_s->length != 0) {
                memcpy(buffer + length, new_s->data, new_s->length);
                length += new_s->length;
            }
            ++replacements;
            if (pos == self->length) break;
            buffer[length++] = self->data[pos++];
        }
    } else {
        while (pos <= self->length) {
            Py68I32 found;
            if (count >= 0 && replacements >= count) {
                memcpy(buffer + length, self->data + pos, self->length - pos);
                length += self->length - pos;
                break;
            }
            found = py68_sm_find_bytes(self->data, old_s->data,
                                       old_s->length, pos, self->length);
            if (found < 0) {
                memcpy(buffer + length, self->data + pos, self->length - pos);
                length += self->length - pos;
                break;
            }
            if ((Py68U32)found > pos) {
                memcpy(buffer + length, self->data + pos, (Py68U32)found - pos);
                length += (Py68U32)found - pos;
            }
            if (new_s->length != 0) {
                memcpy(buffer + length, new_s->data, new_s->length);
                length += new_s->length;
            }
            pos = (Py68U32)found + old_s->length;
            ++replacements;
        }
    }
    status = py68_string_new_copy(runtime, buffer, length, &out);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer, capacity);
    if (status != PY68_STATUS_OK) return status;
    *result = py68_value_from_object(&out->base);
    return PY68_STATUS_OK;
}

static Py68Status py68_sm_get_fillchar(Py68Runtime *runtime, Py68Value value,
                                       char *fill)
{
    Py68String *string;
    if (!py68_sm_require_string(runtime, value, &string,
                                "fillchar must be a string"))
        return PY68_STATUS_RUNTIME_ERROR;
    if (string->length != 1) {
        py68_sm_error(runtime, PY68_ERROR_TYPE,
                      "fillchar must be exactly one character");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    *fill = string->data[0];
    return PY68_STATUS_OK;
}

static Py68Status py68_sm_pad(Py68Runtime *runtime, Py68String *self,
                              Py68I32 width, char fill, int mode,
                              Py68Value *result)
{
    Py68U32 target;
    Py68U32 pad;
    Py68U32 left;
    Py68U32 right;
    char *buffer;
    Py68U32 index;
    Py68String *out;
    Py68Status status;
    if (width <= (Py68I32)self->length) {
        status = py68_string_new_copy(runtime, self->data, self->length, &out);
        if (status != PY68_STATUS_OK) return status;
        *result = py68_value_from_object(&out->base);
        return PY68_STATUS_OK;
    }
    target = (Py68U32)width;
    pad = target - self->length;
    if (mode == 0) {
        left = pad / 2;
        right = pad - left;
    } else if (mode == 1) {
        left = 0;
        right = pad;
    } else {
        left = pad;
        right = 0;
    }
    buffer = (char *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP, target + 1);
    if (buffer == NULL) return PY68_STATUS_MEMORY_ERROR;
    for (index = 0; index < left; ++index) buffer[index] = fill;
    if (self->length != 0)
        memcpy(buffer + left, self->data, self->length);
    for (index = 0; index < right; ++index)
        buffer[left + self->length + index] = fill;
    buffer[target] = '\0';
    status = py68_string_new_copy(runtime, buffer, target, &out);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer, target + 1);
    if (status != PY68_STATUS_OK) return status;
    *result = py68_value_from_object(&out->base);
    return PY68_STATUS_OK;
}

static Py68Status py68_sm_align(Py68Runtime *runtime, Py68U16 argument_count,
                                Py68Value *arguments, int mode,
                                Py68Value *result)
{
    Py68String *self;
    Py68I32 width;
    char fill = ' ';
    if (py68_sm_require_self(runtime, argument_count, arguments, 2, 3, &self) !=
        PY68_STATUS_OK)
        return PY68_STATUS_RUNTIME_ERROR;
    if (arguments[1].type != PY68_VALUE_INT &&
        arguments[1].type != PY68_VALUE_BOOL) {
        py68_sm_error(runtime, PY68_ERROR_TYPE, "width must be int");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    width = arguments[1].as.integer;
    if (argument_count == 3) {
        if (py68_sm_get_fillchar(runtime, arguments[2], &fill) != PY68_STATUS_OK)
            return PY68_STATUS_RUNTIME_ERROR;
    }
    return py68_sm_pad(runtime, self, width, fill, mode, result);
}

Py68Status py68_attr_string_center(Py68Runtime *runtime, Py68U16 argument_count,
                                   Py68Value *arguments, Py68Value *result)
{
    return py68_sm_align(runtime, argument_count, arguments, 0, result);
}

Py68Status py68_attr_string_ljust(Py68Runtime *runtime, Py68U16 argument_count,
                                  Py68Value *arguments, Py68Value *result)
{
    return py68_sm_align(runtime, argument_count, arguments, 1, result);
}

Py68Status py68_attr_string_rjust(Py68Runtime *runtime, Py68U16 argument_count,
                                  Py68Value *arguments, Py68Value *result)
{
    return py68_sm_align(runtime, argument_count, arguments, 2, result);
}

Py68Status py68_attr_string_zfill(Py68Runtime *runtime, Py68U16 argument_count,
                                  Py68Value *arguments, Py68Value *result)
{
    Py68String *self;
    Py68I32 width;
    Py68U32 sign = 0;
    char *buffer;
    Py68U32 target;
    Py68U32 pad;
    Py68U32 index;
    Py68String *out;
    Py68Status status;
    if (py68_sm_require_self(runtime, argument_count, arguments, 2, 2, &self) !=
        PY68_STATUS_OK)
        return PY68_STATUS_RUNTIME_ERROR;
    if (arguments[1].type != PY68_VALUE_INT &&
        arguments[1].type != PY68_VALUE_BOOL) {
        py68_sm_error(runtime, PY68_ERROR_TYPE, "width must be int");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    width = arguments[1].as.integer;
    if (width <= (Py68I32)self->length) {
        status = py68_string_new_copy(runtime, self->data, self->length, &out);
        if (status != PY68_STATUS_OK) return status;
        *result = py68_value_from_object(&out->base);
        return PY68_STATUS_OK;
    }
    if (self->length > 0 &&
        (self->data[0] == '+' || self->data[0] == '-'))
        sign = 1;
    target = (Py68U32)width;
    pad = target - self->length;
    buffer = (char *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP, target + 1);
    if (buffer == NULL) return PY68_STATUS_MEMORY_ERROR;
    if (sign) buffer[0] = self->data[0];
    for (index = 0; index < pad; ++index) buffer[sign + index] = '0';
    if (self->length > sign)
        memcpy(buffer + sign + pad, self->data + sign, self->length - sign);
    buffer[target] = '\0';
    status = py68_string_new_copy(runtime, buffer, target, &out);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer, target + 1);
    if (status != PY68_STATUS_OK) return status;
    *result = py68_value_from_object(&out->base);
    return PY68_STATUS_OK;
}

Py68Status py68_attr_string_expandtabs(Py68Runtime *runtime,
                                       Py68U16 argument_count,
                                       Py68Value *arguments, Py68Value *result)
{
    Py68String *self;
    Py68I32 tabsize = 8;
    Py68U32 index;
    Py68U32 col = 0;
    Py68U32 length = 0;
    Py68U32 capacity;
    char *buffer;
    Py68String *out;
    Py68Status status;
    if (py68_sm_require_self(runtime, argument_count, arguments, 1, 2, &self) !=
        PY68_STATUS_OK)
        return PY68_STATUS_RUNTIME_ERROR;
    if (argument_count == 2) {
        if (arguments[1].type != PY68_VALUE_INT &&
            arguments[1].type != PY68_VALUE_BOOL) {
            py68_sm_error(runtime, PY68_ERROR_TYPE, "tabsize must be int");
            return PY68_STATUS_RUNTIME_ERROR;
        }
        tabsize = arguments[1].as.integer;
    }
    if (tabsize < 0) tabsize = 0;
    capacity = self->length * ((tabsize > 1) ? (Py68U32)tabsize : 1) + 1;
    if (capacity < self->length + 1) capacity = self->length + 1;
    buffer = (char *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP, capacity);
    if (buffer == NULL) return PY68_STATUS_MEMORY_ERROR;
    for (index = 0; index < self->length; ++index) {
        char c = self->data[index];
        if (c == '\t') {
            Py68U32 spaces;
            if (tabsize == 0) continue;
            spaces = (Py68U32)tabsize - (col % (Py68U32)tabsize);
            while (spaces != 0) {
                buffer[length++] = ' ';
                ++col;
                --spaces;
            }
        } else {
            buffer[length++] = c;
            if (c == '\n' || c == '\r')
                col = 0;
            else
                ++col;
        }
    }
    status = py68_string_new_copy(runtime, buffer, length, &out);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer, capacity);
    if (status != PY68_STATUS_OK) return status;
    *result = py68_value_from_object(&out->base);
    return PY68_STATUS_OK;
}

Py68Status py68_attr_string_translate(Py68Runtime *runtime,
                                      Py68U16 argument_count,
                                      Py68Value *arguments, Py68Value *result)
{
    Py68String *self;
    Py68Dict *table;
    Py68U32 index;
    Py68U32 capacity;
    Py68U32 length = 0;
    char *buffer;
    Py68String *out;
    Py68Status status;
    if (py68_sm_require_self(runtime, argument_count, arguments, 2, 2, &self) !=
        PY68_STATUS_OK)
        return PY68_STATUS_RUNTIME_ERROR;
    if (arguments[1].type != PY68_VALUE_OBJECT || arguments[1].as.object == NULL ||
        arguments[1].as.object->type != PY68_OBJECT_DICT) {
        py68_sm_error(runtime, PY68_ERROR_TYPE, "translate table must be a dict");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    table = (Py68Dict *)arguments[1].as.object;
    capacity = self->length * 4 + 1;
    if (capacity < self->length + 1) capacity = self->length + 1;
    buffer = (char *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP, capacity);
    if (buffer == NULL) return PY68_STATUS_MEMORY_ERROR;
    for (index = 0; index < self->length; ++index) {
        Py68Value mapped;
        unsigned char c = (unsigned char)self->data[index];
        status = py68_dict_get_copy(runtime, table, py68_value_int((Py68I32)c),
                                    &mapped);
        if (status == PY68_STATUS_OK) {
            if (mapped.type == PY68_VALUE_NONE) {
                /* delete */
            } else if (mapped.type == PY68_VALUE_INT ||
                       mapped.type == PY68_VALUE_BOOL) {
                if (mapped.as.integer < 0 || mapped.as.integer > 255) {
                    py68_value_release(runtime, mapped);
                    py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer,
                              capacity);
                    py68_sm_error(runtime, PY68_ERROR_VALUE,
                                  "translation code must be 0..255");
                    return PY68_STATUS_RUNTIME_ERROR;
                }
                if (length + 1 > capacity) {
                    py68_value_release(runtime, mapped);
                    py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer,
                              capacity);
                    return PY68_STATUS_MEMORY_ERROR;
                }
                buffer[length++] = (char)mapped.as.integer;
            } else if (mapped.type == PY68_VALUE_OBJECT &&
                       mapped.as.object != NULL &&
                       mapped.as.object->type == PY68_OBJECT_STRING) {
                Py68String *repl = (Py68String *)mapped.as.object;
                if (length + repl->length > capacity) {
                    char *grown;
                    Py68U32 new_cap = capacity * 2 + repl->length + 1;
                    grown = (char *)py68_realloc(&runtime->allocator,
                                                 PY68_MEM_TEMP, buffer, capacity,
                                                 new_cap);
                    if (grown == NULL) {
                        py68_value_release(runtime, mapped);
                        py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer,
                                  capacity);
                        return PY68_STATUS_MEMORY_ERROR;
                    }
                    buffer = grown;
                    capacity = new_cap;
                }
                if (repl->length != 0)
                    memcpy(buffer + length, repl->data, repl->length);
                length += repl->length;
            } else {
                py68_value_release(runtime, mapped);
                py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer, capacity);
                py68_sm_error(runtime, PY68_ERROR_TYPE,
                              "translation value must be int, str, or None");
                return PY68_STATUS_RUNTIME_ERROR;
            }
            py68_value_release(runtime, mapped);
        } else {
            if (length + 1 > capacity) {
                py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer, capacity);
                return PY68_STATUS_MEMORY_ERROR;
            }
            buffer[length++] = (char)c;
        }
    }
    status = py68_string_new_copy(runtime, buffer, length, &out);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer, capacity);
    if (status != PY68_STATUS_OK) return status;
    *result = py68_value_from_object(&out->base);
    return PY68_STATUS_OK;
}

static Py68Status py68_sm_classifier(Py68Runtime *runtime,
                                     Py68U16 argument_count,
                                     Py68Value *arguments, Py68Value *result,
                                     int kind)
{
    Py68String *self;
    Py68U32 index;
    int cased = 0;
    int previous_cased = 0;
    int ok = 1;
    if (py68_sm_require_self(runtime, argument_count, arguments, 1, 1, &self) !=
        PY68_STATUS_OK)
        return PY68_STATUS_RUNTIME_ERROR;
    if (kind == 2 || kind == 9) {
        /* isascii / isprintable: empty is True */
        if (self->length == 0) {
            *result = py68_value_bool(1);
            return PY68_STATUS_OK;
        }
    } else if (self->length == 0) {
        *result = py68_value_bool(0);
        return PY68_STATUS_OK;
    }
    for (index = 0; index < self->length; ++index) {
        unsigned char c = (unsigned char)self->data[index];
        if (kind == 0) {
            if (!py68_sm_is_alnum(c)) ok = 0;
        } else if (kind == 1) {
            if (!py68_sm_is_alpha(c)) ok = 0;
        } else if (kind == 2) {
            if (c >= 128) ok = 0;
        } else if (kind == 3 || kind == 4 || kind == 7) {
            if (!py68_sm_is_digit(c)) ok = 0;
        } else if (kind == 5) {
            if (index == 0) {
                if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                      c == '_'))
                    ok = 0;
            } else if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                         (c >= '0' && c <= '9') || c == '_')) {
                ok = 0;
            }
        } else if (kind == 6) {
            if (py68_sm_is_upper(c))
                ok = 0;
            else if (py68_sm_is_lower(c))
                cased = 1;
        } else if (kind == 8) {
            if (py68_sm_is_lower(c))
                ok = 0;
            else if (py68_sm_is_upper(c))
                cased = 1;
        } else if (kind == 9) {
            if (!py68_sm_is_printable(c)) ok = 0;
        } else if (kind == 10) {
            if (!py68_sm_is_space(c)) ok = 0;
        } else if (kind == 11) {
            if (py68_sm_is_upper(c)) {
                if (previous_cased) ok = 0;
                cased = 1;
                previous_cased = 1;
            } else if (py68_sm_is_lower(c)) {
                if (!previous_cased) ok = 0;
                cased = 1;
                previous_cased = 1;
            } else {
                previous_cased = 0;
            }
        }
        if (!ok) break;
    }
    if (kind == 6 || kind == 8 || kind == 11) {
        *result = py68_value_bool(ok && cased);
    } else {
        *result = py68_value_bool(ok);
    }
    return PY68_STATUS_OK;
}

Py68Status py68_attr_string_isalnum(Py68Runtime *runtime,
                                    Py68U16 argument_count,
                                    Py68Value *arguments, Py68Value *result)
{
    return py68_sm_classifier(runtime, argument_count, arguments, result, 0);
}
Py68Status py68_attr_string_isalpha(Py68Runtime *runtime,
                                    Py68U16 argument_count,
                                    Py68Value *arguments, Py68Value *result)
{
    return py68_sm_classifier(runtime, argument_count, arguments, result, 1);
}
Py68Status py68_attr_string_isascii(Py68Runtime *runtime,
                                    Py68U16 argument_count,
                                    Py68Value *arguments, Py68Value *result)
{
    return py68_sm_classifier(runtime, argument_count, arguments, result, 2);
}
Py68Status py68_attr_string_isdecimal(Py68Runtime *runtime,
                                      Py68U16 argument_count,
                                      Py68Value *arguments, Py68Value *result)
{
    return py68_sm_classifier(runtime, argument_count, arguments, result, 3);
}
Py68Status py68_attr_string_isdigit(Py68Runtime *runtime,
                                    Py68U16 argument_count,
                                    Py68Value *arguments, Py68Value *result)
{
    return py68_sm_classifier(runtime, argument_count, arguments, result, 4);
}
Py68Status py68_attr_string_isidentifier(Py68Runtime *runtime,
                                         Py68U16 argument_count,
                                         Py68Value *arguments,
                                         Py68Value *result)
{
    return py68_sm_classifier(runtime, argument_count, arguments, result, 5);
}
Py68Status py68_attr_string_islower(Py68Runtime *runtime,
                                    Py68U16 argument_count,
                                    Py68Value *arguments, Py68Value *result)
{
    return py68_sm_classifier(runtime, argument_count, arguments, result, 6);
}
Py68Status py68_attr_string_isnumeric(Py68Runtime *runtime,
                                      Py68U16 argument_count,
                                      Py68Value *arguments, Py68Value *result)
{
    return py68_sm_classifier(runtime, argument_count, arguments, result, 7);
}
Py68Status py68_attr_string_isprintable(Py68Runtime *runtime,
                                        Py68U16 argument_count,
                                        Py68Value *arguments,
                                        Py68Value *result)
{
    return py68_sm_classifier(runtime, argument_count, arguments, result, 9);
}
Py68Status py68_attr_string_isspace(Py68Runtime *runtime,
                                    Py68U16 argument_count,
                                    Py68Value *arguments, Py68Value *result)
{
    return py68_sm_classifier(runtime, argument_count, arguments, result, 10);
}
Py68Status py68_attr_string_istitle(Py68Runtime *runtime,
                                    Py68U16 argument_count,
                                    Py68Value *arguments, Py68Value *result)
{
    return py68_sm_classifier(runtime, argument_count, arguments, result, 11);
}
Py68Status py68_attr_string_isupper(Py68Runtime *runtime,
                                    Py68U16 argument_count,
                                    Py68Value *arguments, Py68Value *result)
{
    return py68_sm_classifier(runtime, argument_count, arguments, result, 8);
}

static void py68_sm_append_escape(char *buffer, Py68U32 *length,
                                  unsigned char c, int escape_high)
{
    const char *hex = "0123456789abcdef";
    if (c == '\\') {
        buffer[(*length)++] = '\\';
        buffer[(*length)++] = '\\';
    } else if (c == '\'') {
        buffer[(*length)++] = '\\';
        buffer[(*length)++] = '\'';
    } else if (c == '\n') {
        buffer[(*length)++] = '\\';
        buffer[(*length)++] = 'n';
    } else if (c == '\t') {
        buffer[(*length)++] = '\\';
        buffer[(*length)++] = 't';
    } else if (c == '\r') {
        buffer[(*length)++] = '\\';
        buffer[(*length)++] = 'r';
    } else if (c < 0x20 || c == 0x7f || (escape_high && c >= 0x80)) {
        buffer[(*length)++] = '\\';
        buffer[(*length)++] = 'x';
        buffer[(*length)++] = hex[(c >> 4) & 0xf];
        buffer[(*length)++] = hex[c & 0xf];
    } else {
        buffer[(*length)++] = (char)c;
    }
}

Py68Status py68_string_repr_escape(Py68Runtime *runtime, Py68String *string,
                                   int escape_high, Py68String **result)
{
    Py68U32 capacity = string->length * 4 + 3;
    char *buffer;
    Py68U32 length = 0;
    Py68U32 index;
    Py68Status status;
    buffer = (char *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP, capacity);
    if (buffer == NULL) return PY68_STATUS_MEMORY_ERROR;
    buffer[length++] = '\'';
    for (index = 0; index < string->length; ++index)
        py68_sm_append_escape(buffer, &length, (unsigned char)string->data[index],
                              escape_high);
    buffer[length++] = '\'';
    status = py68_string_new_copy(runtime, buffer, length, result);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer, capacity);
    return status;
}

Py68Status py68_builtin_maketrans(Py68Runtime *runtime, Py68U16 argument_count,
                                  Py68Value *arguments, Py68Value *result)
{
    Py68Dict *dict;
    Py68Status status;
    if (argument_count < 1 || argument_count > 3) {
        py68_sm_error(runtime, PY68_ERROR_TYPE, "maketrans expects 1 to 3 arguments");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    status = py68_dict_new(runtime, &dict);
    if (status != PY68_STATUS_OK) return status;
    if (argument_count == 1) {
        if (arguments[0].type != PY68_VALUE_OBJECT ||
            arguments[0].as.object == NULL ||
            arguments[0].as.object->type != PY68_OBJECT_DICT) {
            py68_object_release(runtime, &dict->base);
            py68_sm_error(runtime, PY68_ERROR_TYPE,
                          "maketrans single argument must be a dict");
            return PY68_STATUS_RUNTIME_ERROR;
        }
        {
            Py68Dict *source = (Py68Dict *)arguments[0].as.object;
            Py68U32 index;
            for (index = 0; index < source->capacity; ++index) {
                if (source->entries[index].used != 1) continue;
                status = py68_dict_set_copy(runtime, dict,
                                            source->entries[index].key,
                                            source->entries[index].value);
                if (status != PY68_STATUS_OK) {
                    py68_object_release(runtime, &dict->base);
                    return status;
                }
            }
        }
    } else {
        Py68String *from;
        Py68String *to;
        Py68U32 index;
        if (!py68_sm_require_string(runtime, arguments[0], &from,
                                    "maketrans x must be a string") ||
            !py68_sm_require_string(runtime, arguments[1], &to,
                                    "maketrans y must be a string")) {
            py68_object_release(runtime, &dict->base);
            return PY68_STATUS_RUNTIME_ERROR;
        }
        if (from->length != to->length) {
            py68_object_release(runtime, &dict->base);
            py68_sm_error(runtime, PY68_ERROR_VALUE,
                          "maketrans x and y must be same length");
            return PY68_STATUS_RUNTIME_ERROR;
        }
        for (index = 0; index < from->length; ++index) {
            status = py68_dict_set_copy(
                runtime, dict,
                py68_value_int((Py68I32)(unsigned char)from->data[index]),
                py68_value_int((Py68I32)(unsigned char)to->data[index]));
            if (status != PY68_STATUS_OK) {
                py68_object_release(runtime, &dict->base);
                return status;
            }
        }
        if (argument_count == 3) {
            Py68String *delchars;
            if (!py68_sm_require_string(runtime, arguments[2], &delchars,
                                        "maketrans z must be a string")) {
                py68_object_release(runtime, &dict->base);
                return PY68_STATUS_RUNTIME_ERROR;
            }
            for (index = 0; index < delchars->length; ++index) {
                status = py68_dict_set_copy(
                    runtime, dict,
                    py68_value_int(
                        (Py68I32)(unsigned char)delchars->data[index]),
                    py68_value_none());
                if (status != PY68_STATUS_OK) {
                    py68_object_release(runtime, &dict->base);
                    return status;
                }
            }
        }
    }
    *result = py68_value_from_object(&dict->base);
    return PY68_STATUS_OK;
}
