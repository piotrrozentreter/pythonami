/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_string.h"
#include "py68k_runtime.h"

#include <string.h>

#define PY68_FNV_OFFSET 2166136261UL
#define PY68_FNV_PRIME 16777619UL

Py68U32 py68_string_hash_bytes(const char *data, Py68U32 length)
{
    Py68U32 hash = PY68_FNV_OFFSET;
    Py68U32 index;
    for (index = 0; index < length; ++index) {
        hash ^= (Py68U8)data[index];
        hash *= PY68_FNV_PRIME;
    }
    return hash;
}

Py68Status py68_string_new_copy(Py68Runtime *runtime, const char *data,
                                Py68U32 length, Py68String **result)
{
    Py68String *string;
    Py68U32 size;
    if (length > (Py68U32)~(Py68U32)0 - (Py68U32)sizeof(Py68String)) {
        return PY68_STATUS_MEMORY_ERROR;
    }
    size = (Py68U32)sizeof(Py68String) + length;
    string = (Py68String *)py68_alloc(&runtime->allocator, PY68_MEM_STRING,
                                      size);
    if (string == NULL) return PY68_STATUS_MEMORY_ERROR;
    string->base.type = PY68_OBJECT_STRING;
    string->base.flags = 0;
    string->base.reference_count = 1;
    string->base.next_object = runtime->live_objects;
    runtime->live_objects = &string->base;
    string->length = length;
    string->hash = py68_string_hash_bytes(data, length);
    if (length != 0) memcpy(string->data, data, length);
    string->data[length] = '\0';
    *result = string;
    return PY68_STATUS_OK;
}

static int py68_string_hex_value(char character, int *value)
{
    if (character >= '0' && character <= '9') {
        *value = character - '0';
        return 1;
    }
    if (character >= 'a' && character <= 'f') {
        *value = 10 + (character - 'a');
        return 1;
    }
    if (character >= 'A' && character <= 'F') {
        *value = 10 + (character - 'A');
        return 1;
    }
    return 0;
}

Py68Status py68_string_new_from_escaped(Py68Runtime *runtime, const char *data,
                                        Py68U32 length, Py68String **result)
{
    char *buffer;
    Py68U32 index = 0;
    Py68U32 out = 0;
    Py68Status status;
    buffer = (char *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP, length + 1);
    if (buffer == NULL) return PY68_STATUS_MEMORY_ERROR;
    while (index < length) {
        char c = data[index++];
        if (c != '\\') {
            buffer[out++] = c;
            continue;
        }
        if (index >= length) {
            py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer, length + 1);
            return PY68_STATUS_SOURCE_ERROR;
        }
        c = data[index++];
        if (c == '\\' || c == '\'' || c == '"') {
            buffer[out++] = c;
        } else if (c == 'n') {
            buffer[out++] = '\n';
        } else if (c == 'r') {
            buffer[out++] = '\r';
        } else if (c == 't') {
            buffer[out++] = '\t';
        } else if (c == 'x') {
            int hi;
            int lo;
            if (index + 1 >= length ||
                !py68_string_hex_value(data[index], &hi) ||
                !py68_string_hex_value(data[index + 1], &lo)) {
                py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer,
                          length + 1);
                return PY68_STATUS_SOURCE_ERROR;
            }
            buffer[out++] = (char)((hi << 4) | lo);
            index += 2;
        } else {
            py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer, length + 1);
            return PY68_STATUS_SOURCE_ERROR;
        }
    }
    buffer[out] = '\0';
    status = py68_string_new_copy(runtime, buffer, out, result);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer, length + 1);
    return status;
}

Py68Status py68_string_concat(Py68Runtime *runtime, Py68String *left,
                              Py68String *right, Py68String **result)
{
    Py68U32 length;
    char *buffer;
    Py68Status status;
    if (left->length > (Py68U32)~(Py68U32)0 - right->length)
        return PY68_STATUS_MEMORY_ERROR;
    length = left->length + right->length;
    buffer = (char *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP, length + 1);
    if (buffer == NULL) return PY68_STATUS_MEMORY_ERROR;
    if (left->length != 0) memcpy(buffer, left->data, left->length);
    if (right->length != 0) memcpy(buffer + left->length, right->data,
                                   right->length);
    buffer[length] = '\0';
    status = py68_string_new_copy(runtime, buffer, length, result);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer, length + 1);
    return status;
}

static Py68Status py68_string_normalize_index(Py68String *string, Py68I32 index,
                                              Py68U32 *normalized)
{
    Py68I32 result = index;
    if (result < 0) result += (Py68I32)string->length;
    if (result < 0 || (Py68U32)result >= string->length)
        return PY68_STATUS_SOURCE_ERROR;
    *normalized = (Py68U32)result;
    return PY68_STATUS_OK;
}

Py68Status py68_string_get_char(Py68Runtime *runtime, Py68String *string,
                                Py68I32 index, Py68String **result)
{
    Py68U32 normalized;
    if (py68_string_normalize_index(string, index, &normalized) !=
        PY68_STATUS_OK)
        return PY68_STATUS_SOURCE_ERROR;
    return py68_string_new_copy(runtime, string->data + normalized, 1, result);
}

Py68Status py68_string_slice(Py68Runtime *runtime, Py68String *string,
                             Py68I32 start, Py68I32 end, int start_omitted,
                             int end_omitted, Py68String **result)
{
    Py68I32 length = (Py68I32)string->length;
    Py68I32 slice_start = start_omitted ? 0 : start;
    Py68I32 slice_end = end_omitted ? length : end;
    if (slice_start < 0) slice_start += length;
    if (slice_end < 0) slice_end += length;
    if (slice_start < 0) slice_start = 0;
    if (slice_end < 0) slice_end = 0;
    if (slice_start > length) slice_start = length;
    if (slice_end > length) slice_end = length;
    if (slice_end < slice_start) slice_end = slice_start;
    return py68_string_new_copy(runtime, string->data + (Py68U32)slice_start,
                                (Py68U32)(slice_end - slice_start), result);
}
