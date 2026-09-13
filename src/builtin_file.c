/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_builtin.h"
#include "py68k_file.h"
#include "py68k_platform.h"
#include "py68k_string.h"

#include <string.h>

static void py68_file_builtin_error(Py68Runtime *runtime, Py68ErrorKind kind,
                                    const char *message)
{
    Py68Location location;
    location.offset = 0;
    location.line = 0;
    location.column = 0;
    location.length = 0;
    py68_error_set(&runtime->error, kind, location, NULL, message);
}

static int py68_require_string(Py68Value value, Py68String **out)
{
    if (value.type != PY68_VALUE_OBJECT || value.as.object == NULL ||
        value.as.object->type != PY68_OBJECT_STRING)
        return 0;
    *out = (Py68String *)value.as.object;
    return 1;
}

static int py68_require_file(Py68Value value, Py68File **out)
{
    if (value.type != PY68_VALUE_OBJECT || value.as.object == NULL ||
        value.as.object->type != PY68_OBJECT_FILE)
        return 0;
    *out = (Py68File *)value.as.object;
    return 1;
}

static Py68Status py68_cstring_from_string(Py68Runtime *runtime,
                                           Py68String *string, char **out)
{
    char *copy;
    copy = (char *)py68_alloc(&runtime->allocator, PY68_MEM_TEMP,
                              string->length + 1);
    if (copy == NULL) return PY68_STATUS_MEMORY_ERROR;
    if (string->length != 0)
        memcpy(copy, string->data, string->length);
    copy[string->length] = '\0';
    *out = copy;
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_fopen(Py68Runtime *runtime, Py68U16 argument_count,
                              Py68Value *arguments, Py68Value *result)
{
    Py68String *path;
    Py68String *mode;
    char *path_c;
    char *mode_c;
    Py68File *file;
    Py68Status status;

    if (argument_count != 2 || !py68_require_string(arguments[0], &path) ||
        !py68_require_string(arguments[1], &mode)) {
        py68_file_builtin_error(runtime, PY68_ERROR_TYPE,
                                "fopen expects path and mode strings");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    status = py68_cstring_from_string(runtime, path, &path_c);
    if (status != PY68_STATUS_OK) return status;
    status = py68_cstring_from_string(runtime, mode, &mode_c);
    if (status != PY68_STATUS_OK) {
        py68_free(&runtime->allocator, PY68_MEM_TEMP, path_c, path->length + 1);
        return status;
    }
    status = py68_file_open(runtime, path_c, mode_c, &file);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, path_c, path->length + 1);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, mode_c, mode->length + 1);
    if (status == PY68_STATUS_SOURCE_ERROR) {
        py68_file_builtin_error(runtime, PY68_ERROR_VALUE,
                                "fopen failed (bad mode or path)");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (status != PY68_STATUS_OK) {
        py68_file_builtin_error(runtime, PY68_ERROR_IO, "fopen failed");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    *result = py68_value_from_object(&file->base);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_fclose(Py68Runtime *runtime, Py68U16 argument_count,
                               Py68Value *arguments, Py68Value *result)
{
    Py68File *file;
    if (argument_count != 1 || !py68_require_file(arguments[0], &file)) {
        py68_file_builtin_error(runtime, PY68_ERROR_TYPE,
                                "fclose expects a file handle");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (py68_file_close(runtime, file) != PY68_STATUS_OK) {
        py68_file_builtin_error(runtime, PY68_ERROR_IO, "fclose failed");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    *result = py68_value_none();
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_fread(Py68Runtime *runtime, Py68U16 argument_count,
                              Py68Value *arguments, Py68Value *result)
{
    Py68File *file;
    Py68U8 *data;
    Py68U32 length;
    Py68String *string;
    Py68Status status;
    Py68I32 count;

    if (argument_count != 2 || !py68_require_file(arguments[0], &file) ||
        (arguments[1].type != PY68_VALUE_INT &&
         arguments[1].type != PY68_VALUE_BOOL)) {
        py68_file_builtin_error(runtime, PY68_ERROR_TYPE,
                                "fread expects file handle and count");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (file->closed || !file->readable) {
        py68_file_builtin_error(runtime, PY68_ERROR_IO,
                                "fread on closed or non-readable file");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    count = arguments[1].as.integer;
    if (count < 0) {
        py68_file_builtin_error(runtime, PY68_ERROR_VALUE,
                                "fread count must be non-negative");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    status = py68_platform_file_read(runtime, file->handle, (Py68U32)count,
                                     &data, &length);
    if (status != PY68_STATUS_OK) {
        py68_file_builtin_error(runtime, PY68_ERROR_IO, "fread failed");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    status = py68_string_new_copy(runtime, (const char *)data, length, &string);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, data, length + 1);
    if (status != PY68_STATUS_OK) return status;
    *result = py68_value_from_object(&string->base);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_freadline(Py68Runtime *runtime, Py68U16 argument_count,
                                  Py68Value *arguments, Py68Value *result)
{
    Py68File *file;
    Py68U8 *data;
    Py68U32 length;
    Py68String *string;
    Py68Status status;

    if (argument_count != 1 || !py68_require_file(arguments[0], &file)) {
        py68_file_builtin_error(runtime, PY68_ERROR_TYPE,
                                "freadline expects a file handle");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (file->closed || !file->readable) {
        py68_file_builtin_error(runtime, PY68_ERROR_IO,
                                "freadline on closed or non-readable file");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    status = py68_platform_file_readline(runtime, file->handle, &data, &length);
    if (status != PY68_STATUS_OK) {
        py68_file_builtin_error(runtime, PY68_ERROR_IO, "freadline failed");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    status = py68_string_new_copy(runtime, (const char *)data, length, &string);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, data, length + 1);
    if (status != PY68_STATUS_OK) return status;
    *result = py68_value_from_object(&string->base);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_fwrite(Py68Runtime *runtime, Py68U16 argument_count,
                               Py68Value *arguments, Py68Value *result)
{
    Py68File *file;
    Py68String *text;
    Py68U32 written;
    Py68Status status;

    if (argument_count != 2 || !py68_require_file(arguments[0], &file) ||
        !py68_require_string(arguments[1], &text)) {
        py68_file_builtin_error(runtime, PY68_ERROR_TYPE,
                                "fwrite expects file handle and string");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    if (file->closed || !file->writable) {
        py68_file_builtin_error(runtime, PY68_ERROR_IO,
                                "fwrite on closed or non-writable file");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    status = py68_platform_file_write(runtime, file->handle, text->data,
                                      text->length, &written);
    if (status != PY68_STATUS_OK) {
        py68_file_builtin_error(runtime, PY68_ERROR_IO, "fwrite failed");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    *result = py68_value_int((Py68I32)written);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_exists(Py68Runtime *runtime, Py68U16 argument_count,
                               Py68Value *arguments, Py68Value *result)
{
    Py68String *path;
    char *path_c;
    Py68Status status;
    int exists;
    (void)runtime;
    if (argument_count != 1 || !py68_require_string(arguments[0], &path)) {
        py68_file_builtin_error(runtime, PY68_ERROR_TYPE,
                                "exists expects a path string");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    status = py68_cstring_from_string(runtime, path, &path_c);
    if (status != PY68_STATUS_OK) return status;
    exists = py68_platform_path_exists(path_c);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, path_c, path->length + 1);
    *result = py68_value_bool(exists);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_remove(Py68Runtime *runtime, Py68U16 argument_count,
                               Py68Value *arguments, Py68Value *result)
{
    Py68String *path;
    char *path_c;
    Py68Status status;
    if (argument_count != 1 || !py68_require_string(arguments[0], &path)) {
        py68_file_builtin_error(runtime, PY68_ERROR_TYPE,
                                "remove expects a path string");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    status = py68_cstring_from_string(runtime, path, &path_c);
    if (status != PY68_STATUS_OK) return status;
    status = py68_platform_path_remove(path_c);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, path_c, path->length + 1);
    if (status != PY68_STATUS_OK) {
        py68_file_builtin_error(runtime, PY68_ERROR_IO, "remove failed");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    *result = py68_value_none();
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_rename(Py68Runtime *runtime, Py68U16 argument_count,
                               Py68Value *arguments, Py68Value *result)
{
    Py68String *old_path;
    Py68String *new_path;
    char *old_c;
    char *new_c;
    Py68Status status;
    if (argument_count != 2 || !py68_require_string(arguments[0], &old_path) ||
        !py68_require_string(arguments[1], &new_path)) {
        py68_file_builtin_error(runtime, PY68_ERROR_TYPE,
                                "rename expects two path strings");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    status = py68_cstring_from_string(runtime, old_path, &old_c);
    if (status != PY68_STATUS_OK) return status;
    status = py68_cstring_from_string(runtime, new_path, &new_c);
    if (status != PY68_STATUS_OK) {
        py68_free(&runtime->allocator, PY68_MEM_TEMP, old_c,
                  old_path->length + 1);
        return status;
    }
    status = py68_platform_path_rename(old_c, new_c);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, old_c, old_path->length + 1);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, new_c, new_path->length + 1);
    if (status != PY68_STATUS_OK) {
        py68_file_builtin_error(runtime, PY68_ERROR_IO, "rename failed");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    *result = py68_value_none();
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_getenv(Py68Runtime *runtime, Py68U16 argument_count,
                               Py68Value *arguments, Py68Value *result)
{
    Py68String *name;
    char *name_c;
    char *value;
    Py68U32 length;
    Py68String *string;
    Py68Status status;

    if (argument_count != 1 || !py68_require_string(arguments[0], &name)) {
        py68_file_builtin_error(runtime, PY68_ERROR_TYPE,
                                "getenv expects a name string");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    status = py68_cstring_from_string(runtime, name, &name_c);
    if (status != PY68_STATUS_OK) return status;
    status = py68_platform_var_get(runtime, name_c, &value, &length);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, name_c, name->length + 1);
    if (status == PY68_STATUS_SOURCE_ERROR) {
        *result = py68_value_none();
        return PY68_STATUS_OK;
    }
    if (status != PY68_STATUS_OK) {
        py68_file_builtin_error(runtime, PY68_ERROR_IO, "getenv failed");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    status = py68_string_new_copy(runtime, value, length, &string);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, value, length + 1);
    if (status != PY68_STATUS_OK) return status;
    *result = py68_value_from_object(&string->base);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_setenv(Py68Runtime *runtime, Py68U16 argument_count,
                               Py68Value *arguments, Py68Value *result)
{
    Py68String *name;
    Py68String *value;
    char *name_c;
    char *value_c;
    Py68Status status;

    if (argument_count != 2 || !py68_require_string(arguments[0], &name) ||
        !py68_require_string(arguments[1], &value)) {
        py68_file_builtin_error(runtime, PY68_ERROR_TYPE,
                                "setenv expects name and value strings");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    status = py68_cstring_from_string(runtime, name, &name_c);
    if (status != PY68_STATUS_OK) return status;
    status = py68_cstring_from_string(runtime, value, &value_c);
    if (status != PY68_STATUS_OK) {
        py68_free(&runtime->allocator, PY68_MEM_TEMP, name_c, name->length + 1);
        return status;
    }
    status = py68_platform_var_set(runtime, name_c, value_c);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, name_c, name->length + 1);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, value_c, value->length + 1);
    if (status != PY68_STATUS_OK) {
        py68_file_builtin_error(runtime, PY68_ERROR_IO, "setenv failed");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    *result = py68_value_none();
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_unsetenv(Py68Runtime *runtime, Py68U16 argument_count,
                                 Py68Value *arguments, Py68Value *result)
{
    Py68String *name;
    char *name_c;
    Py68Status status;

    if (argument_count != 1 || !py68_require_string(arguments[0], &name)) {
        py68_file_builtin_error(runtime, PY68_ERROR_TYPE,
                                "unsetenv expects a name string");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    status = py68_cstring_from_string(runtime, name, &name_c);
    if (status != PY68_STATUS_OK) return status;
    status = py68_platform_var_unset(runtime, name_c);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, name_c, name->length + 1);
    if (status != PY68_STATUS_OK) {
        py68_file_builtin_error(runtime, PY68_ERROR_IO, "unsetenv failed");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    *result = py68_value_none();
    return PY68_STATUS_OK;
}

/* Amiga-facing names; same platform_var_* implementation. */
Py68Status py68_builtin_assign_get(Py68Runtime *runtime,
                                   Py68U16 argument_count,
                                   Py68Value *arguments, Py68Value *result)
{
    return py68_builtin_getenv(runtime, argument_count, arguments, result);
}

Py68Status py68_builtin_assign_add(Py68Runtime *runtime,
                                   Py68U16 argument_count,
                                   Py68Value *arguments, Py68Value *result)
{
    return py68_builtin_setenv(runtime, argument_count, arguments, result);
}

Py68Status py68_builtin_assign_remove(Py68Runtime *runtime,
                                      Py68U16 argument_count,
                                      Py68Value *arguments, Py68Value *result)
{
    return py68_builtin_unsetenv(runtime, argument_count, arguments, result);
}
