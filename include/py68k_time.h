/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_TIME_H
#define PY68K_TIME_H

#include "py68k_object.h"
#include "py68k_status.h"

typedef struct Py68StructTime {
    Py68Object base;
    Py68I32 fields[9];
} Py68StructTime;

Py68Status py68_struct_time_new(struct Py68Runtime *runtime,
                                const Py68I32 fields[9],
                                Py68StructTime **result);

#endif