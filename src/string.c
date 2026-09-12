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
