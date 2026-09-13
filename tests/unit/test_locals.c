#include "py68k_ast_arena.h"
#include "py68k_code.h"
#include "py68k_compiler.h"
#include "py68k_global.h"
#include "py68k_parser.h"
#include "py68k_runtime.h"
#include "py68k_source.h"
#include "py68k_tokenizer.h"
#include "py68k_verify.h"
#include "py68k_vm.h"

#include <stdio.h>
#include <string.h>

static Py68Status run_source(Py68Runtime *runtime, const char *text,
                             Py68Code *code, Py68AstArena *arena,
                             Py68TokenArray *tokens, Py68Source *source,
                             Py68Error *error)
{
    Py68StatementParser parser;
    Py68AstNode *module;
    Py68Status status;

    py68_token_array_initialize(tokens);
    status = py68_source_initialize(&runtime->allocator, source, "locals.py",
                                    (const Py68U8 *)text,
                                    (Py68U32)strlen(text));
    if (status != PY68_STATUS_OK) return status;
    status = py68_tokenize(&runtime->allocator, source, tokens, error);
    if (status != PY68_STATUS_OK) return status;
    py68_ast_arena_initialize(arena, &runtime->allocator);
    parser.expression.allocator = &runtime->allocator;
    parser.expression.source = source;
    parser.expression.tokens = tokens;
    parser.expression.position = 0;
    parser.expression.arena = arena;
    parser.expression.error = error;
    parser.inside_function = 0;
    parser.loop_depth = 0;
    status = py68_parse_module(&parser, &module);
    if (status != PY68_STATUS_OK) return status;
    status = py68_compile_module(&runtime->allocator, source, module,
                                 code, error);
    if (status != PY68_STATUS_OK) return status;
    status = py68_verify_code(code, error);
    if (status != PY68_STATUS_OK) return status;
    return py68_vm_execute(runtime, code);
}

static int get_integer(Py68Runtime *runtime, Py68Code *code,
                       const char *text, const char *name, Py68I32 expected)
{
    Py68U16 index;
    Py68Value value;
    const char *position = strstr(text, name);
    if (position == NULL) return 0;
    if (py68_code_add_name(&runtime->allocator, code,
                           (Py68U32)(position - text),
                           (Py68U16)strlen(name), &index) != PY68_STATUS_OK)
        return 0;
    if (py68_global_get_copy(runtime, index, &value) != PY68_STATUS_OK)
        return 0;
    if (value.type != PY68_VALUE_INT || value.as.integer != expected) {
        py68_value_release(runtime, value);
        return 0;
    }
    py68_value_release(runtime, value);
    return 1;
}

static void cleanup(Py68Runtime *runtime, Py68Code *code,
                    Py68AstArena *arena, Py68TokenArray *tokens,
                    Py68Source *source)
{
    py68_code_destroy(&runtime->allocator, code);
    py68_ast_arena_destroy(arena);
    py68_token_array_destroy(&runtime->allocator, tokens);
    py68_source_destroy(&runtime->allocator, source);
    py68_runtime_shutdown(runtime);
}

int main(void)
{
    const char *text =
        "total = 1\n"
        "total += 2\n"
        "def increment(start):\n"
        "    value = start\n"
        "    value += 2\n"
        "    return value\n"
        "result = increment(5)\n";
    const char *bad_text =
        "def bad():\n"
        "    value = value + 1\n"
        "    return value\n"
        "bad()\n";
    Py68Runtime runtime;
    Py68Code code;
    Py68AstArena arena;
    Py68TokenArray tokens;
    Py68Source source;
    Py68Error error;
    int passed = 1;

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    passed &= run_source(&runtime, text, &code, &arena, &tokens, &source,
                         &error) == PY68_STATUS_OK;
    passed &= get_integer(&runtime, &code, text, "total", 3);
    passed &= get_integer(&runtime, &code, text, "result", 7);
    cleanup(&runtime, &code, &arena, &tokens, &source);
    passed &= runtime.allocator.stats.current_bytes == 0;

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    passed &= run_source(&runtime, bad_text, &code, &arena, &tokens, &source,
                         &error) == PY68_STATUS_RUNTIME_ERROR;
    passed &= runtime.error.kind == PY68_ERROR_NAME;
    passed &= runtime.error.location.offset ==
              (Py68U32)(strstr(bad_text, "value = value") - bad_text);
    passed &= runtime.error.location.length == 5;
    passed &= runtime.error.filename != NULL &&
              strcmp(runtime.error.filename, "locals.py") == 0;
    passed &= runtime.traceback_count == 2;
    passed &= strcmp(runtime.traceback[0].filename, "locals.py") == 0;
    passed &= runtime.traceback[0].function_name_length == 3 &&
              memcmp(runtime.traceback[0].function_name, "bad", 3) == 0;
    cleanup(&runtime, &code, &arena, &tokens, &source);
    passed &= runtime.allocator.stats.current_bytes == 0;

    if (passed) { puts("PASS: local slot tests"); return 0; }
    return 1;
}
