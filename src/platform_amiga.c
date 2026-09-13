/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_platform.h"
#include "py68k_runtime.h"

#include <proto/dos.h>

Py68Status py68_platform_initialize(Py68Runtime *runtime)
{
    runtime->trace_enabled = 0;
    return PY68_STATUS_OK;
}

void py68_platform_shutdown(Py68Runtime *runtime)
{
    runtime->trace_enabled = 0;
}

static Py68Status py68_platform_write(BPTR handle, const char *data,
                                       Py68U32 length)
{
    LONG written;
    if (length > 2147483647UL) return PY68_STATUS_RUNTIME_ERROR;
    written = Write(handle, (CONST_APTR)data, (LONG)length);
    if (written != (LONG)length) {
        return PY68_STATUS_RUNTIME_ERROR;
    }
    return PY68_STATUS_OK;
}

Py68Status py68_platform_write_stdout(Py68Runtime *runtime,
                                      const char *data, Py68U32 length)
{
    Py68Status status = py68_platform_write(Output(), data, length);
    if (status != PY68_STATUS_OK) {
        runtime->error.active = 1;
    }
    return status;
}

Py68Status py68_platform_write_stderr(Py68Runtime *runtime,
                                      const char *data, Py68U32 length)
{
    Py68Status status = py68_platform_write(ErrorOutput(), data, length);
    if (status != PY68_STATUS_OK) {
        runtime->error.active = 1;
    }
    return status;
}

void py68_platform_flush_stdout(void)
{
    Flush(Output());
}

Py68Status py68_platform_read_stdin_line(Py68Runtime *runtime, Py68U8 **data,
                                         Py68U32 *length)
{
    BPTR input = Input();
    Py68U32 capacity = 128;
    Py68U32 used = 0;
    Py68U8 *buffer;
    UBYTE ch;
    LONG got;
    int got_any = 0;

    if (runtime == NULL || data == NULL || length == NULL)
        return PY68_STATUS_INTERNAL_ERROR;
    if (input == 0) return PY68_STATUS_RUNTIME_ERROR;
    buffer = (Py68U8 *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP, capacity);
    if (buffer == NULL) return PY68_STATUS_MEMORY_ERROR;
    for (;;) {
        got = Read(input, &ch, 1);
        if (got == 0) break;
        if (got < 0) {
            py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer, capacity);
            return PY68_STATUS_RUNTIME_ERROR;
        }
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
    if (!got_any) {
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
