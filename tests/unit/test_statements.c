/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_ast_arena.h"
#include "py68k_error.h"
#include "py68k_memory.h"
#include "py68k_parser.h"
#include "py68k_source.h"
#include "py68k_token.h"
#include "py68k_tokenizer.h"

#include <stdio.h>
#include <string.h>

static int check(int condition, const char *message)
{
    if (!condition) fprintf(stderr, "FAIL: %s\n", message);
    return condition;
}

static int parse_module(Py68Allocator *allocator, const char *text,
                        Py68AstArena *arena, Py68AstNode **module,
                        Py68Error *error)
{
    Py68Source source;
    Py68TokenArray *tokens;
    Py68StatementParser parser;
    Py68Status status;

    tokens = (Py68TokenArray *)py68_alloc(allocator, PY68_MEM_TEMP,
                                          (Py68U32)sizeof(Py68TokenArray));
    if (tokens == NULL) return 0;
    if (py68_source_initialize(allocator, &source, "statements.py",
        (const Py68U8 *)text, (Py68U32)strlen(text)) != PY68_STATUS_OK) {
        py68_free(allocator, PY68_MEM_TEMP, tokens,
                  (Py68U32)sizeof(Py68TokenArray));
        return 0;
    }
    status = py68_tokenize(allocator, &source, tokens, error);
    if (status == PY68_STATUS_OK) {
        py68_ast_arena_initialize(arena, allocator);
        parser.expression.allocator = allocator;
        parser.expression.source = &source;
        parser.expression.tokens = tokens;
        parser.expression.position = 0;
        parser.expression.arena = arena;
        parser.expression.error = error;
        parser.inside_function = 0;
        parser.loop_depth = 0;
        status = py68_parse_module(&parser, module);
    }
    py68_token_array_destroy(allocator, tokens);
    py68_source_destroy(allocator, &source);
    py68_free(allocator, PY68_MEM_TEMP, tokens,
              (Py68U32)sizeof(Py68TokenArray));
    return status == PY68_STATUS_OK;
}

int main(void)
{
    const char *program =
        "x = 1\n"
        "if x:\n"
        "    y = 2\n"
        "elif y:\n"
        "    y += 1\n"
        "else:\n"
        "    pass\n"
        "while x:\n"
        "    break\n"
        "for item in items:\n"
        "    continue\n"
        "def add(a, b):\n"
        "    return a + b\n";
    Py68Allocator allocator;
    Py68AstArena arena;
    Py68AstNode *module = NULL;
    Py68Error error;
    int passed = 1;

    py68_allocator_initialize(&allocator);
    passed &= check(parse_module(&allocator, program, &arena, &module, &error),
                    "statement program parses");
    if (module != NULL) {
        passed &= check(module->kind == PY68_AST_MODULE,
                        "module node is created");
        passed &= check(module->as.module.statements.count == 5,
                        "module statement count is stable");
        passed &= check(module->as.module.statements.items[0]->kind ==
                        PY68_AST_ASSIGN, "assignment node is created");
        passed &= check(module->as.module.statements.items[1]->kind ==
                        PY68_AST_IF, "if node is created");
        passed &= check(module->as.module.statements.items[2]->kind ==
                        PY68_AST_WHILE, "while node is created");
        passed &= check(module->as.module.statements.items[3]->kind ==
                        PY68_AST_FOR, "for node is created");
        passed &= check(module->as.module.statements.items[4]->kind ==
                        PY68_AST_FUNCTION_DEF, "function node is created");
        py68_ast_arena_destroy(&arena);
    }
    check(!parse_module(&allocator, "return 1\n", &arena,
                        &module, &error),
          "return outside function is rejected");
    py68_ast_arena_destroy(&arena);
    check(!parse_module(&allocator, "break\n", &arena,
                        &module, &error),
          "break outside loop is rejected");
    py68_ast_arena_destroy(&arena);
    passed &= check(!parse_module(&allocator, "class Thing:\n    pass\n",
                                  &arena, &module, &error),
                    "class is rejected");
    passed &= check(error.kind == PY68_ERROR_SYNTAX &&
                    strstr(error.message, "class is not supported") != NULL,
                    "class diagnostic names the keyword");
    py68_ast_arena_destroy(&arena);
    passed &= check(!parse_module(&allocator, "from .x import y\n",
                                  &arena, &module, &error),
                    "relative import is rejected");
    passed &= check(strstr(error.message, "relative imports") != NULL,
                    "relative import diagnostic is targeted");
    py68_ast_arena_destroy(&arena);
    passed &= check(allocator.stats.current_bytes == 0,
                    "statement parser releases all allocations");
    if (passed) {
        puts("PASS: statement parser tests");
        return 0;
    }
    return 1;
}