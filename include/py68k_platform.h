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

Py68Status py68_platform_initialize(Py68Runtime *runtime);
void py68_platform_shutdown(Py68Runtime *runtime);
Py68Status py68_platform_write_stdout(Py68Runtime *runtime,
                                      const char *data, Py68U32 length);
Py68Status py68_platform_write_stderr(Py68Runtime *runtime,
                                      const char *data, Py68U32 length);
void py68_platform_flush_stdout(void);
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

#endif
