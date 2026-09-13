/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_PARSER_H
#define PY68K_PARSER_H

#include "py68k_ast_arena.h"
#include "py68k_error.h"
#include "py68k_source.h"
#include "py68k_token.h"

typedef struct Py68ExpressionParser {
    Py68Allocator *allocator;
    const Py68Source *source;
    const Py68TokenArray *tokens;
    Py68U32 position;
    Py68AstArena *arena;
    Py68Error *error;
} Py68ExpressionParser;

typedef struct Py68StatementParser {
    Py68ExpressionParser expression;
    Py68U16 inside_function;
    Py68U16 loop_depth;
} Py68StatementParser;

Py68Status py68_parse_expression(Py68ExpressionParser *parser,
                                 Py68AstNode **node_out);
Py68Status py68_parse_module(Py68StatementParser *parser,
                             Py68AstNode **module_out);

#endif