/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_MEMORY_H
#define PY68K_MEMORY_H

#include "py68k_types.h"

typedef enum Py68MemoryTag {
    PY68_MEM_RUNTIME = 0,
    PY68_MEM_SOURCE,
    PY68_MEM_TOKEN,
    PY68_MEM_AST,
    PY68_MEM_SYMBOL,
    PY68_MEM_CODE,
    PY68_MEM_CONSTANT,
    PY68_MEM_STRING,
    PY68_MEM_LIST,
    PY68_MEM_TUPLE,
    PY68_MEM_DICT,
    PY68_MEM_SET,
    PY68_MEM_MODULE,
    PY68_MEM_FUNCTION,
    PY68_MEM_STACK,
    PY68_MEM_TEMP,
    PY68_MEM_TAG_COUNT
} Py68MemoryTag;

typedef struct Py68MemoryStats {
    Py68U32 current_bytes;
    Py68U32 peak_bytes;
    Py68U32 allocation_count;
    Py68U32 free_count;
    Py68U32 failed_count;
    Py68U32 bytes_by_tag[PY68_MEM_TAG_COUNT];
} Py68MemoryStats;

typedef struct Py68Allocator {
    Py68U32 memory_limit;
    Py68U32 fail_after_allocation;
    Py68MemoryStats stats;
} Py68Allocator;

void py68_allocator_initialize(Py68Allocator *allocator);
void *py68_alloc(Py68Allocator *allocator, Py68MemoryTag tag, Py68U32 size);
void *py68_realloc(Py68Allocator *allocator, Py68MemoryTag tag,
                   void *memory, Py68U32 old_size, Py68U32 new_size);
void py68_free(Py68Allocator *allocator, Py68MemoryTag tag,
               void *memory, Py68U32 size);

#endif
