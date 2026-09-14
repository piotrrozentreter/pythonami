/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_compiler.h"
#include "py68k_symbol.h"
#include "py68k_token.h"

#include <stddef.h>
#include <string.h>

static void py68_compile_error(Py68Error *error, const Py68Source *source,
                               Py68Location location, const char *message)
{
    py68_error_set(error, PY68_ERROR_SYNTAX, location, source->filename,
                   message);
}

typedef struct Py68LoopContext {
    Py68U32 continue_target;
    Py68U32 *break_operands;
    Py68U16 break_count;
    Py68U16 break_capacity;
    /* for-loops keep the range iterator under the body; break must POP it. */
    int pop_on_break;
} Py68LoopContext;

typedef struct Py68TryCompile {
    Py68AstList *finally_body;
    struct Py68TryCompile *parent;
} Py68TryCompile;

static void py68_loop_context_initialize(Py68LoopContext *loop,
                                         Py68U32 continue_target,
                                         int pop_on_break)
{
    loop->continue_target = continue_target;
    loop->break_operands = NULL;
    loop->break_count = 0;
    loop->break_capacity = 0;
    loop->pop_on_break = pop_on_break;
}

static Py68Status py68_loop_context_add_break(Py68Allocator *allocator,
                                              Py68LoopContext *loop,
                                              Py68U32 operand_offset)
{
    Py68U16 capacity;
    Py68U32 *replacement;
    if (loop->break_count == loop->break_capacity) {
        capacity = loop->break_capacity == 0 ? 4 :
                   (Py68U16)(loop->break_capacity * 2);
        if (capacity < loop->break_capacity) return PY68_STATUS_MEMORY_ERROR;
        replacement = (Py68U32 *)py68_realloc(
            allocator, PY68_MEM_TEMP, loop->break_operands,
            (Py68U32)loop->break_capacity * sizeof(Py68U32),
            (Py68U32)capacity * sizeof(Py68U32));
        if (replacement == NULL) return PY68_STATUS_MEMORY_ERROR;
        loop->break_operands = replacement;
        loop->break_capacity = capacity;
    }
    loop->break_operands[loop->break_count] = operand_offset;
    ++loop->break_count;
    return PY68_STATUS_OK;
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

static Py68Status py68_loop_context_patch_breaks(Py68Code *code,
                                                 Py68LoopContext *loop,
                                                 Py68U32 target)
{
    Py68U16 index;
    Py68Status status;
    for (index = 0; index < loop->break_count; ++index) {
        status = py68_patch_jump(code, loop->break_operands[index], target);
        if (status != PY68_STATUS_OK) return status;
    }
    return PY68_STATUS_OK;
}

static void py68_loop_context_destroy(Py68Allocator *allocator,
                                      Py68LoopContext *loop)
{
    py68_free(allocator, PY68_MEM_TEMP, loop->break_operands,
             (Py68U32)loop->break_capacity * sizeof(Py68U32));
    loop->break_operands = NULL;
    loop->break_count = 0;
    loop->break_capacity = 0;
}

/* Emit a load of the identifier at source[offset:offset+length]: a local
   slot if it is a parameter/local of `function`, otherwise a module or
   builtin global (resolved by name at runtime via OP_LOAD_GLOBAL). */
static Py68Status py68_emit_load_name(Py68Allocator *allocator,
                                      const Py68Source *source,
                                      const Py68FunctionSymbols *function,
                                      Py68Code *code, Py68U32 offset,
                                      Py68U16 length)
{
    Py68U16 slot;
    Py68U16 name_index;
    Py68Status status;
    if (py68_symbol_lookup_local(source, function, offset, length, &slot)) {
        return py68_emit_u16_op(allocator, code, OP_LOAD_LOCAL, slot);
    }
    status = py68_code_add_name(allocator, code, offset, length, &name_index);
    if (status != PY68_STATUS_OK) return status;
    return py68_emit_u16_op(allocator, code, OP_LOAD_GLOBAL, name_index);
}

static Py68Status py68_emit_store_name(Py68Allocator *allocator,
                                       const Py68Source *source,
                                       const Py68FunctionSymbols *function,
                                       Py68Code *code, Py68U32 offset,
                                       Py68U16 length)
{
    Py68U16 slot;
    Py68U16 name_index;
    Py68Status status;
    if (py68_symbol_lookup_local(source, function, offset, length, &slot)) {
        return py68_emit_u16_op(allocator, code, OP_STORE_LOCAL, slot);
    }
    status = py68_code_add_name(allocator, code, offset, length, &name_index);
    if (status != PY68_STATUS_OK) return status;
    return py68_emit_u16_op(allocator, code, OP_STORE_GLOBAL, name_index);
}

static Py68U8 py68_augmented_opcode(Py68U16 operator_kind)
{
    switch ((Py68TokenKind)operator_kind) {
    case PY68_TOKEN_PLUS_ASSIGN: return OP_ADD;
    case PY68_TOKEN_MINUS_ASSIGN: return OP_SUBTRACT;
    case PY68_TOKEN_STAR_ASSIGN: return OP_MULTIPLY;
    case PY68_TOKEN_SLASH_ASSIGN: return OP_TRUE_DIVIDE;
    case PY68_TOKEN_FLOOR_DIVIDE_ASSIGN: return OP_FLOOR_DIVIDE;
    case PY68_TOKEN_PERCENT_ASSIGN: return OP_MODULO;
    default: return OP_HALT;
    }
}

static Py68Status py68_compile_expression(Py68Allocator *allocator,
                                          const Py68Source *source,
                                          Py68AstNode *node, Py68Code *code,
                                          Py68Error *error,
                                          const Py68SymbolAnalysis *analysis,
                                          const Py68FunctionSymbols *function);

static Py68Status py68_compile_iterable(Py68Allocator *allocator,
                                        const Py68Source *source,
                                        Py68AstNode *iterable, Py68Code *code,
                                        Py68Error *error,
                                        const Py68SymbolAnalysis *analysis,
                                        const Py68FunctionSymbols *function)
{
    Py68Status status;
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
                code, error, analysis, function);
            if (status != PY68_STATUS_OK) return status;
        }
        return py68_emit_u8_op(allocator, code, OP_RANGE_INIT,
                               (Py68U8)arg_count);
    }
    status = py68_compile_expression(allocator, source, iterable, code, error,
                                     analysis, function);
    if (status != PY68_STATUS_OK) return status;
    return py68_emit_u8_op(allocator, code, OP_RANGE_INIT, 1);
}

static Py68Status py68_compile_comp_for(Py68Allocator *allocator,
                                        const Py68Source *source,
                                        Py68AstList *generators, Py68U16 index,
                                        Py68AstNode *elt, Py68AstNode *value,
                                        Py68U8 append_opcode, Py68Code *code,
                                        Py68Error *error,
                                        const Py68SymbolAnalysis *analysis,
                                        const Py68FunctionSymbols *function)
{
    Py68AstNode *clause;
    Py68U32 range_next_op;
    Py68U32 end_operand;
    Py68U32 back_operand;
    Py68U32 filter_operand;
    Py68U16 filter_index;
    Py68U8 append_depth;
    Py68Status status;

    if (index >= generators->count) return PY68_STATUS_INTERNAL_ERROR;
    clause = generators->items[index];
    status = py68_compile_iterable(allocator, source,
                                   clause->as.comprehension_for.iterable,
                                   code, error, analysis, function);
    if (status != PY68_STATUS_OK) return status;
    range_next_op = code->bytecode_length;
    status = py68_emit_jump(allocator, code, OP_RANGE_NEXT, &end_operand);
    if (status != PY68_STATUS_OK) return status;
    status = py68_emit_store_name(allocator, source, function, code,
                                  clause->as.comprehension_for.name_offset,
                                  clause->as.comprehension_for.name_length);
    if (status != PY68_STATUS_OK) return status;
    for (filter_index = 0;
         filter_index < clause->as.comprehension_for.ifs.count;
         ++filter_index) {
        status = py68_compile_expression(
            allocator, source,
            clause->as.comprehension_for.ifs.items[filter_index],
            code, error, analysis, function);
        if (status != PY68_STATUS_OK) return status;
        status = py68_emit_jump(allocator, code, OP_JUMP_IF_FALSE,
                                &filter_operand);
        if (status != PY68_STATUS_OK) return status;
        status = py68_patch_jump(code, filter_operand, range_next_op);
        if (status != PY68_STATUS_OK) return status;
    }
    if ((Py68U16)(index + 1) < generators->count) {
        status = py68_compile_comp_for(allocator, source, generators,
                                       (Py68U16)(index + 1), elt, value,
                                       append_opcode, code, error, analysis,
                                       function);
        if (status != PY68_STATUS_OK) return status;
    } else if (append_opcode == OP_MAP_ADD) {
        if ((Py68U16)(generators->count + 2) > 255U) {
            py68_compile_error(error, source, clause->location,
                               "comprehension is too deeply nested");
            return PY68_STATUS_SOURCE_ERROR;
        }
        status = py68_compile_expression(allocator, source, elt, code, error,
                                         analysis, function);
        if (status != PY68_STATUS_OK) return status;
        status = py68_compile_expression(allocator, source, value, code, error,
                                         analysis, function);
        if (status != PY68_STATUS_OK) return status;
        append_depth = (Py68U8)(generators->count + 2);
        status = py68_emit_u8_op(allocator, code, OP_MAP_ADD, append_depth);
        if (status != PY68_STATUS_OK) return status;
    } else {
        if ((Py68U16)(generators->count + 1) > 255U) {
            py68_compile_error(error, source, clause->location,
                               "comprehension is too deeply nested");
            return PY68_STATUS_SOURCE_ERROR;
        }
        status = py68_compile_expression(allocator, source, elt, code, error,
                                         analysis, function);
        if (status != PY68_STATUS_OK) return status;
        append_depth = (Py68U8)(generators->count + 1);
        status = py68_emit_u8_op(allocator, code, append_opcode, append_depth);
        if (status != PY68_STATUS_OK) return status;
    }
    status = py68_emit_jump(allocator, code, OP_JUMP, &back_operand);
    if (status != PY68_STATUS_OK) return status;
    status = py68_patch_jump(code, back_operand, range_next_op);
    if (status != PY68_STATUS_OK) return status;
    return py68_patch_jump(code, end_operand, code->bytecode_length);
}

static Py68Status py68_compile_comprehension(Py68Allocator *allocator,
                                             const Py68Source *source,
                                             Py68AstNode *node, Py68U8 build_op,
                                             Py68U8 append_op, Py68Code *code,
                                             Py68Error *error,
                                             const Py68SymbolAnalysis *analysis,
                                             const Py68FunctionSymbols *function)
{
    Py68Status status;
    if (node->as.comprehension.generators.count == 0) {
        py68_compile_error(error, source, node->location,
                           "comprehension is missing a for clause");
        return PY68_STATUS_SOURCE_ERROR;
    }
    status = py68_emit_u16_op(allocator, code, build_op, 0);
    if (status != PY68_STATUS_OK) return status;
    return py68_compile_comp_for(allocator, source,
                                 &node->as.comprehension.generators, 0,
                                 node->as.comprehension.elt,
                                 node->as.comprehension.value, append_op,
                                 code, error, analysis, function);
}

static Py68Status py68_compile_finally_chain(Py68Allocator *allocator,
                                             const Py68Source *source,
                                             Py68Code *code, Py68Error *error,
                                             Py68LoopContext *loop,
                                             const Py68SymbolAnalysis *analysis,
                                             const Py68FunctionSymbols *function,
                                             Py68TryCompile *try_ctx);

static Py68Status py68_compile_statements(Py68Allocator *allocator,
                                          const Py68Source *source,
                                          Py68AstList *statements,
                                          Py68Code *code, Py68Error *error,
                                          Py68LoopContext *loop,
                                          const Py68SymbolAnalysis *analysis,
                                          const Py68FunctionSymbols *function,
                                          Py68TryCompile *try_ctx)
{
    Py68U16 index;
    Py68AstNode *statement;
    Py68Status status;
    for (index = 0; index < statements->count; ++index) {
        statement = statements->items[index];
        code->emit_line = statement->location.line;
        switch ((Py68AstKind)statement->kind) {
        case PY68_AST_ASSIGN:
            if (statement->as.assign.target != NULL &&
                statement->as.assign.target->kind == PY68_AST_ATTRIBUTE) {
                Py68AstNode *attr = statement->as.assign.target;
                Py68U16 name_index;
                status = py68_compile_expression(
                    allocator, source, attr->as.attribute.value,
                    code, error, analysis, function);
                if (status != PY68_STATUS_OK) return status;
                status = py68_compile_expression(
                    allocator, source, statement->as.assign.value,
                    code, error, analysis, function);
                if (status != PY68_STATUS_OK) return status;
                status = py68_code_add_name(allocator, code,
                                            attr->as.attribute.name_offset,
                                            attr->as.attribute.name_length,
                                            &name_index);
                if (status != PY68_STATUS_OK) return status;
                status = py68_emit_u16_op(allocator, code, OP_STORE_ATTR,
                                          name_index);
            } else if (statement->as.assign.target != NULL &&
                statement->as.assign.target->kind == PY68_AST_INDEX) {
                Py68AstNode *index_target = statement->as.assign.target;
                status = py68_compile_expression(
                    allocator, source, index_target->as.index.container,
                    code, error, analysis, function);
                if (status != PY68_STATUS_OK) return status;
                status = py68_compile_expression(
                    allocator, source, index_target->as.index.index,
                    code, error, analysis, function);
                if (status != PY68_STATUS_OK) return status;
                status = py68_compile_expression(
                    allocator, source, statement->as.assign.value,
                    code, error, analysis, function);
                if (status != PY68_STATUS_OK) return status;
                status = py68_emit_op(allocator, code, OP_STORE_INDEX);
            } else {
                status = py68_compile_expression(allocator, source,
                                                 statement->as.assign.value,
                                                 code, error, analysis,
                                                 function);
                if (status != PY68_STATUS_OK) return status;
                status = py68_emit_store_name(
                    allocator, source, function, code,
                    statement->as.assign.name_offset,
                    statement->as.assign.name_length);
            }
            break;
        case PY68_AST_AUGMENTED_ASSIGN: {
            Py68U8 opcode = py68_augmented_opcode(
                statement->as.augmented_assign.operator_kind);
            status = py68_emit_load_name(
                allocator, source, function, code,
                statement->as.augmented_assign.target->as.name.offset,
                statement->as.augmented_assign.target->as.name.length);
            if (status != PY68_STATUS_OK) return status;
            status = py68_compile_expression(
                allocator, source, statement->as.augmented_assign.value,
                code, error, analysis, function);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_op(allocator, code, opcode);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_store_name(
                allocator, source, function, code,
                statement->as.augmented_assign.target->as.name.offset,
                statement->as.augmented_assign.target->as.name.length);
            break;
        }
        case PY68_AST_EXPRESSION_STATEMENT:
            status = py68_compile_expression(allocator, source,
                                             statement->as.expression_statement.value,
                                             code, error, analysis, function);
            if (status == PY68_STATUS_OK) status = py68_emit_op(
                allocator, code, OP_POP);
            break;
        case PY68_AST_IF: {
            Py68U32 false_operand;
            Py68U32 end_operand;
            status = py68_compile_expression(allocator, source,
                                              statement->as.if_statement.condition,
                                              code, error, analysis, function);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_jump(allocator, code, OP_JUMP_IF_FALSE,
                                    &false_operand);
            if (status != PY68_STATUS_OK) return status;
            status = py68_compile_statements(allocator, source,
                                             &statement->as.if_statement.body,
                                             code, error, loop, analysis,
                                             function, try_ctx);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_jump(allocator, code, OP_JUMP, &end_operand);
            if (status != PY68_STATUS_OK) return status;
            status = py68_patch_jump(code, false_operand, code->bytecode_length);
            if (status != PY68_STATUS_OK) return status;
            status = py68_compile_statements(allocator, source,
                                             &statement->as.if_statement.else_body,
                                             code, error, loop, analysis,
                                             function, try_ctx);
            if (status != PY68_STATUS_OK) return status;
            status = py68_patch_jump(code, end_operand, code->bytecode_length);
            break;
        }
        case PY68_AST_WHILE: {
            Py68U32 start = code->bytecode_length;
            Py68U32 end_operand;
            Py68U32 back_operand;
            Py68LoopContext while_loop;
            py68_loop_context_initialize(&while_loop, start, 0);
            status = py68_compile_expression(allocator, source,
                                              statement->as.while_statement.condition,
                                              code, error, analysis, function);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_jump(allocator, code, OP_JUMP_IF_FALSE,
                                    &end_operand);
            if (status != PY68_STATUS_OK) return status;
            status = py68_compile_statements(allocator, source,
                                             &statement->as.while_statement.body,
                                             code, error, &while_loop,
                                             analysis, function, try_ctx);
            if (status != PY68_STATUS_OK) {
                py68_loop_context_destroy(allocator, &while_loop);
                return status;
            }
            status = py68_emit_jump(allocator, code, OP_JUMP, &back_operand);
            if (status == PY68_STATUS_OK)
                status = py68_patch_jump(code, back_operand, start);
            if (status == PY68_STATUS_OK)
                status = py68_patch_jump(code, end_operand, code->bytecode_length);
            if (status == PY68_STATUS_OK &&
                statement->as.while_statement.else_body.count > 0) {
                status = py68_compile_statements(
                    allocator, source,
                    &statement->as.while_statement.else_body,
                    code, error, loop, analysis, function, try_ctx);
            }
            if (status == PY68_STATUS_OK)
                status = py68_loop_context_patch_breaks(code, &while_loop,
                                                        code->bytecode_length);
            py68_loop_context_destroy(allocator, &while_loop);
            break;
        }
        case PY68_AST_FOR: {
            Py68U32 range_next_op;
            Py68U32 back_operand;
            Py68U32 end_operand;
            Py68LoopContext for_loop;
            Py68AstNode *iterable = statement->as.for_statement.iterable;
            status = py68_compile_iterable(allocator, source, iterable, code,
                                           error, analysis, function);
            if (status != PY68_STATUS_OK) return status;
            range_next_op = code->bytecode_length;
            py68_loop_context_initialize(&for_loop, range_next_op, 1);
            status = py68_emit_jump(allocator, code, OP_RANGE_NEXT, &end_operand);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_store_name(allocator, source, function, code,
                                          statement->as.for_statement.name_offset,
                                          statement->as.for_statement.name_length);
            if (status != PY68_STATUS_OK) return status;
            status = py68_compile_statements(allocator, source,
                                             &statement->as.for_statement.body,
                                             code, error, &for_loop, analysis,
                                             function, try_ctx);
            if (status != PY68_STATUS_OK) {
                py68_loop_context_destroy(allocator, &for_loop);
                return status;
            }
            status = py68_emit_jump(allocator, code, OP_JUMP, &back_operand);
            if (status == PY68_STATUS_OK)
                status = py68_patch_jump(code, back_operand, range_next_op);
            if (status == PY68_STATUS_OK)
                status = py68_patch_jump(code, end_operand, code->bytecode_length);
            if (status == PY68_STATUS_OK &&
                statement->as.for_statement.else_body.count > 0) {
                status = py68_compile_statements(
                    allocator, source,
                    &statement->as.for_statement.else_body,
                    code, error, loop, analysis, function, try_ctx);
            }
            if (status == PY68_STATUS_OK)
                status = py68_loop_context_patch_breaks(code, &for_loop,
                                                        code->bytecode_length);
            py68_loop_context_destroy(allocator, &for_loop);
            break;
        }
        case PY68_AST_BREAK: {
            Py68U32 break_operand;
            if (loop == NULL) {
                py68_compile_error(error, source, statement->location,
                                   "break outside loop");
                return PY68_STATUS_SOURCE_ERROR;
            }
            status = py68_compile_finally_chain(allocator, source, code, error,
                                               loop, analysis, function,
                                               try_ctx);
            if (status != PY68_STATUS_OK) return status;
            if (loop->pop_on_break) {
                status = py68_emit_op(allocator, code, OP_POP);
                if (status != PY68_STATUS_OK) return status;
            }
            status = py68_emit_jump(allocator, code, OP_JUMP, &break_operand);
            if (status != PY68_STATUS_OK) return status;
            status = py68_loop_context_add_break(allocator, loop, break_operand);
            break;
        }
        case PY68_AST_CONTINUE: {
            Py68U32 continue_operand;
            if (loop == NULL) {
                py68_compile_error(error, source, statement->location,
                                   "continue outside loop");
                return PY68_STATUS_SOURCE_ERROR;
            }
            status = py68_compile_finally_chain(allocator, source, code, error,
                                               loop, analysis, function,
                                               try_ctx);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_jump(allocator, code, OP_JUMP, &continue_operand);
            if (status != PY68_STATUS_OK) return status;
            status = py68_patch_jump(code, continue_operand, loop->continue_target);
            break;
        }
        case PY68_AST_RETURN:
            if (function == NULL) {
                py68_compile_error(error, source, statement->location,
                                   "return outside function");
                return PY68_STATUS_SOURCE_ERROR;
            }
            if (statement->as.return_statement.value != NULL) {
                status = py68_compile_expression(
                    allocator, source, statement->as.return_statement.value,
                    code, error, analysis, function);
                if (status != PY68_STATUS_OK) return status;
                status = py68_compile_finally_chain(allocator, source, code,
                                                    error, loop, analysis,
                                                    function, try_ctx);
                if (status != PY68_STATUS_OK) return status;
                status = py68_emit_op(allocator, code, OP_RETURN_VALUE);
            } else {
                status = py68_compile_finally_chain(allocator, source, code,
                                                    error, loop, analysis,
                                                    function, try_ctx);
                if (status != PY68_STATUS_OK) return status;
                status = py68_emit_op(allocator, code, OP_RETURN_NONE);
            }
            break;
        case PY68_AST_PASS:
            status = PY68_STATUS_OK;
            break;
        case PY68_AST_FUNCTION_DEF: {
            const Py68FunctionSymbols *nested_symbols;
            Py68Code *nested_code;
            Py68U16 nested_index;
            Py68U16 const_index;
            Py68U16 def_name_index;
            Py68Constant constant;
            if (function != NULL) {
                py68_compile_error(error, source, statement->location,
                                   "nested functions are not supported");
                return PY68_STATUS_SOURCE_ERROR;
            }
            nested_symbols = py68_symbol_find_function(analysis, statement);
            if (nested_symbols == NULL) {
                py68_compile_error(error, source, statement->location,
                                   "internal error: missing function symbols");
                return PY68_STATUS_INTERNAL_ERROR;
            }
            status = py68_code_reserve_nested(allocator, code, &nested_code,
                                              &nested_index);
            if (status != PY68_STATUS_OK) return status;
            nested_code->source_data = source->data;
            nested_code->source_length = source->length;
            nested_code->name_offset = statement->as.function_def.name_offset;
            nested_code->name_length = statement->as.function_def.name_length;
            nested_code->argument_count = nested_symbols->parameter_count;
            nested_code->local_count = (Py68U16)(
                nested_symbols->parameter_count + nested_symbols->local_count);
            status = py68_compile_statements(
                allocator, source, &statement->as.function_def.body,
                nested_code, error, NULL, analysis, nested_symbols, NULL);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_op(allocator, nested_code, OP_RETURN_NONE);
            if (status != PY68_STATUS_OK) return status;
            constant.kind = PY68_CONSTANT_CODE;
            constant.flags = 0;
            constant.integer = (Py68I32)nested_index;
            constant.offset = 0;
            constant.length = 0;
            status = py68_code_add_constant(allocator, code, constant,
                                            &const_index);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_u16_op(allocator, code, OP_MAKE_FUNCTION,
                                      const_index);
            if (status != PY68_STATUS_OK) return status;
            status = py68_code_add_name(allocator, code,
                                        statement->as.function_def.name_offset,
                                        statement->as.function_def.name_length,
                                        &def_name_index);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_u16_op(allocator, code, OP_STORE_GLOBAL,
                                      def_name_index);
            break;
        }
        case PY68_AST_RAISE:
            if (statement->as.raise_statement.value != NULL) {
                status = py68_compile_expression(
                    allocator, source, statement->as.raise_statement.value,
                    code, error, analysis, function);
                if (status != PY68_STATUS_OK) return status;
            } else {
                status = py68_emit_op(allocator, code, OP_LOAD_NONE);
                if (status != PY68_STATUS_OK) return status;
            }
            status = py68_emit_op(allocator, code, OP_RAISE);
            break;
        case PY68_AST_IMPORT: {
            Py68U16 name_index;
            status = py68_code_add_name(allocator, code,
                                        statement->as.import_statement.name_offset,
                                        statement->as.import_statement.name_length,
                                        &name_index);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_u16_op(allocator, code, OP_IMPORT_NAME,
                                      name_index);
            if (status != PY68_STATUS_OK) return status;
            if (statement->as.import_statement.as_length != 0)
                status = py68_emit_store_name(
                    allocator, source, function, code,
                    statement->as.import_statement.as_offset,
                    statement->as.import_statement.as_length);
            else
                status = py68_emit_store_name(
                    allocator, source, function, code,
                    statement->as.import_statement.name_offset,
                    statement->as.import_statement.name_length);
            break;
        }
        case PY68_AST_IMPORT_FROM: {
            Py68U16 name_index;
            Py68U16 alias_index;
            status = py68_code_add_name(allocator, code,
                                        statement->as.import_from.module_offset,
                                        statement->as.import_from.module_length,
                                        &name_index);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_u16_op(allocator, code, OP_IMPORT_NAME,
                                      name_index);
            if (status != PY68_STATUS_OK) return status;
            for (index = 0; index < statement->as.import_from.names.count;
                 ++index) {
                Py68AstNode *alias =
                    statement->as.import_from.names.items[index];
                status = py68_emit_op(allocator, code, OP_DUP);
                if (status != PY68_STATUS_OK) return status;
                status = py68_code_add_name(allocator, code,
                                            alias->as.import_alias.name_offset,
                                            alias->as.import_alias.name_length,
                                            &alias_index);
                if (status != PY68_STATUS_OK) return status;
                status = py68_emit_u16_op(allocator, code, OP_IMPORT_FROM,
                                          alias_index);
                if (status != PY68_STATUS_OK) return status;
                if (alias->as.import_alias.as_length != 0)
                    status = py68_emit_store_name(
                        allocator, source, function, code,
                        alias->as.import_alias.as_offset,
                        alias->as.import_alias.as_length);
                else
                    status = py68_emit_store_name(
                        allocator, source, function, code,
                        alias->as.import_alias.name_offset,
                        alias->as.import_alias.name_length);
                if (status != PY68_STATUS_OK) return status;
            }
            status = py68_emit_op(allocator, code, OP_POP);
            break;
        }
        case PY68_AST_TRY: {
            Py68U32 handler_operand;
            Py68U32 success_operand;
            Py68U32 done_jumps[16];
            Py68U16 done_count = 0;
            Py68TryCompile child;
            Py68U16 handler_index;
            child.finally_body = &statement->as.try_statement.finally_body;
            child.parent = try_ctx;
            status = py68_emit_jump(allocator, code, OP_SETUP_TRY,
                                    &handler_operand);
            if (status != PY68_STATUS_OK) return status;
            status = py68_compile_statements(allocator, source,
                                             &statement->as.try_statement.body,
                                             code, error, loop, analysis,
                                             function, &child);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_op(allocator, code, OP_POP_TRY);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_jump(allocator, code, OP_JUMP, &success_operand);
            if (status != PY68_STATUS_OK) return status;
            if (done_count >= 16) return PY68_STATUS_INTERNAL_ERROR;
            done_jumps[done_count++] = success_operand;
            status = py68_patch_jump(code, handler_operand,
                                     code->bytecode_length);
            if (status != PY68_STATUS_OK) return status;
            for (handler_index = 0;
                 handler_index < statement->as.try_statement.handlers.count;
                 ++handler_index) {
                Py68AstNode *handler =
                    statement->as.try_statement.handlers.items[handler_index];
                Py68U32 next_handler = 0;
                Py68U32 handled_jump;
                if (handler->as.except_handler.type != NULL) {
                    status = py68_compile_expression(
                        allocator, source, handler->as.except_handler.type,
                        code, error, analysis, function);
                    if (status != PY68_STATUS_OK) return status;
                    status = py68_emit_jump(allocator, code, OP_CHECK_EXCEPT,
                                            &next_handler);
                    if (status != PY68_STATUS_OK) return status;
                }
                if (handler->as.except_handler.as_length != 0) {
                    status = py68_emit_store_name(
                        allocator, source, function, code,
                        handler->as.except_handler.as_offset,
                        handler->as.except_handler.as_length);
                } else {
                    status = py68_emit_op(allocator, code, OP_POP);
                }
                if (status != PY68_STATUS_OK) return status;
                status = py68_compile_statements(
                    allocator, source, &handler->as.except_handler.body,
                    code, error, loop, analysis, function, &child);
                if (status != PY68_STATUS_OK) return status;
                status = py68_emit_jump(allocator, code, OP_JUMP, &handled_jump);
                if (status != PY68_STATUS_OK) return status;
                if (done_count >= 16) return PY68_STATUS_INTERNAL_ERROR;
                done_jumps[done_count++] = handled_jump;
                if (next_handler != 0) {
                    status = py68_patch_jump(code, next_handler,
                                             code->bytecode_length);
                    if (status != PY68_STATUS_OK) return status;
                }
            }
            if (statement->as.try_statement.finally_body.count > 0) {
                status = py68_compile_statements(
                    allocator, source,
                    &statement->as.try_statement.finally_body,
                    code, error, loop, analysis, function, try_ctx);
                if (status != PY68_STATUS_OK) return status;
            }
            status = py68_emit_op(allocator, code, OP_RAISE);
            if (status != PY68_STATUS_OK) return status;
            for (handler_index = 0; handler_index < done_count;
                 ++handler_index) {
                status = py68_patch_jump(code, done_jumps[handler_index],
                                         code->bytecode_length);
                if (status != PY68_STATUS_OK) return status;
            }
            if (statement->as.try_statement.finally_body.count > 0) {
                status = py68_compile_statements(
                    allocator, source,
                    &statement->as.try_statement.finally_body,
                    code, error, loop, analysis, function, try_ctx);
            }
            break;
        }
        case PY68_AST_WITH: {
            Py68U16 enter_index;
            Py68U16 exit_index;
            Py68U32 handler_operand;
            Py68U32 end_operand;
            status = py68_compile_expression(
                allocator, source, statement->as.with_statement.context,
                code, error, analysis, function);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_op(allocator, code, OP_DUP);
            if (status != PY68_STATUS_OK) return status;
            status = py68_code_add_interned_name(allocator, code, "__enter__",
                                                 9, &enter_index);
            if (status != PY68_STATUS_OK) return status;
            status = py68_code_add_interned_name(allocator, code, "__exit__",
                                                 8, &exit_index);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_u16_op(allocator, code, OP_LOAD_ATTR,
                                      enter_index);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_u8_op(allocator, code, OP_CALL, 0);
            if (status != PY68_STATUS_OK) return status;
            if (statement->as.with_statement.as_length != 0)
                status = py68_emit_store_name(
                    allocator, source, function, code,
                    statement->as.with_statement.as_offset,
                    statement->as.with_statement.as_length);
            else
                status = py68_emit_op(allocator, code, OP_POP);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_jump(allocator, code, OP_SETUP_TRY,
                                    &handler_operand);
            if (status != PY68_STATUS_OK) return status;
            status = py68_compile_statements(allocator, source,
                                             &statement->as.with_statement.body,
                                             code, error, loop, analysis,
                                             function, try_ctx);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_op(allocator, code, OP_POP_TRY);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_u16_op(allocator, code, OP_LOAD_ATTR,
                                      exit_index);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_op(allocator, code, OP_LOAD_NONE);
            if (status == PY68_STATUS_OK)
                status = py68_emit_op(allocator, code, OP_LOAD_NONE);
            if (status == PY68_STATUS_OK)
                status = py68_emit_op(allocator, code, OP_LOAD_NONE);
            if (status == PY68_STATUS_OK)
                status = py68_emit_u8_op(allocator, code, OP_CALL, 3);
            if (status == PY68_STATUS_OK)
                status = py68_emit_op(allocator, code, OP_POP);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_jump(allocator, code, OP_JUMP, &end_operand);
            if (status != PY68_STATUS_OK) return status;
            status = py68_patch_jump(code, handler_operand,
                                     code->bytecode_length);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_op(allocator, code, OP_ROT_TWO);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_u16_op(allocator, code, OP_LOAD_ATTR,
                                      exit_index);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_op(allocator, code, OP_LOAD_NONE);
            if (status == PY68_STATUS_OK)
                status = py68_emit_op(allocator, code, OP_LOAD_NONE);
            if (status == PY68_STATUS_OK)
                status = py68_emit_op(allocator, code, OP_LOAD_NONE);
            if (status == PY68_STATUS_OK)
                status = py68_emit_u8_op(allocator, code, OP_CALL, 3);
            if (status == PY68_STATUS_OK)
                status = py68_emit_op(allocator, code, OP_POP);
            if (status == PY68_STATUS_OK)
                status = py68_emit_op(allocator, code, OP_RAISE);
            if (status != PY68_STATUS_OK) return status;
            status = py68_patch_jump(code, end_operand, code->bytecode_length);
            break;
        }
        default:
            status = PY68_STATUS_OK;
            break;
        }
        if (status != PY68_STATUS_OK) return status;
    }
    return PY68_STATUS_OK;
}

static Py68Status py68_compile_finally_chain(Py68Allocator *allocator,
                                             const Py68Source *source,
                                             Py68Code *code, Py68Error *error,
                                             Py68LoopContext *loop,
                                             const Py68SymbolAnalysis *analysis,
                                             const Py68FunctionSymbols *function,
                                             Py68TryCompile *try_ctx)
{
    Py68TryCompile *cursor = try_ctx;
    Py68Status status = PY68_STATUS_OK;
    while (cursor != NULL && status == PY68_STATUS_OK) {
        if (cursor->finally_body != NULL && cursor->finally_body->count > 0)
            status = py68_compile_statements(
                allocator, source, cursor->finally_body, code, error, loop,
                analysis, function, cursor->parent);
        cursor = cursor->parent;
    }
    return status;
}

static Py68Status py68_compile_expression(Py68Allocator *allocator,
                                          const Py68Source *source,
                                          Py68AstNode *node, Py68Code *code,
                                          Py68Error *error,
                                          const Py68SymbolAnalysis *analysis,
                                          const Py68FunctionSymbols *function)
{
    Py68U16 index;
    Py68U16 argument_count;
    Py68U16 element_count;
    Py68Status status;
    if (node == NULL) return PY68_STATUS_INTERNAL_ERROR;
    code->emit_line = node->location.line;
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
    case PY68_AST_FLOAT: {
        Py68Constant constant;
        constant.kind = PY68_CONSTANT_FLOAT;
        constant.flags = 0;
        constant.integer = (Py68I32)node->as.float_literal.bits;
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
        return py68_emit_load_name(allocator, source, function, code,
                                   node->as.name.offset,
                                   node->as.name.length);
    case PY68_AST_UNARY:
        status = py68_compile_expression(allocator, source,
                                          node->as.unary.operand, code, error,
                                          analysis, function);
        if (status != PY68_STATUS_OK) return status;
        if (node->as.unary.operator_kind == PY68_TOKEN_MINUS)
            return py68_emit_op(allocator, code, OP_NEGATE);
        if (node->as.unary.operator_kind == PY68_TOKEN_PLUS)
            return py68_emit_op(allocator, code, OP_POSITIVE);
        return py68_emit_op(allocator, code, OP_NOT);
    case PY68_AST_BINARY: {
        Py68U32 short_circuit_operand;
        Py68TokenKind operator_kind =
            (Py68TokenKind)node->as.binary.operator_kind;
        /* Short-circuit: leave left on the stack when it already decides. */
        if (operator_kind == PY68_TOKEN_AND || operator_kind == PY68_TOKEN_OR) {
            status = py68_compile_expression(allocator, source,
                                             node->as.binary.left, code, error,
                                             analysis, function);
            if (status != PY68_STATUS_OK) return status;
            status = py68_emit_jump(
                allocator, code,
                operator_kind == PY68_TOKEN_AND ? OP_JUMP_IF_FALSE_OR_POP
                                                : OP_JUMP_IF_TRUE_OR_POP,
                &short_circuit_operand);
            if (status != PY68_STATUS_OK) return status;
            status = py68_compile_expression(allocator, source,
                                             node->as.binary.right, code,
                                             error, analysis, function);
            if (status != PY68_STATUS_OK) return status;
            return py68_patch_jump(code, short_circuit_operand,
                                   code->bytecode_length);
        }
        status = py68_compile_expression(allocator, source,
                                         node->as.binary.left, code, error,
                                         analysis, function);
        if (status != PY68_STATUS_OK) return status;
        status = py68_compile_expression(allocator, source,
                                         node->as.binary.right, code, error,
                                         analysis, function);
        if (status != PY68_STATUS_OK) return status;
        switch (operator_kind) {
        case PY68_TOKEN_PLUS: return py68_emit_op(allocator, code, OP_ADD);
        case PY68_TOKEN_MINUS: return py68_emit_op(allocator, code, OP_SUBTRACT);
        case PY68_TOKEN_STAR: return py68_emit_op(allocator, code, OP_MULTIPLY);
        case PY68_TOKEN_SLASH: return py68_emit_op(allocator, code, OP_TRUE_DIVIDE);
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
    }
    case PY68_AST_LIST:
        element_count = node->as.list_literal.elements.count;
        for (index = 0; index < element_count; ++index) {
            status = py68_compile_expression(allocator, source,
                node->as.list_literal.elements.items[index], code, error,
                analysis, function);
            if (status != PY68_STATUS_OK) return status;
        }
        return py68_emit_u16_op(allocator, code, OP_BUILD_LIST, element_count);
    case PY68_AST_TUPLE:
        element_count = node->as.list_literal.elements.count;
        for (index = 0; index < element_count; ++index) {
            status = py68_compile_expression(allocator, source,
                node->as.list_literal.elements.items[index], code, error,
                analysis, function);
            if (status != PY68_STATUS_OK) return status;
        }
        return py68_emit_u16_op(allocator, code, OP_BUILD_TUPLE, element_count);
    case PY68_AST_SET:
        element_count = node->as.list_literal.elements.count;
        for (index = 0; index < element_count; ++index) {
            status = py68_compile_expression(allocator, source,
                node->as.list_literal.elements.items[index], code, error,
                analysis, function);
            if (status != PY68_STATUS_OK) return status;
        }
        return py68_emit_u16_op(allocator, code, OP_BUILD_SET, element_count);
    case PY68_AST_DICT:
        element_count = node->as.dict_literal.keys.count;
        for (index = 0; index < element_count; ++index) {
            status = py68_compile_expression(allocator, source,
                node->as.dict_literal.keys.items[index], code, error,
                analysis, function);
            if (status != PY68_STATUS_OK) return status;
            status = py68_compile_expression(allocator, source,
                node->as.dict_literal.values.items[index], code, error,
                analysis, function);
            if (status != PY68_STATUS_OK) return status;
        }
        return py68_emit_u16_op(allocator, code, OP_BUILD_DICT, element_count);
    case PY68_AST_ATTRIBUTE: {
        Py68U16 name_index;
        status = py68_compile_expression(allocator, source,
                                          node->as.attribute.value, code,
                                          error, analysis, function);
        if (status != PY68_STATUS_OK) return status;
        status = py68_code_add_name(allocator, code,
                                    node->as.attribute.name_offset,
                                    node->as.attribute.name_length,
                                    &name_index);
        if (status != PY68_STATUS_OK) return status;
        return py68_emit_u16_op(allocator, code, OP_LOAD_ATTR, name_index);
    }
    case PY68_AST_CALL:
        status = py68_compile_expression(allocator, source,
                                         node->as.call.callee, code, error,
                                         analysis, function);
        if (status != PY68_STATUS_OK) return status;
        argument_count = node->as.call.arguments.count;
        for (index = 0; index < argument_count; ++index) {
            status = py68_compile_expression(allocator, source,
                node->as.call.arguments.items[index], code, error, analysis,
                function);
            if (status != PY68_STATUS_OK) return status;
        }
        return py68_emit_op(allocator, code, OP_CALL) == PY68_STATUS_OK
               ? py68_code_emit_u8(allocator, code, (Py68U8)argument_count)
               : PY68_STATUS_MEMORY_ERROR;
    case PY68_AST_INDEX:
        status = py68_compile_expression(allocator, source,
                                          node->as.index.container, code,
                                          error, analysis, function);
        if (status != PY68_STATUS_OK) return status;
        status = py68_compile_expression(allocator, source,
                                          node->as.index.index, code, error,
                                          analysis, function);
        if (status != PY68_STATUS_OK) return status;
        return py68_emit_op(allocator, code, OP_LOAD_INDEX);
    case PY68_AST_SLICE:
        status = py68_compile_expression(allocator, source,
                                          node->as.slice.container, code,
                                          error, analysis, function);
        if (status != PY68_STATUS_OK) return status;
        if (node->as.slice.start != NULL) {
            status = py68_compile_expression(allocator, source,
                                              node->as.slice.start, code,
                                              error, analysis, function);
            if (status != PY68_STATUS_OK) return status;
        } else {
            status = py68_emit_op(allocator, code, OP_LOAD_NONE);
            if (status != PY68_STATUS_OK) return status;
        }
        if (node->as.slice.end != NULL) {
            status = py68_compile_expression(allocator, source,
                                              node->as.slice.end, code, error,
                                              analysis, function);
            if (status != PY68_STATUS_OK) return status;
        } else {
            status = py68_emit_op(allocator, code, OP_LOAD_NONE);
            if (status != PY68_STATUS_OK) return status;
        }
        return py68_emit_op(allocator, code, OP_LOAD_SLICE);
    case PY68_AST_LIST_COMP:
        return py68_compile_comprehension(allocator, source, node, OP_BUILD_LIST,
                                          OP_LIST_APPEND, code, error,
                                          analysis, function);
    case PY68_AST_SET_COMP:
        return py68_compile_comprehension(allocator, source, node, OP_BUILD_SET,
                                          OP_SET_ADD, code, error, analysis,
                                          function);
    case PY68_AST_DICT_COMP:
        return py68_compile_comprehension(allocator, source, node, OP_BUILD_DICT,
                                          OP_MAP_ADD, code, error, analysis,
                                          function);
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
    Py68SymbolAnalysis analysis;
    py68_error_clear(error);
    py68_code_initialize(code);
    status = py68_symbol_analyze(allocator, source, module, &analysis, error);
    if (status == PY68_STATUS_OK) {
        code->source_data = source->data;
        code->source_length = source->length;
        status = py68_compile_statements(allocator, source,
                                         &module->as.module.statements,
                                         code, error, NULL, &analysis, NULL,
                                         NULL);
        if (status == PY68_STATUS_OK) {
            status = py68_emit_op(allocator, code, OP_HALT);
        }
    }
    py68_symbol_analysis_destroy(allocator, &analysis);
    if (status != PY68_STATUS_OK) {
        py68_code_destroy(allocator, code);
        return status;
    }
    return PY68_STATUS_OK;
}
