#include "py68k_error.h"

#include <string.h>

void py68_error_clear(Py68Error *error)
{
    error->kind = PY68_ERROR_NONE;
    error->active = 0;
    error->location.offset = 0;
    error->location.line = 0;
    error->location.column = 0;
    error->location.length = 0;
    error->filename = NULL;
    error->message[0] = '\0';
}

void py68_error_set(Py68Error *error, Py68ErrorKind kind,
                    Py68Location location, const char *filename,
                    const char *message)
{
    if (error->active != 0) {
        return;
    }
    error->kind = (Py68U16)kind;
    error->active = 1;
    error->location = location;
    error->filename = filename;
    strncpy(error->message, message, sizeof(error->message) - 1);
    error->message[sizeof(error->message) - 1] = '\0';
}