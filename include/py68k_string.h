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
Py68U32 py68_string_hash_bytes(const char *data, Py68U32 length);

#endif
