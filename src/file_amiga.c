#include "py68k_platform.h"
#include "py68k_runtime.h"

#include <proto/dos.h>

Py68Status py68_platform_read_file(Py68Runtime *runtime, const char *path,
                                   Py68U8 **data, Py68U32 *length)
{
    BPTR file;
    LONG size;
    LONG read_count;
    Py68U8 *buffer;
    file = Open(path, MODE_OLDFILE);
    if (file == 0) return PY68_STATUS_SOURCE_ERROR;
    Seek(file, 0, OFFSET_END);
    size = Seek(file, 0, OFFSET_CURRENT);
    Seek(file, 0, OFFSET_BEGINNING);
    if (size < 0) { Close(file); return PY68_STATUS_RUNTIME_ERROR; }
    buffer = (Py68U8 *)py68_alloc(&runtime->allocator, PY68_MEM_SOURCE,
                                  (Py68U32)size + 1);
    if (buffer == NULL) { Close(file); return PY68_STATUS_MEMORY_ERROR; }
    read_count = Read(file, buffer, size);
    Close(file);
    if (read_count != size) {
        py68_free(&runtime->allocator, PY68_MEM_SOURCE, buffer, (Py68U32)size + 1);
        return PY68_STATUS_RUNTIME_ERROR;
    }
    buffer[size] = 0;
    *data = buffer;
    *length = (Py68U32)size;
    return PY68_STATUS_OK;
}