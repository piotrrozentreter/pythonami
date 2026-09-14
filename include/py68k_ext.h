/* 2026 by Piotr Rozentreter (Rozsoft) */

/*
 * Public LoadSeg extension ABI for Amiga pythonami plugins (*.py68k).
 *
 * After LoadSeg(path), the first hunk is laid out as:
 *   ULONG next_seg;          // AmigaDOS seglist link (do not touch)
 *   Py68ExtHeader header;    // must begin immediately after next_seg
 *
 * Extension authors compile against this header (and py68k_types /
 * py68k_status / py68k_value) but do not link pythonami. Construct scalar
 * results with the helpers below. Arguments are borrowed; return one owned
 * Py68Value. Keep the library module reachable while calling exports.
 */

#ifndef PY68K_EXT_H
#define PY68K_EXT_H

#include "py68k_status.h"
#include "py68k_types.h"
#include "py68k_value.h"

#define PY68_EXT_MAGIC 0x50593638UL /* 'PY68' */
#define PY68_EXT_ABI_VERSION 1

struct Py68Runtime;

typedef Py68Status (*Py68ExtCallback)(struct Py68Runtime *runtime,
                                      Py68U16 argument_count,
                                      Py68Value *arguments,
                                      Py68Value *result);

typedef struct Py68ExtExport {
    const char *name;
    Py68U16 min_args;
    Py68U16 max_args;
    Py68ExtCallback function;
} Py68ExtExport;

typedef struct Py68ExtHeader {
    Py68U32 magic;
    Py68U16 abi_version;
    Py68U16 export_count;
    const Py68ExtExport *exports;
} Py68ExtHeader;

/* Scalar result helpers (C89; safe to copy into extension TUs). */
static Py68Value py68_ext_value_none(void)
{
    Py68Value value;
    value.type = PY68_VALUE_NONE;
    value.reserved = 0;
    value.as.integer = 0;
    return value;
}

static Py68Value py68_ext_value_bool(int truth)
{
    Py68Value value;
    value.type = PY68_VALUE_BOOL;
    value.reserved = 0;
    value.as.integer = truth != 0;
    return value;
}

static Py68Value py68_ext_value_int(Py68I32 integer)
{
    Py68Value value;
    value.type = PY68_VALUE_INT;
    value.reserved = 0;
    value.as.integer = integer;
    return value;
}

#endif
