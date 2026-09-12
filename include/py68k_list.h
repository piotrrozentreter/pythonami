#ifndef PY68K_LIST_H
#define PY68K_LIST_H

#include "py68k_object.h"
#include "py68k_status.h"

struct Py68List {
    Py68Object base;
    Py68U32 count;
    Py68U32 capacity;
    Py68Value *items;
};
typedef struct Py68List Py68List;

Py68Status py68_list_new(struct Py68Runtime *runtime, Py68List **result);
Py68Status py68_list_append_copy(struct Py68Runtime *runtime, Py68List *list,
                                 Py68Value value);
Py68Status py68_list_append_move(struct Py68Runtime *runtime, Py68List *list,
                                 Py68Value *value);
Py68Status py68_list_get_copy(struct Py68Runtime *runtime, Py68List *list,
                              Py68I32 index, Py68Value *result);
Py68Status py68_list_set_copy(struct Py68Runtime *runtime, Py68List *list,
                              Py68I32 index, Py68Value value);
Py68Status py68_list_set_move(struct Py68Runtime *runtime, Py68List *list,
                              Py68I32 index, Py68Value *value);

#endif
