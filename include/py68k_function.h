/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_FUNCTION_H
#define PY68K_FUNCTION_H

#include "py68k_code.h"
#include "py68k_object.h"
#include "py68k_status.h"

struct Py68Function {
    Py68Object base;
    Py68Code *code;
    Py68U16 argument_count;
    Py68U16 local_count;
};
typedef struct Py68Function Py68Function;

Py68Status py68_function_new(struct Py68Runtime *runtime, Py68Code *code,
                             Py68U16 argument_count, Py68U16 local_count,
                             Py68Function **result);
Py68Status py68_function_check_arguments(Py68Function *function,
                                         Py68U16 argument_count);

#endif
