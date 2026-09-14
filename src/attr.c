/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_attr.h"
#include "py68k_dict.h"
#include "py68k_exception.h"
#include "py68k_file.h"
#include "py68k_list.h"
#include "py68k_module.h"
#include "py68k_native.h"
#include "py68k_runtime.h"
#include "py68k_set.h"
#include "py68k_string.h"

#include <stddef.h>
#include <string.h>

Py68Status py68_bound_method_new(Py68Runtime *runtime, Py68Value self,
                                 Py68NativeFunction *function,
                                 Py68BoundMethod **result)
{
    Py68BoundMethod *method = (Py68BoundMethod *)py68_alloc(
        &runtime->allocator, PY68_MEM_FUNCTION, sizeof(Py68BoundMethod));
    if (method == NULL) return PY68_STATUS_MEMORY_ERROR;
    method->base.type = PY68_OBJECT_BOUND_METHOD;
    method->base.flags = 0;
    method->base.reference_count = 1;
    method->base.next_object = runtime->live_objects;
    runtime->live_objects = &method->base;
    method->self = self;
    py68_value_retain(self);
    method->function = function;
    py68_object_retain(&function->base);
    *result = method;
    return PY68_STATUS_OK;
}

static Py68Status py68_attr_error(Py68Runtime *runtime, Py68ErrorKind kind,
                                  const char *message)
{
    Py68Location location;
    location.offset = 0;
    location.line = 0;
    location.column = 0;
    location.length = 0;
    py68_error_set(&runtime->error, kind, location, NULL, message);
    return PY68_STATUS_RUNTIME_ERROR;
}

static Py68Status py68_attr_list_append(Py68Runtime *runtime,
                                        Py68U16 argument_count,
                                        Py68Value *arguments,
                                        Py68Value *result)
{
    if (argument_count != 2 || arguments[0].type != PY68_VALUE_OBJECT ||
        arguments[0].as.object == NULL ||
        arguments[0].as.object->type != PY68_OBJECT_LIST)
        return py68_attr_error(runtime, PY68_ERROR_TYPE,
                               "append expects a list");
    if (py68_list_append_copy(runtime, (Py68List *)arguments[0].as.object,
                              arguments[1]) != PY68_STATUS_OK)
        return py68_attr_error(runtime, PY68_ERROR_VALUE,
                               "cyclic containers are not supported");
    *result = py68_value_none();
    return PY68_STATUS_OK;
}

static Py68Status py68_attr_list_pop(Py68Runtime *runtime,
                                     Py68U16 argument_count,
                                     Py68Value *arguments, Py68Value *result)
{
    Py68List *list;
    if (argument_count != 1 || arguments[0].type != PY68_VALUE_OBJECT ||
        arguments[0].as.object == NULL ||
        arguments[0].as.object->type != PY68_OBJECT_LIST)
        return py68_attr_error(runtime, PY68_ERROR_TYPE, "pop expects a list");
    list = (Py68List *)arguments[0].as.object;
    if (list->count == 0)
        return py68_attr_error(runtime, PY68_ERROR_INDEX, "pop from empty list");
    *result = list->items[list->count - 1];
    --list->count;
    return PY68_STATUS_OK;
}

static Py68Status py68_attr_dict_get(Py68Runtime *runtime,
                                     Py68U16 argument_count,
                                     Py68Value *arguments, Py68Value *result)
{
    Py68Status status;
    if (argument_count < 2 || argument_count > 3 ||
        arguments[0].type != PY68_VALUE_OBJECT ||
        arguments[0].as.object == NULL ||
        arguments[0].as.object->type != PY68_OBJECT_DICT)
        return py68_attr_error(runtime, PY68_ERROR_TYPE, "get expects a dict");
    status = py68_dict_get_copy(runtime, (Py68Dict *)arguments[0].as.object,
                                arguments[1], result);
    if (status == PY68_STATUS_OK) return PY68_STATUS_OK;
    if (argument_count == 3) {
        *result = arguments[2];
        py68_value_retain(*result);
        return PY68_STATUS_OK;
    }
    *result = py68_value_none();
    return PY68_STATUS_OK;
}

static Py68Status py68_attr_dict_pop(Py68Runtime *runtime,
                                     Py68U16 argument_count,
                                     Py68Value *arguments, Py68Value *result)
{
    if (argument_count != 2 || arguments[0].type != PY68_VALUE_OBJECT ||
        arguments[0].as.object == NULL ||
        arguments[0].as.object->type != PY68_OBJECT_DICT)
        return py68_attr_error(runtime, PY68_ERROR_TYPE, "pop expects a dict");
    if (py68_dict_pop(runtime, (Py68Dict *)arguments[0].as.object,
                      arguments[1], result) != PY68_STATUS_OK)
        return py68_attr_error(runtime, PY68_ERROR_KEY, "key not found");
    return PY68_STATUS_OK;
}

static Py68Status py68_attr_dict_keys(Py68Runtime *runtime,
                                      Py68U16 argument_count,
                                      Py68Value *arguments, Py68Value *result)
{
    Py68List *list;
    if (argument_count != 1 || arguments[0].as.object->type != PY68_OBJECT_DICT)
        return py68_attr_error(runtime, PY68_ERROR_TYPE, "keys expects a dict");
    if (py68_dict_keys(runtime, (Py68Dict *)arguments[0].as.object, &list) !=
        PY68_STATUS_OK)
        return PY68_STATUS_MEMORY_ERROR;
    *result = py68_value_from_object(&list->base);
    return PY68_STATUS_OK;
}

static Py68Status py68_attr_dict_values(Py68Runtime *runtime,
                                        Py68U16 argument_count,
                                        Py68Value *arguments,
                                        Py68Value *result)
{
    Py68List *list;
    if (argument_count != 1 || arguments[0].as.object->type != PY68_OBJECT_DICT)
        return py68_attr_error(runtime, PY68_ERROR_TYPE,
                               "values expects a dict");
    if (py68_dict_values(runtime, (Py68Dict *)arguments[0].as.object, &list) !=
        PY68_STATUS_OK)
        return PY68_STATUS_MEMORY_ERROR;
    *result = py68_value_from_object(&list->base);
    return PY68_STATUS_OK;
}

static Py68Status py68_attr_dict_items(Py68Runtime *runtime,
                                       Py68U16 argument_count,
                                       Py68Value *arguments, Py68Value *result)
{
    Py68List *list;
    if (argument_count != 1 || arguments[0].as.object->type != PY68_OBJECT_DICT)
        return py68_attr_error(runtime, PY68_ERROR_TYPE,
                               "items expects a dict");
    if (py68_dict_items(runtime, (Py68Dict *)arguments[0].as.object, &list) !=
        PY68_STATUS_OK)
        return PY68_STATUS_MEMORY_ERROR;
    *result = py68_value_from_object(&list->base);
    return PY68_STATUS_OK;
}

static Py68Status py68_attr_set_add(Py68Runtime *runtime,
                                    Py68U16 argument_count,
                                    Py68Value *arguments, Py68Value *result)
{
    if (argument_count != 2 || arguments[0].as.object->type != PY68_OBJECT_SET)
        return py68_attr_error(runtime, PY68_ERROR_TYPE, "add expects a set");
    if (py68_set_add(runtime, (Py68Set *)arguments[0].as.object,
                     arguments[1]) != PY68_STATUS_OK)
        return py68_attr_error(runtime, PY68_ERROR_TYPE, "unhashable type");
    *result = py68_value_none();
    return PY68_STATUS_OK;
}

static Py68Status py68_attr_set_remove(Py68Runtime *runtime,
                                       Py68U16 argument_count,
                                       Py68Value *arguments, Py68Value *result)
{
    if (argument_count != 2 || arguments[0].as.object->type != PY68_OBJECT_SET)
        return py68_attr_error(runtime, PY68_ERROR_TYPE,
                               "remove expects a set");
    if (py68_set_remove(runtime, (Py68Set *)arguments[0].as.object,
                        arguments[1]) != PY68_STATUS_OK)
        return py68_attr_error(runtime, PY68_ERROR_KEY, "set remove missing");
    *result = py68_value_none();
    return PY68_STATUS_OK;
}

static Py68Status py68_attr_set_discard(Py68Runtime *runtime,
                                        Py68U16 argument_count,
                                        Py68Value *arguments,
                                        Py68Value *result)
{
    if (argument_count != 2 || arguments[0].as.object->type != PY68_OBJECT_SET)
        return py68_attr_error(runtime, PY68_ERROR_TYPE,
                               "discard expects a set");
    if (py68_set_discard(runtime, (Py68Set *)arguments[0].as.object,
                         arguments[1]) != PY68_STATUS_OK)
        return py68_attr_error(runtime, PY68_ERROR_TYPE, "unhashable type");
    *result = py68_value_none();
    return PY68_STATUS_OK;
}

static Py68Status py68_attr_file_enter(Py68Runtime *runtime,
                                       Py68U16 argument_count,
                                       Py68Value *arguments, Py68Value *result)
{
    if (argument_count != 1 || arguments[0].as.object->type != PY68_OBJECT_FILE)
        return py68_attr_error(runtime, PY68_ERROR_TYPE,
                               "__enter__ expects a file");
    *result = arguments[0];
    py68_value_retain(*result);
    return PY68_STATUS_OK;
}

static Py68Status py68_attr_file_exit(Py68Runtime *runtime,
                                      Py68U16 argument_count,
                                      Py68Value *arguments, Py68Value *result)
{
    if (argument_count < 1 || arguments[0].as.object->type != PY68_OBJECT_FILE)
        return py68_attr_error(runtime, PY68_ERROR_TYPE,
                               "__exit__ expects a file");
    py68_file_close(runtime, (Py68File *)arguments[0].as.object);
    *result = py68_value_none();
    return PY68_STATUS_OK;
}

static Py68Status py68_attr_bind(Py68Runtime *runtime, Py68Value self,
                                 const char *name, Py68U16 min_args,
                                 Py68U16 max_args, Py68NativeCallback callback,
                                 Py68Value *result)
{
    Py68NativeFunction *function;
    Py68BoundMethod *method;
    Py68Status status = py68_native_new(runtime, name, min_args, max_args,
                                        callback, &function);
    if (status != PY68_STATUS_OK) return status;
    status = py68_bound_method_new(runtime, self, function, &method);
    py68_object_release(runtime, &function->base);
    if (status != PY68_STATUS_OK) return status;
    *result = py68_value_from_object(&method->base);
    return PY68_STATUS_OK;
}

static int py68_attr_name_is(const Py68U8 *name, Py68U16 length,
                             const char *expected)
{
    Py68U16 index = 0;
    while (expected[index] != '\0') ++index;
    return length == index && memcmp(name, expected, length) == 0;
}

Py68Status py68_attr_load(Py68Runtime *runtime, Py68Value object,
                          const Py68U8 *name, Py68U16 name_length,
                          Py68Value *result)
{
    Py68U16 type;
    if (object.type != PY68_VALUE_OBJECT || object.as.object == NULL)
        return py68_attr_error(runtime, PY68_ERROR_TYPE,
                               "attribute access requires an object");
    type = object.as.object->type;
    if (type == PY68_OBJECT_MODULE)
        return py68_module_get(runtime, (Py68Module *)object.as.object, name,
                               name_length, result) == PY68_STATUS_OK
               ? PY68_STATUS_OK
               : py68_attr_error(runtime, PY68_ERROR_NAME,
                                 "module has no such attribute");
    if (type == PY68_OBJECT_LIST) {
        if (py68_attr_name_is(name, name_length, "append"))
            return py68_attr_bind(runtime, object, "append", 2, 2,
                                  py68_attr_list_append, result);
        if (py68_attr_name_is(name, name_length, "pop"))
            return py68_attr_bind(runtime, object, "pop", 1, 1,
                                  py68_attr_list_pop, result);
    } else if (type == PY68_OBJECT_DICT) {
        if (py68_attr_name_is(name, name_length, "get"))
            return py68_attr_bind(runtime, object, "get", 2, 3,
                                  py68_attr_dict_get, result);
        if (py68_attr_name_is(name, name_length, "pop"))
            return py68_attr_bind(runtime, object, "pop", 2, 2,
                                  py68_attr_dict_pop, result);
        if (py68_attr_name_is(name, name_length, "keys"))
            return py68_attr_bind(runtime, object, "keys", 1, 1,
                                  py68_attr_dict_keys, result);
        if (py68_attr_name_is(name, name_length, "values"))
            return py68_attr_bind(runtime, object, "values", 1, 1,
                                  py68_attr_dict_values, result);
        if (py68_attr_name_is(name, name_length, "items"))
            return py68_attr_bind(runtime, object, "items", 1, 1,
                                  py68_attr_dict_items, result);
    } else if (type == PY68_OBJECT_SET) {
        if (py68_attr_name_is(name, name_length, "add"))
            return py68_attr_bind(runtime, object, "add", 2, 2,
                                  py68_attr_set_add, result);
        if (py68_attr_name_is(name, name_length, "remove"))
            return py68_attr_bind(runtime, object, "remove", 2, 2,
                                  py68_attr_set_remove, result);
        if (py68_attr_name_is(name, name_length, "discard"))
            return py68_attr_bind(runtime, object, "discard", 2, 2,
                                  py68_attr_set_discard, result);
    } else if (type == PY68_OBJECT_FILE) {
        if (py68_attr_name_is(name, name_length, "__enter__"))
            return py68_attr_bind(runtime, object, "__enter__", 1, 1,
                                  py68_attr_file_enter, result);
        if (py68_attr_name_is(name, name_length, "__exit__"))
            return py68_attr_bind(runtime, object, "__exit__", 1, 4,
                                  py68_attr_file_exit, result);
    }
    return py68_attr_error(runtime, PY68_ERROR_TYPE, "unknown attribute");
}

Py68Status py68_attr_store(Py68Runtime *runtime, Py68Value object,
                           const Py68U8 *name, Py68U16 name_length,
                           Py68Value value)
{
    if (object.type != PY68_VALUE_OBJECT || object.as.object == NULL ||
        object.as.object->type != PY68_OBJECT_MODULE)
        return py68_attr_error(runtime, PY68_ERROR_TYPE,
                               "attribute assignment requires a module");
    return py68_module_set(runtime, (Py68Module *)object.as.object, name,
                           name_length, value);
}
