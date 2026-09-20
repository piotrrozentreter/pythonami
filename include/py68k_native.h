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

/* `consumes_iterable` bit. Py68Object::flags already carries the error kind for
   the exception constructors, so this lives in its own field. */
#define PY68_NATIVE_ITERABLE_ARG 1

typedef struct Py68NativeFunction {
    Py68Object base;
    const char *name;
    Py68U16 minimum_arguments;
    Py68U16 maximum_arguments;
    /* PY68_NATIVE_ITERABLE_ARG when the first argument is consumed as an
       iterable. OP_CALL then drains a generator in that position into a list
       before invoking the callback, because a native cannot re-enter the
       interpreter without the C recursion the explicit VM stacks exist to
       avoid (D-0045). */
    Py68U16 consumes_iterable;
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
