/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_DICT_H
#define PY68K_DICT_H

#include "py68k_object.h"
#include "py68k_status.h"

struct Py68List;

typedef struct Py68DictEntry {
    Py68U32 hash;
    Py68U8 used;
    Py68Value key;
    Py68Value value;
} Py68DictEntry;

struct Py68Dict {
    Py68Object base;
    Py68U32 count;
    Py68U32 capacity;
    Py68DictEntry *entries;
};
typedef struct Py68Dict Py68Dict;

Py68Status py68_dict_new(struct Py68Runtime *runtime, Py68Dict **result);
Py68Status py68_dict_get_copy(struct Py68Runtime *runtime, Py68Dict *dict,
                              Py68Value key, Py68Value *result);
Py68Status py68_dict_set_copy(struct Py68Runtime *runtime, Py68Dict *dict,
                              Py68Value key, Py68Value value);
Py68Status py68_dict_pop(struct Py68Runtime *runtime, Py68Dict *dict,
                         Py68Value key, Py68Value *result);
Py68Status py68_dict_keys(struct Py68Runtime *runtime, Py68Dict *dict,
                          struct Py68List **result);
Py68Status py68_dict_values(struct Py68Runtime *runtime, Py68Dict *dict,
                            struct Py68List **result);
Py68Status py68_dict_items(struct Py68Runtime *runtime, Py68Dict *dict,
                           struct Py68List **result);
int py68_dict_equal(struct Py68Runtime *runtime, Py68Dict *left,
                    Py68Dict *right);

#endif
