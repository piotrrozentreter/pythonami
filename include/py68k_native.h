/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_NATIVE_H
#define PY68K_NATIVE_H

#include "py68k_object.h"
#include "py68k_status.h"

struct Py68NativeFunction;
typedef Py68Status (*Py68NativeCallback)(struct Py68Runtime *runtime,
                                         Py68U16 argument_count,
                                         Py68Value *arguments,
                                         Py68Value *result);

typedef struct Py68NativeFunction {
    Py68Object base;
    const char *name;
    Py68U16 minimum_arguments;
    Py68U16 maximum_arguments;
    Py68NativeCallback callback;
} Py68NativeFunction;

Py68Status py68_native_new(struct Py68Runtime *runtime, const char *name,
                           Py68U16 minimum_arguments,
                           Py68U16 maximum_arguments,
                           Py68NativeCallback callback,
                           Py68NativeFunction **result);
Py68Status py68_native_check_arguments(Py68NativeFunction *function,
                                       Py68U16 argument_count);
Py68Status py68_native_call(Py68NativeFunction *function,
                            struct Py68Runtime *runtime,
                            Py68U16 argument_count, Py68Value *arguments,
                            Py68Value *result);

#endif
