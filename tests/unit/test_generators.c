/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_ast_arena.h"
#include "py68k_builtin.h"
#include "py68k_compiler.h"
#include "py68k_error.h"
#include "py68k_generator.h"
#include "py68k_global.h"
#include "py68k_list.h"
#include "py68k_parser.h"
#include "py68k_runtime.h"
#include "py68k_source.h"
#include "py68k_string.h"
#include "py68k_tokenizer.h"
#include "py68k_verify.h"
#include "py68k_vm.h"

#include <stdio.h>
#include <string.h>

typedef struct Program {
    Py68Source source;
    Py68TokenArray tokens;
    Py68AstArena arena;
    Py68Code code;
} Program;

static int program_build(Py68Runtime *runtime, Program *program,
                         const char *text, Py68Status *parse_status,
                         Py68Error *error)
{
    Py68StatementParser parser;
    Py68AstNode *module;
    int passed = 1;

    py68_token_array_initialize(&program->tokens);
    py68_code_initialize(&program->code);
    passed &= py68_source_initialize(&runtime->allocator, &program->source,
                                     "gen.py", (const Py68U8 *)text,
                                     (Py68U32)strlen(text)) == PY68_STATUS_OK;
    passed &= py68_tokenize(&runtime->allocator, &program->source,
                            &program->tokens, error) == PY68_STATUS_OK;
    py68_ast_arena_initialize(&program->arena, &runtime->allocator);
    parser.expression.allocator = &runtime->allocator;
    parser.expression.source = &program->source;
    parser.expression.tokens = &program->tokens;
    parser.expression.position = 0;
    parser.expression.arena = &program->arena;
    parser.expression.error = error;
    parser.inside_function = 0;
    parser.loop_depth = 0;
    *parse_status = py68_parse_module(&parser, &module);
    if (*parse_status != PY68_STATUS_OK) return passed;
    *parse_status = py68_compile_module(&runtime->allocator, &program->source,
                                       module, &program->code, error);
    return passed;
}

static void program_destroy(Py68Runtime *runtime, Program *program)
{
    py68_code_destroy(&runtime->allocator, &program->code);
    py68_ast_arena_destroy(&program->arena);
    py68_token_array_destroy(&runtime->allocator, &program->tokens);
    py68_source_destroy(&runtime->allocator, &program->source);
}

static int global_int_is(Py68Runtime *runtime, const char *name, Py68I32 want)
{
    Py68Value value;
    int ok;
    if (py68_global_get_copy(runtime, (const Py68U8 *)name,
                            (Py68U16)strlen(name), &value) != PY68_STATUS_OK)
        return 0;
    ok = value.type == PY68_VALUE_INT && value.as.integer == want;
    py68_value_release(runtime, value);
    return ok;
}

/* `yield` is a real keyword, not an unsupported one. */
static int test_tokenizer(void)
{
    Py68Runtime runtime;
    Py68Source source;
    Py68TokenArray tokens;
    Py68Error error;
    const char *text = "yield 1\n";
    int passed = 1;
    int seen = 0;
    Py68U32 index;

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    py68_token_array_initialize(&tokens);
    passed &= py68_source_initialize(&runtime.allocator, &source, "kw.py",
                                     (const Py68U8 *)text,
                                     (Py68U32)strlen(text)) == PY68_STATUS_OK;
    passed &= py68_tokenize(&runtime.allocator, &source, &tokens, &error) ==
              PY68_STATUS_OK;
    for (index = 0; index < tokens.count; ++index)
        if (tokens.items[index].kind == PY68_TOKEN_YIELD) seen = 1;
    passed &= seen;
    py68_token_array_destroy(&runtime.allocator, &tokens);
    py68_source_destroy(&runtime.allocator, &source);
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    return passed;
}

/* Only a body that yields becomes a generator factory. */
static int test_is_generator_flag(void)
{
    Py68Runtime runtime;
    Program program;
    Py68Error error;
    Py68Status status;
    Py68U16 index;
    int generator_count = 0;
    int plain_count = 0;
    int passed = 1;
    const char *text =
        "def plain(a):\n"
        "    return a\n"
        "def gen(a):\n"
        "    while a > 0:\n"
        "        yield a\n"
        "        a = a - 1\n"
        "def guarded():\n"
        "    try:\n"
        "        yield 1\n"
        "    finally:\n"
        "        pass\n";

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    passed &= program_build(&runtime, &program, text, &status, &error);
    passed &= status == PY68_STATUS_OK;
    if (status == PY68_STATUS_OK) {
        passed &= program.code.is_generator == 0;
        passed &= program.code.nested_count == 3;
        for (index = 0; index < program.code.nested_count; ++index) {
            if (program.code.nested[index].is_generator != 0)
                ++generator_count;
            else
                ++plain_count;
        }
        /* gen and guarded yield; plain does not. */
        passed &= generator_count == 2 && plain_count == 1;
    }
    program_destroy(&runtime, &program);
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    return passed;
}

/* `yield` outside a function is rejected before compilation. */
static int test_yield_outside_function(void)
{
    Py68Runtime runtime;
    Program program;
    Py68Error error;
    Py68Status status;
    int passed = 1;

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    passed &= program_build(&runtime, &program, "yield 1\n", &status, &error);
    passed &= status == PY68_STATUS_SOURCE_ERROR;
    program_destroy(&runtime, &program);
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    return passed;
}

/* The verifier rejects OP_YIELD_VALUE in code that is not a generator. */
static int test_verify_rejects_stray_yield(void)
{
    Py68Runtime runtime;
    Py68Code code;
    Py68Error error;
    int passed = 1;

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    py68_code_initialize(&code);
    passed &= py68_code_emit_u8(&runtime.allocator, &code, OP_LOAD_NONE) ==
              PY68_STATUS_OK;
    passed &= py68_code_emit_u8(&runtime.allocator, &code, OP_YIELD_VALUE) ==
              PY68_STATUS_OK;
    passed &= py68_code_emit_u8(&runtime.allocator, &code, OP_HALT) ==
              PY68_STATUS_OK;
    passed &= py68_verify_code(&code, &error) == PY68_STATUS_SOURCE_ERROR;
    code.is_generator = 1;
    py68_error_clear(&error);
    passed &= py68_verify_code(&code, &error) == PY68_STATUS_OK;
    py68_code_destroy(&runtime.allocator, &code);
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    return passed;
}

/* Lifecycle without the VM: arguments are retained, finish is idempotent. */
static int test_generator_lifecycle(void)
{
    Py68Runtime runtime;
    Py68Code code;
    Py68Generator *generator = NULL;
    Py68String *string;
    Py68Value arguments[2];
    int passed = 1;

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    py68_code_initialize(&code);
    code.local_count = 3;
    code.argument_count = 2;
    code.maximum_stack = 4;
    passed &= py68_string_new_copy(&runtime, "arg", 3, &string) ==
              PY68_STATUS_OK;
    arguments[0] = py68_value_from_object(&string->base);
    arguments[1] = py68_value_int(7);
    passed &= py68_generator_new(&runtime, &code, NULL, 2, arguments,
                                 &generator) == PY68_STATUS_OK;
    passed &= generator != NULL;
    if (generator != NULL) {
        passed &= generator->base.type == PY68_OBJECT_GENERATOR;
        passed &= generator->base.reference_count == 1;
        passed &= generator->state == PY68_GENERATOR_CREATED;
        passed &= generator->local_count == 3;
        passed &= generator->stack_capacity == 4;
        passed &= generator->stack_count == 0;
        /* Argument slots are retained; the remaining slot stays unbound. */
        passed &= string->base.reference_count == 2;
        passed &= generator->locals[1].type == PY68_VALUE_INT &&
                  generator->locals[1].as.integer == 7;
        passed &= generator->locals[2].type == PY68_VALUE_UNBOUND;
        /* Storing operands moves ownership into the generator. */
        arguments[0] = py68_value_from_object(&string->base);
        py68_value_retain(arguments[0]);
        passed &= string->base.reference_count == 3;
        passed &= py68_generator_store_stack(generator, arguments, 1) ==
                  PY68_STATUS_OK;
        passed &= generator->stack_count == 1;
        passed &= py68_generator_store_stack(generator, arguments, 5) ==
                  PY68_STATUS_INTERNAL_ERROR;
        py68_generator_finish(&runtime, generator);
        passed &= generator->state == PY68_GENERATOR_DONE;
        passed &= generator->locals == NULL && generator->stack == NULL;
        passed &= string->base.reference_count == 1;
        /* Idempotent: a second finish must not double-free. */
        py68_generator_finish(&runtime, generator);
        passed &= generator->state == PY68_GENERATOR_DONE;
        py68_object_release(&runtime, &generator->base);
    }
    py68_object_release(&runtime, &string->base);
    passed &= runtime.live_objects == NULL;
    py68_code_destroy(&runtime.allocator, &code);
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    return passed;
}

/* Every allocation inside py68_generator_new must roll back cleanly. */
static int test_generator_allocation_failure(void)
{
    Py68Runtime runtime;
    Py68Code code;
    Py68U32 attempt;
    int passed = 1;
    int saw_failure = 0;

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    py68_code_initialize(&code);
    code.local_count = 2;
    code.argument_count = 0;
    code.maximum_stack = 3;
    for (attempt = 1; attempt <= 3; ++attempt) {
        Py68Generator *generator = NULL;
        Py68U32 before = runtime.allocator.stats.current_bytes;
        Py68Status status;
        runtime.allocator.fail_after_allocation = attempt;
        status = py68_generator_new(&runtime, &code, NULL, 0, NULL, &generator);
        runtime.allocator.fail_after_allocation = 0;
        if (status == PY68_STATUS_MEMORY_ERROR) {
            saw_failure = 1;
            passed &= generator == NULL;
            passed &= runtime.allocator.stats.current_bytes == before;
            passed &= runtime.live_objects == NULL;
        } else {
            passed &= status == PY68_STATUS_OK && generator != NULL;
            if (generator != NULL)
                py68_object_release(&runtime, &generator->base);
            passed &= runtime.allocator.stats.current_bytes == before;
        }
    }
    passed &= saw_failure;
    py68_code_destroy(&runtime.allocator, &code);
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    return passed;
}

/* Calling a generator factory runs no bytecode; the VM drives every resume. */
static int test_generator_execution(void)
{
    Py68Runtime runtime;
    Program program;
    Py68Error error;
    Py68Status status;
    int passed = 1;
    const char *text =
        "def counter(n):\n"
        "    i = 0\n"
        "    while i < n:\n"
        "        yield i\n"
        "        i = i + 1\n"
        "made = counter(3)\n"
        "first = next(made)\n"
        "total = 0\n"
        "steps = 0\n"
        "for v in counter(4):\n"
        "    total = total + v\n"
        "    steps = steps + 1\n"
        "fallback = next(counter(0), -5)\n"
        "same = 0\n"
        "held = counter(2)\n"
        "if iter(held) is held:\n"
        "    same = 1\n";

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    passed &= py68_builtins_install(&runtime) == PY68_STATUS_OK;
    passed &= program_build(&runtime, &program, text, &status, &error);
    passed &= status == PY68_STATUS_OK;
    if (status == PY68_STATUS_OK) {
        passed &= py68_vm_execute(&runtime, &program.code) == PY68_STATUS_OK;
        passed &= runtime.value_stack_count == 0;
        passed &= runtime.frame_count == 0;
        passed &= global_int_is(&runtime, "first", 0);
        passed &= global_int_is(&runtime, "total", 6);
        passed &= global_int_is(&runtime, "steps", 4);
        passed &= global_int_is(&runtime, "fallback", -5);
        passed &= global_int_is(&runtime, "same", 1);
    }
    program_destroy(&runtime, &program);
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    return passed;
}

/* next() past the end raises StopIteration; a suspended generator that is
   dropped releases its locals and saved operands. */
static int test_generator_exhaustion(void)
{
    Py68Runtime runtime;
    Program program;
    Py68Error error;
    Py68Status status;
    int passed = 1;
    const char *text =
        "def one():\n"
        "    yield 1\n"
        "    yield 2\n"
        "g = one()\n"
        "a = next(g)\n"
        "dropped = one()\n"
        "b = next(dropped)\n"
        "dropped = 0\n"
        "c = next(g)\n"
        "d = next(g)\n";

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    passed &= py68_builtins_install(&runtime) == PY68_STATUS_OK;
    passed &= program_build(&runtime, &program, text, &status, &error);
    passed &= status == PY68_STATUS_OK;
    if (status == PY68_STATUS_OK) {
        passed &= py68_vm_execute(&runtime, &program.code) ==
                  PY68_STATUS_RUNTIME_ERROR;
        passed &= runtime.error.kind == PY68_ERROR_STOP_ITERATION;
        passed &= runtime.value_stack_count == 0;
        passed &= runtime.frame_count == 0;
        passed &= global_int_is(&runtime, "a", 1);
        passed &= global_int_is(&runtime, "b", 1);
        passed &= global_int_is(&runtime, "c", 2);
        py68_error_clear(&runtime.error);
    }
    program_destroy(&runtime, &program);
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    return passed;
}

/* try/except/finally survive a suspend: the try stack travels with the
   generator and its depths are rebased on every resume. */
static int test_generator_try_state(void)
{
    Py68Runtime runtime;
    Program program;
    Py68Error error;
    Py68Status status;
    int passed = 1;
    const char *text =
        "log = 0\n"
        "def guarded():\n"
        "    try:\n"
        "        yield 1\n"
        "        raise ValueError(\"boom\")\n"
        "    except ValueError:\n"
        "        yield 2\n"
        "    finally:\n"
        "        yield 3\n"
        "for v in guarded():\n"
        "    log = log * 10 + v\n"
        "escaped = 0\n"
        "def raiser():\n"
        "    yield 1\n"
        "    raise ValueError(\"escapes\")\n"
        "try:\n"
        "    for v in raiser():\n"
        "        pass\n"
        "except ValueError:\n"
        "    escaped = 1\n";

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    passed &= py68_builtins_install(&runtime) == PY68_STATUS_OK;
    passed &= program_build(&runtime, &program, text, &status, &error);
    passed &= status == PY68_STATUS_OK;
    if (status == PY68_STATUS_OK) {
        passed &= py68_vm_execute(&runtime, &program.code) == PY68_STATUS_OK;
        passed &= runtime.value_stack_count == 0;
        passed &= runtime.frame_count == 0;
        passed &= global_int_is(&runtime, "log", 123);
        passed &= global_int_is(&runtime, "escaped", 1);
    }
    program_destroy(&runtime, &program);
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    return passed;
}

/* Abandoning a generator mid-iteration (break) and re-entering an exhausted
   generator must both leave the runtime consistent. */
static int test_generator_abandon(void)
{
    Py68Runtime runtime;
    Program program;
    Py68Error error;
    Py68Status status;
    int passed = 1;
    const char *text =
        "def three():\n"
        "    yield 1\n"
        "    yield 2\n"
        "    yield 3\n"
        "seen = 0\n"
        "for v in three():\n"
        "    seen = seen + 1\n"
        "    if v == 2:\n"
        "        break\n"
        "g = three()\n"
        "used = 0\n"
        "for v in g:\n"
        "    used = used + v\n"
        "again = 0\n"
        "for v in g:\n"
        "    again = again + 1\n"
        "def nested():\n"
        "    for v in three():\n"
        "        yield v * 10\n"
        "outer = 0\n"
        "for v in nested():\n"
        "    outer = outer + v\n";

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    passed &= py68_builtins_install(&runtime) == PY68_STATUS_OK;
    passed &= program_build(&runtime, &program, text, &status, &error);
    passed &= status == PY68_STATUS_OK;
    if (status == PY68_STATUS_OK) {
        passed &= py68_vm_execute(&runtime, &program.code) == PY68_STATUS_OK;
        passed &= runtime.value_stack_count == 0;
        passed &= runtime.frame_count == 0;
        passed &= global_int_is(&runtime, "seen", 2);
        passed &= global_int_is(&runtime, "used", 6);
        passed &= global_int_is(&runtime, "again", 0);
        passed &= global_int_is(&runtime, "outer", 60);
    }
    program_destroy(&runtime, &program);
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    return passed;
}

/* A generator expression compiles to a synthetic nested generator code object
   with one hidden iterator parameter; it never binds its loop variable in the
   enclosing scope. */
static int test_generator_exp_shape(void)
{
    Py68Runtime runtime;
    Program program;
    Py68Error error;
    Py68Status status;
    Py68Value leaked;
    int passed = 1;
    const char *text = "items = [1, 2]\n"
                       "g = (x for x in items)\n";

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    passed &= py68_builtins_install(&runtime) == PY68_STATUS_OK;
    passed &= program_build(&runtime, &program, text, &status, &error);
    passed &= status == PY68_STATUS_OK;
    if (status == PY68_STATUS_OK) {
        passed &= program.code.nested_count == 1;
        if (program.code.nested_count == 1) {
            Py68Code *body = &program.code.nested[0];
            passed &= body->is_generator == 1;
            /* Hidden iterator only: `items` is a global, so nothing is
               snapshotted at module level. */
            passed &= body->argument_count == 1;
            passed &= body->local_count == 2;
        }
        passed &= py68_vm_execute(&runtime, &program.code) == PY68_STATUS_OK;
        passed &= runtime.value_stack_count == 0;
        passed &= runtime.frame_count == 0;
        /* The loop variable belongs to the generator, not the module. */
        passed &= py68_global_get_copy(&runtime, (const Py68U8 *)"x", 1,
                                      &leaked) != PY68_STATUS_OK;
        py68_error_clear(&runtime.error);
    }
    program_destroy(&runtime, &program);
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    return passed;
}

/* Free variables of an enclosing function are snapshotted by value at creation
   time, so later rebinding is not observed (D-0046). */
static int test_generator_exp_snapshot(void)
{
    Py68Runtime runtime;
    Program program;
    Py68Error error;
    Py68Status status;
    int passed = 1;
    const char *text =
        "def snapshot():\n"
        "    bias = 100\n"
        "    g = (x + bias for x in [1, 2])\n"
        "    bias = 900\n"
        "    total = 0\n"
        "    for v in g:\n"
        "        total = total + v\n"
        "    return total\n"
        "captured = snapshot()\n"
        "def filtered(limit):\n"
        "    g = (x for x in [1, 2, 3, 4] if x < limit)\n"
        "    total = 0\n"
        "    for v in g:\n"
        "        total = total + v\n"
        "    return total\n"
        "kept = filtered(3)\n"
        "lazy = 0\n"
        "spy = (x for x in [1, 2, 3])\n"
        "first = next(spy)\n";

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    passed &= py68_builtins_install(&runtime) == PY68_STATUS_OK;
    passed &= program_build(&runtime, &program, text, &status, &error);
    passed &= status == PY68_STATUS_OK;
    if (status == PY68_STATUS_OK) {
        /* A genexp inside a function body is nested under that function's code
           object, which is where OP_MAKE_FUNCTION resolves it from. */
        passed &= program.code.nested_count == 3;
        if (program.code.nested_count == 3) {
            Py68Code *snapshot_fn = &program.code.nested[0];
            Py68Code *filtered_fn = &program.code.nested[1];
            passed &= snapshot_fn->is_generator == 0;
            passed &= snapshot_fn->nested_count == 1;
            if (snapshot_fn->nested_count == 1) {
                /* Hidden iterator plus the captured `bias`. */
                passed &= snapshot_fn->nested[0].is_generator == 1;
                passed &= snapshot_fn->nested[0].argument_count == 2;
                passed &= snapshot_fn->nested[0].local_count == 3;
            }
            passed &= filtered_fn->nested_count == 1;
            if (filtered_fn->nested_count == 1) {
                /* Hidden iterator plus the captured parameter `limit`. */
                passed &= filtered_fn->nested[0].is_generator == 1;
                passed &= filtered_fn->nested[0].argument_count == 2;
            }
            /* The module-level genexp binding `spy`. */
            passed &= program.code.nested[2].is_generator == 1;
            passed &= program.code.nested[2].argument_count == 1;
        }
        passed &= py68_vm_execute(&runtime, &program.code) == PY68_STATUS_OK;
        passed &= runtime.value_stack_count == 0;
        passed &= runtime.frame_count == 0;
        passed &= global_int_is(&runtime, "captured", 203);
        passed &= global_int_is(&runtime, "kept", 3);
        passed &= global_int_is(&runtime, "first", 1);
    }
    program_destroy(&runtime, &program);
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    return passed;
}

/* Builtins that consume an iterable drain a generator argument through a
   VM-driven collect loop, so no native ever re-enters the interpreter. */
static int test_generator_collect(void)
{
    Py68Runtime runtime;
    Program program;
    Py68Error error;
    Py68Status status;
    Py68Value value;
    int passed = 1;
    const char *text =
        "def gen(n):\n"
        "    i = 0\n"
        "    while i < n:\n"
        "        yield i\n"
        "        i = i + 1\n"
        "total = sum(gen(5))\n"
        "biased = sum(gen(3), 100)\n"
        "squares = sum(x * x for x in range(4))\n"
        "collected = list(gen(3))\n"
        "empty = list(gen(0))\n"
        "ordered = sorted(x for x in [3, 1, 2])\n"
        "some = any(x > 3 for x in range(5))\n"
        "every = all(x > 3 for x in range(5))\n"
        "partial = gen(3)\n"
        "head = next(partial)\n"
        "tail = list(partial)\n"
        "again = list(partial)\n"
        "deep = sum(sum(y for y in range(x)) for x in range(4))\n";

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    passed &= py68_builtins_install(&runtime) == PY68_STATUS_OK;
    passed &= program_build(&runtime, &program, text, &status, &error);
    passed &= status == PY68_STATUS_OK;
    if (status == PY68_STATUS_OK) {
        passed &= py68_vm_execute(&runtime, &program.code) == PY68_STATUS_OK;
        passed &= runtime.value_stack_count == 0;
        passed &= runtime.frame_count == 0;
        passed &= global_int_is(&runtime, "total", 10);
        passed &= global_int_is(&runtime, "biased", 103);
        passed &= global_int_is(&runtime, "squares", 14);
        passed &= global_int_is(&runtime, "head", 0);
        passed &= global_int_is(&runtime, "deep", 4);
        if (py68_global_get_copy(&runtime, (const Py68U8 *)"collected", 9,
                                &value) == PY68_STATUS_OK) {
            passed &= value.type == PY68_VALUE_OBJECT &&
                      value.as.object->type == PY68_OBJECT_LIST &&
                      ((Py68List *)value.as.object)->count == 3;
            py68_value_release(&runtime, value);
        } else {
            passed = 0;
        }
        if (py68_global_get_copy(&runtime, (const Py68U8 *)"empty", 5,
                                &value) == PY68_STATUS_OK) {
            passed &= value.type == PY68_VALUE_OBJECT &&
                      value.as.object->type == PY68_OBJECT_LIST &&
                      ((Py68List *)value.as.object)->count == 0;
            py68_value_release(&runtime, value);
        } else {
            passed = 0;
        }
        /* The partially consumed generator yields its remainder once. */
        if (py68_global_get_copy(&runtime, (const Py68U8 *)"tail", 4,
                                &value) == PY68_STATUS_OK) {
            passed &= ((Py68List *)value.as.object)->count == 2;
            py68_value_release(&runtime, value);
        } else {
            passed = 0;
        }
        if (py68_global_get_copy(&runtime, (const Py68U8 *)"again", 5,
                                &value) == PY68_STATUS_OK) {
            passed &= ((Py68List *)value.as.object)->count == 0;
            py68_value_release(&runtime, value);
        } else {
            passed = 0;
        }
        if (py68_global_get_copy(&runtime, (const Py68U8 *)"some", 4,
                                &value) == PY68_STATUS_OK) {
            passed &= value.type == PY68_VALUE_BOOL && value.as.integer != 0;
            py68_value_release(&runtime, value);
        } else {
            passed = 0;
        }
        if (py68_global_get_copy(&runtime, (const Py68U8 *)"every", 5,
                                &value) == PY68_STATUS_OK) {
            passed &= value.type == PY68_VALUE_BOOL && value.as.integer == 0;
            py68_value_release(&runtime, value);
        } else {
            passed = 0;
        }
    }
    program_destroy(&runtime, &program);
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    return passed;
}

/* An exception raised part-way through a collect unwinds the activation, the
   partially filled list and the generator argument. */
static int test_generator_collect_error(void)
{
    Py68Runtime runtime;
    Program program;
    Py68Error error;
    Py68Status status;
    int passed = 1;
    const char *text =
        "def boom():\n"
        "    yield 1\n"
        "    raise ValueError(\"inside\")\n"
        "caught = 0\n"
        "try:\n"
        "    held = list(boom())\n"
        "except ValueError:\n"
        "    caught = 1\n";

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    passed &= py68_builtins_install(&runtime) == PY68_STATUS_OK;
    passed &= program_build(&runtime, &program, text, &status, &error);
    passed &= status == PY68_STATUS_OK;
    if (status == PY68_STATUS_OK) {
        passed &= py68_vm_execute(&runtime, &program.code) == PY68_STATUS_OK;
        passed &= runtime.value_stack_count == 0;
        passed &= runtime.frame_count == 0;
        passed &= global_int_is(&runtime, "caught", 1);
    }
    program_destroy(&runtime, &program);
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;
    return passed;
}

int main(void)
{
    int passed = 1;

    passed &= test_tokenizer();
    passed &= test_is_generator_flag();
    passed &= test_yield_outside_function();
    passed &= test_verify_rejects_stray_yield();
    passed &= test_generator_lifecycle();
    passed &= test_generator_allocation_failure();
    passed &= test_generator_execution();
    passed &= test_generator_exhaustion();
    passed &= test_generator_try_state();
    passed &= test_generator_abandon();
    passed &= test_generator_exp_shape();
    passed &= test_generator_exp_snapshot();
    passed &= test_generator_collect();
    passed &= test_generator_collect_error();
    if (passed) { puts("PASS: generator tests"); return 0; }
    return 1;
}
