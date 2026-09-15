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

    {
        const char *comp_text = "[x * 2 for x in items if x]";
        py68_allocator_initialize(&allocator);
        passed &= check(py68_source_initialize(&allocator, &source, "comp.py",
            (const Py68U8 *)comp_text, (Py68U32)strlen(comp_text)) ==
            PY68_STATUS_OK, "comprehension source initializes");
        passed &= check(py68_tokenize(&allocator, &source, &tokens, &error) ==
                        PY68_STATUS_OK, "comprehension tokenizes");
        py68_ast_arena_initialize(&arena, &allocator);
        parser.source = &source;
        parser.tokens = &tokens;
        parser.position = 0;
        parser.arena = &arena;
        parser.error = &error;
        passed &= check(py68_parse_expression(&parser, &node) == PY68_STATUS_OK,
                        "list comprehension parses");
        passed &= check(node != NULL && node->kind == PY68_AST_LIST_COMP,
                        "root is list comprehension");
        passed &= check(node->as.comprehension.generators.count == 1,
                        "one generator clause");
        passed &= check(node->as.comprehension.generators.items[0]
                            ->as.comprehension_for.ifs.count == 1,
                        "one if filter is attached to the for clause");
        py68_ast_arena_destroy(&arena);
        py68_token_array_destroy(&allocator, &tokens);
        py68_source_destroy(&allocator, &source);
        passed &= check(allocator.stats.current_bytes == 0,
                        "comprehension parser releases all allocations");
    }

    {
        const char *in_text = "a in b";
        const char *not_in_text = "a not in b";
        py68_allocator_initialize(&allocator);
        passed &= check(py68_source_initialize(&allocator, &source, "in.py",
            (const Py68U8 *)in_text, (Py68U32)strlen(in_text)) ==
            PY68_STATUS_OK, "membership source initializes");
        passed &= check(py68_tokenize(&allocator, &source, &tokens, &error) ==
                        PY68_STATUS_OK, "membership tokenizes");
        py68_ast_arena_initialize(&arena, &allocator);
        parser.source = &source;
        parser.tokens = &tokens;
        parser.position = 0;
        parser.arena = &arena;
        parser.error = &error;
        passed &= check(py68_parse_expression(&parser, &node) == PY68_STATUS_OK,
                        "a in b parses");
        passed &= check(node != NULL && node->kind == PY68_AST_BINARY &&
                        node->as.binary.operator_kind == PY68_TOKEN_IN,
                        "in is binary comparison");
        py68_ast_arena_destroy(&arena);
        py68_token_array_destroy(&allocator, &tokens);
        py68_source_destroy(&allocator, &source);
        passed &= check(allocator.stats.current_bytes == 0,
                        "membership parser releases all allocations");

        py68_allocator_initialize(&allocator);
        passed &= check(py68_source_initialize(&allocator, &source, "notin.py",
            (const Py68U8 *)not_in_text, (Py68U32)strlen(not_in_text)) ==
            PY68_STATUS_OK, "not-in source initializes");
        passed &= check(py68_tokenize(&allocator, &source, &tokens, &error) ==
                        PY68_STATUS_OK, "not-in tokenizes");
        py68_ast_arena_initialize(&arena, &allocator);
        parser.source = &source;
        parser.tokens = &tokens;
        parser.position = 0;
        parser.arena = &arena;
        parser.error = &error;
        passed &= check(py68_parse_expression(&parser, &node) == PY68_STATUS_OK,
                        "a not in b parses");
        passed &= check(node != NULL && node->kind == PY68_AST_BINARY &&
                        node->as.binary.operator_kind == PY68_TOKEN_NOT_IN,
                        "not in is compound binary comparison");
        py68_ast_arena_destroy(&arena);
        py68_token_array_destroy(&allocator, &tokens);
        py68_source_destroy(&allocator, &source);
        passed &= check(allocator.stats.current_bytes == 0,
                        "not-in parser releases all allocations");
    }

    {
        const char *gen_text = "(x for x in items)";
        py68_allocator_initialize(&allocator);
        passed &= check(py68_source_initialize(&allocator, &source, "gen.py",
            (const Py68U8 *)gen_text, (Py68U32)strlen(gen_text)) ==
            PY68_STATUS_OK, "generator-expression source initializes");
        passed &= check(py68_tokenize(&allocator, &source, &tokens, &error) ==
                        PY68_STATUS_OK, "generator-expression tokenizes");
        py68_ast_arena_initialize(&arena, &allocator);
        parser.source = &source;
        parser.tokens = &tokens;
        parser.position = 0;
        parser.arena = &arena;
        parser.error = &error;
        passed &= check(py68_parse_expression(&parser, &node) != PY68_STATUS_OK,
                        "generator expressions are rejected");
        passed &= check(strstr(error.message, "generator expressions") != NULL,
                        "generator-expression diagnostic is targeted");
        py68_ast_arena_destroy(&arena);
        py68_token_array_destroy(&allocator, &tokens);
        py68_source_destroy(&allocator, &source);
        passed &= check(allocator.stats.current_bytes == 0,
                        "rejected generator expression releases allocations");
    }

    if (passed) {
        puts("PASS: expression parser tests");
        return 0;
    }
    return 1;
}