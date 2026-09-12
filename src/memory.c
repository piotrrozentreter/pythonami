#include "py68k_memory.h"

#include <stdlib.h>

static int py68_size_add_overflows(Py68U32 left, Py68U32 right)
{
    return right > (Py68U32)~(Py68U32)0 - left;
}

void py68_allocator_initialize(Py68Allocator *allocator)
{
    Py68U16 index;
    allocator->memory_limit = 0;
    allocator->fail_after_allocation = 0;
    allocator->stats.current_bytes = 0;
    allocator->stats.peak_bytes = 0;
    allocator->stats.allocation_count = 0;
    allocator->stats.free_count = 0;
    allocator->stats.failed_count = 0;
    for (index = 0; index < PY68_MEM_TAG_COUNT; ++index) {
        allocator->stats.bytes_by_tag[index] = 0;
    }
}

void *py68_alloc(Py68Allocator *allocator, Py68MemoryTag tag, Py68U32 size)
{
    void *memory;
    Py68U32 next_bytes;

    if (size == 0 || tag >= PY68_MEM_TAG_COUNT ||
        py68_size_add_overflows(allocator->stats.current_bytes, size) ||
        (allocator->memory_limit != 0 &&
         size > allocator->memory_limit - allocator->stats.current_bytes) ||
        (allocator->fail_after_allocation != 0 &&
         allocator->stats.allocation_count >= allocator->fail_after_allocation)) {
        ++allocator->stats.failed_count;
        return NULL;
    }

    memory = malloc((size_t)size);
    if (memory == NULL) {
        ++allocator->stats.failed_count;
        return NULL;
    }

    next_bytes = allocator->stats.current_bytes + size;
    allocator->stats.current_bytes = next_bytes;
    allocator->stats.bytes_by_tag[tag] += size;
    if (next_bytes > allocator->stats.peak_bytes) {
        allocator->stats.peak_bytes = next_bytes;
    }
    ++allocator->stats.allocation_count;
    return memory;
}

void *py68_realloc(Py68Allocator *allocator, Py68MemoryTag tag,
                   void *memory, Py68U32 old_size, Py68U32 new_size)
{
    void *replacement;
    Py68U32 retained_bytes;

    if (memory == NULL) {
        return py68_alloc(allocator, tag, new_size);
    }
    if (tag >= PY68_MEM_TAG_COUNT || old_size > allocator->stats.current_bytes ||
        old_size > allocator->stats.bytes_by_tag[tag]) {
        ++allocator->stats.failed_count;
        return NULL;
    }
    retained_bytes = allocator->stats.current_bytes - old_size;
    if (new_size == 0 || py68_size_add_overflows(retained_bytes, new_size) ||
        (allocator->memory_limit != 0 &&
         new_size > allocator->memory_limit - retained_bytes) ||
        (allocator->fail_after_allocation != 0 &&
         allocator->stats.allocation_count >= allocator->fail_after_allocation)) {
        ++allocator->stats.failed_count;
        return NULL;
    }

    replacement = realloc(memory, (size_t)new_size);
    if (replacement == NULL) {
        ++allocator->stats.failed_count;
        return NULL;
    }
    allocator->stats.current_bytes = retained_bytes + new_size;
    allocator->stats.bytes_by_tag[tag] -= old_size;
    allocator->stats.bytes_by_tag[tag] += new_size;
    if (allocator->stats.current_bytes > allocator->stats.peak_bytes) {
        allocator->stats.peak_bytes = allocator->stats.current_bytes;
    }
    ++allocator->stats.allocation_count;
    return replacement;
}

void py68_free(Py68Allocator *allocator, Py68MemoryTag tag,
               void *memory, Py68U32 size)
{
    if (memory == NULL) {
        return;
    }
    free(memory);
    if (tag < PY68_MEM_TAG_COUNT && size <= allocator->stats.bytes_by_tag[tag]) {
        allocator->stats.bytes_by_tag[tag] -= size;
    }
    if (size <= allocator->stats.current_bytes) {
        allocator->stats.current_bytes -= size;
    }
    ++allocator->stats.free_count;
}
