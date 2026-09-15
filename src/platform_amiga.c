/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_platform.h"
#include "py68k_runtime.h"

#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>

Py68Status py68_platform_initialize(Py68Runtime *runtime)
{
    runtime->trace_enabled = 0;
    return PY68_STATUS_OK;
}

void py68_platform_shutdown(Py68Runtime *runtime)
{
    runtime->trace_enabled = 0;
}

Py68Status py68_platform_poll(Py68Runtime *runtime, Py68U32 flags)
{
    (void)runtime;
    if ((flags & PY68_POLL_BREAK) != 0 &&
        (CheckSignal(SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C) != 0)
        return PY68_STATUS_RUNTIME_ERROR;
    if ((flags & PY68_POLL_YIELD) != 0) {
        /* Exec has no Yield(); Permit() reschedules when attention is set. */
        Forbid();
        Permit();
    }
    return PY68_STATUS_OK;
}

void py68_platform_signal_break(Py68Runtime *runtime)
{
    (void)runtime;
    Signal(FindTask(NULL), SIGBREAKF_CTRL_C);
}

/*
 * ErrorOutput() is dos.library V47 (AmigaOS 3.2). On Kickstart 2.x–3.1 the
 * LVO is missing and calling it Guru Meditation / hard-crashes. Mirror the
 * V47 semantics: pr_CES when set, otherwise Output().
 */
static BPTR py68_amiga_error_output(void)
{
    BPTR handle = 0;

    if (DOSBase != NULL && DOSBase->dl_lib.lib_Version >= 47)
        return ErrorOutput();

    {
        struct Process *process = (struct Process *)FindTask(NULL);
        if (process != NULL)
            handle = process->pr_CES;
    }
    if (handle == 0)
        handle = Output();
    return handle;
}

static Py68Status py68_platform_write(BPTR handle, const char *data,
                                       Py68U32 length)
{
    LONG written;
    if (handle == 0) return PY68_STATUS_RUNTIME_ERROR;
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
    Py68Status status = py68_platform_write(py68_amiga_error_output(),
                                            data, length);
    if (status != PY68_STATUS_OK) {
        runtime->error.active = 1;
    }
    return status;
}

void py68_platform_flush_stdout(void)
{
    Flush(Output());
}

Py68Status py68_platform_time_epoch(Py68Runtime *runtime, Py68U32 *seconds,
                                    Py68U32 *microseconds)
{
    struct DateStamp stamp;
    (void)runtime;
    DateStamp(&stamp);
    *seconds = (Py68U32)stamp.ds_Days * 86400UL +
               (Py68U32)stamp.ds_Minute * 60UL +
               (Py68U32)stamp.ds_Tick / 50UL + 252460800UL;
    *microseconds = ((Py68U32)stamp.ds_Tick % 50UL) * 20000UL;
    return PY68_STATUS_OK;
}

Py68Status py68_platform_time_monotonic(Py68Runtime *runtime,
                                        Py68U32 *seconds,
                                        Py68U32 *microseconds)
{
    return py68_platform_time_epoch(runtime, seconds, microseconds);
}

Py68Status py68_platform_time_tick(Py68Runtime *runtime,
                                   Py68U32 *milliseconds)
{
    struct DateStamp stamp;
    Py68U32 total;
    (void)runtime;
    DateStamp(&stamp);
    total = ((Py68U32)stamp.ds_Days * 86400000UL) & 0x7FFFFFFFUL;
    total = (total + (Py68U32)stamp.ds_Minute * 60000UL) & 0x7FFFFFFFUL;
    total = (total + (Py68U32)stamp.ds_Tick * 20UL) & 0x7FFFFFFFUL;
    *milliseconds = total;
    return PY68_STATUS_OK;
}

Py68Status py68_platform_sleep(Py68Runtime *runtime, Py68U32 seconds,
                                Py68U32 microseconds)
{
    struct DateStamp delay;
    (void)runtime;
    delay.ds_Days = (LONG)seconds / 86400L;
    delay.ds_Minute = ((LONG)seconds % 86400L) / 60L;
    delay.ds_Tick = ((LONG)seconds % 60L) * 50L +
                    (LONG)(microseconds / 20000UL);
    Delay(delay.ds_Days * 4320000L + delay.ds_Minute * 3000L + delay.ds_Tick);
    return PY68_STATUS_OK;
}

Py68Status py68_platform_system(Py68Runtime *runtime, const char *command,
                                Py68I32 *return_code)
{
    LONG status;
    (void)runtime;
    if (command == NULL || return_code == NULL) return PY68_STATUS_INTERNAL_ERROR;
    /*
     * Use SystemTagList (dos.library V36+), not Execute.
     * Execute returns DOSTRUE (-1) / DOSFALSE (0) for launch success only and
     * does not expose the command return code. SystemTagList returns the
     * AmigaDOS/program status, or -1 if the shell could not be started.
     */
    status = SystemTagList((STRPTR)command, NULL);
    if (status == -1) return PY68_STATUS_RUNTIME_ERROR;
    *return_code = (Py68I32)status;
    return PY68_STATUS_OK;
}

/* Build "T:py68k_<hex task pointer>.tmp" without pulling in libc stdio. */
static void py68_amiga_temp_path(char *buffer, ULONG value)
{
    static const char hex[] = "0123456789abcdef";
    static const char prefix[] = "T:py68k_";
    static const char suffix[] = ".tmp";
    int position = 0;
    int index;
    for (index = 0; prefix[index] != '\0'; ++index) buffer[position++] = prefix[index];
    for (index = 7; index >= 0; --index)
        buffer[position++] = hex[(value >> (index * 4)) & 0xFUL];
    for (index = 0; suffix[index] != '\0'; ++index) buffer[position++] = suffix[index];
    buffer[position] = '\0';
}

Py68Status py68_platform_system_capture(Py68Runtime *runtime,
                                        const char *command,
                                        Py68I32 *return_code,
                                        Py68U8 **data, Py68U32 *length)
{
    char temp_path[32];
    BPTR output_handle;
    BPTR read_handle;
    struct TagItem tags[2];
    LONG status;
    LONG file_size;
    LONG read_count;
    Py68U8 *buffer;

    if (runtime == NULL || command == NULL || return_code == NULL ||
        data == NULL || length == NULL)
        return PY68_STATUS_INTERNAL_ERROR;
    *data = NULL;
    *length = 0;
    py68_amiga_temp_path(temp_path, (ULONG)(APTR)FindTask(NULL));
    output_handle = Open(temp_path, MODE_NEWFILE);
    if (output_handle == 0) return PY68_STATUS_RUNTIME_ERROR;
    /* SystemTagList takes ownership of SYS_Output and closes it for us. */
    tags[0].ti_Tag = SYS_Output;
    tags[0].ti_Data = (ULONG)output_handle;
    tags[1].ti_Tag = TAG_DONE;
    tags[1].ti_Data = 0;
    status = SystemTagList((STRPTR)command, tags);
    if (status == -1) {
        DeleteFile(temp_path);
        return PY68_STATUS_RUNTIME_ERROR;
    }
    read_handle = Open(temp_path, MODE_OLDFILE);
    if (read_handle == 0) {
        DeleteFile(temp_path);
        return PY68_STATUS_RUNTIME_ERROR;
    }
    Seek(read_handle, 0, OFFSET_END);
    file_size = Seek(read_handle, 0, OFFSET_CURRENT);
    Seek(read_handle, 0, OFFSET_BEGINNING);
    if (file_size < 0) {
        Close(read_handle);
        DeleteFile(temp_path);
        return PY68_STATUS_RUNTIME_ERROR;
    }
    buffer = (Py68U8 *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP,
                                  (Py68U32)file_size + 1UL);
    if (buffer == NULL) {
        Close(read_handle);
        DeleteFile(temp_path);
        return PY68_STATUS_MEMORY_ERROR;
    }
    read_count = Read(read_handle, buffer, file_size);
    Close(read_handle);
    DeleteFile(temp_path);
    if (read_count != file_size) {
        py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer,
                 (Py68U32)file_size + 1UL);
        return PY68_STATUS_RUNTIME_ERROR;
    }
    buffer[file_size] = 0;
    *return_code = (Py68I32)status;
    *data = buffer;
    *length = (Py68U32)file_size;
    return PY68_STATUS_OK;
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
