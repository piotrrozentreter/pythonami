#include "py68k_value.h"
#include "py68k_object.h"
#include "py68k_runtime.h"

Py68Value py68_value_none(void)
{
    Py68Value value;
    value.type = PY68_VALUE_NONE;
    value.reserved = 0;
    value.as.integer = 0;
    return value;
}

Py68Value py68_value_unbound(void)
{
    Py68Value value = py68_value_none();
    value.type = PY68_VALUE_UNBOUND;
    return value;
}

Py68Value py68_value_bool(int truth)
{
    Py68Value value = py68_value_none();
    value.type = PY68_VALUE_BOOL;
    value.as.integer = truth != 0;
    return value;
}

Py68Value py68_value_int(Py68I32 integer)
{
    Py68Value value = py68_value_none();
    value.type = PY68_VALUE_INT;
    value.as.integer = integer;
    return value;
}

Py68Value py68_value_from_object(Py68Object *object)
{
    Py68Value value = py68_value_none();
    value.type = PY68_VALUE_OBJECT;
    value.as.object = object;
    return value;
}

void py68_value_retain(Py68Value value)
{
    if (value.type == PY68_VALUE_OBJECT) py68_object_retain(value.as.object);
}

void py68_value_release(Py68Runtime *runtime, Py68Value value)
{
    if (value.type == PY68_VALUE_OBJECT) {
        py68_object_release(runtime, value.as.object);
    }
}
