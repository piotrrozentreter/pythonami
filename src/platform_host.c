/* 2026 by Piotr Rozentreter (Rozsoft) */

#define _DEFAULT_SOURCE

#include "py68k_platform.h"
#include "py68k_runtime.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

static volatile sig_atomic_t py68_host_break_pending = 0;
static void (*py68_host_previous_sigint)(int) = NULL;

static void py68_host_sigint(int signal_number)
{
    (void)signal_number;
    py68_host_break_pending = 1;
    signal(SIGINT, py68_host_sigint);
}

Py68Status py68_platform_initialize(Py68Runtime *runtime)
{
    (void)runtime;
    py68_host_break_pending = 0;
    py68_host_previous_sigint = signal(SIGINT, py68_host_sigint);
    return PY68_STATUS_OK;
}

void py68_platform_shutdown(Py68Runtime *runtime)
{
    (void)runtime;
    if (py68_host_previous_sigint != SIG_ERR)
        signal(SIGINT, py68_host_previous_sigint);
    py68_host_previous_sigint = NULL;
    py68_host_break_pending = 0;
}

Py68Status py68_platform_poll(Py68Runtime *runtime, Py68U32 flags)
{
    (void)runtime;
    if ((flags & PY68_POLL_BREAK) != 0 && py68_host_break_pending != 0) {
        py68_host_break_pending = 0;
        return PY68_STATUS_RUNTIME_ERROR;
    }
    return PY68_STATUS_OK;
}

void py68_platform_signal_break(Py68Runtime *runtime)
{
    (void)runtime;
    py68_host_break_pending = 1;
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

/* Build a per-process temporary file path under TEMP/TMP/TMPDIR or ".". */
static int py68_host_temp_path(char *buffer, size_t buffer_size)
{
    const char *directory = getenv("TEMP");
    unsigned long pid;
    if (directory == NULL) directory = getenv("TMP");
    if (directory == NULL) directory = getenv("TMPDIR");
    if (directory == NULL) directory = ".";
#if defined(_WIN32)
    pid = (unsigned long)GetCurrentProcessId();
#else
    pid = (unsigned long)getpid();
#endif
    if ((size_t)snprintf(buffer, buffer_size, "%s/py68k_popen_%08lx.tmp",
                         directory, pid) >= buffer_size) {
        return 0;
    }
    return 1;
}

Py68Status py68_platform_system_capture(Py68Runtime *runtime,
                                        const char *command,
                                        Py68I32 *return_code,
                                        Py68U8 **data, Py68U32 *length)
{
    char temp_path[512];
    char *full_command;
    Py68U32 command_capacity;
    long status;
    FILE *file;
    long file_size;
    Py68U8 *buffer;
    size_t read_count;

    if (runtime == NULL || command == NULL || return_code == NULL ||
        data == NULL || length == NULL)
        return PY68_STATUS_INTERNAL_ERROR;
    *data = NULL;
    *length = 0;
    if (!py68_host_temp_path(temp_path, sizeof temp_path))
        return PY68_STATUS_RUNTIME_ERROR;
    command_capacity = (Py68U32)strlen(command) + (Py68U32)strlen(temp_path) +
                       16UL;
    full_command = (char *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP,
                                      command_capacity);
    if (full_command == NULL) return PY68_STATUS_MEMORY_ERROR;
    sprintf(full_command, "%s > \"%s\" 2>&1", command, temp_path);
    status = (long)system(full_command);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, full_command,
             command_capacity);
    if (status > 2147483647L || status < (-2147483647L - 1L)) {
        remove(temp_path);
        return PY68_STATUS_RUNTIME_ERROR;
    }
    file = fopen(temp_path, "rb");
    if (file == NULL) {
        remove(temp_path);
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (fseek(file, 0, SEEK_END) != 0 || (file_size = ftell(file)) < 0 ||
        fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        remove(temp_path);
        return PY68_STATUS_RUNTIME_ERROR;
    }
    buffer = (Py68U8 *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP,
                                  (Py68U32)file_size + 1UL);
    if (buffer == NULL) {
        fclose(file);
        remove(temp_path);
        return PY68_STATUS_MEMORY_ERROR;
    }
    read_count = fread(buffer, 1, (size_t)file_size, file);
    fclose(file);
    remove(temp_path);
    buffer[read_count] = 0;
    *return_code = (Py68I32)status;
    *data = buffer;
    *length = (Py68U32)read_count;
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
