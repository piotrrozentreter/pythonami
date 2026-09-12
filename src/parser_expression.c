#include "py68k_parser.h"

#include <stddef.h>

static Py68Token *py68_current(Py68ExpressionParser *parser)
{
    if (parser->position >= parser->tokens->count) {
        return NULL;
    }
    return &parser->tokens->items[parser->position];
}

static void py68_parser_error(Py68ExpressionParser *parser,
                              Py68Location location, const char *message)
{
    py68_error_set(parser->error, PY68_ERROR_SYNTAX, location,
                   parser->source->filename, message);
}

static int py68_accept(Py68ExpressionParser *parser, Py68TokenKind kind)
{
    Py68Token *token = py68_current(parser);
    if (token != NULL && token->kind == kind) {
        ++parser->position;
        return 1;
    }
    return 0;
}

static Py68Status py68_integer_value(const Py68U8 *data, Py68U32 start,
                                     Py68U16 length, Py68I32 *value_out)
{
    Py68U32 position = start;
    Py68U32 end = start + length;
    Py68U32 base = 10;
    Py68U32 value = 0;
    Py68U32 digit;
    Py68I32 signed_value;

    if (length >= 2 && data[start] == (Py68U8)'0') {
        if (data[start + 1] == (Py68U8)'x' ||
            data[start + 1] == (Py68U8)'X') { base = 16; position += 2; }
        else if (data[start + 1] == (Py68U8)'b' ||
                 data[start + 1] == (Py68U8)'B') { base = 2; position += 2; }
        else if (data[start + 1] == (Py68U8)'o' ||
                 data[start + 1] == (Py68U8)'O') { base = 8; position += 2; }
    }
    while (position < end) {
        if (data[position] == (Py68U8)'_') {
            ++position;
            continue;
        }
        if (data[position] >= (Py68U8)'0' &&
            data[position] <= (Py68U8)'9') {
            digit = (Py68U32)(data[position] - (Py68U8)'0');
        } else if (data[position] >= (Py68U8)'a' &&
                   data[position] <= (Py68U8)'f') {
            digit = (Py68U32)(data[position] - (Py68U8)'a') + 10;
        } else {
            digit = (Py68U32)(data[position] - (Py68U8)'A') + 10;
        }
        value = value * base + digit;
        ++position;
    }
    if (value > 2147483648UL) {
        return PY68_STATUS_SOURCE_ERROR;
    }
    if (value == 2147483648UL) {
        signed_value = (Py68I32)-2147483647 - 1;
    } else {
        signed_value = (Py68I32)value;
    }
    *value_out = signed_value;
    return PY68_STATUS_OK;
}

static int py68_precedence(Py68TokenKind kind)
{
    if (kind == PY68_TOKEN_OR) return 1;
    if (kind == PY68_TOKEN_AND) return 2;
    if (kind >= PY68_TOKEN_EQUAL && kind <= PY68_TOKEN_GREATER_EQUAL) return 3;
    if (kind == PY68_TOKEN_PLUS || kind == PY68_TOKEN_MINUS) return 4;
    if (kind == PY68_TOKEN_STAR || kind == PY68_TOKEN_FLOOR_DIVIDE ||
        kind == PY68_TOKEN_PERCENT) return 5;
    return 0;
}

static Py68Status py68_parse_primary(Py68ExpressionParser *parser,
                                     Py68AstNode **node_out)
{
    Py68Token *token = py68_current(parser);
    Py68AstNode *node;
    Py68Status status;
    Py68I32 value;

    if (token == NULL) return PY68_STATUS_SOURCE_ERROR;
    if (token->kind == PY68_TOKEN_INTEGER) {
        status = py68_integer_value(parser->source->data,
                                    token->location.offset,
                                    token->location.length, &value);
        if (status != PY68_STATUS_OK) return status;
        status = py68_ast_arena_new(parser->arena, PY68_AST_INTEGER,
                                    token->location, &node);
        if (status == PY68_STATUS_OK) node->as.integer_literal.value = value;
        ++parser->position;
        *node_out = node;
        return status;
    }
    if (token->kind == PY68_TOKEN_STRING) {
        status = py68_ast_arena_new(parser->arena, PY68_AST_STRING,
                                    token->location, &node);
        if (status == PY68_STATUS_OK) {
            node->as.string_literal.offset = token->location.offset + 1;
            node->as.string_literal.length = token->location.length - 2;
            node->as.string_literal.quote_flags =
                (Py68U16)parser->source->data[token->location.offset];
        }
        ++parser->position;
        *node_out = node;
        return status;
    }
    if (token->kind == PY68_TOKEN_NAME) {
        status = py68_ast_arena_new(parser->arena, PY68_AST_NAME,
                                    token->location, &node);
        if (status == PY68_STATUS_OK) {
            node->as.name.offset = token->location.offset;
            node->as.name.length = token->location.length;
        }
        ++parser->position;
        *node_out = node;
        return status;
    }
    if (token->kind == PY68_TOKEN_TRUE || token->kind == PY68_TOKEN_FALSE) {
        status = py68_ast_arena_new(parser->arena, PY68_AST_BOOL,
                                    token->location, &node);
        if (status == PY68_STATUS_OK) {
            node->as.boolean_literal.value = token->kind == PY68_TOKEN_TRUE;
        }
        ++parser->position;
        *node_out = node;
        return status;
    }
    if (token->kind == PY68_TOKEN_NONE) {
        status = py68_ast_arena_new(parser->arena, PY68_AST_NONE,
                                    token->location, &node);
        ++parser->position;
        *node_out = node;
        return status;
    }
    if (py68_accept(parser, PY68_TOKEN_LEFT_BRACKET)) {
        status = py68_ast_arena_new(parser->arena, PY68_AST_LIST,
                                    token->location, &node);
        if (status != PY68_STATUS_OK) return status;
        py68_ast_list_initialize(&node->as.list_literal.elements);
        if (!py68_accept(parser, PY68_TOKEN_RIGHT_BRACKET)) {
            for (;;) {
                Py68AstNode *element;
                status = py68_parse_expression(parser, &element);
                if (status != PY68_STATUS_OK) return status;
                status = py68_ast_list_append(parser->arena,
                                              &node->as.list_literal.elements,
                                              element);
                if (status != PY68_STATUS_OK) return status;
                if (py68_accept(parser, PY68_TOKEN_RIGHT_BRACKET)) break;
                if (!py68_accept(parser, PY68_TOKEN_COMMA)) {
                    token = py68_current(parser);
                    py68_parser_error(parser, token->location,
                                      "expected comma or closing bracket");
                    return PY68_STATUS_SOURCE_ERROR;
                }
                if (py68_accept(parser, PY68_TOKEN_RIGHT_BRACKET)) break;
            }
        }
        node->location.length = (Py68U16)(py68_current(parser)->location.offset -
                                          node->location.offset);
        *node_out = node;
        return PY68_STATUS_OK;
    }
    if (py68_accept(parser, PY68_TOKEN_LEFT_PAREN)) {
        status = py68_parse_expression(parser, node_out);
        if (status != PY68_STATUS_OK ||
            !py68_accept(parser, PY68_TOKEN_RIGHT_PAREN)) {
            token = py68_current(parser);
            py68_parser_error(parser, token->location,
                              "expected closing parenthesis");
            return PY68_STATUS_SOURCE_ERROR;
        }
        return status;
    }
    py68_parser_error(parser, token->location, "expected expression");
    return PY68_STATUS_SOURCE_ERROR;
}

static Py68Status py68_parse_postfix(Py68ExpressionParser *parser,
                                     Py68AstNode **node_out);

static Py68Status py68_parse_unary(Py68ExpressionParser *parser,
                                   Py68AstNode **node_out)
{
    Py68Token *token = py68_current(parser);
    Py68AstNode *node;
    Py68AstNode *operand;
    Py68Status status;

    if (token != NULL && (token->kind == PY68_TOKEN_PLUS ||
                          token->kind == PY68_TOKEN_MINUS ||
                          token->kind == PY68_TOKEN_NOT)) {
        ++parser->position;
        status = py68_parse_unary(parser, &operand);
        if (status != PY68_STATUS_OK) return status;
        status = py68_ast_arena_new(parser->arena, PY68_AST_UNARY,
                                    token->location, &node);
        if (status != PY68_STATUS_OK) return status;
        node->as.unary.operator_kind = token->kind;
        node->as.unary.operand = operand;
        node->location.length = (Py68U16)(operand->location.offset +
                                          operand->location.length -
                                          token->location.offset);
        *node_out = node;
        return PY68_STATUS_OK;
    }
    return py68_parse_postfix(parser, node_out);
}

static Py68Status py68_parse_postfix(Py68ExpressionParser *parser,
                                     Py68AstNode **node_out)
{
    Py68AstNode *node;
    Py68AstNode *argument;
    Py68Token *token;
    Py68Status status = py68_parse_primary(parser, &node);

    if (status != PY68_STATUS_OK) return status;
    for (;;) {
        token = py68_current(parser);
        if (token == NULL) break;
        if (py68_accept(parser, PY68_TOKEN_LEFT_PAREN)) {
            Py68AstNode *call;
            status = py68_ast_arena_new(parser->arena, PY68_AST_CALL,
                                        node->location, &call);
            if (status != PY68_STATUS_OK) return status;
            call->as.call.callee = node;
            py68_ast_list_initialize(&call->as.call.arguments);
            if (!py68_accept(parser, PY68_TOKEN_RIGHT_PAREN)) {
                for (;;) {
                    status = py68_parse_expression(parser, &argument);
                    if (status != PY68_STATUS_OK) return status;
                    status = py68_ast_list_append(
                        parser->arena, &call->as.call.arguments, argument);
                    if (status != PY68_STATUS_OK) return status;
                    if (py68_accept(parser, PY68_TOKEN_RIGHT_PAREN)) break;
                    if (!py68_accept(parser, PY68_TOKEN_COMMA)) {
                        token = py68_current(parser);
                        py68_parser_error(parser, token->location,
                                          "expected comma or closing parenthesis");
                        return PY68_STATUS_SOURCE_ERROR;
                    }
                    if (py68_accept(parser, PY68_TOKEN_RIGHT_PAREN)) break;
                }
            }
            call->location.length = (Py68U16)(py68_current(parser)->location.offset -
                                              call->location.offset);
            node = call;
            continue;
        }
        if (py68_accept(parser, PY68_TOKEN_LEFT_BRACKET)) {
            Py68AstNode *container = node;
            Py68AstNode *index = NULL;
            Py68AstNode *start = NULL;
            Py68AstNode *end = NULL;
            Py68AstNode *suffix;
            if (!py68_accept(parser, PY68_TOKEN_COLON)) {
                status = py68_parse_expression(parser, &index);
                if (status != PY68_STATUS_OK) return status;
            }
            if (index != NULL && py68_accept(parser, PY68_TOKEN_RIGHT_BRACKET)) {
                status = py68_ast_arena_new(parser->arena, PY68_AST_INDEX,
                                            container->location, &suffix);
                if (status != PY68_STATUS_OK) return status;
                suffix->as.index.container = container;
                suffix->as.index.index = index;
                suffix->location.length =
                    (Py68U16)(py68_current(parser)->location.offset -
                              suffix->location.offset);
                node = suffix;
                continue;
            }
            if (index == NULL) {
                start = NULL;
            } else {
                start = index;
            }
            if (start == NULL && !py68_accept(parser, PY68_TOKEN_COLON)) {
                token = py68_current(parser);
                py68_parser_error(parser, token->location,
                                  "expected slice bound or closing bracket");
                return PY68_STATUS_SOURCE_ERROR;
            }
            if (start != NULL && !py68_accept(parser, PY68_TOKEN_COLON)) {
                token = py68_current(parser);
                py68_parser_error(parser, token->location,
                                  "expected colon in slice");
                return PY68_STATUS_SOURCE_ERROR;
            }
            if (!py68_accept(parser, PY68_TOKEN_RIGHT_BRACKET)) {
                status = py68_parse_expression(parser, &end);
                if (status != PY68_STATUS_OK ||
                    !py68_accept(parser, PY68_TOKEN_RIGHT_BRACKET)) {
                    token = py68_current(parser);
                    py68_parser_error(parser, token->location,
                                      "expected closing bracket");
                    return PY68_STATUS_SOURCE_ERROR;
                }
            }
            status = py68_ast_arena_new(parser->arena, PY68_AST_SLICE,
                                        container->location, &suffix);
            if (status != PY68_STATUS_OK) return status;
            suffix->as.slice.container = container;
            suffix->as.slice.start = start;
            suffix->as.slice.end = end;
            node = suffix;
            continue;
        }
        break;
    }
    *node_out = node;
    return PY68_STATUS_OK;
}

static Py68Status py68_parse_precedence(Py68ExpressionParser *parser,
                                        int minimum,
                                        Py68AstNode **node_out)
{
    Py68AstNode *left;
    Py68AstNode *right;
    Py68AstNode *binary;
    Py68Token *token;
    Py68Status status = py68_parse_unary(parser, &left);

    if (status != PY68_STATUS_OK) return status;
    for (;;) {
        token = py68_current(parser);
        if (token == NULL || py68_precedence(token->kind) < minimum) break;
        ++parser->position;
        status = py68_parse_precedence(parser,
                                       py68_precedence(token->kind) + 1,
                                       &right);
        if (status != PY68_STATUS_OK) return status;
        status = py68_ast_arena_new(parser->arena, PY68_AST_BINARY,
                                    token->location, &binary);
        if (status != PY68_STATUS_OK) return status;
        binary->as.binary.operator_kind = token->kind;
        binary->as.binary.left = left;
        binary->as.binary.right = right;
        binary->location.offset = left->location.offset;
        binary->location.length = (Py68U16)(right->location.offset +
                                            right->location.length -
                                            left->location.offset);
        left = binary;
    }
    *node_out = left;
    return PY68_STATUS_OK;
}

Py68Status py68_parse_expression(Py68ExpressionParser *parser,
                                 Py68AstNode **node_out)
{
    return py68_parse_precedence(parser, 1, node_out);
}