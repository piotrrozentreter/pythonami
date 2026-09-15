/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_platform.h"
#include "py68k_runtime.h"

#include <time.h>
#include <stdio.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

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

Py68Status py68_platform_time_epoch(Py68Runtime *runtime, Py68U32 *seconds,
                                    Py68U32 *microseconds)
{
    time_t value;
    (void)runtime;
    value = time(NULL);
    if (value == (time_t)-1) return PY68_STATUS_RUNTIME_ERROR;
    *seconds = (Py68U32)value;
    *microseconds = 0;
    return PY68_STATUS_OK;
}

Py68Status py68_platform_time_monotonic(Py68Runtime *runtime,
                                        Py68U32 *seconds,
                                        Py68U32 *microseconds)
{
#if defined(_WIN32)
    DWORD value;
    (void)runtime;
    value = GetTickCount();
    *seconds = (Py68U32)(value / 1000UL);
    *microseconds = (Py68U32)(value % 1000UL) * 1000UL;
#else
    struct timeval value;
    (void)runtime;
    if (gettimeofday(&value, NULL) != 0) return PY68_STATUS_RUNTIME_ERROR;
    *seconds = (Py68U32)value.tv_sec;
    *microseconds = (Py68U32)value.tv_usec;
#endif
    return PY68_STATUS_OK;
}

Py68Status py68_platform_time_tick(Py68Runtime *runtime,
                                   Py68U32 *milliseconds)
{
#if defined(_WIN32)
    (void)runtime;
    *milliseconds = (Py68U32)(GetTickCount() & 0x7FFFFFFFUL);
#else
    struct timeval value;
    (void)runtime;
    if (gettimeofday(&value, NULL) != 0) return PY68_STATUS_RUNTIME_ERROR;
    *milliseconds = ((Py68U32)value.tv_sec * 1000UL +
                     (Py68U32)value.tv_usec / 1000UL) & 0x7FFFFFFFUL;
#endif
    return PY68_STATUS_OK;
}

Py68Status py68_platform_sleep(Py68Runtime *runtime, Py68U32 seconds,
                                Py68U32 microseconds)
{
    Py68U32 total = seconds > 4294UL ? 0xFFFFFFFFUL : seconds * 1000000UL;
    (void)runtime;
    if (total != 0xFFFFFFFFUL) total += microseconds;
    if (total == 0xFFFFFFFFUL) return PY68_STATUS_RUNTIME_ERROR;
#if defined(_WIN32)
    Sleep((DWORD)((total + 999UL) / 1000UL));
    return PY68_STATUS_OK;
#else
    if (usleep((unsigned int)total) != 0) return PY68_STATUS_RUNTIME_ERROR;
    return PY68_STATUS_OK;
#endif
}

Py68Status py68_platform_system(Py68Runtime *runtime, const char *command,
                                Py68I32 *return_code)
{
    long status;
    (void)runtime;
    if (command == NULL || return_code == NULL) return PY68_STATUS_INTERNAL_ERROR;
    status = (long)system(command);
    if (status > 2147483647L || status < (-2147483647L - 1L))
        return PY68_STATUS_RUNTIME_ERROR;
    *return_code = (Py68I32)status;
    return PY68_STATUS_OK;
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
