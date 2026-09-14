/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_EXT_LOAD_H
#define PY68K_EXT_LOAD_H

#include "py68k_runtime.h"
#include "py68k_status.h"
#include "py68k_value.h"

/* Amiga-only: LoadSeg a *.py68k plugin and return a module of natives. */
Py68Status py68_ext_load_library(Py68Runtime *runtime, const char *path,
                                 Py68Value *result);
Py68Status py68_builtin_load_library(Py68Runtime *runtime,
                                     Py68U16 argument_count,
                                     Py68Value *arguments, Py68Value *result);

#endif
