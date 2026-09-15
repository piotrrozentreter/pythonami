/* 2026 by Piotr Rozentreter (Rozsoft) */

#include "py68k_ast_arena.h"
#include "py68k_builtin.h"
#include "py68k_compiler.h"
#include "py68k_global.h"
#include "py68k_parser.h"
#include "py68k_platform.h"
#include "py68k_runtime.h"
#include "py68k_source.h"
#include "py68k_tokenizer.h"
#include "py68k_vm.h"

#include <stdio.h>
#include <string.h>

typedef struct GlobalCheck {
    const char *name;
    Py68U16 length;
    Py68I32 expected;
} GlobalCheck;

static int global_int_equals(Py68Runtime *runtime, const GlobalCheck *check)
{
    Py68Value value;
    int equal;
    if (py68_global_get_copy(runtime, (const Py68U8 *)check->name,
                             check->length, &value) != PY68_STATUS_OK)
        return 0;
    equal = value.type == PY68_VALUE_INT && value.as.integer == check->expected;
    py68_value_release(runtime, value);
    return equal;
}

/* Global names borrow source bytes, so checks must run before teardown. */
static int run_source(Py68Runtime *runtime, const char *text,
                      Py68Status *status_out, const GlobalCheck *checks,
                      Py68U16 check_count)
{
    Py68Source source;
    Py68TokenArray tokens;
    Py68AstArena arena;
    Py68StatementParser parser;
    Py68AstNode *module;
    Py68Code code;
    Py68Error error;
    Py68U16 index;
    int ok = 1;

    py68_token_array_initialize(&tokens);
    ok &= py68_source_initialize(&runtime->allocator, &source, "poll.py",
        (const Py68U8 *)text, (Py68U32)strlen(text)) == PY68_STATUS_OK;
    ok &= py68_tokenize(&runtime->allocator, &source, &tokens, &error) ==
          PY68_STATUS_OK;
    py68_ast_arena_initialize(&arena, &runtime->allocator);
    parser.expression.allocator = &runtime->allocator;
    parser.expression.source = &source;
    parser.expression.tokens = &tokens;
    parser.expression.position = 0;
    parser.expression.arena = &arena;
    parser.expression.error = &error;
    parser.inside_function = 0;
    parser.loop_depth = 0;
    ok &= py68_parse_module(&parser, &module) == PY68_STATUS_OK;
    ok &= py68_compile_module(&runtime->allocator, &source, module, &code,
                              &error) == PY68_STATUS_OK;
    ok &= py68_builtins_install(runtime) == PY68_STATUS_OK;
    *status_out = py68_vm_execute(runtime, &code);
    for (index = 0; index < check_count; ++index)
        ok &= global_int_equals(runtime, &checks[index]);
    py68_code_destroy(&runtime->allocator, &code);
    py68_ast_arena_destroy(&arena);
    py68_token_array_destroy(&runtime->allocator, &tokens);
    py68_source_destroy(&runtime->allocator, &source);
    return ok;
}

int main(void)
{
    Py68Runtime runtime;
    Py68Status status;
    int passed = 1;
    static const GlobalCheck surface_checks[] = {
        { "interval", 8, 0 },
        /* The break is consumed once, so exactly one round observes it. */
        { "seen", 4, 1 },
        { "rounds", 6, 4 }
    };
    /* Unbounded loop: only the poll hook can end it. */
    const char *endless =
        "total = 0\n"
        "while True:\n"
        "    total = total + 1\n";
    const char *bounded =
        "total = 0\n"
        "for index in range(100):\n"
        "    total = total + index\n";

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    runtime.poll_interval = 1;
    runtime.poll_counter = 1;
    py68_platform_signal_break(&runtime);
    passed &= run_source(&runtime, endless, &status, NULL, 0);
    passed &= status == PY68_STATUS_RUNTIME_ERROR;
    passed &= runtime.error.kind == PY68_ERROR_INTERRUPT;
    passed &= runtime.error.active != 0;
    passed &= runtime.value_stack_count == 0 && runtime.frame_count == 0;
    py68_runtime_shutdown(&runtime);
    passed &= runtime.live_objects == NULL;
    passed &= runtime.allocator.stats.current_bytes == 0;

    /* A pending break must not be catchable by a bare except. */
    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    runtime.poll_interval = 1;
    runtime.poll_counter = 1;
    py68_platform_signal_break(&runtime);
    passed &= run_source(&runtime,
        "caught = 0\n"
        "try:\n"
        "    while True:\n"
        "        caught = caught\n"
        "except:\n"
        "    caught = 1\n", &status, NULL, 0);
    passed &= status == PY68_STATUS_RUNTIME_ERROR;
    passed &= runtime.error.kind == PY68_ERROR_INTERRUPT;
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;

    /* No pending break: polling must not disturb normal execution. */
    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    runtime.poll_interval = 1;
    runtime.poll_counter = 1;
    passed &= run_source(&runtime, bounded, &status, NULL, 0);
    passed &= status == PY68_STATUS_OK;
    passed &= runtime.value_stack_count == 0 && runtime.frame_count == 0;
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;

    /* poll_interval == 0 disables the check entirely. */
    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    runtime.poll_interval = 0;
    runtime.poll_counter = 0;
    py68_platform_signal_break(&runtime);
    passed &= run_source(&runtime, bounded, &status, NULL, 0);
    passed &= status == PY68_STATUS_OK;
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;

    /* Script-visible surface: set/get_poll_interval, check_break, yield_cpu. */
    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    py68_platform_signal_break(&runtime);
    passed &= run_source(&runtime,
        "set_poll_interval(0)\n"
        "interval = get_poll_interval()\n"
        "seen = 0\n"
        "rounds = 0\n"
        "while rounds < 4:\n"
        "    rounds = rounds + 1\n"
        "    yield_cpu()\n"
        "    if check_break():\n"
        "        seen = seen + 1\n", &status, surface_checks, 3);
    passed &= status == PY68_STATUS_OK;
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;

    /* set_poll_interval rejects non-integer and negative arguments. */
    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    passed &= run_source(&runtime, "set_poll_interval(-1)\n", &status,
                         NULL, 0);
    passed &= status == PY68_STATUS_RUNTIME_ERROR;
    passed &= runtime.error.kind == PY68_ERROR_VALUE;
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;

    passed &= py68_runtime_initialize(&runtime) == PY68_STATUS_OK;
    passed &= run_source(&runtime, "set_poll_interval('x')\n", &status,
                         NULL, 0);
    passed &= status == PY68_STATUS_RUNTIME_ERROR;
    passed &= runtime.error.kind == PY68_ERROR_TYPE;
    py68_runtime_shutdown(&runtime);
    passed &= runtime.allocator.stats.current_bytes == 0;

    if (passed) { puts("PASS: interrupt tests"); return 0; }
    return 1;
}
