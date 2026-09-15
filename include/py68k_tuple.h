/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_TUPLE_H
#define PY68K_TUPLE_H

#include "py68k_object.h"
#include "py68k_status.h"

struct Py68Tuple {
    Py68Object base;
    Py68U32 count;
    Py68Value *items;
};
typedef struct Py68Tuple Py68Tuple;

Py68Status py68_tuple_new(struct Py68Runtime *runtime, Py68U32 count,
                          Py68Tuple **result);
Py68Status py68_tuple_from_values(struct Py68Runtime *runtime,
                                  Py68Value *items, Py68U32 count,
                                  Py68Tuple **result);
Py68Status py68_tuple_get_copy(struct Py68Runtime *runtime, Py68Tuple *tuple,
                               Py68I32 index, Py68Value *result);
Py68Status py68_tuple_concat(struct Py68Runtime *runtime, Py68Tuple *left,
                             Py68Tuple *right, Py68Tuple **result);
Py68Status py68_tuple_slice(struct Py68Runtime *runtime, Py68Tuple *tuple,
                            Py68I32 start, Py68I32 end, int start_omitted,
                            int end_omitted, Py68Tuple **result);
int py68_tuple_equal(struct Py68Runtime *runtime, Py68Tuple *left,
                     Py68Tuple *right);
int py68_tuple_hash(struct Py68Runtime *runtime, Py68Tuple *tuple,
                    Py68U32 *hash_out);
int py68_tuple_has_item(struct Py68Runtime *runtime, Py68Tuple *tuple,
                        Py68Value value);

#endif
