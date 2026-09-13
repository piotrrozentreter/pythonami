/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_FILE_H
#define PY68K_FILE_H

#include "py68k_object.h"
#include "py68k_platform.h"
#include "py68k_status.h"

struct Py68File {
    Py68Object base;
    Py68PlatformFileHandle handle;
    Py68U16 closed;
    Py68U16 readable;
    Py68U16 writable;
    Py68U16 binary;
};
typedef struct Py68File Py68File;

Py68Status py68_file_open(struct Py68Runtime *runtime, const char *path,
                          const char *mode, Py68File **result);
Py68Status py68_file_close(struct Py68Runtime *runtime, Py68File *file);
void py68_file_destroy(struct Py68Runtime *runtime, Py68File *file);

#endif
