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

int main(void)
{
    const char *text = "1 + 2 * -3";
    const char *postfix_text = "f(1, [2])[0:1]";
    Py68Allocator allocator;
    Py68Source source;
    Py68TokenArray tokens;
    Py68AstArena arena;
    Py68ExpressionParser parser;
    Py68AstNode *node = NULL;
    Py68Error error;
    int passed = 1;

    py68_allocator_initialize(&allocator);
    passed &= check(py68_source_initialize(&allocator, &source, "expr.py",
        (const Py68U8 *)text, (Py68U32)strlen(text)) == PY68_STATUS_OK,
        "source initializes");
    passed &= check(py68_tokenize(&allocator, &source, &tokens, &error) ==
                    PY68_STATUS_OK, "expression tokenizes");
    py68_ast_arena_initialize(&arena, &allocator);
    parser.allocator = &allocator;
    parser.source = &source;
    parser.tokens = &tokens;
    parser.position = 0;
    parser.arena = &arena;
    parser.error = &error;
    passed &= check(py68_parse_expression(&parser, &node) == PY68_STATUS_OK,
                    "expression parses");
    passed &= check(node != NULL && node->kind == PY68_AST_BINARY,
                    "root is binary expression");
    passed &= check(node->as.binary.operator_kind == PY68_TOKEN_PLUS,
                    "addition is root precedence");
    passed &= check(node->as.binary.right->kind == PY68_AST_BINARY &&
                    node->as.binary.right->as.binary.operator_kind ==
                    PY68_TOKEN_STAR,
                    "multiplication binds tighter than addition");
    py68_ast_arena_destroy(&arena);
    py68_token_array_destroy(&allocator, &tokens);
    py68_source_destroy(&allocator, &source);
    passed &= check(allocator.stats.current_bytes == 0,
                    "parser test releases all allocations");

    py68_allocator_initialize(&allocator);
    passed &= check(py68_source_initialize(&allocator, &source, "postfix.py",
        (const Py68U8 *)postfix_text, (Py68U32)strlen(postfix_text)) ==
        PY68_STATUS_OK, "postfix source initializes");
    passed &= check(py68_tokenize(&allocator, &source, &tokens, &error) ==
                    PY68_STATUS_OK, "postfix expression tokenizes");
    py68_ast_arena_initialize(&arena, &allocator);
    parser.source = &source;
    parser.tokens = &tokens;
    parser.position = 0;
    parser.arena = &arena;
    parser.error = &error;
    passed &= check(py68_parse_expression(&parser, &node) == PY68_STATUS_OK,
                    "postfix expression parses");
    passed &= check(node != NULL && node->kind == PY68_AST_SLICE,
                    "slice is outer postfix node");
    passed &= check(node->as.slice.container->kind == PY68_AST_CALL,
                    "call is preserved under slice");
    py68_ast_arena_destroy(&arena);
    py68_token_array_destroy(&allocator, &tokens);
    py68_source_destroy(&allocator, &source);
    passed &= check(allocator.stats.current_bytes == 0,
                    "postfix parser releases all allocations");

    if (passed) {
        puts("PASS: expression parser tests");
        return 0;
    }
    return 1;
}