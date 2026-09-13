#ifndef PY68K_VALUE_H
#define PY68K_VALUE_H

#include "py68k_types.h"

typedef struct Py68Object Py68Object;
struct Py68Runtime;

typedef enum Py68ValueType {
    PY68_VALUE_NONE = 0,
    PY68_VALUE_BOOL = 1,
    PY68_VALUE_INT = 2,
    PY68_VALUE_OBJECT = 3,
    PY68_VALUE_UNBOUND = 4
} Py68ValueType;

typedef struct Py68Value {
    Py68U16 type;
    Py68U16 reserved;
    union {
        Py68I32 integer;
        Py68Object *object;
    } as;
} Py68Value;

Py68Value py68_value_none(void);
Py68Value py68_value_bool(int truth);
Py68Value py68_value_int(Py68I32 integer);
Py68Value py68_value_from_object(Py68Object *object);
void py68_value_retain(Py68Value value);
void py68_value_release(struct Py68Runtime *runtime, Py68Value value);

#endif
