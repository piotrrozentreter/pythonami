/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_VM_H
#define PY68K_VM_H

#include "py68k_code.h"
#include "py68k_runtime.h"
#include "py68k_status.h"

Py68Status py68_vm_push_owned(Py68Runtime *runtime, Py68Value value);

enum {
    PY68_VM_NONE = 0,
    PY68_VM_BOOL = 1,
    PY68_VM_INT = 2
};

Py68Status py68_vm_execute(Py68Runtime *runtime, Py68Code *code);

#endif