/* 2026 by Piotr Rozentreter (Rozsoft) */

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
    /* VM-internal marker for a declared-but-not-yet-assigned local slot.
       Never user-visible: OP_LOAD_LOCAL raises NameError instead of
       exposing it. */
    PY68_VALUE_UNBOUND = 4,
    PY68_VALUE_FLOAT = 5
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
Py68Value py68_value_unbound(void);
Py68Value py68_value_bool(int truth);
Py68Value py68_value_int(Py68I32 integer);
Py68Value py68_value_from_object(Py68Object *object);
Py68Value py68_value_float_bits(Py68U32 bits);
Py68U32 py68_value_float_bits_get(Py68Value value);
int py68_value_is_number(Py68Value value);
int py68_value_truthy(Py68Value value);
int py68_value_equal(struct Py68Runtime *runtime, Py68Value left,
                     Py68Value right);
int py68_value_identical(Py68Value left, Py68Value right);
int py68_value_hash(struct Py68Runtime *runtime, Py68Value value,
                    Py68U32 *hash_out);
int py68_value_hashable(Py68Value value);
int py68_error_is_catchable(Py68U16 kind);
void py68_value_retain(Py68Value value);
void py68_value_release(struct Py68Runtime *runtime, Py68Value value);

#endif
