/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_parser.h"
#include "py68k_float.h"
#include "py68k_tokenizer.h"

#include <stddef.h>
#include <string.h>

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

static int py68_check(Py68ExpressionParser *parser, Py68TokenKind kind)
{
    Py68Token *token = py68_current(parser);
    return token != NULL && token->kind == kind;
}

static Py68Status py68_parse_or_test(Py68ExpressionParser *parser,
                                     Py68AstNode **node_out);

static Py68Status py68_parse_comprehension_fors(Py68ExpressionParser *parser,
                                               Py68AstList *generators)
{
    Py68Token *token;
    Py68AstNode *clause;
    Py68AstNode *iterable;
    Py68AstNode *filter;
    Py68Status status;
    int saw_for = 0;

    for (;;) {
        token = py68_current(parser);
        if (token == NULL || token->kind != PY68_TOKEN_FOR) {
            if (!saw_for) {
                Py68Location location;
                if (token != NULL) {
                    location = token->location;
                } else {
                    location.offset = parser->source->length;
                    location.line = 0;
                    location.column = 0;
                    location.length = 0;
                }
                py68_parser_error(parser, location,
                                  "expected for in comprehension");
                return PY68_STATUS_SOURCE_ERROR;
            }
            return PY68_STATUS_OK;
        }
        ++parser->position;
        saw_for = 1;
        status = py68_ast_arena_new(parser->arena, PY68_AST_COMP_FOR,
                                    token->location, &clause);
        if (status != PY68_STATUS_OK) return status;
        token = py68_current(parser);
        if (token == NULL || token->kind != PY68_TOKEN_NAME) {
            py68_parser_error(parser,
                              token != NULL ? token->location : clause->location,
                              "expected comprehension loop variable");
            return PY68_STATUS_SOURCE_ERROR;
        }
        clause->as.comprehension_for.name_offset = token->location.offset;
        clause->as.comprehension_for.name_length = token->location.length;
        clause->as.comprehension_for.iterable = NULL;
        py68_ast_list_initialize(&clause->as.comprehension_for.ifs);
        ++parser->position;
        if (!py68_accept(parser, PY68_TOKEN_IN)) {
            token = py68_current(parser);
            py68_parser_error(parser,
                              token != NULL ? token->location : clause->location,
                              "expected in after comprehension loop variable");
            return PY68_STATUS_SOURCE_ERROR;
        }
        /* Iterable and filters are or_test, not full expressions, so a
           following `if` is a comprehension filter rather than a ternary. */
        status = py68_parse_or_test(parser, &iterable);
        if (status != PY68_STATUS_OK) return status;
        clause->as.comprehension_for.iterable = iterable;
        while (py68_accept(parser, PY68_TOKEN_IF)) {
            status = py68_parse_or_test(parser, &filter);
            if (status != PY68_STATUS_OK) return status;
            status = py68_ast_list_append(parser->arena,
                                          &clause->as.comprehension_for.ifs,
                                          filter);
            if (status != PY68_STATUS_OK) return status;
        }
        status = py68_ast_list_append(parser->arena, generators, clause);
        if (status != PY68_STATUS_OK) return status;
    }
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
    if (kind == PY68_TOKEN_IN || kind == PY68_TOKEN_NOT_IN) return 3;
    if (kind == PY68_TOKEN_IS || kind == PY68_TOKEN_IS_NOT) return 3;
    if (kind == PY68_TOKEN_PLUS || kind == PY68_TOKEN_MINUS) return 4;
    if (kind == PY68_TOKEN_STAR || kind == PY68_TOKEN_SLASH ||
        kind == PY68_TOKEN_FLOOR_DIVIDE || kind == PY68_TOKEN_PERCENT)
        return 5;
    return 0;
}

static void py68_ast_rebase_offsets(Py68AstNode *node, Py68U32 base)
{
    Py68U16 index;
    if (node == NULL) return;
    node->location.offset += base;
    switch ((Py68AstKind)node->kind) {
    case PY68_AST_STRING:
        node->as.string_literal.offset += base;
        break;
    case PY68_AST_NAME:
        node->as.name.offset += base;
        break;
    case PY68_AST_UNARY:
        py68_ast_rebase_offsets(node->as.unary.operand, base);
        break;
    case PY68_AST_BINARY:
        py68_ast_rebase_offsets(node->as.binary.left, base);
        py68_ast_rebase_offsets(node->as.binary.right, base);
        break;
    case PY68_AST_IF_EXP:
        py68_ast_rebase_offsets(node->as.if_exp.body, base);
        py68_ast_rebase_offsets(node->as.if_exp.condition, base);
        py68_ast_rebase_offsets(node->as.if_exp.else_body, base);
        break;
    case PY68_AST_CALL:
        py68_ast_rebase_offsets(node->as.call.callee, base);
        for (index = 0; index < node->as.call.arguments.count; ++index) {
            py68_ast_rebase_offsets(node->as.call.arguments.items[index],
                                    base);
        }
        break;
    case PY68_AST_INDEX:
        py68_ast_rebase_offsets(node->as.index.container, base);
        py68_ast_rebase_offsets(node->as.index.index, base);
        break;
    case PY68_AST_SLICE:
        py68_ast_rebase_offsets(node->as.slice.container, base);
        py68_ast_rebase_offsets(node->as.slice.start, base);
        py68_ast_rebase_offsets(node->as.slice.end, base);
        break;
    case PY68_AST_ATTRIBUTE:
        node->as.attribute.name_offset += base;
        py68_ast_rebase_offsets(node->as.attribute.value, base);
        break;
    case PY68_AST_LIST:
    case PY68_AST_TUPLE:
    case PY68_AST_SET:
        for (index = 0; index < node->as.list_literal.elements.count;
             ++index) {
            py68_ast_rebase_offsets(
                node->as.list_literal.elements.items[index], base);
        }
        break;
    case PY68_AST_DICT:
        for (index = 0; index < node->as.dict_literal.keys.count; ++index) {
            py68_ast_rebase_offsets(node->as.dict_literal.keys.items[index],
                                    base);
            py68_ast_rebase_offsets(node->as.dict_literal.values.items[index],
                                    base);
        }
        break;
    case PY68_AST_LIST_COMP:
    case PY68_AST_SET_COMP:
    case PY68_AST_DICT_COMP:
    case PY68_AST_GENERATOR_EXP:
        py68_ast_rebase_offsets(node->as.comprehension.elt, base);
        py68_ast_rebase_offsets(node->as.comprehension.value, base);
        for (index = 0; index < node->as.comprehension.generators.count;
             ++index) {
            py68_ast_rebase_offsets(
                node->as.comprehension.generators.items[index], base);
        }
        break;
    case PY68_AST_COMP_FOR: {
        Py68U16 filter_index;
        node->as.comprehension_for.name_offset += base;
        py68_ast_rebase_offsets(node->as.comprehension_for.iterable, base);
        for (filter_index = 0;
             filter_index < node->as.comprehension_for.ifs.count;
             ++filter_index) {
            py68_ast_rebase_offsets(
                node->as.comprehension_for.ifs.items[filter_index], base);
        }
        break;
    }
    case PY68_AST_JOINED_STR:
        for (index = 0; index < node->as.joined_str.parts.count; ++index) {
            py68_ast_rebase_offsets(node->as.joined_str.parts.items[index],
                                    base);
        }
        break;
    case PY68_AST_FORMATTED_VALUE:
        py68_ast_rebase_offsets(node->as.formatted_value.value, base);
        py68_ast_rebase_offsets(node->as.formatted_value.format_spec, base);
        break;
    default:
        break;
    }
}

static Py68Status py68_fstring_append_literal(Py68ExpressionParser *parser,
                                              Py68AstNode *joined,
                                              Py68Location token_location,
                                              Py68U32 offset,
                                              Py68U16 length)
{
    Py68AstNode *literal;
    Py68Status status;
    Py68Location location;

    if (length == 0) return PY68_STATUS_OK;
    location = token_location;
    location.offset = offset;
    location.length = length;
    status = py68_ast_arena_new(parser->arena, PY68_AST_STRING, location,
                                &literal);
    if (status != PY68_STATUS_OK) return status;
    literal->as.string_literal.offset = offset;
    literal->as.string_literal.length = length;
    literal->as.string_literal.quote_flags = (Py68U16)'"';
    return py68_ast_list_append(parser->arena, &joined->as.joined_str.parts,
                                literal);
}

static Py68Status py68_parse_fstring_expression(
    Py68ExpressionParser *parser, Py68U32 expr_start, Py68U16 expr_len,
    Py68AstNode **expr_out)
{
    Py68Source expr_source;
    Py68TokenArray expr_tokens;
    Py68ExpressionParser expr_parser;
    Py68AstNode *expr_node;
    Py68Status status;
    Py68U32 index;
    Py68Token *end_token;
    const Py68U8 *data = parser->source->data;

    /* Trim spaces so a leading blank does not become an INDENT token. */
    while (expr_len > 0 &&
           (data[expr_start] == (Py68U8)' ' ||
            data[expr_start] == (Py68U8)'\t')) {
        ++expr_start;
        --expr_len;
    }
    while (expr_len > 0 &&
           (data[expr_start + expr_len - 1] == (Py68U8)' ' ||
            data[expr_start + expr_len - 1] == (Py68U8)'\t')) {
        --expr_len;
    }

    if (expr_len == 0) {
        Py68Location location;
        location.offset = expr_start;
        location.line = 0;
        location.column = 0;
        location.length = 0;
        py68_parser_error(parser, location, "empty expression in f-string");
        return PY68_STATUS_SOURCE_ERROR;
    }

    expr_source.filename = parser->source->filename;
    expr_source.data = parser->source->data + expr_start;
    expr_source.length = expr_len;
    py68_token_array_initialize(&expr_tokens);
    status = py68_tokenize(parser->allocator, &expr_source, &expr_tokens,
                           parser->error);
    if (status != PY68_STATUS_OK) {
        py68_token_array_destroy(parser->allocator, &expr_tokens);
        return status;
    }
    for (index = 0; index < expr_tokens.count; ++index) {
        if (expr_tokens.items[index].kind == PY68_TOKEN_FSTRING) {
            Py68Location location = expr_tokens.items[index].location;
            location.offset += expr_start;
            py68_parser_error(parser, location,
                              "nested f-strings are not supported");
            py68_token_array_destroy(parser->allocator, &expr_tokens);
            return PY68_STATUS_SOURCE_ERROR;
        }
    }
    expr_parser.allocator = parser->allocator;
    expr_parser.source = &expr_source;
    expr_parser.tokens = &expr_tokens;
    expr_parser.position = 0;
    expr_parser.arena = parser->arena;
    expr_parser.error = parser->error;
    status = py68_parse_expression(&expr_parser, &expr_node);
    if (status != PY68_STATUS_OK) {
        py68_token_array_destroy(parser->allocator, &expr_tokens);
        return status;
    }
    while (expr_parser.position < expr_tokens.count &&
           expr_tokens.items[expr_parser.position].kind ==
               PY68_TOKEN_NEWLINE) {
        ++expr_parser.position;
    }
    end_token = expr_parser.position < expr_tokens.count
                ? &expr_tokens.items[expr_parser.position]
                : NULL;
    if (end_token == NULL || end_token->kind != PY68_TOKEN_EOF) {
        Py68Location location;
        if (end_token != NULL) {
            location = end_token->location;
            location.offset += expr_start;
        } else {
            location.offset = expr_start + expr_len;
            location.line = 0;
            location.column = 0;
            location.length = 0;
        }
        py68_parser_error(parser, location,
                          "invalid expression in f-string");
        py68_token_array_destroy(parser->allocator, &expr_tokens);
        return PY68_STATUS_SOURCE_ERROR;
    }
    py68_ast_rebase_offsets(expr_node, expr_start);
    py68_token_array_destroy(parser->allocator, &expr_tokens);
    *expr_out = expr_node;
    return PY68_STATUS_OK;
}

static Py68Status py68_parse_fstring(Py68ExpressionParser *parser,
                                     Py68Token *token,
                                     Py68AstNode **node_out)
{
    Py68AstNode *joined;
    Py68AstNode *formatted;
    Py68AstNode *expr_node;
    Py68AstNode *format_spec;
    Py68Status status;
    Py68U32 body_start;
    Py68U32 body_end;
    Py68U32 index;
    Py68U32 literal_start;
    Py68U16 conversion;
    const Py68U8 *data;

    status = py68_ast_arena_new(parser->arena, PY68_AST_JOINED_STR,
                                token->location, &joined);
    if (status != PY68_STATUS_OK) return status;
    py68_ast_list_initialize(&joined->as.joined_str.parts);

    /* Token is f"..." — body starts after f and opening quote. */
    body_start = token->location.offset + 2;
    body_end = token->location.offset + token->location.length - 1;
    data = parser->source->data;
    index = body_start;
    literal_start = body_start;

    while (index < body_end) {
        if (data[index] == (Py68U8)'{' && index + 1 < body_end &&
            data[index + 1] == (Py68U8)'{') {
            status = py68_fstring_append_literal(
                parser, joined, token->location, literal_start,
                (Py68U16)(index - literal_start));
            if (status != PY68_STATUS_OK) return status;
            status = py68_fstring_append_literal(
                parser, joined, token->location, index, 1);
            if (status != PY68_STATUS_OK) return status;
            index += 2;
            literal_start = index;
            continue;
        }
        if (data[index] == (Py68U8)'}' && index + 1 < body_end &&
            data[index + 1] == (Py68U8)'}') {
            status = py68_fstring_append_literal(
                parser, joined, token->location, literal_start,
                (Py68U16)(index - literal_start));
            if (status != PY68_STATUS_OK) return status;
            status = py68_fstring_append_literal(
                parser, joined, token->location, index, 1);
            if (status != PY68_STATUS_OK) return status;
            index += 2;
            literal_start = index;
            continue;
        }
        if (data[index] == (Py68U8)'}') {
            Py68Location location = token->location;
            location.offset = index;
            location.length = 1;
            py68_parser_error(parser, location,
                              "single '}' is not allowed in f-string");
            return PY68_STATUS_SOURCE_ERROR;
        }
        if (data[index] == (Py68U8)'{') {
            Py68U32 expr_start;
            Py68U32 scan;
            Py68U32 depth;
            Py68U32 expr_end;
            Py68U32 close;

            status = py68_fstring_append_literal(
                parser, joined, token->location, literal_start,
                (Py68U16)(index - literal_start));
            if (status != PY68_STATUS_OK) return status;

            expr_start = index + 1;
            scan = expr_start;
            depth = 0;
            conversion = 0;
            format_spec = NULL;
            expr_end = 0;
            close = 0;

            while (scan < body_end) {
                if (data[scan] == (Py68U8)'{') {
                    ++depth;
                    ++scan;
                    continue;
                }
                if (data[scan] == (Py68U8)'}') {
                    if (depth == 0) {
                        if (expr_end == 0) expr_end = scan;
                        close = scan;
                        break;
                    }
                    --depth;
                    ++scan;
                    continue;
                }
                if (depth == 0 && data[scan] == (Py68U8)'!' &&
                    expr_end == 0) {
                    expr_end = scan;
                    if (scan + 1 >= body_end) {
                        Py68Location location = token->location;
                        location.offset = scan;
                        location.length = 1;
                        py68_parser_error(parser, location,
                                          "invalid conversion in f-string");
                        return PY68_STATUS_SOURCE_ERROR;
                    }
                    if (data[scan + 1] == (Py68U8)'s' ||
                        data[scan + 1] == (Py68U8)'r' ||
                        data[scan + 1] == (Py68U8)'a') {
                        conversion = (Py68U16)data[scan + 1];
                        scan += 2;
                        if (scan < body_end && data[scan] == (Py68U8)':') {
                            /* fall through to format-spec handling below */
                        } else if (scan < body_end &&
                                   data[scan] == (Py68U8)'}') {
                            close = scan;
                            break;
                        } else {
                            Py68Location location = token->location;
                            location.offset = scan;
                            location.length = 1;
                            py68_parser_error(
                                parser, location,
                                "invalid conversion in f-string");
                            return PY68_STATUS_SOURCE_ERROR;
                        }
                    } else {
                        Py68Location location = token->location;
                        location.offset = scan;
                        location.length = 1;
                        py68_parser_error(parser, location,
                                          "invalid conversion in f-string");
                        return PY68_STATUS_SOURCE_ERROR;
                    }
                }
                if (depth == 0 && data[scan] == (Py68U8)':' &&
                    (expr_end == 0 || conversion != 0)) {
                    Py68U32 spec_start;
                    if (expr_end == 0) expr_end = scan;
                    spec_start = scan + 1;
                    ++scan;
                    while (scan < body_end && data[scan] != (Py68U8)'}') {
                        if (data[scan] == (Py68U8)'{') {
                            Py68Location location = token->location;
                            location.offset = scan;
                            location.length = 1;
                            py68_parser_error(
                                parser, location,
                                "nested format specs are not supported");
                            return PY68_STATUS_SOURCE_ERROR;
                        }
                        ++scan;
                    }
                    if (scan >= body_end) {
                        Py68Location location = token->location;
                        location.offset = index;
                        location.length = 1;
                        py68_parser_error(parser, location,
                                          "unclosed '{' in f-string");
                        return PY68_STATUS_SOURCE_ERROR;
                    }
                    status = py68_ast_arena_new(
                        parser->arena, PY68_AST_STRING, token->location,
                        &format_spec);
                    if (status != PY68_STATUS_OK) return status;
                    format_spec->as.string_literal.offset = spec_start;
                    format_spec->as.string_literal.length =
                        (Py68U16)(scan - spec_start);
                    format_spec->as.string_literal.quote_flags =
                        (Py68U16)'"';
                    format_spec->location.offset = spec_start;
                    format_spec->location.length =
                        (Py68U16)(scan - spec_start);
                    close = scan;
                    break;
                }
                ++scan;
            }

            if (close == 0) {
                Py68Location location = token->location;
                location.offset = index;
                location.length = 1;
                py68_parser_error(parser, location,
                                  "unclosed '{' in f-string");
                return PY68_STATUS_SOURCE_ERROR;
            }
            if (expr_end == 0) expr_end = close;

            status = py68_parse_fstring_expression(
                parser, expr_start, (Py68U16)(expr_end - expr_start),
                &expr_node);
            if (status != PY68_STATUS_OK) return status;

            status = py68_ast_arena_new(parser->arena,
                                        PY68_AST_FORMATTED_VALUE,
                                        token->location, &formatted);
            if (status != PY68_STATUS_OK) return status;
            formatted->as.formatted_value.value = expr_node;
            formatted->as.formatted_value.conversion = conversion;
            formatted->as.formatted_value.format_spec = format_spec;
            status = py68_ast_list_append(parser->arena,
                                          &joined->as.joined_str.parts,
                                          formatted);
            if (status != PY68_STATUS_OK) return status;

            index = close + 1;
            literal_start = index;
            continue;
        }
        ++index;
    }

    status = py68_fstring_append_literal(
        parser, joined, token->location, literal_start,
        (Py68U16)(body_end - literal_start));
    if (status != PY68_STATUS_OK) return status;

    ++parser->position;
    *node_out = joined;
    return PY68_STATUS_OK;
}

static Py68Status py68_parse_primary(Py68ExpressionParser *parser,
                                     Py68AstNode **node_out)
{
    Py68Token *token = py68_current(parser);
    Py68AstNode *node;
    Py68Status status;
    Py68I32 value;

    if (token == NULL) return PY68_STATUS_SOURCE_ERROR;
    if (token->kind == PY68_TOKEN_FLOAT) {
        char buffer[64];
        Py68U32 index;
        Py68U32 out = 0;
        Py68U32 bits;
        if (token->location.length >= 63) {
            py68_parser_error(parser, token->location, "float literal is too long");
            return PY68_STATUS_SOURCE_ERROR;
        }
        for (index = 0; index < token->location.length; ++index) {
            if (parser->source->data[token->location.offset + index] ==
                (Py68U8)'_')
                continue;
            buffer[out++] =
                (char)parser->source->data[token->location.offset + index];
        }
        buffer[out] = '\0';
        if (!py68_f32_parse(buffer, out, &bits)) {
            py68_parser_error(parser, token->location,
                              "non-finite float literal");
            return PY68_STATUS_SOURCE_ERROR;
        }
        status = py68_ast_arena_new(parser->arena, PY68_AST_FLOAT,
                                    token->location, &node);
        if (status == PY68_STATUS_OK) node->as.float_literal.bits = bits;
        ++parser->position;
        *node_out = node;
        return status;
    }
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
    if (token->kind == PY68_TOKEN_FSTRING) {
        return py68_parse_fstring(parser, token, node_out);
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
        Py68AstNode *first;
        if (py68_accept(parser, PY68_TOKEN_RIGHT_BRACKET)) {
            status = py68_ast_arena_new(parser->arena, PY68_AST_LIST,
                                        token->location, &node);
            if (status != PY68_STATUS_OK) return status;
            py68_ast_list_initialize(&node->as.list_literal.elements);
            *node_out = node;
            return PY68_STATUS_OK;
        }
        status = py68_parse_expression(parser, &first);
        if (status != PY68_STATUS_OK) return status;
        if (py68_check(parser, PY68_TOKEN_FOR)) {
            status = py68_ast_arena_new(parser->arena, PY68_AST_LIST_COMP,
                                        token->location, &node);
            if (status != PY68_STATUS_OK) return status;
            node->as.comprehension.elt = first;
            node->as.comprehension.value = NULL;
            py68_ast_list_initialize(&node->as.comprehension.generators);
            status = py68_parse_comprehension_fors(
                parser, &node->as.comprehension.generators);
            if (status != PY68_STATUS_OK) return status;
            if (!py68_accept(parser, PY68_TOKEN_RIGHT_BRACKET)) {
                token = py68_current(parser);
                py68_parser_error(parser, token->location,
                                  "expected closing bracket");
                return PY68_STATUS_SOURCE_ERROR;
            }
            node->location.length =
                (Py68U16)(py68_current(parser)->location.offset -
                          node->location.offset);
            *node_out = node;
            return PY68_STATUS_OK;
        }
        status = py68_ast_arena_new(parser->arena, PY68_AST_LIST,
                                    token->location, &node);
        if (status != PY68_STATUS_OK) return status;
        py68_ast_list_initialize(&node->as.list_literal.elements);
        status = py68_ast_list_append(parser->arena,
                                      &node->as.list_literal.elements, first);
        if (status != PY68_STATUS_OK) return status;
        if (!py68_accept(parser, PY68_TOKEN_RIGHT_BRACKET)) {
            for (;;) {
                Py68AstNode *element;
                if (!py68_accept(parser, PY68_TOKEN_COMMA)) {
                    token = py68_current(parser);
                    py68_parser_error(parser, token->location,
                                      "expected comma or closing bracket");
                    return PY68_STATUS_SOURCE_ERROR;
                }
                if (py68_accept(parser, PY68_TOKEN_RIGHT_BRACKET)) break;
                status = py68_parse_expression(parser, &element);
                if (status != PY68_STATUS_OK) return status;
                status = py68_ast_list_append(parser->arena,
                                              &node->as.list_literal.elements,
                                              element);
                if (status != PY68_STATUS_OK) return status;
                if (py68_accept(parser, PY68_TOKEN_RIGHT_BRACKET)) break;
            }
        }
        node->location.length = (Py68U16)(py68_current(parser)->location.offset -
                                          node->location.offset);
        *node_out = node;
        return PY68_STATUS_OK;
    }
    if (py68_accept(parser, PY68_TOKEN_LEFT_PAREN)) {
        Py68AstNode *first;
        if (py68_accept(parser, PY68_TOKEN_RIGHT_PAREN)) {
            status = py68_ast_arena_new(parser->arena, PY68_AST_TUPLE,
                                        token->location, &node);
            if (status != PY68_STATUS_OK) return status;
            py68_ast_list_initialize(&node->as.list_literal.elements);
            *node_out = node;
            return PY68_STATUS_OK;
        }
        status = py68_parse_expression(parser, &first);
        if (status != PY68_STATUS_OK) return status;
        if (py68_check(parser, PY68_TOKEN_FOR)) {
            status = py68_ast_arena_new(parser->arena, PY68_AST_GENERATOR_EXP,
                                        token->location, &node);
            if (status != PY68_STATUS_OK) return status;
            node->as.comprehension.elt = first;
            node->as.comprehension.value = NULL;
            py68_ast_list_initialize(&node->as.comprehension.generators);
            status = py68_parse_comprehension_fors(
                parser, &node->as.comprehension.generators);
            if (status != PY68_STATUS_OK) return status;
            if (!py68_accept(parser, PY68_TOKEN_RIGHT_PAREN)) {
                token = py68_current(parser);
                py68_parser_error(parser, token->location,
                                  "expected closing parenthesis");
                return PY68_STATUS_SOURCE_ERROR;
            }
            node->location.length =
                (Py68U16)(py68_current(parser)->location.offset -
                          node->location.offset);
            *node_out = node;
            return PY68_STATUS_OK;
        }
        if (py68_accept(parser, PY68_TOKEN_COMMA)) {
            status = py68_ast_arena_new(parser->arena, PY68_AST_TUPLE,
                                        token->location, &node);
            if (status != PY68_STATUS_OK) return status;
            py68_ast_list_initialize(&node->as.list_literal.elements);
            status = py68_ast_list_append(parser->arena,
                                          &node->as.list_literal.elements,
                                          first);
            if (status != PY68_STATUS_OK) return status;
            if (!py68_accept(parser, PY68_TOKEN_RIGHT_PAREN)) {
                for (;;) {
                    Py68AstNode *element;
                    status = py68_parse_expression(parser, &element);
                    if (status != PY68_STATUS_OK) return status;
                    status = py68_ast_list_append(
                        parser->arena, &node->as.list_literal.elements,
                        element);
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
            *node_out = node;
            return PY68_STATUS_OK;
        }
        if (!py68_accept(parser, PY68_TOKEN_RIGHT_PAREN)) {
            token = py68_current(parser);
            py68_parser_error(parser, token->location,
                              "expected closing parenthesis");
            return PY68_STATUS_SOURCE_ERROR;
        }
        *node_out = first;
        return PY68_STATUS_OK;
    }
    if (py68_accept(parser, PY68_TOKEN_LEFT_BRACE)) {
        Py68AstNode *first;
        if (py68_accept(parser, PY68_TOKEN_RIGHT_BRACE)) {
            status = py68_ast_arena_new(parser->arena, PY68_AST_DICT,
                                        token->location, &node);
            if (status != PY68_STATUS_OK) return status;
            py68_ast_list_initialize(&node->as.dict_literal.keys);
            py68_ast_list_initialize(&node->as.dict_literal.values);
            *node_out = node;
            return PY68_STATUS_OK;
        }
        status = py68_parse_expression(parser, &first);
        if (status != PY68_STATUS_OK) return status;
        if (py68_accept(parser, PY68_TOKEN_COLON)) {
            Py68AstNode *value_node;
            status = py68_parse_expression(parser, &value_node);
            if (status != PY68_STATUS_OK) return status;
            if (py68_check(parser, PY68_TOKEN_FOR)) {
                status = py68_ast_arena_new(parser->arena, PY68_AST_DICT_COMP,
                                            token->location, &node);
                if (status != PY68_STATUS_OK) return status;
                node->as.comprehension.elt = first;
                node->as.comprehension.value = value_node;
                py68_ast_list_initialize(&node->as.comprehension.generators);
                status = py68_parse_comprehension_fors(
                    parser, &node->as.comprehension.generators);
                if (status != PY68_STATUS_OK) return status;
                if (!py68_accept(parser, PY68_TOKEN_RIGHT_BRACE)) {
                    token = py68_current(parser);
                    py68_parser_error(parser, token->location,
                                      "expected closing brace");
                    return PY68_STATUS_SOURCE_ERROR;
                }
                *node_out = node;
                return PY68_STATUS_OK;
            }
            status = py68_ast_arena_new(parser->arena, PY68_AST_DICT,
                                        token->location, &node);
            if (status != PY68_STATUS_OK) return status;
            py68_ast_list_initialize(&node->as.dict_literal.keys);
            py68_ast_list_initialize(&node->as.dict_literal.values);
            status = py68_ast_list_append(parser->arena,
                                          &node->as.dict_literal.keys, first);
            if (status == PY68_STATUS_OK)
                status = py68_ast_list_append(parser->arena,
                                              &node->as.dict_literal.values,
                                              value_node);
            if (status != PY68_STATUS_OK) return status;
            if (!py68_accept(parser, PY68_TOKEN_RIGHT_BRACE)) {
                for (;;) {
                    Py68AstNode *key;
                    Py68AstNode *item;
                    if (!py68_accept(parser, PY68_TOKEN_COMMA)) {
                        token = py68_current(parser);
                        py68_parser_error(parser, token->location,
                                          "expected comma or closing brace");
                        return PY68_STATUS_SOURCE_ERROR;
                    }
                    if (py68_accept(parser, PY68_TOKEN_RIGHT_BRACE)) break;
                    status = py68_parse_expression(parser, &key);
                    if (status != PY68_STATUS_OK ||
                        !py68_accept(parser, PY68_TOKEN_COLON)) {
                        token = py68_current(parser);
                        py68_parser_error(parser, token->location,
                                          "expected dictionary key and colon");
                        return PY68_STATUS_SOURCE_ERROR;
                    }
                    status = py68_parse_expression(parser, &item);
                    if (status != PY68_STATUS_OK) return status;
                    status = py68_ast_list_append(parser->arena,
                                                  &node->as.dict_literal.keys,
                                                  key);
                    if (status == PY68_STATUS_OK)
                        status = py68_ast_list_append(
                            parser->arena, &node->as.dict_literal.values, item);
                    if (status != PY68_STATUS_OK) return status;
                    if (py68_accept(parser, PY68_TOKEN_RIGHT_BRACE)) break;
                }
            }
            *node_out = node;
            return PY68_STATUS_OK;
        }
        if (py68_check(parser, PY68_TOKEN_FOR)) {
            status = py68_ast_arena_new(parser->arena, PY68_AST_SET_COMP,
                                        token->location, &node);
            if (status != PY68_STATUS_OK) return status;
            node->as.comprehension.elt = first;
            node->as.comprehension.value = NULL;
            py68_ast_list_initialize(&node->as.comprehension.generators);
            status = py68_parse_comprehension_fors(
                parser, &node->as.comprehension.generators);
            if (status != PY68_STATUS_OK) return status;
            if (!py68_accept(parser, PY68_TOKEN_RIGHT_BRACE)) {
                token = py68_current(parser);
                py68_parser_error(parser, token->location,
                                  "expected closing brace");
                return PY68_STATUS_SOURCE_ERROR;
            }
            *node_out = node;
            return PY68_STATUS_OK;
        }
        status = py68_ast_arena_new(parser->arena, PY68_AST_SET,
                                    token->location, &node);
        if (status != PY68_STATUS_OK) return status;
        py68_ast_list_initialize(&node->as.list_literal.elements);
        status = py68_ast_list_append(parser->arena,
                                      &node->as.list_literal.elements, first);
        if (status != PY68_STATUS_OK) return status;
        if (!py68_accept(parser, PY68_TOKEN_RIGHT_BRACE)) {
            for (;;) {
                Py68AstNode *element;
                if (!py68_accept(parser, PY68_TOKEN_COMMA)) {
                    token = py68_current(parser);
                    py68_parser_error(parser, token->location,
                                      "expected comma or closing brace");
                    return PY68_STATUS_SOURCE_ERROR;
                }
                if (py68_accept(parser, PY68_TOKEN_RIGHT_BRACE)) break;
                status = py68_parse_expression(parser, &element);
                if (status != PY68_STATUS_OK) return status;
                status = py68_ast_list_append(parser->arena,
                                              &node->as.list_literal.elements,
                                              element);
                if (status != PY68_STATUS_OK) return status;
                if (py68_accept(parser, PY68_TOKEN_RIGHT_BRACE)) break;
            }
        }
        *node_out = node;
        return PY68_STATUS_OK;
    }
    if (token->kind == PY68_TOKEN_YIELD) {
        py68_parser_error(parser, token->location,
                          "yield is a statement in Python68K Language "
                          "Level 0.7; it produces no value");
        return PY68_STATUS_SOURCE_ERROR;
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
                          token->kind == PY68_TOKEN_MINUS)) {
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
                    /* `f(x for x in xs)`: the call parentheses double as the
                       generator-expression parentheses, so this is only legal
                       as the single argument. */
                    if (py68_check(parser, PY68_TOKEN_FOR)) {
                        Py68AstNode *generator_exp;
                        if (call->as.call.arguments.count != 0) {
                            py68_parser_error(
                                parser, py68_current(parser)->location,
                                "a generator expression must be parenthesized "
                                "when it is not the only argument");
                            return PY68_STATUS_SOURCE_ERROR;
                        }
                        status = py68_ast_arena_new(parser->arena,
                                                    PY68_AST_GENERATOR_EXP,
                                                    argument->location,
                                                    &generator_exp);
                        if (status != PY68_STATUS_OK) return status;
                        generator_exp->as.comprehension.elt = argument;
                        generator_exp->as.comprehension.value = NULL;
                        py68_ast_list_initialize(
                            &generator_exp->as.comprehension.generators);
                        status = py68_parse_comprehension_fors(
                            parser,
                            &generator_exp->as.comprehension.generators);
                        if (status != PY68_STATUS_OK) return status;
                        argument = generator_exp;
                    }
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
        if (py68_accept(parser, PY68_TOKEN_DOT)) {
            Py68AstNode *attribute;
            Py68Token *name_token = py68_current(parser);
            if (name_token == NULL || name_token->kind != PY68_TOKEN_NAME) {
                token = py68_current(parser);
                py68_parser_error(parser, token->location,
                                  "expected attribute name");
                return PY68_STATUS_SOURCE_ERROR;
            }
            ++parser->position;
            status = py68_ast_arena_new(parser->arena, PY68_AST_ATTRIBUTE,
                                        node->location, &attribute);
            if (status != PY68_STATUS_OK) return status;
            attribute->as.attribute.value = node;
            attribute->as.attribute.name_offset = name_token->location.offset;
            attribute->as.attribute.name_length = name_token->location.length;
            node = attribute;
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
    Py68Token *next;
    Py68TokenKind operator_kind;
    int operator_prec;
    Py68Status status;

    token = py68_current(parser);
    /* Prefix `not` binds less tightly than comparisons so `not a in b` and
       `not x is y` are `not (a in b)` / `not (x is y)`, matching Python.
       `a not in b` and `x is not y` are compound infix operators. */
    if (token != NULL && token->kind == PY68_TOKEN_NOT && minimum <= 3) {
        next = NULL;
        if (parser->position + 1 < parser->tokens->count)
            next = &parser->tokens->items[parser->position + 1];
        if (next == NULL || next->kind != PY68_TOKEN_IN) {
            Py68AstNode *node;
            Py68AstNode *operand;
            Py68Location not_location = token->location;
            ++parser->position;
            status = py68_parse_precedence(parser, 3, &operand);
            if (status != PY68_STATUS_OK) return status;
            status = py68_ast_arena_new(parser->arena, PY68_AST_UNARY,
                                        not_location, &node);
            if (status != PY68_STATUS_OK) return status;
            node->as.unary.operator_kind = PY68_TOKEN_NOT;
            node->as.unary.operand = operand;
            node->location.length = (Py68U16)(operand->location.offset +
                                              operand->location.length -
                                              not_location.offset);
            left = node;
        } else {
            status = py68_parse_unary(parser, &left);
            if (status != PY68_STATUS_OK) return status;
        }
    } else {
        status = py68_parse_unary(parser, &left);
        if (status != PY68_STATUS_OK) return status;
    }

    for (;;) {
        token = py68_current(parser);
        if (token == NULL) break;
        operator_kind = (Py68TokenKind)token->kind;
        operator_prec = py68_precedence(operator_kind);
        if (token->kind == PY68_TOKEN_NOT) {
            next = NULL;
            if (parser->position + 1 < parser->tokens->count)
                next = &parser->tokens->items[parser->position + 1];
            if (next != NULL && next->kind == PY68_TOKEN_IN) {
                operator_kind = PY68_TOKEN_NOT_IN;
                operator_prec = 3;
            } else {
                break;
            }
        }
        if (token->kind == PY68_TOKEN_IS) {
            next = NULL;
            if (parser->position + 1 < parser->tokens->count)
                next = &parser->tokens->items[parser->position + 1];
            if (next != NULL && next->kind == PY68_TOKEN_NOT) {
                operator_kind = PY68_TOKEN_IS_NOT;
                operator_prec = 3;
            }
        }
        if (operator_prec < minimum) break;
        if (operator_kind == PY68_TOKEN_NOT_IN ||
            operator_kind == PY68_TOKEN_IS_NOT) {
            ++parser->position;
            ++parser->position;
        } else {
            ++parser->position;
        }
        status = py68_parse_precedence(parser, operator_prec + 1, &right);
        if (status != PY68_STATUS_OK) return status;
        status = py68_ast_arena_new(parser->arena, PY68_AST_BINARY,
                                    token->location, &binary);
        if (status != PY68_STATUS_OK) return status;
        binary->as.binary.operator_kind = (Py68U16)operator_kind;
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

static Py68Status py68_parse_or_test(Py68ExpressionParser *parser,
                                     Py68AstNode **node_out)
{
    return py68_parse_precedence(parser, 1, node_out);
}

Py68Status py68_parse_expression(Py68ExpressionParser *parser,
                                 Py68AstNode **node_out)
{
    Py68AstNode *body;
    Py68AstNode *condition;
    Py68AstNode *else_body;
    Py68AstNode *node;
    Py68Token *if_token;
    Py68Token *token;
    Py68Status status;

    status = py68_parse_or_test(parser, &body);
    if (status != PY68_STATUS_OK) return status;
    if_token = py68_current(parser);
    if (if_token == NULL || if_token->kind != PY68_TOKEN_IF) {
        *node_out = body;
        return PY68_STATUS_OK;
    }
    ++parser->position;
    status = py68_parse_or_test(parser, &condition);
    if (status != PY68_STATUS_OK) return status;
    if (!py68_accept(parser, PY68_TOKEN_ELSE)) {
        token = py68_current(parser);
        py68_parser_error(parser,
                          token != NULL ? token->location : if_token->location,
                          "expected else in conditional expression");
        return PY68_STATUS_SOURCE_ERROR;
    }
    /* Else-clause is a full expression so `a if c1 else b if c2 else c`
       associates to the right, matching Python. */
    status = py68_parse_expression(parser, &else_body);
    if (status != PY68_STATUS_OK) return status;
    status = py68_ast_arena_new(parser->arena, PY68_AST_IF_EXP,
                                if_token->location, &node);
    if (status != PY68_STATUS_OK) return status;
    node->as.if_exp.body = body;
    node->as.if_exp.condition = condition;
    node->as.if_exp.else_body = else_body;
    node->location.offset = body->location.offset;
    node->location.length = (Py68U16)(else_body->location.offset +
                                      else_body->location.length -
                                      body->location.offset);
    *node_out = node;
    return PY68_STATUS_OK;
}