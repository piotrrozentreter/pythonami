/* 2026 by Piotr Rozentreter (Rozsoft) */

/*
 * demo_add.py68k — sample LoadSeg extension for pythonami.
 *
 * Exports:
 *   add(a, b)  — C implementation
 *   mul(a, b)  — assembler implementation (demo_add_asm.s)
 *
 * The header is emitted in assembler as the first bytes of the first CODE
 * hunk so LoadSeg + BADDR(seg)+4 finds Py68ExtHeader immediately.
 */

#include "py68k_ext.h"

#include <stddef.h>

Py68Status demo_mul(struct Py68Runtime *runtime, Py68U16 argument_count,
                    Py68Value *arguments, Py68Value *result);

Py68Status demo_add(struct Py68Runtime *runtime, Py68U16 argument_count,
                    Py68Value *arguments, Py68Value *result)
{
    Py68I32 left;
    Py68I32 right;
    (void)runtime;
    (void)argument_count;
    if (arguments[0].type != PY68_VALUE_INT &&
        arguments[0].type != PY68_VALUE_BOOL)
        return PY68_STATUS_RUNTIME_ERROR;
    if (arguments[1].type != PY68_VALUE_INT &&
        arguments[1].type != PY68_VALUE_BOOL)
        return PY68_STATUS_RUNTIME_ERROR;
    left = arguments[0].as.integer;
    right = arguments[1].as.integer;
    *result = py68_ext_value_int(left + right);
    return PY68_STATUS_OK;
}

/* Export table consumed by demo_add_header.s */
const Py68ExtExport demo_add_exports[2] = {
    { "add", 2, 2, demo_add },
    { "mul", 2, 2, demo_mul }
};
