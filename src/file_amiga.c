/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_platform.h"
#include "py68k_runtime.h"

#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

#include <string.h>

/* Box BPTR in a pointer-sized handle without truncating on 32-bit. */
static Py68PlatformFileHandle py68_box_bptr(BPTR handle)
{
    return (Py68PlatformFileHandle)(unsigned long)handle;
}

static BPTR py68_unbox_bptr(Py68PlatformFileHandle handle)
{
    return (BPTR)(unsigned long)handle;
}

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
    if (size < 0) {
        Close(file);
        return PY68_STATUS_RUNTIME_ERROR;
    }
    buffer = (Py68U8 *)py68_alloc(&runtime->allocator, PY68_MEM_SOURCE,
                                  (Py68U32)size + 1);
    if (buffer == NULL) {
        Close(file);
        return PY68_STATUS_MEMORY_ERROR;
    }
    read_count = Read(file, buffer, size);
    Close(file);
    if (read_count != size) {
        py68_free(&runtime->allocator, PY68_MEM_SOURCE, buffer,
                  (Py68U32)size + 1);
        return PY68_STATUS_RUNTIME_ERROR;
    }
    buffer[size] = 0;
    *data = buffer;
    *length = (Py68U32)size;
    return PY68_STATUS_OK;
}

Py68Status py68_platform_file_open(Py68Runtime *runtime, const char *path,
                                   Py68PlatformFileMode mode, int binary,
                                   Py68PlatformFileHandle *handle_out)
{
    BPTR file;
    (void)runtime;
    (void)binary; /* AmigaDOS Open has no separate text/binary mode. */
    if (path == NULL || handle_out == NULL) return PY68_STATUS_INTERNAL_ERROR;
    if (mode == PY68_PFILE_READ) {
        file = Open(path, MODE_OLDFILE);
    } else if (mode == PY68_PFILE_WRITE) {
        file = Open(path, MODE_NEWFILE);
    } else if (mode == PY68_PFILE_APPEND) {
        file = Open(path, MODE_READWRITE);
        if (file == 0) file = Open(path, MODE_NEWFILE);
        if (file != 0) Seek(file, 0, OFFSET_END);
    } else {
        return PY68_STATUS_SOURCE_ERROR;
    }
    if (file == 0) return PY68_STATUS_SOURCE_ERROR;
    *handle_out = py68_box_bptr(file);
    return PY68_STATUS_OK;
}

void py68_platform_file_close(Py68PlatformFileHandle handle)
{
    BPTR file = py68_unbox_bptr(handle);
    if (file != 0) Close(file);
}

Py68Status py68_platform_file_read(Py68Runtime *runtime,
                                   Py68PlatformFileHandle handle,
                                   Py68U32 max_count, Py68U8 **data,
                                   Py68U32 *length)
{
    BPTR file = py68_unbox_bptr(handle);
    Py68U8 *buffer;
    LONG got;
    if (file == 0 || data == NULL || length == NULL)
        return PY68_STATUS_INTERNAL_ERROR;
    if (max_count > 2147483647UL) return PY68_STATUS_MEMORY_ERROR;
    buffer = (Py68U8 *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP,
                                  max_count + 1);
    if (buffer == NULL) return PY68_STATUS_MEMORY_ERROR;
    if (max_count == 0) {
        buffer[0] = 0;
        *data = buffer;
        *length = 0;
        return PY68_STATUS_OK;
    }
    got = Read(file, buffer, (LONG)max_count);
    if (got < 0) {
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
    BPTR file = py68_unbox_bptr(handle);
    Py68U32 capacity = 128;
    Py68U32 used = 0;
    Py68U8 *buffer;
    UBYTE ch;
    LONG got;
    if (file == 0 || data == NULL || length == NULL)
        return PY68_STATUS_INTERNAL_ERROR;
    buffer = (Py68U8 *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP, capacity);
    if (buffer == NULL) return PY68_STATUS_MEMORY_ERROR;
    for (;;) {
        got = Read(file, &ch, 1);
        if (got == 0) break;
        if (got < 0) {
            py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer, capacity);
            return PY68_STATUS_RUNTIME_ERROR;
        }
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
    BPTR file = py68_unbox_bptr(handle);
    LONG got;
    (void)runtime;
    if (file == 0 || (data == NULL && length != 0) || written == NULL)
        return PY68_STATUS_INTERNAL_ERROR;
    if (length > 2147483647UL) return PY68_STATUS_MEMORY_ERROR;
    if (length == 0) {
        *written = 0;
        return PY68_STATUS_OK;
    }
    got = Write(file, (CONST_APTR)data, (LONG)length);
    if (got < 0) {
        *written = 0;
        return PY68_STATUS_RUNTIME_ERROR;
    }
    *written = (Py68U32)got;
    if ((Py68U32)got != length) return PY68_STATUS_RUNTIME_ERROR;
    return PY68_STATUS_OK;
}

int py68_platform_path_exists(const char *path)
{
    BPTR lock;
    if (path == NULL) return 0;
    lock = Lock(path, ACCESS_READ);
    if (lock == 0) return 0;
    UnLock(lock);
    return 1;
}

Py68Status py68_platform_path_remove(const char *path)
{
    if (path == NULL) return PY68_STATUS_INTERNAL_ERROR;
    if (!DeleteFile(path)) return PY68_STATUS_SOURCE_ERROR;
    return PY68_STATUS_OK;
}

Py68Status py68_platform_path_rename(const char *old_path,
                                     const char *new_path)
{
    if (old_path == NULL || new_path == NULL)
        return PY68_STATUS_INTERNAL_ERROR;
    if (!Rename(old_path, new_path)) return PY68_STATUS_SOURCE_ERROR;
    return PY68_STATUS_OK;
}

static Py68U32 py68_c_strlen(const char *text)
{
    Py68U32 length = 0;
    if (text == NULL) return 0;
    while (text[length] != '\0') ++length;
    return length;
}

Py68Status py68_platform_var_get(Py68Runtime *runtime, const char *name,
                                 char **value_out, Py68U32 *length_out)
{
    /* Resolve DOS assign "name:" via Lock + NameFromLock (Amiga-first).
     * Suppress DOS requesters: Lock("missing:") otherwise pops
     * "Please insert volume ..." instead of failing with lock == 0. */
    char query[260];
    char pathbuf[512];
    Py68U32 name_len;
    BPTR lock;
    char *copy;
    Py68U32 length;
    LONG ok;
    struct Process *process;
    APTR old_window;

    if (runtime == NULL || name == NULL || value_out == NULL ||
        length_out == NULL)
        return PY68_STATUS_INTERNAL_ERROR;
    name_len = py68_c_strlen(name);
    if (name_len == 0 || name_len > 250) return PY68_STATUS_SOURCE_ERROR;
    memcpy(query, name, name_len);
    query[name_len] = ':';
    query[name_len + 1] = '\0';
    process = (struct Process *)FindTask(NULL);
    old_window = NULL;
    if (process != NULL) {
        old_window = process->pr_WindowPtr;
        process->pr_WindowPtr = (APTR)-1L;
    }
    lock = Lock(query, ACCESS_READ);
    if (process != NULL)
        process->pr_WindowPtr = old_window;
    if (lock == 0) {
        *value_out = NULL;
        *length_out = 0;
        return PY68_STATUS_SOURCE_ERROR;
    }
    ok = NameFromLock(lock, pathbuf, (LONG)sizeof(pathbuf));
    UnLock(lock);
    if (!ok) {
        *value_out = NULL;
        *length_out = 0;
        return PY68_STATUS_RUNTIME_ERROR;
    }
    length = py68_c_strlen(pathbuf);
    copy = (char *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP, length + 1);
    if (copy == NULL) return PY68_STATUS_MEMORY_ERROR;
    memcpy(copy, pathbuf, length + 1);
    *value_out = copy;
    *length_out = length;
    return PY68_STATUS_OK;
}

Py68Status py68_platform_var_set(Py68Runtime *runtime, const char *name,
                                 const char *value)
{
    /* AssignPath: late/non-binding style path assign (DOS). */
    (void)runtime;
    if (name == NULL || value == NULL) return PY68_STATUS_INTERNAL_ERROR;
    if (!AssignPath(name, value)) return PY68_STATUS_RUNTIME_ERROR;
    return PY68_STATUS_OK;
}

Py68Status py68_platform_var_unset(Py68Runtime *runtime, const char *name)
{
    /* AssignLock(name, ZERO) removes an existing assign. */
    (void)runtime;
    if (name == NULL) return PY68_STATUS_INTERNAL_ERROR;
    if (AssignLock(name, (BPTR)0) == 0) return PY68_STATUS_RUNTIME_ERROR;
    return PY68_STATUS_OK;
}

void py68_platform_unload_seg(void *seg)
{
    if (seg != NULL)
        UnLoadSeg((BPTR)(unsigned long)seg);
}
