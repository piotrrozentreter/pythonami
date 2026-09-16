/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_platform.h"
#include "py68k_runtime.h"

#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>

#include <string.h>

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

static ULONG py68_amiga_pipe_sequence = 0;

static int py68_amiga_append_hex(char *buffer, int position, ULONG value)
{
    static const char hex[] = "0123456789abcdef";
    int index;
    for (index = 7; index >= 0; --index)
        buffer[position++] = hex[(value >> (index * 4)) & 0xFUL];
    return position;
}

static void py68_amiga_pipe_name(char *buffer)
{
    static const char prefix[] = "PIPE:py68k_";
    ULONG sequence;
    int position = 0;
    int index;

    sequence = ++py68_amiga_pipe_sequence;
    if (sequence == 0) sequence = ++py68_amiga_pipe_sequence;
    for (index = 0; prefix[index] != '\0'; ++index)
        buffer[position++] = prefix[index];
    position = py68_amiga_append_hex(
        buffer, position, (ULONG)(APTR)FindTask(NULL));
    buffer[position++] = '_';
    position = py68_amiga_append_hex(buffer, position, sequence);
    buffer[position] = '\0';
}

Py68Status py68_platform_system_capture(Py68Runtime *runtime,
                                        const char *command,
                                        Py68I32 *return_code,
                                        Py68U8 **data, Py68U32 *length)
{
    char pipe_name[40];
    char *shell_command;
    Py68U32 command_length;
    Py68U32 shell_length;
    BPTR shell_output;
    BPTR pipe;
    struct TagItem tags[5];
    LONG launch_status;
    LONG read_count;
    Py68U8 *buffer;
    Py68U32 capacity = 256;
    Py68U32 used = 0;

    if (runtime == NULL || command == NULL || return_code == NULL ||
        data == NULL || length == NULL)
        return PY68_STATUS_INTERNAL_ERROR;
    *data = NULL;
    *length = 0;
    py68_amiga_pipe_name(pipe_name);
    command_length = (Py68U32)strlen(command);
    if (command_length > 0xFFFFFFFFUL - (Py68U32)strlen(pipe_name) - 3UL)
        return PY68_STATUS_MEMORY_ERROR;
    shell_length = command_length + (Py68U32)strlen(pipe_name) + 2UL;
    shell_command = (char *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP,
                                      shell_length + 1UL);
    if (shell_command == NULL) return PY68_STATUS_MEMORY_ERROR;
    memcpy(shell_command, command, command_length);
    shell_command[command_length] = ' ';
    shell_command[command_length + 1UL] = '>';
    memcpy(shell_command + command_length + 2UL, pipe_name,
           (Py68U32)strlen(pipe_name) + 1UL);

        /* Async SystemTagList closes its input and output streams. */
    shell_output = Open("NIL:", MODE_NEWFILE);
    if (shell_output == 0) {
        py68_free(&runtime->allocator, PY68_MEM_TEMP, shell_command,
                  shell_length + 1UL);
        return PY68_STATUS_RUNTIME_ERROR;
    }
    tags[0].ti_Tag = SYS_Asynch;
    tags[0].ti_Data = TRUE;
    tags[1].ti_Tag = SYS_UserShell;
    tags[1].ti_Data = TRUE;
    tags[2].ti_Tag = SYS_Input;
    tags[2].ti_Data = 0;
    tags[3].ti_Tag = SYS_Output;
    tags[3].ti_Data = (ULONG)shell_output;
    tags[4].ti_Tag = TAG_DONE;
    tags[4].ti_Data = 0;
    launch_status = SystemTagList((STRPTR)shell_command, tags);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, shell_command,
              shell_length + 1UL);
    if (launch_status == -1) {
        Close(shell_output);
        return PY68_STATUS_RUNTIME_ERROR;
    }

    pipe = Open(pipe_name, MODE_OLDFILE);
    if (pipe == 0)
        return PY68_STATUS_RUNTIME_ERROR;
    buffer = (Py68U8 *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP,
                                  capacity);
    if (buffer == NULL) {
        Close(pipe);
        return PY68_STATUS_MEMORY_ERROR;
    }
    for (;;) {
        if (used + 1UL >= capacity) {
            Py68U32 new_capacity = capacity * 2UL;
            Py68U8 *replacement;
            if (capacity == 1048576UL) {
                UBYTE extra;
                read_count = Read(pipe, &extra, 1);
                if (read_count == 0) break;
                py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer,
                          capacity);
                Close(pipe);
                return read_count < 0 ? PY68_STATUS_RUNTIME_ERROR :
                                        PY68_STATUS_MEMORY_ERROR;
            }
            if (new_capacity < capacity || new_capacity > 1048576UL) {
                py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer,
                          capacity);
                Close(pipe);
                return PY68_STATUS_MEMORY_ERROR;
            }
            replacement = (Py68U8 *)py68_realloc(
                &runtime->allocator, PY68_MEM_TEMP, buffer, capacity,
                new_capacity);
            if (replacement == NULL) {
                py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer,
                          capacity);
                Close(pipe);
                return PY68_STATUS_MEMORY_ERROR;
            }
            buffer = replacement;
            capacity = new_capacity;
        }
        read_count = Read(pipe, buffer + used, (LONG)(capacity - used - 1UL));
        if (read_count < 0) {
            py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer, capacity);
            Close(pipe);
            return PY68_STATUS_RUNTIME_ERROR;
        }
        if (read_count == 0) break;
        used += (Py68U32)read_count;
    }
    Close(pipe);
    buffer[used] = 0;
    if (used + 1UL < capacity) {
        Py68U8 *exact = (Py68U8 *)py68_realloc(
            &runtime->allocator, PY68_MEM_TEMP, buffer, capacity, used + 1UL);
        if (exact == NULL) {
            py68_free(&runtime->allocator, PY68_MEM_TEMP, buffer, capacity);
            return PY68_STATUS_MEMORY_ERROR;
        }
        buffer = exact;
    }
    *return_code = 0;
    *data = buffer;
    *length = used;
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
