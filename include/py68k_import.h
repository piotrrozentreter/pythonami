/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_IMPORT_H
#define PY68K_IMPORT_H

#include "py68k_status.h"
#include "py68k_value.h"

Py68Status py68_import_name(struct Py68Runtime *runtime, const Py68U8 *name,
                            Py68U16 name_length, Py68Value *result);
Py68Status py68_sys_install(struct Py68Runtime *runtime);
void py68_set_script_dir(struct Py68Runtime *runtime, const char *path);
Py68Status py68_sys_set_argv(struct Py68Runtime *runtime, int argc,
                             char **argv);

#endif
