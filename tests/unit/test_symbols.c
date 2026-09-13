/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_ast_arena.h"
#include "py68k_error.h"
#include "py68k_memory.h"
#include "py68k_parser.h"
#include "py68k_source.h"
#include "py68k_symbol.h"
#include "py68k_token.h"
#include "py68k_tokenizer.h"

#include <stdio.h>
#include <string.h>

static int check(int condition, const char *message)
{
    if (!condition) fprintf(stderr, "FAIL: %s\n", message);
    return condition;
}

static Py68Status parse_source(Py68Allocator *allocator, const char *text,
                               Py68Source *source, Py68TokenArray *tokens,
                               Py68AstArena *arena, Py68AstNode **module,
                               Py68Error *error)
{
    Py68StatementParser parser;
    Py68Status status;

    status = py68_source_initialize(allocator, source, "symbols.py",
                                    (const Py68U8 *)text,
                                    (Py68U32)strlen(text));
    if (status != PY68_STATUS_OK) return status;
    status = py68_tokenize(allocator, source, tokens, error);
    if (status != PY68_STATUS_OK) return status;
    py68_ast_arena_initialize(arena, allocator);
    parser.expression.allocator = allocator;
    parser.expression.source = source;
    parser.expression.tokens = tokens;
    parser.expression.position = 0;
    parser.expression.arena = arena;
    parser.expression.error = error;
    parser.inside_function = 0;
    parser.loop_depth = 0;
    return py68_parse_module(&parser, module);
}

int main(void)
{
    const char *text =
        "x = 10\n"
        "def f(value, other):\n"
        "    print(x)\n"
        "    x = value\n"
        "    for item in other:\n"
        "        x += item\n"
        "    return x\n";
    const char *duplicate = "def bad(value, value):\n    pass\n";
    const char *nested = "def outer():\n    def inner():\n        pass\n";
    const char *builtins[] = { "print", "range" };
    Py68Allocator allocator;
    Py68Source source;
    Py68TokenArray tokens;
    Py68AstArena arena;
    Py68AstNode *module = NULL;
    Py68SymbolAnalysis analysis;
    Py68Error error;
    Py68FunctionSymbols *function;
    Py68AstNode *print_expression;
    Py68AstNode *print_name;
    int passed = 1;

    py68_allocator_initialize(&allocator);
    py68_token_array_initialize(&tokens);
    passed &= check(parse_source(&allocator, text, &source, &tokens, &arena,
                                 &module, &error) == PY68_STATUS_OK,
                    "symbol fixture parses");
    passed &= check(py68_symbol_analyze(&allocator, &source, module,
                                        &analysis, &error) == PY68_STATUS_OK,
                    "symbol fixture analyzes");
    passed &= check(analysis.function_count == 1,
                    "one function record is created");
    function = &analysis.functions[0];
    passed &= check(function->parameter_count == 2,
                    "parameters occupy first symbols");
    passed &= check(function->parameters[0].slot == 0 &&
                    function->parameters[1].slot == 1,
                    "parameter slots are source ordered");
    passed &= check(function->local_count == 2,
                    "assignment and loop targets become locals");
    passed &= check(function->locals[0].slot == 2 &&
                    function->locals[1].slot == 3,
                    "local slots follow parameters deterministically");
    passed &= check(py68_symbol_is_unbound(function->locals[0].slot,
                                           function->parameter_count),
                    "non-parameter slots begin unbound");
    print_expression = function->function->as.function_def.body.items[0]
                       ->as.expression_statement.value;
    print_name = print_expression->as.call.callee;
    passed &= check(py68_symbol_classify(&source, function,
                                         &analysis,
                                         print_name->as.name.offset,
                                         print_name->as.name.length,
                                         builtins, 2) == PY68_SYMBOL_BUILTIN,
                    "unassigned print resolves to builtin");
    py68_symbol_analysis_destroy(&allocator, &analysis);
    py68_ast_arena_destroy(&arena);
    py68_token_array_destroy(&allocator, &tokens);
    py68_source_destroy(&allocator, &source);

    py68_token_array_initialize(&tokens);
    passed &= check(parse_source(&allocator, duplicate, &source, &tokens,
                                 &arena, &module, &error) == PY68_STATUS_OK,
                    "duplicate fixture parses");
    passed &= check(py68_symbol_analyze(&allocator, &source, module,
                                        &analysis, &error) ==
                    PY68_STATUS_SOURCE_ERROR && error.active != 0,
                    "duplicate parameters are rejected");
    py68_symbol_analysis_destroy(&allocator, &analysis);
    py68_ast_arena_destroy(&arena);
    py68_token_array_destroy(&allocator, &tokens);
    py68_source_destroy(&allocator, &source);

    py68_token_array_initialize(&tokens);
    passed &= check(parse_source(&allocator, nested, &source, &tokens,
                                 &arena, &module, &error) == PY68_STATUS_OK,
                    "nested fixture parses");
    passed &= check(py68_symbol_analyze(&allocator, &source, module,
                                        &analysis, &error) ==
                    PY68_STATUS_SOURCE_ERROR,
                    "nested functions are rejected");
    py68_symbol_analysis_destroy(&allocator, &analysis);
    py68_ast_arena_destroy(&arena);
    py68_token_array_destroy(&allocator, &tokens);
    py68_source_destroy(&allocator, &source);
    passed &= check(allocator.stats.current_bytes == 0,
                    "symbol analysis releases all allocations");
    if (passed) {
        puts("PASS: symbol analysis tests");
        return 0;
    }
    return 1;
}
