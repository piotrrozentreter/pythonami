/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_RANGE_H
#define PY68K_RANGE_H

#include "py68k_object.h"
#include "py68k_list.h"
#include "py68k_status.h"
#include "py68k_value.h"

struct Py68Range {
    Py68Object base;
    Py68List *list;
    Py68I32 current;
    Py68I32 stop;
    Py68I32 step;
};
typedef struct Py68Range Py68Range;

Py68Status py68_range_new(struct Py68Runtime *runtime,
                         Py68I32 start, Py68I32 stop, Py68I32 step,
                         Py68Range **result);
Py68Status py68_range_new_list(struct Py68Runtime *runtime,
                              Py68List *list,
                              Py68Range **result);
Py68Status py68_range_next_value(struct Py68Runtime *runtime,
                                 Py68Range *range,
                                 Py68Value *result,
                                 int *has_next);
/* Convert list/tuple/str/dict/set/RANGE into a RANGE iterator.
 * Returns PY68_STATUS_SOURCE_ERROR when the value is not iterable. */
Py68Status py68_iterable_get_iter(struct Py68Runtime *runtime,
                                  Py68Value value,
                                  Py68Range **result);

#endif
