/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_platform.h"
#include "py68k_runtime.h"

#include <stdio.h>
#include <stdlib.h>

Py68Status py68_platform_read_file(Py68Runtime *runtime, const char *path,
                                   Py68U8 **data, Py68U32 *length)
{
    FILE *file;
    long size;
    Py68U8 *buffer;
    file = fopen(path, "rb");
    if (file == NULL) return PY68_STATUS_SOURCE_ERROR;
    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return PY68_STATUS_RUNTIME_ERROR; }
    size = ftell(file);
    if (size < 0 || (unsigned long)size > 0xffffffffUL) { fclose(file); return PY68_STATUS_MEMORY_ERROR; }
    if (fseek(file, 0, SEEK_SET) != 0) { fclose(file); return PY68_STATUS_RUNTIME_ERROR; }
    buffer = (Py68U8 *)py68_alloc(&runtime->allocator, PY68_MEM_SOURCE,
                                  (Py68U32)size + 1);
    if (buffer == NULL) { fclose(file); return PY68_STATUS_MEMORY_ERROR; }
    if (size != 0 && fread(buffer, 1, (size_t)size, file) != (size_t)size) {
        py68_free(&runtime->allocator, PY68_MEM_SOURCE, buffer, (Py68U32)size + 1);
        fclose(file);
        return PY68_STATUS_RUNTIME_ERROR;
    }
    fclose(file);
    buffer[size] = 0;
    *data = buffer;
    *length = (Py68U32)size;
    return PY68_STATUS_OK;
}