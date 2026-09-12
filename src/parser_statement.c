#include "py68k_parser.h"

#include <stddef.h>

static Py68Token *py68_statement_current(Py68StatementParser *parser)
{
    if (parser->expression.position >= parser->expression.tokens->count) {
        return NULL;
    }
    return &parser->expression.tokens->items[parser->expression.position];
}

static Py68Status py68_statement_error(Py68StatementParser *parser,
                                       Py68Token *token,
                                       const char *message)
{
    Py68Location location;
    if (token == NULL) {
        location.offset = parser->expression.source->length;
        location.line = 0;
        location.column = 0;
        location.length = 0;
    } else {
        location = token->location;
    }
    py68_error_set(parser->expression.error, PY68_ERROR_SYNTAX, location,
                   parser->expression.source->filename, message);
    return PY68_STATUS_SOURCE_ERROR;
}

static int py68_statement_accept(Py68StatementParser *parser,
                                 Py68TokenKind kind)
{
    Py68Token *token = py68_statement_current(parser);
    if (token != NULL && token->kind == kind) {
        ++parser->expression.position;
        return 1;
    }
    return 0;
}

static Py68Status py68_statement_new(Py68StatementParser *parser,
                                     Py68AstKind kind, Py68Token *token,
                                     Py68AstNode **node_out)
{
    return py68_ast_arena_new(parser->expression.arena, kind,
                              token->location, node_out);
}

static int py68_statement_needs_newline(Py68AstKind kind)
{
    return kind == PY68_AST_ASSIGN || kind == PY68_AST_AUGMENTED_ASSIGN ||
           kind == PY68_AST_RETURN || kind == PY68_AST_BREAK ||
           kind == PY68_AST_CONTINUE || kind == PY68_AST_PASS ||
           kind == PY68_AST_EXPRESSION_STATEMENT;
}

static Py68Status py68_parse_suite(Py68StatementParser *parser,
                                   Py68AstList *body);

static Py68Status py68_parse_statement(Py68StatementParser *parser,
                                        Py68AstNode **node_out)
{
    Py68Token *token = py68_statement_current(parser);
    Py68AstNode *node;
    Py68AstNode *chain;
    Py68AstNode *value;
    Py68Status status;

    if (token == NULL) return py68_statement_error(parser, token,
                                                   "expected statement");
    if (token->kind == PY68_TOKEN_IF) {
        Py68AstNode *condition;
        ++parser->expression.position;
        status = py68_parse_expression(&parser->expression, &condition);
        if (status != PY68_STATUS_OK || !py68_statement_accept(parser,
                                                               PY68_TOKEN_COLON)) {
            return py68_statement_error(parser, py68_statement_current(parser),
                                        "expected condition and colon");
        }
        status = py68_statement_new(parser, PY68_AST_IF, token, &node);
        if (status != PY68_STATUS_OK) return status;
        node->as.if_statement.condition = condition;
        py68_ast_list_initialize(&node->as.if_statement.body);
        py68_ast_list_initialize(&node->as.if_statement.else_body);
        chain = node;
        status = py68_parse_suite(parser, &node->as.if_statement.body);
        if (status != PY68_STATUS_OK) return status;
        while (py68_statement_accept(parser, PY68_TOKEN_ELIF)) {
            Py68AstNode *elif_node;
            Py68AstNode *elif_condition;
            Py68Token *elif_token = &parser->expression.tokens->items[
                parser->expression.position - 1];
            status = py68_parse_expression(&parser->expression,
                                           &elif_condition);
            if (status != PY68_STATUS_OK ||
                !py68_statement_accept(parser, PY68_TOKEN_COLON)) {
                return py68_statement_error(parser,
                                            py68_statement_current(parser),
                                            "expected elif condition and colon");
            }
            status = py68_statement_new(parser, PY68_AST_IF, elif_token,
                                        &elif_node);
            if (status != PY68_STATUS_OK) return status;
            elif_node->as.if_statement.condition = elif_condition;
            py68_ast_list_initialize(&elif_node->as.if_statement.body);
            py68_ast_list_initialize(&elif_node->as.if_statement.else_body);
            status = py68_parse_suite(parser, &elif_node->as.if_statement.body);
            if (status != PY68_STATUS_OK) return status;
            status = py68_ast_list_append(parser->expression.arena,
                                          &chain->as.if_statement.else_body,
                                          elif_node);
            if (status != PY68_STATUS_OK) return status;
            chain = elif_node;
        }
        if (py68_statement_accept(parser, PY68_TOKEN_ELSE)) {
            if (!py68_statement_accept(parser, PY68_TOKEN_COLON)) {
                return py68_statement_error(parser,
                                            py68_statement_current(parser),
                                            "expected else colon");
            }
            status = py68_parse_suite(parser, &chain->as.if_statement.else_body);
            if (status != PY68_STATUS_OK) return status;
        }
        *node_out = node;
        return PY68_STATUS_OK;
    }
    if (token->kind == PY68_TOKEN_WHILE) {
        ++parser->expression.position;
        status = py68_parse_expression(&parser->expression, &value);
        if (status != PY68_STATUS_OK || !py68_statement_accept(parser,
                                                               PY68_TOKEN_COLON)) {
            return py68_statement_error(parser, py68_statement_current(parser),
                                        "expected while condition and colon");
        }
        status = py68_statement_new(parser, PY68_AST_WHILE, token, &node);
        if (status != PY68_STATUS_OK) return status;
        node->as.while_statement.condition = value;
        py68_ast_list_initialize(&node->as.while_statement.body);
        py68_ast_list_initialize(&node->as.while_statement.else_body);
        ++parser->loop_depth;
        status = py68_parse_suite(parser, &node->as.while_statement.body);
        --parser->loop_depth;
        if (status != PY68_STATUS_OK) return status;
        if (py68_statement_accept(parser, PY68_TOKEN_ELSE)) {
            if (!py68_statement_accept(parser, PY68_TOKEN_COLON)) {
                return py68_statement_error(parser,
                                            py68_statement_current(parser),
                                            "expected else colon");
            }
            status = py68_parse_suite(parser, &node->as.while_statement.else_body);
            if (status != PY68_STATUS_OK) return status;
        }
        *node_out = node;
        return PY68_STATUS_OK;
    }
    if (token->kind == PY68_TOKEN_FOR) {
        Py68Token *name_token;
        ++parser->expression.position;
        name_token = py68_statement_current(parser);
        if (name_token == NULL || name_token->kind != PY68_TOKEN_NAME) {
            return py68_statement_error(parser, name_token,
                                        "expected loop variable");
        }
        ++parser->expression.position;
        if (!py68_statement_accept(parser, PY68_TOKEN_IN)) {
            return py68_statement_error(parser, py68_statement_current(parser),
                                        "expected in after loop variable");
        }
        status = py68_parse_expression(&parser->expression, &value);
        if (status != PY68_STATUS_OK || !py68_statement_accept(parser,
                                                               PY68_TOKEN_COLON)) {
            return py68_statement_error(parser, py68_statement_current(parser),
                                        "expected iterable and colon");
        }
        status = py68_statement_new(parser, PY68_AST_FOR, token, &node);
        if (status != PY68_STATUS_OK) return status;
        if (status != PY68_STATUS_OK) return status;
        node->as.for_statement.name_offset = name_token->location.offset;
        node->as.for_statement.name_length = name_token->location.length;
        node->as.for_statement.iterable = value;
        py68_ast_list_initialize(&node->as.for_statement.body);
        py68_ast_list_initialize(&node->as.for_statement.else_body);
        ++parser->loop_depth;
        status = py68_parse_suite(parser, &node->as.for_statement.body);
        --parser->loop_depth;
        if (status != PY68_STATUS_OK) return status;
        if (py68_statement_accept(parser, PY68_TOKEN_ELSE)) {
            if (!py68_statement_accept(parser, PY68_TOKEN_COLON)) {
                return py68_statement_error(parser,
                                            py68_statement_current(parser),
                                            "expected else colon");
            }
            status = py68_parse_suite(parser, &node->as.for_statement.else_body);
            if (status != PY68_STATUS_OK) return status;
        }
        *node_out = node;
        return PY68_STATUS_OK;
    }
    if (token->kind == PY68_TOKEN_DEF) {
        Py68Token *name_token;
        ++parser->expression.position;
        name_token = py68_statement_current(parser);
        if (name_token == NULL || name_token->kind != PY68_TOKEN_NAME) {
            return py68_statement_error(parser, name_token,
                                        "expected function name");
        }
        ++parser->expression.position;
        if (!py68_statement_accept(parser, PY68_TOKEN_LEFT_PAREN)) {
            return py68_statement_error(parser, py68_statement_current(parser),
                                        "expected opening parenthesis");
        }
        status = py68_statement_new(parser, PY68_AST_FUNCTION_DEF, token, &node);
        if (status != PY68_STATUS_OK) return status;
        node->as.function_def.name_offset = name_token->location.offset;
        node->as.function_def.name_length = name_token->location.length;
        py68_ast_list_initialize(&node->as.function_def.parameters);
        py68_ast_list_initialize(&node->as.function_def.body);
        if (!py68_statement_accept(parser, PY68_TOKEN_RIGHT_PAREN)) {
            for (;;) {
                Py68Token *parameter = py68_statement_current(parser);
                Py68AstNode *parameter_node;
                if (parameter == NULL || parameter->kind != PY68_TOKEN_NAME) {
                    return py68_statement_error(parser, parameter,
                                                "expected parameter name");
                }
                status = py68_ast_arena_new(parser->expression.arena,
                                            PY68_AST_NAME,
                                            parameter->location,
                                            &parameter_node);
                if (status != PY68_STATUS_OK) return status;
                parameter_node->as.name.offset = parameter->location.offset;
                parameter_node->as.name.length = parameter->location.length;
                ++parser->expression.position;
                status = py68_ast_list_append(parser->expression.arena,
                                              &node->as.function_def.parameters,
                                              parameter_node);
                if (status != PY68_STATUS_OK) return status;
                if (py68_statement_accept(parser, PY68_TOKEN_RIGHT_PAREN)) break;
                if (!py68_statement_accept(parser, PY68_TOKEN_COMMA)) {
                    return py68_statement_error(parser,
                                                py68_statement_current(parser),
                                                "expected parameter comma");
                }
                if (py68_statement_accept(parser, PY68_TOKEN_RIGHT_PAREN)) break;
            }
        }
        if (!py68_statement_accept(parser, PY68_TOKEN_COLON)) {
            return py68_statement_error(parser, py68_statement_current(parser),
                                        "expected function colon");
        }
        {
            Py68U16 old_inside = parser->inside_function;
            Py68U16 old_loop = parser->loop_depth;
            parser->inside_function = 1;
            parser->loop_depth = 0;
            status = py68_parse_suite(parser, &node->as.function_def.body);
            parser->inside_function = old_inside;
            parser->loop_depth = old_loop;
        }
        if (status != PY68_STATUS_OK) return status;
        *node_out = node;
        return PY68_STATUS_OK;
    }
    if (token->kind == PY68_TOKEN_RETURN) {
        ++parser->expression.position;
        if (!parser->inside_function) {
            return py68_statement_error(parser, token,
                                        "return outside function");
        }
        status = py68_statement_new(parser, PY68_AST_RETURN, token, &node);
        if (status != PY68_STATUS_OK) return status;
        if (py68_statement_current(parser)->kind == PY68_TOKEN_NEWLINE) {
            value = NULL;
        } else {
            status = py68_parse_expression(&parser->expression, &value);
            if (status != PY68_STATUS_OK) return status;
        }
        node->as.return_statement.value = value;
        *node_out = node;
        return PY68_STATUS_OK;
    }
    if (token->kind == PY68_TOKEN_BREAK || token->kind == PY68_TOKEN_CONTINUE) {
        Py68AstKind kind = token->kind == PY68_TOKEN_BREAK ?
                           PY68_AST_BREAK : PY68_AST_CONTINUE;
        if (parser->loop_depth == 0) {
            return py68_statement_error(parser, token,
                                        "loop control outside loop");
        }
        ++parser->expression.position;
        status = py68_statement_new(parser, kind, token, &node);
        *node_out = node;
        return status;
    }
    if (token->kind == PY68_TOKEN_PASS) {
        ++parser->expression.position;
        status = py68_statement_new(parser, PY68_AST_PASS, token, &node);
        *node_out = node;
        return status;
    }
    if (token->kind == PY68_TOKEN_NAME &&
        parser->expression.position + 1 < parser->expression.tokens->count) {
        Py68TokenKind next_kind = parser->expression.tokens->items[
            parser->expression.position + 1].kind;
        if (next_kind == PY68_TOKEN_ASSIGN ||
            next_kind == PY68_TOKEN_PLUS_ASSIGN ||
            next_kind == PY68_TOKEN_MINUS_ASSIGN ||
            next_kind == PY68_TOKEN_STAR_ASSIGN ||
            next_kind == PY68_TOKEN_FLOOR_DIVIDE_ASSIGN ||
            next_kind == PY68_TOKEN_PERCENT_ASSIGN) {
            Py68Token target_token = *token;
            ++parser->expression.position;
            ++parser->expression.position;
            status = py68_parse_expression(&parser->expression, &value);
            if (status != PY68_STATUS_OK) return status;
            if (next_kind == PY68_TOKEN_ASSIGN) {
                status = py68_statement_new(parser, PY68_AST_ASSIGN,
                                            &target_token, &node);
                if (status != PY68_STATUS_OK) return status;
                node->as.assign.name_offset = target_token.location.offset;
                node->as.assign.name_length = target_token.location.length;
                node->as.assign.value = value;
            } else {
                Py68AstNode *target;
                status = py68_statement_new(parser, PY68_AST_AUGMENTED_ASSIGN,
                                            &target_token, &node);
                if (status != PY68_STATUS_OK) return status;
                status = py68_ast_arena_new(parser->expression.arena,
                                            PY68_AST_NAME,
                                            target_token.location, &target);
                if (status != PY68_STATUS_OK) return status;
                target->as.name.offset = target_token.location.offset;
                target->as.name.length = target_token.location.length;
                node->as.augmented_assign.operator_kind = next_kind;
                node->as.augmented_assign.target = target;
                node->as.augmented_assign.value = value;
            }
            *node_out = node;
            return PY68_STATUS_OK;
        }
    }
    status = py68_parse_expression(&parser->expression, &value);
    if (status != PY68_STATUS_OK) return status;
    status = py68_statement_new(parser, PY68_AST_EXPRESSION_STATEMENT,
                                token, &node);
    if (status == PY68_STATUS_OK) node->as.expression_statement.value = value;
    *node_out = node;
    return status;
}

static Py68Status py68_parse_suite(Py68StatementParser *parser,
                                   Py68AstList *body)
{
    Py68AstNode *statement;
    Py68Status status;
    if (py68_statement_accept(parser, PY68_TOKEN_NEWLINE)) {
        if (!py68_statement_accept(parser, PY68_TOKEN_INDENT)) {
            return py68_statement_error(parser, py68_statement_current(parser),
                                        "expected indented suite");
        }
        while (py68_statement_current(parser) != NULL &&
               py68_statement_current(parser)->kind != PY68_TOKEN_DEDENT &&
               py68_statement_current(parser)->kind != PY68_TOKEN_EOF) {
            if (py68_statement_accept(parser, PY68_TOKEN_NEWLINE)) continue;
            status = py68_parse_statement(parser, &statement);
            if (status != PY68_STATUS_OK) return status;
            status = py68_ast_list_append(parser->expression.arena, body,
                                          statement);
            if (status != PY68_STATUS_OK) return status;
            if (!py68_statement_accept(parser, PY68_TOKEN_NEWLINE) &&
                py68_statement_current(parser) != NULL &&
                py68_statement_current(parser)->kind != PY68_TOKEN_DEDENT &&
                py68_statement_needs_newline(statement->kind)) {
                return py68_statement_error(parser,
                                            py68_statement_current(parser),
                                            "expected statement newline");
            }
        }
        if (!py68_statement_accept(parser, PY68_TOKEN_DEDENT)) {
            return py68_statement_error(parser, py68_statement_current(parser),
                                        "expected dedent");
        }
        return PY68_STATUS_OK;
    }
    status = py68_parse_statement(parser, &statement);
    if (status != PY68_STATUS_OK) return status;
    status = py68_ast_list_append(parser->expression.arena, body, statement);
    if (status != PY68_STATUS_OK) return status;
    if (py68_statement_needs_newline(statement->kind) &&
        !py68_statement_accept(parser, PY68_TOKEN_NEWLINE)) {
        return py68_statement_error(parser, py68_statement_current(parser),
                                    "expected suite newline");
    }
    return PY68_STATUS_OK;
}

Py68Status py68_parse_module(Py68StatementParser *parser,
                             Py68AstNode **module_out)
{
    Py68AstNode *module;
    Py68AstNode *statement;
    Py68Token token;
    Py68Status status;

    token = *py68_statement_current(parser);
    status = py68_ast_arena_new(parser->expression.arena, PY68_AST_MODULE,
                                token.location, &module);
    if (status != PY68_STATUS_OK) return status;
    py68_ast_list_initialize(&module->as.module.statements);
    while (py68_statement_current(parser) != NULL &&
           py68_statement_current(parser)->kind != PY68_TOKEN_EOF) {
        if (py68_statement_accept(parser, PY68_TOKEN_NEWLINE)) continue;
        status = py68_parse_statement(parser, &statement);
        if (status != PY68_STATUS_OK) return status;
        status = py68_ast_list_append(parser->expression.arena,
                                      &module->as.module.statements,
                                      statement);
        if (status != PY68_STATUS_OK) return status;
        if (py68_statement_needs_newline(statement->kind) &&
            !py68_statement_accept(parser, PY68_TOKEN_NEWLINE)) {
            return py68_statement_error(parser,
                                        py68_statement_current(parser),
                                        "expected statement newline");
        }
    }
    *module_out = module;
    return PY68_STATUS_OK;
}