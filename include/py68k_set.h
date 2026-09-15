/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_SET_H
#define PY68K_SET_H

#include "py68k_object.h"
#include "py68k_status.h"

struct Py68List;

typedef struct Py68SetEntry {
    Py68U32 hash;
    Py68U8 used;
    Py68Value value;
} Py68SetEntry;

struct Py68Set {
    Py68Object base;
    Py68U32 count;
    Py68U32 capacity;
    Py68SetEntry *entries;
};
typedef struct Py68Set Py68Set;

Py68Status py68_set_new(struct Py68Runtime *runtime, Py68Set **result);
Py68Status py68_set_add(struct Py68Runtime *runtime, Py68Set *set,
                        Py68Value value);
Py68Status py68_set_remove(struct Py68Runtime *runtime, Py68Set *set,
                           Py68Value value);
Py68Status py68_set_discard(struct Py68Runtime *runtime, Py68Set *set,
                            Py68Value value);
Py68Status py68_set_values(struct Py68Runtime *runtime, Py68Set *set,
                           struct Py68List **result);
int py68_set_equal(struct Py68Runtime *runtime, Py68Set *left, Py68Set *right);
/* Returns 1 if value is present, 0 if absent. Unhashable → SOURCE_ERROR. */
Py68Status py68_set_contains(struct Py68Runtime *runtime, Py68Set *set,
                             Py68Value value, int *found);

#endif
