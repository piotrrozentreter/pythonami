# Testing

Host checks are driven by `make test`. Phase 0 covers type/runtime bootstrap state, tracked allocation, reallocation, injected failure, and cleanup. Phase 1 adds integer/string/name tokenization, comments, spans, indentation, delimiter nesting, unsupported-keyword classification, negative diagnostics, tokenizer allocation failure, AST arena cleanup, and expression precedence/postfix parsing. Phase 2 adds statement suites, assignments, conditionals, loops, functions, returns, and invalid loop/return context checks. Phase 3 adds deterministic parameter/local slots, duplicate-parameter rejection, nested-function rejection, global/local/builtin/undefined classification, and UNBOUND slot metadata. Phase 4 adds opcode metadata, bytecode growth and big-endian branch patching, constant/name tables, and module/function-body emission. Phase 5 adds linear instruction boundaries, operand/index checks, branch targets, CFG traversal, stack-depth consistency, maximum-stack calculation, and malformed-bytecode rejection. The VM slice adds verified scalar dispatch, explicit value/frame stacks, checked integer operations, and cleanup on runtime errors. Runtime-value tests cover tracked reference-counted strings and lists, aliases, indexed replacement, bounds errors, cycle rejection, release, and shutdown cleanup. Host sanitizer validation uses AddressSanitizer and UndefinedBehaviorSanitizer when supported; the full `make test` suite (all unit tests plus `pythonami -V`/`--help`) has been run under `-fsanitize=address,undefined` with zero leaks and zero UB findings.

`tests/unit/test_compiler.c` covers `break`/`continue` bytecode generation and jump patching: `break` inside a `while` body, `continue` inside a `for` body (verified via executed VM state), and `while`-`else` compilation for both the normal-completion path (else runs) and the `break`-exits-early path (else is skipped), each checked with `allocator.stats.current_bytes == 0` after teardown. It also covers source-level compilation and VM execution of `def` functions: parameter binding, recursion (`fibonacci`), local-variable shadowing of globals, `UNBOUND` local read errors, rejection of nested `def` statements, and rejection of duplicate parameter names.

## Cross-target fixture comparison (host vs. Amiga emulator/hardware)

The host build (Intel/Linux, this environment) can run and be verified directly. The Amiga Hunk build (`vbcc +aos68k`, produced by `Makefile.amiga`) cannot execute in this sandboxed environment — it requires an AmigaDOS runtime, a 680x0 emulator (e.g. WinUAE/FS-UAE), or real hardware, and must be tested by the project owner. Every increment therefore builds and tests **both** targets:

- Host: `make -f Makefile.host test` runs the full unit-test suite and smoke-checks `-V`/`--help` on Linux. `build/host/pythonami <script.py>` executes example/feature scripts directly.
- Amiga: `export VBCC=/home/piotr/local/vbcc && make -f Makefile.amiga clean && make -f Makefile.amiga amiga-debug && make -f Makefile.amiga amiga-release` produces `pythonami-debug` and `pythonami` Hunk executables. These are compile/link-verified on the host but **not execution-verified** here.

`examples/print_values.py` is the platform-neutral fixture: its expected stdout is recorded in `tests/fixtures/print_values.expected.txt`. `examples/test_features.py` is the growing incremental feature-test script — it is extended with every new language feature and is intended to be run on the Amiga side (emulator or hardware) by the project owner and diffed against the host's output for the same script, which is captured here for reference:

```
=== 1. Scalar Arithmetic ===
13
7
30
3
1
=== 2. Comparisons & Logic ===
True
True
False
=== 3. Conditional Statements ===
a is greater than 5
=== 4. While Loops ===
3
2
1
=== 5. Builtin Functions & Lists ===
[10, 20, 30]
3
20
30
[10, 20]
2
=== 6. For Loops & Range ===
10
25
=== 7. Augmented Assignment ===
15
12
24
6
2
=== 8. Function Definitions ===
7
0
1
1
2
3
5
8
13
21
34
15
=== 9. Lists with list_append ===
[1, 2, 3, 4]
4
=== 10. Short-circuit and/or ===
2
5
0
4
=== 11. Strings ===
abcd
e
ell
5
=== 12. Remaining Builtins ===
42
7
False
True
3
2
9
=== Feature Test Complete ===
```

Host exit code for `examples/test_features.py` is `0`. CLI `-c` is also supported (`pythonami -c 'print(1+2)'`). If the Amiga run produces different output or a non-zero exit code, that is a real target-specific bug to report (not a host/logic bug, since the host build already exercises the identical bytecode/VM path).
