#include "py68k_platform.h"
#include "py68k_runtime.h"

#include <stdio.h>

Py68Status py68_platform_initialize(Py68Runtime *runtime)
{
    (void)runtime;
    return PY68_STATUS_OK;
}

void py68_platform_shutdown(Py68Runtime *runtime)
{
    (void)runtime;
}

static Py68Status py68_platform_write(FILE *stream, const char *data,
                                       Py68U32 length)
{
    if (length != 0 && fwrite(data, 1, (size_t)length, stream) != length) {
        return PY68_STATUS_RUNTIME_ERROR;
    }
    return PY68_STATUS_OK;
}

Py68Status py68_platform_write_stdout(Py68Runtime *runtime,
                                      const char *data, Py68U32 length)
{
    (void)runtime;
    return py68_platform_write(stdout, data, length);
}

Py68Status py68_platform_write_stderr(Py68Runtime *runtime,
                                      const char *data, Py68U32 length)
{
    (void)runtime;
    return py68_platform_write(stderr, data, length);
}
