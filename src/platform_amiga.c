/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_platform.h"
#include "py68k_runtime.h"

#include <proto/dos.h>

Py68Status py68_platform_initialize(Py68Runtime *runtime)
{
    runtime->trace_enabled = 0;
    return PY68_STATUS_OK;
}

void py68_platform_shutdown(Py68Runtime *runtime)
{
    runtime->trace_enabled = 0;
}

static Py68Status py68_platform_write(BPTR handle, const char *data,
                                       Py68U32 length)
{
    LONG written;
    if (length > 2147483647UL) return PY68_STATUS_RUNTIME_ERROR;
    written = Write(handle, (CONST_APTR)data, (LONG)length);
    if (written != (LONG)length) {
        return PY68_STATUS_RUNTIME_ERROR;
    }
    return PY68_STATUS_OK;
}

Py68Status py68_platform_write_stdout(Py68Runtime *runtime,
                                      const char *data, Py68U32 length)
{
    Py68Status status = py68_platform_write(Output(), data, length);
    if (status != PY68_STATUS_OK) {
        runtime->error.active = 1;
    }
    return status;
}

Py68Status py68_platform_write_stderr(Py68Runtime *runtime,
                                      const char *data, Py68U32 length)
{
    Py68Status status = py68_platform_write(ErrorOutput(), data, length);
    if (status != PY68_STATUS_OK) {
        runtime->error.active = 1;
    }
    return status;
}
