#ifndef PY68K_LOCATION_H
#define PY68K_LOCATION_H

#include "py68k_types.h"

typedef struct Py68Location {
    Py68U32 offset;
    Py68U32 line;
    Py68U16 column;
    Py68U16 length;
} Py68Location;

#endif
