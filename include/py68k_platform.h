/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_PLATFORM_H
#define PY68K_PLATFORM_H

#include "py68k_status.h"
#include "py68k_types.h"

struct Py68Runtime;
typedef struct Py68Runtime Py68Runtime;

/* Opaque platform file handle (FILE* on host, BPTR boxed on Amiga). */
typedef void *Py68PlatformFileHandle;

typedef enum Py68PlatformFileMode {
    PY68_PFILE_READ = 1,
    PY68_PFILE_WRITE = 2,
    PY68_PFILE_APPEND = 3
} Py68PlatformFileMode;

/* py68_platform_poll flags. */
#define PY68_POLL_BREAK 0x0001U /* test for a pending user break (Ctrl-C) */
#define PY68_POLL_YIELD 0x0002U /* give up the remaining scheduler quantum */

Py68Status py68_platform_initialize(Py68Runtime *runtime);
void py68_platform_shutdown(Py68Runtime *runtime);
Py68Status py68_platform_write_stdout(Py68Runtime *runtime,
                                      const char *data, Py68U32 length);
Py68Status py68_platform_write_stderr(Py68Runtime *runtime,
                                      const char *data, Py68U32 length);
void py68_platform_flush_stdout(void);
Py68Status py68_platform_time_epoch(Py68Runtime *runtime, Py68U32 *seconds,
                                     Py68U32 *microseconds);
Py68Status py68_platform_time_monotonic(Py68Runtime *runtime,
                                         Py68U32 *seconds,
                                         Py68U32 *microseconds);
Py68Status py68_platform_time_tick(Py68Runtime *runtime, Py68U32 *milliseconds);
Py68Status py68_platform_sleep(Py68Runtime *runtime, Py68U32 seconds,
                               Py68U32 microseconds);
/* Execute a synchronous DOS command using inherited standard handles. */
Py68Status py68_platform_system(Py68Runtime *runtime, const char *command,
                                 Py68I32 *return_code);
/*
 * Execute a command and capture stdout into a PY68_MEM_TEMP buffer of
 * *length bytes plus a NUL terminator. The call returns after capture EOF.
 * The caller frees *data with py68_free(..., PY68_MEM_TEMP, *data,
 * *length + 1).
 */
Py68Status py68_platform_system_capture(Py68Runtime *runtime,
                                        const char *command,
                                        Py68I32 *return_code,
                                        Py68U8 **data, Py68U32 *length);
/* Read one line from console stdin (Input()/stdin). Strips trailing CR/LF.
   Empty line at EOF with no data returns PY68_STATUS_SOURCE_ERROR (EOF).
   Buffer is PY68_MEM_TEMP sized length+1. */
Py68Status py68_platform_read_stdin_line(Py68Runtime *runtime, Py68U8 **data,
                                         Py68U32 *length);
Py68Status py68_platform_read_file(Py68Runtime *runtime, const char *path,
                                   Py68U8 **data, Py68U32 *length);

Py68Status py68_platform_file_open(Py68Runtime *runtime, const char *path,
                                   Py68PlatformFileMode mode, int binary,
                                   Py68PlatformFileHandle *handle_out);
void py68_platform_file_close(Py68PlatformFileHandle handle);
Py68Status py68_platform_file_read(Py68Runtime *runtime,
                                   Py68PlatformFileHandle handle,
                                   Py68U32 max_count, Py68U8 **data,
                                   Py68U32 *length);
Py68Status py68_platform_file_readline(Py68Runtime *runtime,
                                       Py68PlatformFileHandle handle,
                                       Py68U8 **data, Py68U32 *length);
Py68Status py68_platform_file_write(Py68Runtime *runtime,
                                    Py68PlatformFileHandle handle,
                                    const char *data, Py68U32 length,
                                    Py68U32 *written);
int py68_platform_path_exists(const char *path);
Py68Status py68_platform_path_remove(const char *path);
Py68Status py68_platform_path_rename(const char *old_path,
                                     const char *new_path);

/*
 * Host: environment variables.
 * Amiga: DOS assigns (AssignPath / AssignLock / Lock+NameFromLock).
 * Missing get returns PY68_STATUS_SOURCE_ERROR with *value_out left NULL.
 */
Py68Status py68_platform_var_get(Py68Runtime *runtime, const char *name,
                                 char **value_out, Py68U32 *length_out);
Py68Status py68_platform_var_set(Py68Runtime *runtime, const char *name,
                                 const char *value);
Py68Status py68_platform_var_unset(Py68Runtime *runtime, const char *name);

/* Amiga: UnLoadSeg for LoadSeg plugins. Host: no-op. */
void py68_platform_unload_seg(void *seg);

/*
 * Cooperative check point called from the VM on backward branches.
 * PY68_POLL_BREAK consumes a pending break and returns
 * PY68_STATUS_RUNTIME_ERROR; the caller owns the diagnostic.
 * AmigaOS is preemptively multitasking, so PY68_POLL_YIELD is only a
 * politeness hint (Forbid/Permit reschedule), never required for fairness.
 */
Py68Status py68_platform_poll(Py68Runtime *runtime, Py68U32 flags);
/* Post a break to this process, as the shell does for Ctrl-C. */
void py68_platform_signal_break(Py68Runtime *runtime);

#endif
