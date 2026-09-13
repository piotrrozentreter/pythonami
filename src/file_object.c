/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_file.h"
#include "py68k_runtime.h"

#include <string.h>

static int py68_file_parse_mode(const char *mode, Py68PlatformFileMode *out_mode,
                                int *binary, int *readable, int *writable)
{
    if (mode == NULL) return 0;
    *binary = 0;
    if (strcmp(mode, "r") == 0) {
        *out_mode = PY68_PFILE_READ;
        *readable = 1;
        *writable = 0;
        return 1;
    }
    if (strcmp(mode, "rb") == 0) {
        *out_mode = PY68_PFILE_READ;
        *binary = 1;
        *readable = 1;
        *writable = 0;
        return 1;
    }
    if (strcmp(mode, "w") == 0) {
        *out_mode = PY68_PFILE_WRITE;
        *readable = 0;
        *writable = 1;
        return 1;
    }
    if (strcmp(mode, "wb") == 0) {
        *out_mode = PY68_PFILE_WRITE;
        *binary = 1;
        *readable = 0;
        *writable = 1;
        return 1;
    }
    if (strcmp(mode, "a") == 0) {
        *out_mode = PY68_PFILE_APPEND;
        *readable = 0;
        *writable = 1;
        return 1;
    }
    if (strcmp(mode, "ab") == 0) {
        *out_mode = PY68_PFILE_APPEND;
        *binary = 1;
        *readable = 0;
        *writable = 1;
        return 1;
    }
    return 0;
}

Py68Status py68_file_open(Py68Runtime *runtime, const char *path,
                          const char *mode, Py68File **result)
{
    Py68PlatformFileMode pmode;
    int binary;
    int readable;
    int writable;
    Py68PlatformFileHandle handle;
    Py68File *file;
    Py68Status status;

    if (runtime == NULL || path == NULL || mode == NULL || result == NULL)
        return PY68_STATUS_INTERNAL_ERROR;
    if (!py68_file_parse_mode(mode, &pmode, &binary, &readable, &writable))
        return PY68_STATUS_SOURCE_ERROR;

    status = py68_platform_file_open(runtime, path, pmode, binary, &handle);
    if (status != PY68_STATUS_OK) return status;

    file = (Py68File *)py68_alloc(&runtime->allocator, PY68_MEM_RUNTIME,
                                  sizeof(Py68File));
    if (file == NULL) {
        py68_platform_file_close(handle);
        return PY68_STATUS_MEMORY_ERROR;
    }
    file->base.type = PY68_OBJECT_FILE;
    file->base.flags = 0;
    file->base.reference_count = 1;
    file->base.next_object = runtime->live_objects;
    runtime->live_objects = &file->base;
    file->handle = handle;
    file->closed = 0;
    file->readable = (Py68U16)readable;
    file->writable = (Py68U16)writable;
    file->binary = (Py68U16)binary;
    *result = file;
    return PY68_STATUS_OK;
}

Py68Status py68_file_close(Py68Runtime *runtime, Py68File *file)
{
    (void)runtime;
    if (file == NULL) return PY68_STATUS_INTERNAL_ERROR;
    if (file->closed) return PY68_STATUS_OK;
    py68_platform_file_close(file->handle);
    file->handle = NULL;
    file->closed = 1;
    return PY68_STATUS_OK;
}

void py68_file_destroy(Py68Runtime *runtime, Py68File *file)
{
    if (file == NULL) return;
    if (!file->closed && file->handle != NULL)
        py68_platform_file_close(file->handle);
    py68_free(&runtime->allocator, PY68_MEM_RUNTIME, file, sizeof(Py68File));
}
