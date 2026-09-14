/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_ext_load.h"
#include "py68k_ext.h"
#include "py68k_error.h"
#include "py68k_module.h"
#include "py68k_native.h"
#include "py68k_string.h"

#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dos.h>

#include <string.h>

#define PY68_EXT_MAX_EXPORTS 256

static void py68_ext_error(Py68Runtime *runtime, Py68ErrorKind kind,
                           const char *message)
{
    Py68Location location;
    location.offset = 0;
    location.line = 0;
    location.column = 0;
    location.length = 0;
    py68_error_set(&runtime->error, kind, location, NULL, message);
}

static Py68U16 py68_ext_strlen(const char *text)
{
    Py68U16 length = 0;
    if (text == NULL) return 0;
    while (text[length] != '\0') ++length;
    return length;
}

static void py68_ext_basename(const char *path, char *out, Py68U16 out_size)
{
    const char *start = path;
    const char *cursor;
    Py68U16 length;
    if (out_size == 0) return;
    out[0] = '\0';
    if (path == NULL) return;
    for (cursor = path; *cursor != '\0'; ++cursor) {
        if (*cursor == '/' || *cursor == ':' || *cursor == '\\')
            start = cursor + 1;
    }
    length = py68_ext_strlen(start);
    if (length >= out_size) length = (Py68U16)(out_size - 1);
    if (length != 0) memcpy(out, start, length);
    out[length] = '\0';
}

static int py68_ext_require_string(Py68Value value, Py68String **out)
{
    if (value.type != PY68_VALUE_OBJECT || value.as.object == NULL ||
        value.as.object->type != PY68_OBJECT_STRING)
        return 0;
    *out = (Py68String *)value.as.object;
    return 1;
}

static Py68Status py68_ext_cstring_from_string(Py68Runtime *runtime,
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

Py68Status py68_ext_load_library(Py68Runtime *runtime, const char *path,
                                 Py68Value *result)
{
    BPTR seg;
    ULONG *hunk;
    const Py68ExtHeader *header;
    Py68Module *module;
    char name[64];
    Py68U16 index;
    Py68Status status;

    if (runtime == NULL || path == NULL || result == NULL)
        return PY68_STATUS_INTERNAL_ERROR;
    if (path[0] == '\0') {
        py68_ext_error(runtime, PY68_ERROR_VALUE, "load_library path is empty");
        return PY68_STATUS_RUNTIME_ERROR;
    }

    seg = LoadSeg((CONST_STRPTR)path);
    if (seg == 0) {
        py68_ext_error(runtime, PY68_ERROR_IMPORT,
                       "load_library could not LoadSeg path");
        return PY68_STATUS_RUNTIME_ERROR;
    }

    hunk = (ULONG *)BADDR(seg);
    /* Skip AmigaDOS next-segment longword; header must follow immediately. */
    header = (const Py68ExtHeader *)(hunk + 1);
    if (header->magic != PY68_EXT_MAGIC ||
        header->abi_version != PY68_EXT_ABI_VERSION ||
        header->exports == NULL ||
        header->export_count == 0 ||
        header->export_count > PY68_EXT_MAX_EXPORTS) {
        UnLoadSeg(seg);
        py68_ext_error(runtime, PY68_ERROR_IMPORT,
                       "load_library invalid extension header");
        return PY68_STATUS_RUNTIME_ERROR;
    }

    py68_ext_basename(path, name, (Py68U16)sizeof(name));
    if (name[0] == '\0') {
        UnLoadSeg(seg);
        py68_ext_error(runtime, PY68_ERROR_VALUE, "load_library bad path");
        return PY68_STATUS_RUNTIME_ERROR;
    }

    status = py68_module_new(runtime, name, path, &module);
    if (status != PY68_STATUS_OK) {
        UnLoadSeg(seg);
        return status;
    }
    module->native_seg = (void *)(unsigned long)seg;

    for (index = 0; index < header->export_count; ++index) {
        const Py68ExtExport *export_entry = &header->exports[index];
        Py68NativeFunction *function;
        Py68U16 name_length;

        if (export_entry->name == NULL || export_entry->function == NULL ||
            export_entry->min_args > export_entry->max_args) {
            py68_object_release(runtime, &module->base);
            py68_ext_error(runtime, PY68_ERROR_IMPORT,
                           "load_library invalid export entry");
            return PY68_STATUS_RUNTIME_ERROR;
        }
        name_length = py68_ext_strlen(export_entry->name);
        if (name_length == 0) {
            py68_object_release(runtime, &module->base);
            py68_ext_error(runtime, PY68_ERROR_IMPORT,
                           "load_library empty export name");
            return PY68_STATUS_RUNTIME_ERROR;
        }
        status = py68_native_new(
            runtime, export_entry->name, export_entry->min_args,
            export_entry->max_args,
            (Py68NativeCallback)export_entry->function, &function);
        if (status != PY68_STATUS_OK) {
            py68_object_release(runtime, &module->base);
            return status;
        }
        status = py68_module_set(runtime, module,
                                 (const Py68U8 *)export_entry->name,
                                 name_length,
                                 py68_value_from_object(&function->base));
        py68_object_release(runtime, &function->base);
        if (status != PY68_STATUS_OK) {
            py68_object_release(runtime, &module->base);
            return status;
        }
    }

    *result = py68_value_from_object(&module->base);
    return PY68_STATUS_OK;
}

Py68Status py68_builtin_load_library(Py68Runtime *runtime,
                                     Py68U16 argument_count,
                                     Py68Value *arguments, Py68Value *result)
{
    Py68String *path;
    char *path_c;
    Py68Status status;

    if (argument_count != 1 || !py68_ext_require_string(arguments[0], &path)) {
        py68_ext_error(runtime, PY68_ERROR_TYPE,
                       "load_library expects a path string");
        return PY68_STATUS_RUNTIME_ERROR;
    }
    status = py68_ext_cstring_from_string(runtime, path, &path_c);
    if (status != PY68_STATUS_OK) return status;
    status = py68_ext_load_library(runtime, path_c, result);
    py68_free(&runtime->allocator, PY68_MEM_TEMP, path_c, path->length + 1);
    return status;
}
