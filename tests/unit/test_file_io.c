/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_builtin.h"
#include "py68k_file.h"
#include "py68k_platform.h"
#include "py68k_runtime.h"
#include "py68k_string.h"

#include <stdio.h>
#include <string.h>

static int expect(int condition, const char *label, int *passed)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", label);
        *passed = 0;
        return 0;
    }
    return 1;
}

int main(void)
{
    Py68Runtime runtime;
    Py68Value result;
    Py68Value args[3];
    Py68String *s_path;
    Py68String *s_mode;
    Py68String *s_text;
    Py68String *s_name;
    Py68String *s_value;
    Py68File *file;
    int passed = 1;
    const char *path = "build/host/test_file_io.tmp";
    const char *path2 = "build/host/test_file_io_renamed.tmp";

    expect(py68_runtime_initialize(&runtime) == PY68_STATUS_OK,
           "runtime init", &passed);

    expect(py68_string_new_copy(&runtime, path, (Py68U32)strlen(path),
                                &s_path) == PY68_STATUS_OK,
           "path string", &passed);
    expect(py68_string_new_copy(&runtime, "w", 1, &s_mode) == PY68_STATUS_OK,
           "mode string", &passed);
    expect(py68_string_new_copy(&runtime, "hello\nworld\n", 12, &s_text) ==
               PY68_STATUS_OK,
           "text string", &passed);

    args[0] = py68_value_from_object(&s_path->base);
    args[1] = py68_value_from_object(&s_mode->base);
    expect(py68_builtin_fopen(&runtime, 2, args, &result) == PY68_STATUS_OK,
           "fopen w", &passed);
    expect(result.type == PY68_VALUE_OBJECT && result.as.object != NULL &&
               result.as.object->type == PY68_OBJECT_FILE,
           "fopen returns file", &passed);
    file = (Py68File *)result.as.object;

    args[0] = result;
    args[1] = py68_value_from_object(&s_text->base);
    expect(py68_builtin_fwrite(&runtime, 2, args, &result) == PY68_STATUS_OK,
           "fwrite", &passed);
    expect(result.type == PY68_VALUE_INT && result.as.integer == 12,
           "fwrite count", &passed);
    py68_value_release(&runtime, result);

    args[0] = py68_value_from_object(&file->base);
    expect(py68_builtin_fclose(&runtime, 1, args, &result) == PY68_STATUS_OK,
           "fclose", &passed);
    py68_value_release(&runtime, result);
    py68_object_release(&runtime, &file->base);
    py68_object_release(&runtime, &s_mode->base);
    py68_object_release(&runtime, &s_text->base);

    expect(py68_string_new_copy(&runtime, "r", 1, &s_mode) == PY68_STATUS_OK,
           "mode r", &passed);
    args[0] = py68_value_from_object(&s_path->base);
    args[1] = py68_value_from_object(&s_mode->base);
    expect(py68_builtin_fopen(&runtime, 2, args, &result) == PY68_STATUS_OK,
           "fopen r", &passed);
    file = (Py68File *)result.as.object;

    args[0] = py68_value_from_object(&file->base);
    expect(py68_builtin_freadline(&runtime, 1, args, &result) == PY68_STATUS_OK,
           "freadline", &passed);
    expect(result.type == PY68_VALUE_OBJECT &&
               ((Py68String *)result.as.object)->length == 6 &&
               memcmp(((Py68String *)result.as.object)->data, "hello\n", 6) ==
                   0,
           "freadline content", &passed);
    py68_value_release(&runtime, result);

    args[0] = py68_value_from_object(&file->base);
    args[1] = py68_value_int(5);
    expect(py68_builtin_fread(&runtime, 2, args, &result) == PY68_STATUS_OK,
           "fread", &passed);
    expect(result.type == PY68_VALUE_OBJECT &&
               ((Py68String *)result.as.object)->length == 5 &&
               memcmp(((Py68String *)result.as.object)->data, "world", 5) == 0,
           "fread content", &passed);
    py68_value_release(&runtime, result);

    args[0] = py68_value_from_object(&file->base);
    expect(py68_builtin_fclose(&runtime, 1, args, &result) == PY68_STATUS_OK,
           "fclose2", &passed);
    py68_value_release(&runtime, result);
    py68_object_release(&runtime, &file->base);
    py68_object_release(&runtime, &s_mode->base);

    expect(py68_string_new_copy(&runtime, "r", 1, &s_mode) == PY68_STATUS_OK,
           "mode r all", &passed);
    args[0] = py68_value_from_object(&s_path->base);
    args[1] = py68_value_from_object(&s_mode->base);
    expect(py68_builtin_fopen(&runtime, 2, args, &result) == PY68_STATUS_OK,
           "fopen r all", &passed);
    file = (Py68File *)result.as.object;
    args[0] = py68_value_from_object(&file->base);
    args[1] = py68_value_int(2147483647);
    expect(py68_builtin_fread(&runtime, 2, args, &result) == PY68_STATUS_OK,
           "fread large count", &passed);
    expect(result.type == PY68_VALUE_OBJECT &&
               ((Py68String *)result.as.object)->length == 12 &&
               memcmp(((Py68String *)result.as.object)->data, "hello\nworld\n",
                      12) == 0,
           "fread large count content", &passed);
    py68_value_release(&runtime, result);
    args[0] = py68_value_from_object(&file->base);
    expect(py68_builtin_fclose(&runtime, 1, args, &result) == PY68_STATUS_OK,
           "fclose all", &passed);
    py68_value_release(&runtime, result);
    py68_object_release(&runtime, &file->base);
    py68_object_release(&runtime, &s_mode->base);

    args[0] = py68_value_from_object(&s_path->base);
    expect(py68_builtin_exists(&runtime, 1, args, &result) == PY68_STATUS_OK &&
               result.type == PY68_VALUE_BOOL && result.as.integer == 1,
           "exists true", &passed);

    expect(py68_string_new_copy(&runtime, path2, (Py68U32)strlen(path2),
                                &s_value) == PY68_STATUS_OK,
           "path2", &passed);
    args[0] = py68_value_from_object(&s_path->base);
    args[1] = py68_value_from_object(&s_value->base);
    expect(py68_builtin_rename(&runtime, 2, args, &result) == PY68_STATUS_OK,
           "rename", &passed);
    py68_value_release(&runtime, result);

    args[0] = py68_value_from_object(&s_value->base);
    expect(py68_builtin_remove(&runtime, 1, args, &result) == PY68_STATUS_OK,
           "remove", &passed);
    py68_value_release(&runtime, result);
    py68_object_release(&runtime, &s_path->base);
    py68_object_release(&runtime, &s_value->base);

    expect(py68_string_new_copy(&runtime, "PY68K_TEST_ENV", 15, &s_name) ==
               PY68_STATUS_OK,
           "env name", &passed);
    expect(py68_string_new_copy(&runtime, "amiga-first", 11, &s_value) ==
               PY68_STATUS_OK,
           "env value", &passed);
    args[0] = py68_value_from_object(&s_name->base);
    args[1] = py68_value_from_object(&s_value->base);
    expect(py68_builtin_setenv(&runtime, 2, args, &result) == PY68_STATUS_OK,
           "setenv", &passed);
    py68_value_release(&runtime, result);
    args[0] = py68_value_from_object(&s_name->base);
    expect(py68_builtin_getenv(&runtime, 1, args, &result) == PY68_STATUS_OK &&
               result.type == PY68_VALUE_OBJECT &&
               ((Py68String *)result.as.object)->length == 11,
           "getenv", &passed);
    py68_value_release(&runtime, result);
    args[0] = py68_value_from_object(&s_name->base);
    expect(py68_builtin_unsetenv(&runtime, 1, args, &result) == PY68_STATUS_OK,
           "unsetenv", &passed);
    py68_value_release(&runtime, result);
    args[0] = py68_value_from_object(&s_name->base);
    expect(py68_builtin_getenv(&runtime, 1, args, &result) == PY68_STATUS_OK &&
               result.type == PY68_VALUE_NONE,
           "getenv missing", &passed);
    py68_object_release(&runtime, &s_name->base);
    py68_object_release(&runtime, &s_value->base);

    py68_runtime_shutdown(&runtime);
    expect(runtime.allocator.stats.current_bytes == 0, "no leak", &passed);

    if (passed) {
        printf("PASS: file and env IO tests\n");
        return 0;
    }
    return 1;
}
