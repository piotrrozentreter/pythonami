#include "py68k_code.h"
#include "py68k_memory.h"

#include <stdio.h>

int main(void)
{
    Py68Allocator allocator;
    Py68Code parent;
    Py68Code *nested;
    Py68Constant constant;
    Py68U16 index;
    int passed = 1;

    py68_allocator_initialize(&allocator);
    py68_code_initialize(&parent);
    nested = (Py68Code *)py68_alloc(&allocator, PY68_MEM_CODE,
                                    sizeof(Py68Code));
    passed &= nested != NULL;
    if (nested != NULL) {
        py68_code_initialize(nested);
        passed &= py68_code_emit_u8(&allocator, nested, OP_RETURN_NONE) ==
                  PY68_STATUS_OK;
        passed &= py68_code_add_code_move(&allocator, &parent, nested,
                                          &index) == PY68_STATUS_OK;
        passed &= parent.constant_count == 1 &&
                  parent.constants[index].kind == PY68_CONSTANT_CODE &&
                  parent.constants[index].code == nested;
    }
    constant.kind = PY68_CONSTANT_INTEGER;
    constant.flags = 0;
    constant.integer = 1;
    constant.offset = 0;
    constant.length = 0;
    constant.code = NULL;
    passed &= py68_code_add_constant(&allocator, &parent, constant,
                                     &index) == PY68_STATUS_OK;
    py68_code_destroy(&allocator, &parent);
    passed &= allocator.stats.current_bytes == 0;
    if (passed) { puts("PASS: code ownership tests"); return 0; }
    return 1;
}