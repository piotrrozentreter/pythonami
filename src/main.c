/* 2026 by Piotr Rozentreter (Rozsoft) */

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
#define PY68K_HELP \
    "Usage: pythonami [-V|--help] [-c cmd | script.py]\n"

static Py68Status py68_write_literal(Py68Runtime *runtime, const char *text)
{
    return py68_platform_write_stdout(runtime, text, (Py68U32)strlen(text));
}

static Py68U32 py68_format_u32(char *buffer, Py68U32 capacity, Py68U32 value)
{
    char digits[10];
    Py68U32 digit_count = 0;
    Py68U32 written = 0;
    do {
        digits[digit_count++] = (char)('0' + (value % 10));
        value /= 10;
    } while (value != 0 && digit_count < sizeof(digits));
    while (digit_count != 0 && written < capacity) {
        buffer[written++] = digits[--digit_count];
    }
    return written;
}

static const char *py68_error_kind_name(Py68ErrorKind kind)
{
    switch (kind) {
    case PY68_ERROR_TOKEN: return "TokenError";
    case PY68_ERROR_SYNTAX: return "SyntaxError";
    case PY68_ERROR_NAME: return "NameError";
    case PY68_ERROR_TYPE: return "TypeError";
    case PY68_ERROR_VALUE: return "ValueError";
    case PY68_ERROR_INDEX: return "IndexError";
    case PY68_ERROR_ZERO_DIVISION: return "ZeroDivisionError";
    case PY68_ERROR_OVERFLOW: return "OverflowError";
    case PY68_ERROR_RECURSION: return "RecursionError";
    case PY68_ERROR_MEMORY: return "MemoryError";
    case PY68_ERROR_IO: return "IOError";
    case PY68_ERROR_BYTECODE: return "BytecodeError";
    case PY68_ERROR_INTERNAL: return "InternalError";
    default: return "Error";
    }
}

static Py68Status py68_append_text(char *buffer, Py68U32 capacity,
                                   Py68U32 *length, const char *text)
{
    Py68U32 index;
    for (index = 0; text[index] != '\0' && *length < capacity - 1; ++index)
        buffer[(*length)++] = text[index];
    return PY68_STATUS_OK;
}

static Py68Status py68_report_error(Py68Runtime *runtime, const char *path)
{
    /* Note: runtime->error.filename may already be a dangling pointer into
       a destroyed Py68Source by the time this runs, so always use the
       caller-owned `path` string instead. */
    char buffer[640];
    Py68U32 length = 0;
    const char *kind_name = py68_error_kind_name(runtime->error.kind);

    py68_append_text(buffer, sizeof(buffer), &length, kind_name);
    py68_append_text(buffer, sizeof(buffer), &length, ": ");
    py68_append_text(buffer, sizeof(buffer), &length, runtime->error.message);
    if (length < sizeof(buffer) - 1) buffer[length++] = '\n';

    if (runtime->error.location.line != 0) {
        py68_append_text(buffer, sizeof(buffer), &length, "  at ");
        py68_append_text(buffer, sizeof(buffer), &length,
                         path != NULL && path[0] != '\0' ? path : "<string>");
        if (length < sizeof(buffer) - 1) buffer[length++] = ':';
        length += py68_format_u32(buffer + length,
                                  (Py68U32)sizeof(buffer) - length,
                                  runtime->error.location.line);
        if (length < sizeof(buffer) - 1) buffer[length++] = '\n';
    }

    if (runtime->traceback_length != 0) {
        Py68U32 index;
        for (index = 0;
             index < runtime->traceback_length && length < sizeof(buffer) - 1;
             ++index)
            buffer[length++] = runtime->traceback[index];
    }

    return py68_platform_write_stderr(runtime, buffer, length);
}

static Py68Status py68_execute_source(Py68Runtime *runtime, const char *path,
                                      const Py68U8 *data, Py68U32 length)
{
    Py68Source source;
    Py68TokenArray tokens;
    Py68AstArena arena;
    Py68StatementParser parser;
    Py68AstNode *module;
    Py68Code code;
    Py68Status status;

    status = py68_source_initialize(&runtime->allocator, &source, path,
                                    data, length);
    if (status != PY68_STATUS_OK) return status;
    py68_token_array_initialize(&tokens);
    py68_error_clear(&runtime->error);
    runtime->traceback_length = 0;
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
    status = py68_builtins_install(runtime);
    if (status != PY68_STATUS_OK) goto cleanup_code;
    status = py68_vm_execute(runtime, &code);
cleanup_code:
    py68_code_destroy(&runtime->allocator, &code);
cleanup_arena:
    py68_ast_arena_destroy(&arena);
    py68_token_array_destroy(&runtime->allocator, &tokens);
cleanup_source:
    py68_source_destroy(&runtime->allocator, &source);
    if (status != PY68_STATUS_OK && status != PY68_STATUS_EXIT) {
        py68_report_error(runtime, path);
    }
    return status;
}

static Py68Status py68_execute_file(Py68Runtime *runtime, const char *path)
{
    Py68U8 *file_data;
    Py68U32 file_length;
    Py68Status status;

    status = py68_platform_read_file(runtime, path, &file_data, &file_length);
    if (status != PY68_STATUS_OK) return status;
    status = py68_execute_source(runtime, path, file_data, file_length);
    py68_free(&runtime->allocator, PY68_MEM_SOURCE, file_data, file_length + 1);
    return status;
}

static Py68Status py68_execute_command(Py68Runtime *runtime, const char *command)
{
    return py68_execute_source(runtime, "<string>",
                               (const Py68U8 *)command,
                               (Py68U32)strlen(command));
}

static int py68_exit_status(Py68Runtime *runtime, Py68Status status)
{
    Py68I32 code;
    if (status == PY68_STATUS_OK) return 0;
    if (status == PY68_STATUS_EXIT) {
        code = runtime->requested_exit_code;
        if (code < 0) code = 1;
        if (code > 255) code = 255;
        return (int)code;
    }
    return (int)status;
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
    } else if (argc == 3 && strcmp(argv[1], "-c") == 0) {
        status = py68_execute_command(&runtime, argv[2]);
    } else if (argc == 2) {
        status = py68_execute_file(&runtime, argv[1]);
    } else {
        status = py68_platform_write_stderr(
            &runtime, "error: invalid arguments\n", 25);
        if (status == PY68_STATUS_OK) status = PY68_STATUS_SOURCE_ERROR;
    }

    {
        int exit_code = py68_exit_status(&runtime, status);
        py68_runtime_shutdown(&runtime);
        return exit_code;
    }
}
