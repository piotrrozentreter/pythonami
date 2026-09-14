/* 2026 by Piotr Rozentreter (Rozsoft) */

#define _DEFAULT_SOURCE

#include "py68k_platform.h"
#include "py68k_runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Py68Status py68_platform_read_file(Py68Runtime *runtime, const char *path,
                                   Py68U8 **data, Py68U32 *length)
{
    FILE *file;
    long size;
    Py68U8 *buffer;
    file = fopen(path, "rb");
    if (file == NULL) return PY68_STATUS_SOURCE_ERROR;
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return PY68_STATUS_RUNTIME_ERROR;
    }
    size = ftell(file);
    if (size < 0 || (unsigned long)size > 0xffffffffUL) {
        fclose(file);
        return PY68_STATUS_MEMORY_ERROR;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return PY68_STATUS_RUNTIME_ERROR;
    }
    buffer = (Py68U8 *)py68_alloc(&runtime->allocator, PY68_MEM_SOURCE,
                                  (Py68U32)size + 1);
    if (buffer == NULL) {
        fclose(file);
        return PY68_STATUS_MEMORY_ERROR;
    }
    if (size != 0 && fread(buffer, 1, (size_t)size, file) != (size_t)size) {
        py68_free(&runtime->allocator, PY68_MEM_SOURCE, buffer,
                  (Py68U32)size + 1);
        fclose(file);
        return PY68_STATUS_RUNTIME_ERROR;
    }
    fclose(file);
    buffer[size] = 0;
    *data = buffer;
    *length = (Py68U32)size;
    return PY68_STATUS_OK;
}

Py68Status py68_platform_file_open(Py68Runtime *runtime, const char *path,
                                   Py68PlatformFileMode mode, int binary,
                                   Py68PlatformFileHandle *handle_out)
{
    FILE *file;
    const char *cmode;
    (void)runtime;
    if (path == NULL || handle_out == NULL) return PY68_STATUS_INTERNAL_ERROR;
    if (mode == PY68_PFILE_READ)
        cmode = binary ? "rb" : "r";
    else if (mode == PY68_PFILE_WRITE)
        cmode = binary ? "wb" : "w";
    else if (mode == PY68_PFILE_APPEND)
        cmode = binary ? "ab" : "a";
    else
        return PY68_STATUS_SOURCE_ERROR;
    file = fopen(path, cmode);
    if (file == NULL) return PY68_STATUS_SOURCE_ERROR;
    *handle_out = (Py68PlatformFileHandle)file;
    return PY68_STATUS_OK;
}

void py68_platform_file_close(Py68PlatformFileHandle handle)
{
    if (handle != NULL) fclose((FILE *)handle);
}

Py68Status py68_platform_file_read(Py68Runtime *runtime,
                                   Py68PlatformFileHandle handle,
                                   Py68U32 max_count, Py68U8 **data,
                                   Py68U32 *length)
{
    FILE *file = (FILE *)handle;
    Py68U8 *buffer;
    size_t got;
    if (file == NULL || data == NULL || length == NULL)
        return PY68_STATUS_INTERNAL_ERROR;
    if (max_count == 0) {
        buffer = (Py68U8 *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP, 1);
        if (buffer == NULL) return PY68_STATUS_MEMORY_ERROR;
        buffer[0] = 0;
        *data = buffer;
        *length = 0;
        return PY68_STATUS_OK;
    }
    buffer = (Py68U8 *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP,
                                  max_count + 1);
    if (buffer == NULL) return PY68_STATUS_MEMORY_ERROR;
    got = fread(buffer, 1, (size_t)max_count, file);
    if (ferror(file)) {
        py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer, max_count + 1);
        return PY68_STATUS_RUNTIME_ERROR;
    }
    buffer[got] = 0;
    *data = buffer;
    *length = (Py68U32)got;
    return PY68_STATUS_OK;
}

Py68Status py68_platform_file_readline(Py68Runtime *runtime,
                                       Py68PlatformFileHandle handle,
                                       Py68U8 **data, Py68U32 *length)
{
    FILE *file = (FILE *)handle;
    Py68U32 capacity = 128;
    Py68U32 used = 0;
    Py68U8 *buffer;
    int ch;
    if (file == NULL || data == NULL || length == NULL)
        return PY68_STATUS_INTERNAL_ERROR;
    buffer = (Py68U8 *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP, capacity);
    if (buffer == NULL) return PY68_STATUS_MEMORY_ERROR;
    for (;;) {
        ch = fgetc(file);
        if (ch == EOF) break;
        if (used + 1 >= capacity) {
            Py68U32 new_cap = capacity * 2;
            Py68U8 *replacement;
            if (new_cap < capacity || new_cap > 1048576UL) {
                py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer, capacity);
                return PY68_STATUS_MEMORY_ERROR;
            }
            replacement = (Py68U8 *)py68_realloc(
                &runtime->allocator, PY68_MEM_TEMP, buffer, capacity, new_cap);
            if (replacement == NULL) {
                py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer, capacity);
                return PY68_STATUS_MEMORY_ERROR;
            }
            buffer = replacement;
            capacity = new_cap;
        }
        buffer[used++] = (Py68U8)ch;
        if (ch == '\n') break;
    }
    if (ferror(file)) {
        py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer, capacity);
        return PY68_STATUS_RUNTIME_ERROR;
    }
    {
        Py68U8 *exact = (Py68U8 *)py68_realloc(
            &runtime->allocator, PY68_MEM_TEMP, buffer, capacity, used + 1);
        if (exact == NULL) {
            py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer, capacity);
            return PY68_STATUS_MEMORY_ERROR;
        }
        buffer = exact;
    }
    buffer[used] = 0;
    *data = buffer;
    *length = used;
    return PY68_STATUS_OK;
}

Py68Status py68_platform_file_write(Py68Runtime *runtime,
                                    Py68PlatformFileHandle handle,
                                    const char *data, Py68U32 length,
                                    Py68U32 *written)
{
    FILE *file = (FILE *)handle;
    size_t got;
    (void)runtime;
    if (file == NULL || (data == NULL && length != 0) || written == NULL)
        return PY68_STATUS_INTERNAL_ERROR;
    if (length == 0) {
        *written = 0;
        return PY68_STATUS_OK;
    }
    got = fwrite(data, 1, (size_t)length, file);
    *written = (Py68U32)got;
    if (got != (size_t)length) return PY68_STATUS_RUNTIME_ERROR;
    return PY68_STATUS_OK;
}

int py68_platform_path_exists(const char *path)
{
    FILE *file;
    if (path == NULL) return 0;
    file = fopen(path, "rb");
    if (file == NULL) return 0;
    fclose(file);
    return 1;
}

Py68Status py68_platform_path_remove(const char *path)
{
    if (path == NULL) return PY68_STATUS_INTERNAL_ERROR;
    if (remove(path) != 0) return PY68_STATUS_SOURCE_ERROR;
    return PY68_STATUS_OK;
}

Py68Status py68_platform_path_rename(const char *old_path,
                                     const char *new_path)
{
    if (old_path == NULL || new_path == NULL)
        return PY68_STATUS_INTERNAL_ERROR;
    if (rename(old_path, new_path) != 0) return PY68_STATUS_SOURCE_ERROR;
    return PY68_STATUS_OK;
}

Py68Status py68_platform_var_get(Py68Runtime *runtime, const char *name,
                                 char **value_out, Py68U32 *length_out)
{
    const char *value;
    Py68U32 length;
    char *copy;
    if (runtime == NULL || name == NULL || value_out == NULL ||
        length_out == NULL)
        return PY68_STATUS_INTERNAL_ERROR;
    value = getenv(name);
    if (value == NULL) {
        *value_out = NULL;
        *length_out = 0;
        return PY68_STATUS_SOURCE_ERROR;
    }
    length = 0;
    while (value[length] != '\0') {
        if (length == 0xffffffffUL) return PY68_STATUS_MEMORY_ERROR;
        ++length;
    }
    copy = (char *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP, length + 1);
    if (copy == NULL) return PY68_STATUS_MEMORY_ERROR;
    memcpy(copy, value, length + 1);
    *value_out = copy;
    *length_out = length;
    return PY68_STATUS_OK;
}

Py68Status py68_platform_var_set(Py68Runtime *runtime, const char *name,
                                 const char *value)
{
    (void)runtime;
    if (name == NULL || value == NULL) return PY68_STATUS_INTERNAL_ERROR;
#if defined(_WIN32)
    if (_putenv_s(name, value) != 0) return PY68_STATUS_RUNTIME_ERROR;
#else
    if (setenv(name, value, 1) != 0) return PY68_STATUS_RUNTIME_ERROR;
#endif
    return PY68_STATUS_OK;
}

Py68Status py68_platform_var_unset(Py68Runtime *runtime, const char *name)
{
    (void)runtime;
    if (name == NULL) return PY68_STATUS_INTERNAL_ERROR;
#if defined(_WIN32)
    if (_putenv_s(name, "") != 0) return PY68_STATUS_RUNTIME_ERROR;
#else
    if (unsetenv(name) != 0) return PY68_STATUS_RUNTIME_ERROR;
#endif
    return PY68_STATUS_OK;
}
