#include "py68k_runtime.h"
#include "py68k_source.h"
#include "py68k_token.h"
#include "py68k_tokenizer.h"
#include "py68k_ast_arena.h"
#include "py68k_parser.h"
#include "py68k_compiler.h"
#include "py68k_verify.h"
#include "py68k_vm.h"
#include "py68k_builtin.h"
#include "py68k_native.h"

#include <string.h>

#define PY68K_VERSION "Python68K 0.1.0\n"
#define PY68K_HELP "Usage: pythonami [-V|--help] script.py\n"

static Py68Status py68_write_literal(Py68Runtime *runtime, const char *text)
{
    return py68_platform_write_stdout(runtime, text, (Py68U32)strlen(text));
}

static Py68Status py68_write_decimal(Py68Runtime *runtime, Py68U32 value)
{
    char digits[10];
    Py68U16 count = 0;
    Py68U16 index;
    if (value == 0) return py68_write_literal(runtime, "0");
    while (value != 0) {
        digits[count++] = (char)('0' + (value % 10));
        value /= 10;
    }
    for (index = count; index != 0; --index) {
        Py68Status status = py68_platform_write_stderr(
            runtime, &digits[index - 1], 1);
        if (status != PY68_STATUS_OK) return status;
    }
    return PY68_STATUS_OK;
}

static Py68Status py68_write_traceback(Py68Runtime *runtime)
{
    Py68U16 index;
    Py68Status status;
    for (index = 0; index < runtime->traceback_count; ++index) {
        const Py68TraceEntry *entry = &runtime->traceback[index];
        status = py68_platform_write_stderr(runtime, "  at ", 6);
        if (status != PY68_STATUS_OK) return status;
        if (entry->function_name_length != 0) {
            status = py68_platform_write_stderr(
                runtime,
                entry->function_name,
                entry->function_name_length);
        } else {
            status = py68_platform_write_stderr(runtime, "<module>", 8);
        }
        if (status != PY68_STATUS_OK) return status;
        status = py68_platform_write_stderr(runtime, " (", 2);
        if (status == PY68_STATUS_OK && entry->filename[0] != '\0')
            status = py68_platform_write_stderr(
                runtime, entry->filename,
                (Py68U32)strlen(entry->filename));
        if (status == PY68_STATUS_OK) status = py68_platform_write_stderr(runtime, ":", 1);
        if (status == PY68_STATUS_OK) status = py68_write_decimal(runtime, entry->line);
        if (status == PY68_STATUS_OK) status = py68_platform_write_stderr(runtime, ":", 1);
        if (status == PY68_STATUS_OK) status = py68_write_decimal(runtime, entry->column);
        if (status == PY68_STATUS_OK) status = py68_platform_write_stderr(runtime, ")\n", 2);
        if (status != PY68_STATUS_OK) return status;
    }
    return PY68_STATUS_OK;
}

static Py68Status py68_execute_file(Py68Runtime *runtime, const char *path)
{
    Py68U8 *file_data;
    Py68U32 file_length;
    Py68Source source;
    Py68TokenArray tokens;
    Py68AstArena arena;
    Py68StatementParser parser;
    Py68AstNode *module;
    Py68Code code;
    Py68U16 name_index;
    Py68U16 index;
    Py68Status status;

    status = py68_platform_read_file(runtime, path, &file_data, &file_length);
    if (status != PY68_STATUS_OK) return status;
    status = py68_source_initialize(&runtime->allocator, &source, path,
                                    file_data, file_length);
    py68_free(&runtime->allocator, PY68_MEM_SOURCE, file_data, file_length + 1);
    if (status != PY68_STATUS_OK) return status;
    py68_token_array_initialize(&tokens);
    py68_error_clear(&runtime->error);
    status = py68_tokenize(&runtime->allocator, &source, &tokens,
                           &runtime->error);
    if (status != PY68_STATUS_OK) goto cleanup_source;
    py68_ast_arena_initialize(&arena, &runtime->allocator);
    parser.expression.allocator = &runtime->allocator;
    parser.expression.source = &source;
    parser.expression.tokens = &tokens;
    parser.expression.position = 0;
    parser.expression.arena = &arena;
    parser.expression.error = &runtime->error;
    parser.inside_function = 0;
    parser.loop_depth = 0;
    status = py68_parse_module(&parser, &module);
    if (status != PY68_STATUS_OK) goto cleanup_arena;
    status = py68_compile_module(&runtime->allocator, &source, module,
                                 &code, &runtime->error);
    if (status != PY68_STATUS_OK) goto cleanup_arena;
    status = py68_code_intern_names(runtime, &code);
    if (status != PY68_STATUS_OK) goto cleanup_code;
    for (index = 0; index < code.name_count; ++index) {
        Py68U16 len = code.name_lengths[index];
        const char *name_str = (const char *)source.data + code.name_offsets[index];
        Py68NativeFunction *builtin_fn = NULL;
        Py68NativeCallback cb = NULL;
        Py68U16 min_args = 0, max_args = 0;
        const char *fn_name = NULL;

        if (len == 5 && memcmp(name_str, "print", 5) == 0) {
            cb = py68_builtin_print; min_args = 0; max_args = 65535; fn_name = "print";
        } else if (len == 3 && memcmp(name_str, "len", 3) == 0) {
            cb = py68_builtin_len; min_args = 1; max_args = 1; fn_name = "len";
        } else if (len == 5 && memcmp(name_str, "range", 5) == 0) {
            cb = py68_builtin_range; min_args = 1; max_args = 3; fn_name = "range";
        } else if (len == 8 && memcmp(name_str, "list_pop", 8) == 0) {
            cb = py68_builtin_list_pop; min_args = 1; max_args = 1; fn_name = "list_pop";
        }

        if (cb != NULL) {
            name_index = index;
            status = py68_native_new(runtime, fn_name, min_args, max_args,
                                     cb, &builtin_fn);
            if (status != PY68_STATUS_OK) goto cleanup_code;
            if (code.name_strings != NULL)
                status = py68_builtin_set_string_copy(
                    runtime, name_index, code.name_strings[name_index],
                    py68_value_from_object(&builtin_fn->base));
            else
                status = py68_builtin_set_copy(
                    runtime, name_index, py68_value_from_object(&builtin_fn->base));
            py68_object_release(runtime, &builtin_fn->base);
            if (status != PY68_STATUS_OK) goto cleanup_code;
        }
    }
    status = py68_vm_execute(runtime, &code);
cleanup_code:
    py68_code_destroy(&runtime->allocator, &code);
cleanup_arena:
    py68_ast_arena_destroy(&arena);
    py68_token_array_destroy(&runtime->allocator, &tokens);
cleanup_source:
    py68_source_destroy(&runtime->allocator, &source);
    return status;
}

int main(int argc, char **argv)
{
    Py68Runtime runtime;
    Py68Status status;

    status = py68_runtime_initialize(&runtime);
    if (status != PY68_STATUS_OK) {
        return (int)status;
    }

    if (argc == 2 && (strcmp(argv[1], "-V") == 0 ||
                      strcmp(argv[1], "--version") == 0)) {
        status = py68_write_literal(&runtime, PY68K_VERSION);
    } else if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        status = py68_write_literal(&runtime, PY68K_HELP);
    } else if (argc == 1) {
        status = py68_write_literal(&runtime, PY68K_HELP);
    } else if (argc == 2) {
        status = py68_execute_file(&runtime, argv[1]);
    } else {
        status = py68_platform_write_stderr(
            &runtime, "error: source execution is not implemented in Phase 0\n",
            53);
        if (status == PY68_STATUS_OK) {
            status = PY68_STATUS_SOURCE_ERROR;
        }
    }

    if (status != PY68_STATUS_OK && runtime.error.message[0] != '\0') {
        py68_platform_write_stderr(&runtime, runtime.error.message,
                                   (Py68U32)strlen(runtime.error.message));
        py68_platform_write_stderr(&runtime, "\n", 1);
        py68_write_traceback(&runtime);
    }

    py68_runtime_shutdown(&runtime);
    return status == PY68_STATUS_OK ? 0 : (int)status;
}
