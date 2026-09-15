/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_STRING_H
#define PY68K_STRING_H

#include "py68k_object.h"
#include "py68k_status.h"

struct Py68String {
    Py68Object base;
    Py68U32 length;
    Py68U32 hash;
    char data[1];
};
typedef struct Py68String Py68String;

Py68Status py68_string_new_copy(struct Py68Runtime *runtime,
                                const char *data, Py68U32 length,
                                Py68String **result);
/* Decode Python68K string-literal escapes (\\ \' \" \n \r \t \xHH). */
Py68Status py68_string_new_from_escaped(struct Py68Runtime *runtime,
                                        const char *data, Py68U32 length,
                                        Py68String **result);
Py68U32 py68_string_hash_bytes(const char *data, Py68U32 length);
Py68Status py68_string_concat(struct Py68Runtime *runtime,
                              Py68String *left, Py68String *right,
                              Py68String **result);
Py68Status py68_string_get_char(struct Py68Runtime *runtime,
                                Py68String *string, Py68I32 index,
                                Py68String **result);
Py68Status py68_string_slice(struct Py68Runtime *runtime,
                             Py68String *string, Py68I32 start, Py68I32 end,
                             int start_omitted, int end_omitted,
                             Py68String **result);
/* Substring search; empty needle is always found (Python membership). */
int py68_string_contains(Py68String *haystack, Py68String *needle);

#endif
