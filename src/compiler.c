#include "py68k_compiler.h"
#include "py68k_token.h"

#include <stddef.h>
#include <string.h>

static void py68_compile_error(Py68Error *error, const Py68Source *source,
                               Py68Location location, const char *message)
{
    py68_error_set(error, PY68_ERROR_SYNTAX, location, source->filename,
                   message);
}

static Py68Status py68_emit_op(Py68Allocator *allocator, Py68Code *code,
                               Py68U8 opcode)
{
    return py68_code_emit_u8(allocator, code, opcode);
}

static Py68Status py68_emit_u8_op(Py68Allocator *allocator, Py68Code *code,
                                  Py68U8 opcode, Py68U8 operand)
{
    Py68Status status = py68_emit_op(allocator, code, opcode);
    if (status != PY68_STATUS_OK) return status;
    return py68_code_emit_u8(allocator, code, operand);
}

static Py68Status py68_emit_u16_op(Py68Allocator *allocator, Py68Code *code,
                                   Py68U8 opcode, Py68U16 operand)
{
    Py68Status status = py68_emit_op(allocator, code, opcode);
    if (status != PY68_STATUS_OK) return status;
    return py68_code_emit_u16_be(allocator, code, operand);
}

static Py68Status py68_emit_jump(Py68Allocator *allocator, Py68Code *code,
                                 Py68U8 opcode, Py68U32 *operand_offset)
{
    Py68Status status = py68_emit_op(allocator, code, opcode);
    if (status != PY68_STATUS_OK) return status;
    *operand_offset = code->bytecode_length;
    return py68_code_emit_u16_be(allocator, code, 0);
}

static Py68Status py68_patch_jump(Py68Code *code, Py68U32 operand_offset,
                                  Py68U32 target)
{
    Py68I32 displacement = (Py68I32)target -
                           (Py68I32)(operand_offset + 2);
    return py68_code_patch_i16_be(code, operand_offset, displacement);
}

static Py68Status py68_compile_expression(Py68Allocator *allocator,
                                          const Py68Source *source,
                                          Py68AstNode *node, Py68Code *code,
                                          Py68Error *error);

static Py68Status py68_compile_statements(Py68Allocator *allocator,
                                          const Py68Source *source,
                                          Py68AstList *statements,
                                          Py68Code *code, Py68Error *error)
{
    Py68U16 index;
    Py68U16 name_index;
    Py68AstNode *statement;
    Py68Status status;
    for (index = 0; index < statements->count; ++index) {
        statement = statements->items[index];
        switch ((Py68AstKind)statement->kind) {
        case PY68_AST_ASSIGN:
            status = py68_compile_expression(allocator, source,
                                             statement->as.assign.value,
                                             code, error);
            if (status != PY68_STATUS_OK) return status;
            status = py68_code_add_name(allocator, code,
                                        statement->as.assign.name_offset,
                                        statement->as.assign.name_length,
                                        &name_index);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_u16_op(allocator, code, OP_STORE_GLOBAL,
                                      name_index);
            break;
        case PY68_AST_EXPRESSION_STATEMENT:
            status = py68_compile_expression(allocator, source,
                                             statement->as.expression_statement.value,
                                             code, error);
            if (status == PY68_STATUS_OK) status = py68_emit_op(
                allocator, code, OP_POP);
            break;
        case PY68_AST_IF: {
            Py68U32 false_operand;
            Py68U32 end_operand;
            status = py68_compile_expression(allocator, source,
                                              statement->as.if_statement.condition,
                                              code, error);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_jump(allocator, code, OP_JUMP_IF_FALSE,
                                    &false_operand);
            if (status != PY68_STATUS_OK) return status;
            status = py68_compile_statements(allocator, source,
                                             &statement->as.if_statement.body,
                                             code, error);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_jump(allocator, code, OP_JUMP, &end_operand);
            if (status != PY68_STATUS_OK) return status;
            status = py68_patch_jump(code, false_operand, code->bytecode_length);
            if (status != PY68_STATUS_OK) return status;
            status = py68_compile_statements(allocator, source,
                                             &statement->as.if_statement.else_body,
                                             code, error);
            if (status != PY68_STATUS_OK) return status;
            status = py68_patch_jump(code, end_operand, code->bytecode_length);
            break;
        }
        case PY68_AST_WHILE: {
            Py68U32 start = code->bytecode_length;
            Py68U32 end_operand;
            Py68U32 back_operand;
            status = py68_compile_expression(allocator, source,
                                              statement->as.while_statement.condition,
                                              code, error);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_jump(allocator, code, OP_JUMP_IF_FALSE,
                                    &end_operand);
            if (status != PY68_STATUS_OK) return status;
            status = py68_compile_statements(allocator, source,
                                             &statement->as.while_statement.body,
                                             code, error);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_jump(allocator, code, OP_JUMP, &back_operand);
            if (status != PY68_STATUS_OK) return status;
            status = py68_patch_jump(code, back_operand, start);
            if (status != PY68_STATUS_OK) return status;
            status = py68_patch_jump(code, end_operand, code->bytecode_length);
            break;
        }
        case PY68_AST_FOR: {
            Py68U32 range_next_op;
            Py68U32 back_operand;
            Py68U32 end_operand;
            Py68U16 name_index;
            Py68AstNode *iterable = statement->as.for_statement.iterable;
            if (iterable != NULL && iterable->kind == PY68_AST_CALL &&
                iterable->as.call.callee != NULL &&
                iterable->as.call.callee->kind == PY68_AST_NAME &&
                iterable->as.call.callee->as.name.length == 5 &&
                memcmp(source->data + iterable->as.call.callee->as.name.offset,
                       "range", 5) == 0) {
                Py68U16 arg_count = iterable->as.call.arguments.count;
                Py68U16 arg_idx;
                if (arg_count < 1 || arg_count > 3) {
                    py68_compile_error(error, source, iterable->location,
                                       "range expects 1 to 3 arguments");
                    return PY68_STATUS_SOURCE_ERROR;
                }
                for (arg_idx = 0; arg_idx < arg_count; ++arg_idx) {
                    status = py68_compile_expression(
                        allocator, source,
                        iterable->as.call.arguments.items[arg_idx],
                        code, error);
                    if (status != PY68_STATUS_OK) return status;
                }
                status = py68_emit_u8_op(allocator, code, OP_RANGE_INIT,
                                        (Py68U8)arg_count);
                if (status != PY68_STATUS_OK) return status;
            } else {
                status = py68_compile_expression(allocator, source, iterable,
                                                  code, error);
                if (status != PY68_STATUS_OK) return status;
                status = py68_emit_u8_op(allocator, code, OP_RANGE_INIT, 1);
                if (status != PY68_STATUS_OK) return status;
            }
            range_next_op = code->bytecode_length;
            status = py68_emit_jump(allocator, code, OP_RANGE_NEXT, &end_operand);
            if (status != PY68_STATUS_OK) return status;
            status = py68_code_add_name(allocator, code,
                                        statement->as.for_statement.name_offset,
                                        statement->as.for_statement.name_length,
                                        &name_index);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_u16_op(allocator, code, OP_STORE_GLOBAL,
                                      name_index);
            if (status != PY68_STATUS_OK) return status;
            status = py68_compile_statements(allocator, source,
                                             &statement->as.for_statement.body,
                                             code, error);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_jump(allocator, code, OP_JUMP, &back_operand);
            if (status != PY68_STATUS_OK) return status;
            status = py68_patch_jump(code, back_operand, range_next_op);
            if (status != PY68_STATUS_OK) return status;
            status = py68_patch_jump(code, end_operand, code->bytecode_length);
            if (status != PY68_STATUS_OK) return status;
            if (statement->as.for_statement.else_body.count > 0) {
                status = py68_compile_statements(
                    allocator, source,
                    &statement->as.for_statement.else_body,
                    code, error);
                if (status != PY68_STATUS_OK) return status;
            }
            break;
        }
        case PY68_AST_FUNCTION_DEF:
            py68_compile_error(error, source, statement->location,
                               "function bytecode generation is not implemented");
            return PY68_STATUS_SOURCE_ERROR;
        default:
            status = PY68_STATUS_OK;
            break;
        }
        if (status != PY68_STATUS_OK) return status;
    }
    return PY68_STATUS_OK;
}

static Py68Status py68_compile_expression(Py68Allocator *allocator,
                                          const Py68Source *source,
                                          Py68AstNode *node, Py68Code *code,
                                          Py68Error *error)
{
    Py68U16 index;
    Py68U16 argument_count;
    Py68U16 element_count;
    Py68Status status;
    if (node == NULL) return PY68_STATUS_INTERNAL_ERROR;
    switch ((Py68AstKind)node->kind) {
    case PY68_AST_INTEGER: {
        Py68Constant constant;
        constant.kind = PY68_CONSTANT_INTEGER;
        constant.flags = 0;
        constant.integer = node->as.integer_literal.value;
        constant.offset = 0;
        constant.length = 0;
        status = py68_code_add_constant(allocator, code, constant, &index);
        if (status != PY68_STATUS_OK) return status;
        return py68_emit_u16_op(allocator, code, OP_LOAD_CONST, index);
    }
    case PY68_AST_STRING: {
        Py68Constant constant;
        constant.kind = PY68_CONSTANT_STRING;
        constant.flags = node->as.string_literal.quote_flags;
        constant.integer = 0;
        constant.offset = node->as.string_literal.offset;
        constant.length = node->as.string_literal.length;
        status = py68_code_add_constant(allocator, code, constant, &index);
        if (status != PY68_STATUS_OK) return status;
        return py68_emit_u16_op(allocator, code, OP_LOAD_CONST, index);
    }
    case PY68_AST_BOOL:
        return py68_emit_op(allocator, code,
                            node->as.boolean_literal.value ? OP_LOAD_TRUE : OP_LOAD_FALSE);
    case PY68_AST_NONE:
        return py68_emit_op(allocator, code, OP_LOAD_NONE);
    case PY68_AST_NAME:
        status = py68_code_add_name(allocator, code, node->as.name.offset,
                                    node->as.name.length, &index);
        if (status != PY68_STATUS_OK) return status;
        return py68_emit_u16_op(allocator, code, OP_LOAD_GLOBAL, index);
    case PY68_AST_UNARY:
        status = py68_compile_expression(allocator, source,
                                          node->as.unary.operand, code, error);
        if (status != PY68_STATUS_OK) return status;
        if (node->as.unary.operator_kind == PY68_TOKEN_MINUS)
            return py68_emit_op(allocator, code, OP_NEGATE);
        if (node->as.unary.operator_kind == PY68_TOKEN_PLUS)
            return py68_emit_op(allocator, code, OP_POSITIVE);
        return py68_emit_op(allocator, code, OP_NOT);
    case PY68_AST_BINARY:
        status = py68_compile_expression(allocator, source,
                                         node->as.binary.left, code, error);
        if (status != PY68_STATUS_OK) return status;
        status = py68_compile_expression(allocator, source,
                                         node->as.binary.right, code, error);
        if (status != PY68_STATUS_OK) return status;
        switch ((Py68TokenKind)node->as.binary.operator_kind) {
        case PY68_TOKEN_PLUS: return py68_emit_op(allocator, code, OP_ADD);
        case PY68_TOKEN_MINUS: return py68_emit_op(allocator, code, OP_SUBTRACT);
        case PY68_TOKEN_STAR: return py68_emit_op(allocator, code, OP_MULTIPLY);
        case PY68_TOKEN_FLOOR_DIVIDE: return py68_emit_op(allocator, code, OP_FLOOR_DIVIDE);
        case PY68_TOKEN_PERCENT: return py68_emit_op(allocator, code, OP_MODULO);
        case PY68_TOKEN_EQUAL: return py68_emit_op(allocator, code, OP_EQUAL);
        case PY68_TOKEN_NOT_EQUAL: return py68_emit_op(allocator, code, OP_NOT_EQUAL);
        case PY68_TOKEN_LESS: return py68_emit_op(allocator, code, OP_LESS);
        case PY68_TOKEN_LESS_EQUAL: return py68_emit_op(allocator, code, OP_LESS_EQUAL);
        case PY68_TOKEN_GREATER: return py68_emit_op(allocator, code, OP_GREATER);
        case PY68_TOKEN_GREATER_EQUAL: return py68_emit_op(allocator, code, OP_GREATER_EQUAL);
        default: return PY68_STATUS_SOURCE_ERROR;
        }
    case PY68_AST_LIST:
        element_count = node->as.list_literal.elements.count;
        for (index = 0; index < element_count; ++index) {
            status = py68_compile_expression(allocator, source,
                node->as.list_literal.elements.items[index], code, error);
            if (status != PY68_STATUS_OK) return status;
        }
        return py68_emit_u16_op(allocator, code, OP_BUILD_LIST, element_count);
    case PY68_AST_CALL:
        status = py68_compile_expression(allocator, source,
                                         node->as.call.callee, code, error);
        if (status != PY68_STATUS_OK) return status;
        argument_count = node->as.call.arguments.count;
        for (index = 0; index < argument_count; ++index) {
            status = py68_compile_expression(allocator, source,
                node->as.call.arguments.items[index], code, error);
            if (status != PY68_STATUS_OK) return status;
        }
        return py68_emit_op(allocator, code, OP_CALL) == PY68_STATUS_OK
               ? py68_code_emit_u8(allocator, code, (Py68U8)argument_count)
               : PY68_STATUS_MEMORY_ERROR;
    case PY68_AST_INDEX:
        status = py68_compile_expression(allocator, source,
                                          node->as.index.container, code, error);
        if (status != PY68_STATUS_OK) return status;
        status = py68_compile_expression(allocator, source,
                                          node->as.index.index, code, error);
        if (status != PY68_STATUS_OK) return status;
        return py68_emit_op(allocator, code, OP_LOAD_INDEX);
    case PY68_AST_SLICE:
        status = py68_compile_expression(allocator, source,
                                          node->as.slice.container, code, error);
        if (status != PY68_STATUS_OK) return status;
        if (node->as.slice.start != NULL) {
            status = py68_compile_expression(allocator, source,
                                              node->as.slice.start, code, error);
            if (status != PY68_STATUS_OK) return status;
        } else {
            status = py68_emit_op(allocator, code, OP_LOAD_NONE);
            if (status != PY68_STATUS_OK) return status;
        }
        if (node->as.slice.end != NULL) {
            status = py68_compile_expression(allocator, source,
                                              node->as.slice.end, code, error);
            if (status != PY68_STATUS_OK) return status;
        } else {
            status = py68_emit_op(allocator, code, OP_LOAD_NONE);
            if (status != PY68_STATUS_OK) return status;
        }
        return py68_emit_op(allocator, code, OP_LOAD_SLICE);
    default:
        py68_compile_error(error, source, node->location,
                           "unsupported expression for compiler");
        return PY68_STATUS_SOURCE_ERROR;
    }
}

Py68Status py68_compile_module(Py68Allocator *allocator,
                               const Py68Source *source,
                               Py68AstNode *module,
                               Py68Code *code,
                               Py68Error *error)
{
    Py68Status status;
    py68_code_initialize(code);
    code->source_data = source->data;
    code->source_length = source->length;
    py68_error_clear(error);
    status = py68_compile_statements(allocator, source,
                                     &module->as.module.statements,
                                     code, error);
    if (status != PY68_STATUS_OK) {
        py68_code_destroy(allocator, code);
        return status;
    }
    return py68_emit_op(allocator, code, OP_HALT);
}
