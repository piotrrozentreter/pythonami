/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_MODULE_H
#define PY68K_MODULE_H

#include "py68k_code.h"
#include "py68k_object.h"
#include "py68k_runtime.h"
#include "py68k_status.h"

struct Py68Module {
    Py68Object base;
    char name[64];
    char path[PY68_PATH_MAX];
    Py68U8 *owned_source;
    Py68U32 owned_source_length;
    /* Retains compiled bytecode (and nested function bodies) for as long as
       this module's function objects may still be callable. */
    Py68Code owned_code;
    Py68GlobalEntry *globals;
    Py68U16 global_count;
    Py68U16 global_capacity;
    /* Amiga LoadSeg handle for *.py68k libraries; NULL for .py modules. */
    void *native_seg;
};
typedef struct Py68Module Py68Module;

Py68Status py68_module_new(struct Py68Runtime *runtime, const char *name,
                           const char *path, Py68Module **result);
Py68Status py68_module_set(struct Py68Runtime *runtime, Py68Module *module,
                           const Py68U8 *name, Py68U16 name_length,
                           Py68Value value);
Py68Status py68_module_get(struct Py68Runtime *runtime, Py68Module *module,
                           const Py68U8 *name, Py68U16 name_length,
                           Py68Value *result);
void py68_module_clear(struct Py68Runtime *runtime, Py68Module *module);

#endif
