/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_PLATFORM_H
#define PY68K_PLATFORM_H

#include "py68k_status.h"
#include "py68k_types.h"

struct Py68Runtime;
typedef struct Py68Runtime Py68Runtime;

Py68Status py68_platform_initialize(Py68Runtime *runtime);
void py68_platform_shutdown(Py68Runtime *runtime);
Py68Status py68_platform_write_stdout(Py68Runtime *runtime,
                                      const char *data, Py68U32 length);
Py68Status py68_platform_write_stderr(Py68Runtime *runtime,
                                      const char *data, Py68U32 length);
Py68Status py68_platform_read_file(Py68Runtime *runtime, const char *path,
                                   Py68U8 **data, Py68U32 *length);

#endif
