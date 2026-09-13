/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_platform.h"
#include "py68k_runtime.h"

#include <stdio.h>

Py68Status py68_platform_initialize(Py68Runtime *runtime)
{
    (void)runtime;
    return PY68_STATUS_OK;
}

void py68_platform_shutdown(Py68Runtime *runtime)
{
    (void)runtime;
}

static Py68Status py68_platform_write(FILE *stream, const char *data,
                                       Py68U32 length)
{
    if (length != 0 && fwrite(data, 1, (size_t)length, stream) != length) {
        return PY68_STATUS_RUNTIME_ERROR;
    }
    return PY68_STATUS_OK;
}

Py68Status py68_platform_write_stdout(Py68Runtime *runtime,
                                      const char *data, Py68U32 length)
{
    (void)runtime;
    return py68_platform_write(stdout, data, length);
}

Py68Status py68_platform_write_stderr(Py68Runtime *runtime,
                                      const char *data, Py68U32 length)
{
    (void)runtime;
    return py68_platform_write(stderr, data, length);
}

void py68_platform_flush_stdout(void)
{
    fflush(stdout);
}

Py68Status py68_platform_read_stdin_line(Py68Runtime *runtime, Py68U8 **data,
                                         Py68U32 *length)
{
    Py68U32 capacity = 128;
    Py68U32 used = 0;
    Py68U8 *buffer;
    int ch;
    int got_any = 0;

    if (runtime == NULL || data == NULL || length == NULL)
        return PY68_STATUS_INTERNAL_ERROR;
    buffer = (Py68U8 *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP, capacity);
    if (buffer == NULL) return PY68_STATUS_MEMORY_ERROR;
    for (;;) {
        ch = fgetc(stdin);
        if (ch == EOF) break;
        got_any = 1;
        if (ch == '\n') break;
        if (ch == '\r') continue;
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
    }
    if (!got_any && ch == EOF) {
        py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer, capacity);
        *data = NULL;
        *length = 0;
        return PY68_STATUS_SOURCE_ERROR;
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
