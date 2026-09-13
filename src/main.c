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

static Py68Status py68_report_error(Py68Runtime *runtime, const char *path)
{
    /* Note: runtime->error.filename may already be a dangling pointer into
       a destroyed Py68Source by the time this runs, so always use the
       caller-owned `path` string instead. */
    char buffer[320];
    Py68U32 length = 0;
    Py68U32 index;

    for (index = 0; path[index] != '\0' && length < sizeof(buffer) - 1;
         ++index) buffer[length++] = path[index];
    if (length < sizeof(buffer) - 1) buffer[length++] = ':';
    length += py68_format_u32(buffer + length, (Py68U32)sizeof(buffer) - length,
                              runtime->error.location.line);
    if (length < sizeof(buffer) - 1) buffer[length++] = ':';
    length += py68_format_u32(buffer + length, (Py68U32)sizeof(buffer) - length,
                              runtime->error.location.column);
    if (length < sizeof(buffer) - 2) {
        buffer[length++] = ':';
        buffer[length++] = ' ';
    }
    for (index = 0; runtime->error.message[index] != '\0' &&
         length < sizeof(buffer) - 1; ++index)
        buffer[length++] = runtime->error.message[index];
    if (length < sizeof(buffer) - 1) buffer[length++] = '\n';
    return py68_platform_write_stderr(runtime, buffer, length);
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

    py68_runtime_shutdown(&runtime);
    return status == PY68_STATUS_OK ? 0 : (int)status;
}
