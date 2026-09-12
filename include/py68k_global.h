#ifndef PY68K_GLOBAL_H
#define PY68K_GLOBAL_H

#include "py68k_runtime.h"

Py68Status py68_global_set_copy(Py68Runtime *runtime, Py68U16 name_index,
                                Py68Value value);
Py68Status py68_global_get_copy(Py68Runtime *runtime, Py68U16 name_index,
                                Py68Value *result);
void py68_global_clear(Py68Runtime *runtime);

#endif