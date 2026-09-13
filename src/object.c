#include "py68k_object.h"
#include "py68k_runtime.h"
#include "py68k_list.h"
#include "py68k_string.h"
#include "py68k_function.h"
#include "py68k_native.h"
#include "py68k_range.h"

#include <stddef.h>

void py68_object_retain(Py68Object *object)
{
    if (object == NULL) return;
    if (object->reference_count != (Py68U32)~(Py68U32)0) {
        ++object->reference_count;
    }
}

static void py68_object_unlink(Py68Runtime *runtime, Py68Object *object)
{
    Py68Object **cursor = &runtime->live_objects;
    while (*cursor != NULL) {
        if (*cursor == object) {
            *cursor = object->next_object;
            return;
        }
        cursor = &(*cursor)->next_object;
    }
}

void py68_object_release(Py68Runtime *runtime, Py68Object *object)
{
    if (object == NULL || object->reference_count == 0) return;
    --object->reference_count;
    if (object->reference_count != 0) return;
    py68_object_unlink(runtime, object);
    if (object->type == PY68_OBJECT_STRING) {
        Py68String *string = (Py68String *)object;
        py68_free(&runtime->allocator, PY68_MEM_STRING, string,
                  (Py68U32)sizeof(Py68String) + string->length);
    } else if (object->type == PY68_OBJECT_LIST) {
        Py68List *list = (Py68List *)object;
        Py68U32 index;
        for (index = 0; index < list->count; ++index)
            py68_value_release(runtime, list->items[index]);
        py68_free(&runtime->allocator, PY68_MEM_LIST, list->items,
                  list->capacity * sizeof(Py68Value));
        py68_free(&runtime->allocator, PY68_MEM_LIST, list,
                  sizeof(Py68List));
    } else if (object->type == PY68_OBJECT_FUNCTION) {
        Py68Function *function = (Py68Function *)object;
        if ((function->base.flags & 1) != 0 && function->code != NULL) {
            py68_code_destroy(&runtime->allocator, function->code);
            py68_free(&runtime->allocator, PY68_MEM_CODE,
                      function->code, sizeof(Py68Code));
        }
        py68_free(&runtime->allocator, PY68_MEM_FUNCTION, function,
                  sizeof(Py68Function));
    } else if (object->type == PY68_OBJECT_NATIVE_FUNCTION) {
        Py68NativeFunction *function = (Py68NativeFunction *)object;
        py68_free(&runtime->allocator, PY68_MEM_FUNCTION, function,
                  sizeof(Py68NativeFunction));
    } else if (object->type == PY68_OBJECT_RANGE) {
        Py68Range *range = (Py68Range *)object;
        if (range->list != NULL) {
            py68_object_release(runtime, &range->list->base);
        }
        py68_free(&runtime->allocator, PY68_MEM_RUNTIME, range,
                  sizeof(Py68Range));
    } else {
        py68_free(&runtime->allocator, PY68_MEM_RUNTIME, object,
                  (Py68U32)sizeof(Py68Object));
    }
}
