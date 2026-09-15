/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_ast_arena.h"
#include "py68k_builtin.h"
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
    passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"total", 5,
                                   &total_val) == PY68_STATUS_OK;
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
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"total", 5,
                                       &break_total) == PY68_STATUS_OK;
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
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"total", 5,
                                       &continue_total) == PY68_STATUS_OK;
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
        const char *for_break_text =
            "total = 0\n"
            "for i in range(10):\n"
            "    if i == 3:\n"
            "        break\n"
            "    total = total + i\n";
        Py68Value for_break_total;
        passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
        py68_token_array_initialize(&tokens);
        passed &= py68_source_initialize(&runtime.allocator, &source,
            "for_break_test.py", (const Py68U8 *)for_break_text,
            (Py68U32)strlen(for_break_text)) == PY68_STATUS_OK;
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
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"total", 5,
                                       &for_break_total) == PY68_STATUS_OK;
        passed &= for_break_total.type == PY68_VALUE_INT &&
                  for_break_total.as.integer == 3;
        py68_value_release(&runtime, for_break_total);
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
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"marker", 6,
                                       &marker_val) == PY68_STATUS_OK;
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
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"marker", 6,
                                       &marker_val) == PY68_STATUS_OK;
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

    {
        const char *fib_text =
            "def fibonacci(n):\n"
            "    if n < 2:\n"
            "        return n\n"
            "    return fibonacci(n - 1) + fibonacci(n - 2)\n"
            "result = fibonacci(10)\n";
        Py68Value result_val;
        passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
        py68_token_array_initialize(&tokens);
        passed &= py68_source_initialize(&runtime.allocator, &source,
            "fib_test.py", (const Py68U8 *)fib_text,
            (Py68U32)strlen(fib_text)) == PY68_STATUS_OK;
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
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"result", 6,
                                       &result_val) == PY68_STATUS_OK;
        passed &= result_val.type == PY68_VALUE_INT &&
                  result_val.as.integer == 55;
        py68_value_release(&runtime, result_val);
        py68_code_destroy(&runtime.allocator, &code);
        py68_ast_arena_destroy(&arena);
        py68_token_array_destroy(&runtime.allocator, &tokens);
        py68_source_destroy(&runtime.allocator, &source);
        py68_runtime_shutdown(&runtime);
        passed &= runtime.allocator.stats.current_bytes == 0;
    }

    {
        const char *locals_text =
            "def accumulate(n):\n"
            "    total = 0\n"
            "    for i in range(n):\n"
            "        total += i\n"
            "    return total\n"
            "n = 100\n"
            "result = accumulate(5)\n";
        Py68Value result_val, shadow_val;
        passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
        py68_token_array_initialize(&tokens);
        passed &= py68_source_initialize(&runtime.allocator, &source,
            "locals_test.py", (const Py68U8 *)locals_text,
            (Py68U32)strlen(locals_text)) == PY68_STATUS_OK;
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
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"result", 6,
                                       &result_val) == PY68_STATUS_OK;
        passed &= result_val.type == PY68_VALUE_INT &&
                  result_val.as.integer == 10;
        py68_value_release(&runtime, result_val);
        /* the function's local "n" must not have clobbered the module
           global "n" (parameter/local slots are per-frame, not shared) */
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"n", 1,
                                       &shadow_val) == PY68_STATUS_OK;
        passed &= shadow_val.type == PY68_VALUE_INT &&
                  shadow_val.as.integer == 100;
        py68_value_release(&runtime, shadow_val);
        py68_code_destroy(&runtime.allocator, &code);
        py68_ast_arena_destroy(&arena);
        py68_token_array_destroy(&runtime.allocator, &tokens);
        py68_source_destroy(&runtime.allocator, &source);
        py68_runtime_shutdown(&runtime);
        passed &= runtime.allocator.stats.current_bytes == 0;
    }

    {
        const char *unbound_text =
            "def f():\n"
            "    print(x)\n"
            "    x = 5\n"
            "f()\n";
        passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
        passed &= py68_builtins_install(&runtime) == PY68_STATUS_OK;
        py68_token_array_initialize(&tokens);
        passed &= py68_source_initialize(&runtime.allocator, &source,
            "unbound_test.py", (const Py68U8 *)unbound_text,
            (Py68U32)strlen(unbound_text)) == PY68_STATUS_OK;
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
        passed &= py68_vm_execute(&runtime, &code) ==
                  PY68_STATUS_RUNTIME_ERROR;
        passed &= runtime.error.kind == PY68_ERROR_NAME;
        passed &= runtime.value_stack_count == 0 && runtime.frame_count == 0;
        py68_code_destroy(&runtime.allocator, &code);
        py68_ast_arena_destroy(&arena);
        py68_token_array_destroy(&runtime.allocator, &tokens);
        py68_source_destroy(&runtime.allocator, &source);
        py68_runtime_shutdown(&runtime);
        passed &= runtime.allocator.stats.current_bytes == 0;
    }

    {
        const char *nested_def_text =
            "def outer():\n"
            "    def inner():\n"
            "        return 1\n"
            "    return inner()\n";
        py68_allocator_initialize(&allocator);
        py68_token_array_initialize(&tokens);
        passed &= py68_source_initialize(&allocator, &source,
            "nested_def_test.py", (const Py68U8 *)nested_def_text,
            (Py68U32)strlen(nested_def_text)) == PY68_STATUS_OK;
        passed &= py68_tokenize(&allocator, &source, &tokens,
                                &error) == PY68_STATUS_OK;
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
                                      &error) == PY68_STATUS_SOURCE_ERROR;
        passed &= error.active != 0;
        py68_ast_arena_destroy(&arena);
        py68_token_array_destroy(&allocator, &tokens);
        py68_source_destroy(&allocator, &source);
        passed &= allocator.stats.current_bytes == 0;
    }

    {
        const char *dup_param_text =
            "def f(a, a):\n"
            "    return a\n";
        py68_allocator_initialize(&allocator);
        py68_token_array_initialize(&tokens);
        passed &= py68_source_initialize(&allocator, &source,
            "dup_param_test.py", (const Py68U8 *)dup_param_text,
            (Py68U32)strlen(dup_param_text)) == PY68_STATUS_OK;
        passed &= py68_tokenize(&allocator, &source, &tokens,
                                &error) == PY68_STATUS_OK;
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
                                      &error) == PY68_STATUS_SOURCE_ERROR;
        passed &= error.active != 0;
        py68_ast_arena_destroy(&arena);
        py68_token_array_destroy(&allocator, &tokens);
        py68_source_destroy(&allocator, &source);
        passed &= allocator.stats.current_bytes == 0;
    }

    {
        const char *comp_text =
            "xs = [x * x for x in range(5) if x != 2]\n"
            "total = 0\n"
            "for item in xs:\n"
            "    total = total + item\n";
        Py68Value total_val;
        Py68Value last_x;
        passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
        py68_token_array_initialize(&tokens);
        passed &= py68_source_initialize(&runtime.allocator, &source,
            "comp_test.py", (const Py68U8 *)comp_text,
            (Py68U32)strlen(comp_text)) == PY68_STATUS_OK;
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
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"total", 5,
                                       &total_val) == PY68_STATUS_OK;
        passed &= total_val.type == PY68_VALUE_INT &&
                  total_val.as.integer == 26;
        py68_value_release(&runtime, total_val);
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"x", 1,
                                       &last_x) == PY68_STATUS_OK;
        passed &= last_x.type == PY68_VALUE_INT && last_x.as.integer == 4;
        py68_value_release(&runtime, last_x);
        py68_code_destroy(&runtime.allocator, &code);
        py68_ast_arena_destroy(&arena);
        py68_token_array_destroy(&runtime.allocator, &tokens);
        py68_source_destroy(&runtime.allocator, &source);
        py68_runtime_shutdown(&runtime);
        passed &= runtime.allocator.stats.current_bytes == 0;
    }

    {
        const char *is_text =
            "x = None\n"
            "same_none = x is None\n"
            "not_none = x is not None\n"
            "a = [1, 2]\n"
            "b = [1, 2]\n"
            "alias = a\n"
            "same_list = a is a\n"
            "alias_ok = a is alias\n"
            "distinct = a is b\n"
            "equal = a == b\n"
            "compound = 0 is not 1\n"
            "grouped = 0 is (not 1)\n";
        Py68Value flag;
        passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
        py68_token_array_initialize(&tokens);
        passed &= py68_source_initialize(&runtime.allocator, &source,
            "is_test.py", (const Py68U8 *)is_text,
            (Py68U32)strlen(is_text)) == PY68_STATUS_OK;
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
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"same_none", 9,
                                       &flag) == PY68_STATUS_OK;
        passed &= flag.type == PY68_VALUE_BOOL && flag.as.integer == 1;
        py68_value_release(&runtime, flag);
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"not_none", 8,
                                       &flag) == PY68_STATUS_OK;
        passed &= flag.type == PY68_VALUE_BOOL && flag.as.integer == 0;
        py68_value_release(&runtime, flag);
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"same_list", 9,
                                       &flag) == PY68_STATUS_OK;
        passed &= flag.type == PY68_VALUE_BOOL && flag.as.integer == 1;
        py68_value_release(&runtime, flag);
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"alias_ok", 8,
                                       &flag) == PY68_STATUS_OK;
        passed &= flag.type == PY68_VALUE_BOOL && flag.as.integer == 1;
        py68_value_release(&runtime, flag);
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"distinct", 8,
                                       &flag) == PY68_STATUS_OK;
        passed &= flag.type == PY68_VALUE_BOOL && flag.as.integer == 0;
        py68_value_release(&runtime, flag);
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"equal", 5,
                                       &flag) == PY68_STATUS_OK;
        passed &= flag.type == PY68_VALUE_BOOL && flag.as.integer == 1;
        py68_value_release(&runtime, flag);
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"compound", 8,
                                       &flag) == PY68_STATUS_OK;
        passed &= flag.type == PY68_VALUE_BOOL && flag.as.integer == 1;
        py68_value_release(&runtime, flag);
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"grouped", 7,
                                       &flag) == PY68_STATUS_OK;
        passed &= flag.type == PY68_VALUE_BOOL && flag.as.integer == 0;
        py68_value_release(&runtime, flag);
        py68_code_destroy(&runtime.allocator, &code);
        py68_ast_arena_destroy(&arena);
        py68_token_array_destroy(&runtime.allocator, &tokens);
        py68_source_destroy(&runtime.allocator, &source);
        py68_runtime_shutdown(&runtime);
        passed &= runtime.allocator.stats.current_bytes == 0;
    }

    {
        const char *unpack_text =
            "a, b = (1, 2)\n"
            "c, d = [3, 4]\n"
            "e, f = 5, 6\n"
            "left = 1\n"
            "right = 2\n"
            "left, right = right, left\n"
            "total = 0\n"
            "for x, y in [(1, 10), (2, 20)]:\n"
            "    total = total + x + y\n";
        Py68Value flag;
        Py68U32 bytecode_index;
        int saw_unpack = 0;
        passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
        py68_token_array_initialize(&tokens);
        passed &= py68_source_initialize(&runtime.allocator, &source,
            "unpack_test.py", (const Py68U8 *)unpack_text,
            (Py68U32)strlen(unpack_text)) == PY68_STATUS_OK;
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
        for (bytecode_index = 0; bytecode_index < code.bytecode_length;
             ++bytecode_index) {
            if (code.bytecode[bytecode_index] == OP_UNPACK) saw_unpack = 1;
        }
        passed &= saw_unpack;
        passed &= py68_verify_code(&code, &error) == PY68_STATUS_OK;
        passed &= py68_vm_execute(&runtime, &code) == PY68_STATUS_OK;
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"a", 1,
                                       &flag) == PY68_STATUS_OK;
        passed &= flag.type == PY68_VALUE_INT && flag.as.integer == 1;
        py68_value_release(&runtime, flag);
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"b", 1,
                                       &flag) == PY68_STATUS_OK;
        passed &= flag.type == PY68_VALUE_INT && flag.as.integer == 2;
        py68_value_release(&runtime, flag);
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"e", 1,
                                       &flag) == PY68_STATUS_OK;
        passed &= flag.type == PY68_VALUE_INT && flag.as.integer == 5;
        py68_value_release(&runtime, flag);
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"total", 5,
                                       &flag) == PY68_STATUS_OK;
        passed &= flag.type == PY68_VALUE_INT && flag.as.integer == 33;
        py68_value_release(&runtime, flag);
        py68_code_destroy(&runtime.allocator, &code);
        py68_ast_arena_destroy(&arena);
        py68_token_array_destroy(&runtime.allocator, &tokens);
        py68_source_destroy(&runtime.allocator, &source);
        py68_runtime_shutdown(&runtime);
        passed &= runtime.allocator.stats.current_bytes == 0;
    }

    {
        const char *ifexp_text =
            "x = 1 if 0 else 2\n"
            "y = 3 if 1 else 4\n"
            "z = 5 if 0 else 6 if 0 else 7\n"
            "seen = [0]\n"
            "def mark(n):\n"
            "    seen[0] = seen[0] + 1\n"
            "    return n\n"
            "chosen = mark(10) if 1 else mark(20)\n";
        Py68Value flag;
        Py68U32 bytecode_index;
        int saw_jump_if_false = 0;
        passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
        py68_token_array_initialize(&tokens);
        passed &= py68_source_initialize(&runtime.allocator, &source,
            "ifexp_test.py", (const Py68U8 *)ifexp_text,
            (Py68U32)strlen(ifexp_text)) == PY68_STATUS_OK;
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
        for (bytecode_index = 0; bytecode_index < code.bytecode_length;
             ++bytecode_index) {
            if (code.bytecode[bytecode_index] == OP_JUMP_IF_FALSE)
                saw_jump_if_false = 1;
        }
        passed &= saw_jump_if_false;
        passed &= py68_verify_code(&code, &error) == PY68_STATUS_OK;
        passed &= py68_vm_execute(&runtime, &code) == PY68_STATUS_OK;
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"x", 1,
                                       &flag) == PY68_STATUS_OK;
        passed &= flag.type == PY68_VALUE_INT && flag.as.integer == 2;
        py68_value_release(&runtime, flag);
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"y", 1,
                                       &flag) == PY68_STATUS_OK;
        passed &= flag.type == PY68_VALUE_INT && flag.as.integer == 3;
        py68_value_release(&runtime, flag);
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"z", 1,
                                       &flag) == PY68_STATUS_OK;
        passed &= flag.type == PY68_VALUE_INT && flag.as.integer == 7;
        py68_value_release(&runtime, flag);
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"chosen", 6,
                                       &flag) == PY68_STATUS_OK;
        passed &= flag.type == PY68_VALUE_INT && flag.as.integer == 10;
        py68_value_release(&runtime, flag);
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
