#ifndef PY68K_INTERN_H
#define PY68K_INTERN_H

#include "py68k_string.h"

typedef struct Py68InternEntry {
    Py68String *string;
    Py68U32 hash;
} Py68InternEntry;

typedef struct Py68InternTable {
    Py68InternEntry *entries;
    Py68U16 count;
    Py68U16 capacity;
} Py68InternTable;

void py68_intern_initialize(Py68InternTable *table);
void py68_intern_destroy(struct Py68Runtime *runtime,
                         Py68InternTable *table);
Py68Status py68_intern_get_copy(struct Py68Runtime *runtime,
                                Py68InternTable *table,
                                const char *data, Py68U32 length,
                                Py68String **result);

#endif