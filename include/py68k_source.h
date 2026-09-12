#ifndef PY68K_SOURCE_H
#define PY68K_SOURCE_H

#include "py68k_memory.h"
#include "py68k_status.h"
#include "py68k_types.h"

typedef struct Py68Source {
    char *filename;
    Py68U8 *data;
    Py68U32 length;
} Py68Source;

Py68Status py68_source_initialize(Py68Allocator *allocator,
                                   Py68Source *source,
                                   const char *filename,
                                   const Py68U8 *data,
                                   Py68U32 length);
void py68_source_destroy(Py68Allocator *allocator, Py68Source *source);

#endif