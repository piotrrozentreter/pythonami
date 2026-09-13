/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_memory.h"
#include "py68k_runtime.h"

#include <stdio.h>
#include <string.h>

static int check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    Py68Allocator allocator;
    Py68Runtime runtime;
    void *first;
    void *second;
    int passed = 1;

    py68_allocator_initialize(&allocator);
    first = py68_alloc(&allocator, PY68_MEM_TEMP, 16);
    passed &= check(first != NULL, "initial allocation succeeds");
    passed &= check(allocator.stats.current_bytes == 16,
                    "current bytes track allocation");
    second = py68_realloc(&allocator, PY68_MEM_TEMP, first, 16, 32);
    passed &= check(second != NULL, "reallocation succeeds");
    passed &= check(allocator.stats.current_bytes == 32,
                    "current bytes track reallocation");
    py68_free(&allocator, PY68_MEM_TEMP, second, 32);
    passed &= check(allocator.stats.current_bytes == 0,
                    "free returns current bytes to zero");

    py68_allocator_initialize(&allocator);
    allocator.fail_after_allocation = 1;
    first = py68_alloc(&allocator, PY68_MEM_TEMP, 8);
    passed &= check(first != NULL, "failure sweep first allocation succeeds");
    second = py68_alloc(&allocator, PY68_MEM_TEMP, 8);
    passed &= check(second == NULL, "injected allocation failure is controlled");
    py68_free(&allocator, PY68_MEM_TEMP, first, 8);
    passed &= check(allocator.stats.failed_count == 1,
                    "failed allocation is counted");
    passed &= check(allocator.stats.current_bytes == 0,
                    "failure cleanup leaves zero bytes");

    passed &= check(py68_runtime_initialize(&runtime) == PY68_STATUS_OK,
                    "runtime initializes");
    passed &= check(runtime.value_stack_count == 0,
                    "value stack starts empty");
    passed &= check(runtime.frame_count == 0, "frame stack starts empty");
    passed &= check(runtime.live_objects == NULL, "live object list starts empty");
    py68_runtime_shutdown(&runtime);
    passed &= check(runtime.value_stack_count == 0,
                    "shutdown leaves value stack empty");
    passed &= check(runtime.frame_count == 0,
                    "shutdown leaves frame stack empty");
    passed &= check(runtime.live_objects == NULL,
                    "shutdown leaves live object list empty");
    passed &= check(runtime.allocator.stats.current_bytes == 0,
                    "shutdown leaves allocator empty");

    if (passed) {
        puts("PASS: memory and runtime bootstrap tests");
        return 0;
    }
    return 1;
}
