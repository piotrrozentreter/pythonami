#include "py68k_intern.h"
#include "py68k_runtime.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    static const char *names[] = {
        "name0", "name1", "name2", "name3", "name4", "name5",
        "name6", "name7", "name8", "name9", "name10", "name11"
    };
    Py68Runtime runtime;
    Py68InternTable table;
    Py68String *first;
    Py68String *same;
    Py68String *strings[12];
    Py68U16 index;
    int passed = 1;

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    py68_intern_initialize(&table);
    passed &= py68_intern_get_copy(&runtime, &table, "answer", 6,
                                   &first) == PY68_STATUS_OK;
    passed &= py68_intern_get_copy(&runtime, &table, "answer", 6,
                                   &same) == PY68_STATUS_OK;
    passed &= first == same;
    passed &= py68_string_hash_bytes("", 0) == 2166136261UL;
    passed &= py68_string_hash_bytes("a", 1) == 3826002220UL;
    passed &= first->hash == py68_string_hash_bytes("answer", 6);
    passed &= first->base.reference_count == 3;
    py68_object_release(&runtime, &first->base);
    py68_object_release(&runtime, &same->base);

    for (index = 0; index < 12; ++index) {
        passed &= py68_intern_get_copy(&runtime, &table, names[index],
                                       (Py68U32)strlen(names[index]),
                                       &strings[index]) == PY68_STATUS_OK;
        py68_object_release(&runtime, &strings[index]->base);
    }
    passed &= table.capacity >= 16 && table.count == 13;
    py68_intern_destroy(&runtime, &table);
    passed &= runtime.live_objects == NULL;
    passed &= runtime.allocator.stats.current_bytes == 0;
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    if (passed) { puts("PASS: intern table tests"); return 0; }
    return 1;
}