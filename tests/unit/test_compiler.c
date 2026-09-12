#include "py68k_ast_arena.h"
#include "py68k_code.h"
#include "py68k_compiler.h"
#include "py68k_global.h"
#include "py68k_memory.h"
#include "py68k_parser.h"
#include "py68k_runtime.h"
#include "py68k_source.h"
#include "py68k_token.h"
#include "py68k_tokenizer.h"
#include "py68k_verify.h"
#include "py68k_vm.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    const char *text = "answer = 1 + 2 * 3\n";
    const char *for_text =
        "total = 0\n"
        "for i in range(4):\n"
        "    total = total + i\n";
    Py68Allocator allocator;
    Py68Runtime runtime;
    Py68Source source;
    Py68TokenArray tokens;
    Py68AstArena arena;
    Py68StatementParser parser;
    Py68AstNode *module;
    Py68Code code;
    Py68Error error;
    Py68Value total_val;
    Py68U16 name_idx;
    int passed = 1;

    py68_allocator_initialize(&allocator);
    py68_token_array_initialize(&tokens);
    passed &= py68_source_initialize(&allocator, &source, "compile.py",
        (const Py68U8 *)text, (Py68U32)strlen(text)) == PY68_STATUS_OK;
    passed &= py68_tokenize(&allocator, &source, &tokens, &error) == PY68_STATUS_OK;
    py68_ast_arena_initialize(&arena, &allocator);
    parser.expression.allocator = &allocator;
    parser.expression.source = &source;
    parser.expression.tokens = &tokens;
    parser.expression.position = 0;
    parser.expression.arena = &arena;
    parser.expression.error = &error;
    parser.inside_function = 0;
    parser.loop_depth = 0;
    passed &= py68_parse_module(&parser, &module) == PY68_STATUS_OK;
    passed &= py68_compile_module(&allocator, &source, module, &code,
                                  &error) == PY68_STATUS_OK;
    passed &= code.constant_count == 3;
    passed &= code.name_count == 1;
    passed &= code.bytecode_length >= 10;
    passed &= code.bytecode[code.bytecode_length - 1] == OP_HALT;
    py68_code_destroy(&allocator, &code);
    py68_ast_arena_destroy(&arena);
    py68_token_array_destroy(&allocator, &tokens);
    py68_source_destroy(&allocator, &source);
    passed &= allocator.stats.current_bytes == 0;

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    py68_token_array_initialize(&tokens);
    passed &= py68_source_initialize(&runtime.allocator, &source, "for_test.py",
        (const Py68U8 *)for_text, (Py68U32)strlen(for_text)) == PY68_STATUS_OK;
    passed &= py68_tokenize(&runtime.allocator, &source, &tokens, &error) == PY68_STATUS_OK;
    py68_ast_arena_initialize(&arena, &runtime.allocator);
    parser.expression.allocator = &runtime.allocator;
    parser.expression.source = &source;
    parser.expression.tokens = &tokens;
    parser.expression.position = 0;
    parser.expression.arena = &arena;
    parser.expression.error = &error;
    parser.inside_function = 0;
    parser.loop_depth = 0;
    passed &= py68_parse_module(&parser, &module) == PY68_STATUS_OK;
    passed &= py68_compile_module(&runtime.allocator, &source, module, &code,
                                  &error) == PY68_STATUS_OK;
    passed &= py68_verify_code(&code, &error) == PY68_STATUS_OK;
    passed &= py68_vm_execute(&runtime, &code) == PY68_STATUS_OK;
    passed &= py68_code_add_name(&runtime.allocator, &code, 0, 5, &name_idx) == PY68_STATUS_OK;
    passed &= py68_global_get_copy(&runtime, name_idx, &total_val) == PY68_STATUS_OK;
    passed &= total_val.type == PY68_VALUE_INT && total_val.as.integer == 6;
    py68_value_release(&runtime, total_val);
    py68_code_destroy(&runtime.allocator, &code);
    py68_ast_arena_destroy(&arena);
    py68_token_array_destroy(&runtime.allocator, &tokens);
    py68_source_destroy(&runtime.allocator, &source);
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;

    {
        const char *break_text =
            "total = 0\n"
            "i = 0\n"
            "while i < 10:\n"
            "    if i == 3:\n"
            "        break\n"
            "    total = total + i\n"
            "    i = i + 1\n";
        Py68Value break_total;
        passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
        py68_token_array_initialize(&tokens);
        passed &= py68_source_initialize(&runtime.allocator, &source,
            "break_test.py", (const Py68U8 *)break_text,
            (Py68U32)strlen(break_text)) == PY68_STATUS_OK;
        passed &= py68_tokenize(&runtime.allocator, &source, &tokens,
                                &error) == PY68_STATUS_OK;
        py68_ast_arena_initialize(&arena, &runtime.allocator);
        parser.expression.allocator = &runtime.allocator;
        parser.expression.source = &source;
        parser.expression.tokens = &tokens;
        parser.expression.position = 0;
        parser.expression.arena = &arena;
        parser.expression.error = &error;
        parser.inside_function = 0;
        parser.loop_depth = 0;
        passed &= py68_parse_module(&parser, &module) == PY68_STATUS_OK;
        passed &= py68_compile_module(&runtime.allocator, &source, module,
                                      &code, &error) == PY68_STATUS_OK;
        passed &= py68_verify_code(&code, &error) == PY68_STATUS_OK;
        passed &= py68_vm_execute(&runtime, &code) == PY68_STATUS_OK;
        passed &= py68_code_add_name(&runtime.allocator, &code, 0, 5,
                                     &name_idx) == PY68_STATUS_OK;
        passed &= py68_global_get_copy(&runtime, name_idx, &break_total) ==
                  PY68_STATUS_OK;
        passed &= break_total.type == PY68_VALUE_INT &&
                  break_total.as.integer == 3;
        py68_value_release(&runtime, break_total);
        py68_code_destroy(&runtime.allocator, &code);
        py68_ast_arena_destroy(&arena);
        py68_token_array_destroy(&runtime.allocator, &tokens);
        py68_source_destroy(&runtime.allocator, &source);
        py68_runtime_shutdown(&runtime);
        passed &= runtime.allocator.stats.current_bytes == 0;
    }

    {
        const char *continue_text =
            "total = 0\n"
            "for i in range(5):\n"
            "    if i == 2:\n"
            "        continue\n"
            "    total = total + i\n";
        Py68Value continue_total;
        passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
        py68_token_array_initialize(&tokens);
        passed &= py68_source_initialize(&runtime.allocator, &source,
            "continue_test.py", (const Py68U8 *)continue_text,
            (Py68U32)strlen(continue_text)) == PY68_STATUS_OK;
        passed &= py68_tokenize(&runtime.allocator, &source, &tokens,
                                &error) == PY68_STATUS_OK;
        py68_ast_arena_initialize(&arena, &runtime.allocator);
        parser.expression.allocator = &runtime.allocator;
        parser.expression.source = &source;
        parser.expression.tokens = &tokens;
        parser.expression.position = 0;
        parser.expression.arena = &arena;
        parser.expression.error = &error;
        parser.inside_function = 0;
        parser.loop_depth = 0;
        passed &= py68_parse_module(&parser, &module) == PY68_STATUS_OK;
        passed &= py68_compile_module(&runtime.allocator, &source, module,
                                      &code, &error) == PY68_STATUS_OK;
        passed &= py68_verify_code(&code, &error) == PY68_STATUS_OK;
        passed &= py68_vm_execute(&runtime, &code) == PY68_STATUS_OK;
        passed &= py68_code_add_name(&runtime.allocator, &code, 0, 5,
                                     &name_idx) == PY68_STATUS_OK;
        passed &= py68_global_get_copy(&runtime, name_idx, &continue_total) ==
                  PY68_STATUS_OK;
        passed &= continue_total.type == PY68_VALUE_INT &&
                  continue_total.as.integer == 8;
        py68_value_release(&runtime, continue_total);
        py68_code_destroy(&runtime.allocator, &code);
        py68_ast_arena_destroy(&arena);
        py68_token_array_destroy(&runtime.allocator, &tokens);
        py68_source_destroy(&runtime.allocator, &source);
        py68_runtime_shutdown(&runtime);
        passed &= runtime.allocator.stats.current_bytes == 0;
    }

    {
        const char *while_else_text =
            "marker = 0\n"
            "i = 0\n"
            "while i < 3:\n"
            "    i = i + 1\n"
            "else:\n"
            "    marker = 1\n";
        Py68Value marker_val;
        passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
        py68_token_array_initialize(&tokens);
        passed &= py68_source_initialize(&runtime.allocator, &source,
            "while_else_test.py", (const Py68U8 *)while_else_text,
            (Py68U32)strlen(while_else_text)) == PY68_STATUS_OK;
        passed &= py68_tokenize(&runtime.allocator, &source, &tokens,
                                &error) == PY68_STATUS_OK;
        py68_ast_arena_initialize(&arena, &runtime.allocator);
        parser.expression.allocator = &runtime.allocator;
        parser.expression.source = &source;
        parser.expression.tokens = &tokens;
        parser.expression.position = 0;
        parser.expression.arena = &arena;
        parser.expression.error = &error;
        parser.inside_function = 0;
        parser.loop_depth = 0;
        passed &= py68_parse_module(&parser, &module) == PY68_STATUS_OK;
        passed &= py68_compile_module(&runtime.allocator, &source, module,
                                      &code, &error) == PY68_STATUS_OK;
        passed &= py68_verify_code(&code, &error) == PY68_STATUS_OK;
        passed &= py68_vm_execute(&runtime, &code) == PY68_STATUS_OK;
        passed &= py68_code_add_name(&runtime.allocator, &code, 0, 6,
                                     &name_idx) == PY68_STATUS_OK;
        passed &= py68_global_get_copy(&runtime, name_idx, &marker_val) ==
                  PY68_STATUS_OK;
        passed &= marker_val.type == PY68_VALUE_INT &&
                  marker_val.as.integer == 1;
        py68_value_release(&runtime, marker_val);
        py68_code_destroy(&runtime.allocator, &code);
        py68_ast_arena_destroy(&arena);
        py68_token_array_destroy(&runtime.allocator, &tokens);
        py68_source_destroy(&runtime.allocator, &source);
        py68_runtime_shutdown(&runtime);
        passed &= runtime.allocator.stats.current_bytes == 0;
    }

    {
        const char *while_else_break_text =
            "marker = 0\n"
            "i = 0\n"
            "while i < 3:\n"
            "    if i == 1:\n"
            "        break\n"
            "    i = i + 1\n"
            "else:\n"
            "    marker = 1\n";
        Py68Value marker_val;
        passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
        py68_token_array_initialize(&tokens);
        passed &= py68_source_initialize(&runtime.allocator, &source,
            "while_else_break_test.py", (const Py68U8 *)while_else_break_text,
            (Py68U32)strlen(while_else_break_text)) == PY68_STATUS_OK;
        passed &= py68_tokenize(&runtime.allocator, &source, &tokens,
                                &error) == PY68_STATUS_OK;
        py68_ast_arena_initialize(&arena, &runtime.allocator);
        parser.expression.allocator = &runtime.allocator;
        parser.expression.source = &source;
        parser.expression.tokens = &tokens;
        parser.expression.position = 0;
        parser.expression.arena = &arena;
        parser.expression.error = &error;
        parser.inside_function = 0;
        parser.loop_depth = 0;
        passed &= py68_parse_module(&parser, &module) == PY68_STATUS_OK;
        passed &= py68_compile_module(&runtime.allocator, &source, module,
                                      &code, &error) == PY68_STATUS_OK;
        passed &= py68_verify_code(&code, &error) == PY68_STATUS_OK;
        passed &= py68_vm_execute(&runtime, &code) == PY68_STATUS_OK;
        passed &= py68_code_add_name(&runtime.allocator, &code, 0, 6,
                                     &name_idx) == PY68_STATUS_OK;
        passed &= py68_global_get_copy(&runtime, name_idx, &marker_val) ==
                  PY68_STATUS_OK;
        passed &= marker_val.type == PY68_VALUE_INT &&
                  marker_val.as.integer == 0;
        py68_value_release(&runtime, marker_val);
        py68_code_destroy(&runtime.allocator, &code);
        py68_ast_arena_destroy(&arena);
        py68_token_array_destroy(&runtime.allocator, &tokens);
        py68_source_destroy(&runtime.allocator, &source);
        py68_runtime_shutdown(&runtime);
        passed &= runtime.allocator.stats.current_bytes == 0;
    }

    if (passed) { puts("PASS: compiler tests"); return 0; }
    return 1;
}
