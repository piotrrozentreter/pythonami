/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_ATTR_H
#define PY68K_ATTR_H

#include "py68k_object.h"
#include "py68k_status.h"
#include "py68k_value.h"

struct Py68BoundMethod {
    Py68Object base;
    Py68Value self;
    struct Py68NativeFunction *function;
};
typedef struct Py68BoundMethod Py68BoundMethod;

Py68Status py68_bound_method_new(struct Py68Runtime *runtime, Py68Value self,
                                 struct Py68NativeFunction *function,
                                 Py68BoundMethod **result);
Py68Status py68_attr_load(struct Py68Runtime *runtime, Py68Value object,
                          const Py68U8 *name, Py68U16 name_length,
                          Py68Value *result);
Py68Status py68_attr_store(struct Py68Runtime *runtime, Py68Value object,
                           const Py68U8 *name, Py68U16 name_length,
                           Py68Value value);

#endif
