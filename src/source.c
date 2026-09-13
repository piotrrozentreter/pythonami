/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_source.h"

#include <string.h>

static int py68_source_size_add_overflows(Py68U32 left, Py68U32 right)
{
    return right > (Py68U32)~(Py68U32)0 - left;
}

void py68_source_destroy(Py68Allocator *allocator, Py68Source *source)
{
    if (source->filename != NULL) {
        py68_free(allocator, PY68_MEM_SOURCE, source->filename,
                  (Py68U32)(strlen(source->filename) + 1));
    }
    py68_free(allocator, PY68_MEM_SOURCE, source->data,
              source->length + 1);
    source->filename = NULL;
    source->data = NULL;
    source->length = 0;
}

Py68Status py68_source_initialize(Py68Allocator *allocator,
                                   Py68Source *source,
                                   const char *filename,
                                   const Py68U8 *data,
                                   Py68U32 length)
{
    Py68U32 filename_size;

    source->filename = NULL;
    source->data = NULL;
    source->length = 0;
    filename_size = (Py68U32)(strlen(filename) + 1);
    if (py68_source_size_add_overflows(length, 1)) {
        return PY68_STATUS_MEMORY_ERROR;
    }
    source->filename = (char *)py68_alloc(allocator, PY68_MEM_SOURCE,
                                          filename_size);
    if (source->filename == NULL) {
        return PY68_STATUS_MEMORY_ERROR;
    }
    source->data = (Py68U8 *)py68_alloc(allocator, PY68_MEM_SOURCE,
                                        length + 1);
    if (source->data == NULL) {
        py68_free(allocator, PY68_MEM_SOURCE, source->filename,
                  filename_size);
        source->filename = NULL;
        return PY68_STATUS_MEMORY_ERROR;
    }
    memcpy(source->filename, filename, filename_size);
    if (length != 0) {
        memcpy(source->data, data, (size_t)length);
    }
    source->data[length] = '\0';
    source->length = length;
    return PY68_STATUS_OK;
}