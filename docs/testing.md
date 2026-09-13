# Testing

Host checks are driven by `make test`. Phase 0 covers type/runtime bootstrap state, tracked allocation, reallocation, injected failure, and cleanup. Phase 1 adds integer/string/name tokenization, comments, spans, indentation, delimiter nesting, unsupported-keyword classification, negative diagnostics, tokenizer allocation failure, AST arena cleanup, and expression precedence/postfix parsing. Phase 2 adds statement suites, assignments, conditionals, loops, functions, returns, and invalid loop/return context checks. Phase 3 adds deterministic parameter/local slots, duplicate-parameter rejection, nested-function rejection, global/local/builtin/undefined classification, and UNBOUND slot metadata. Phase 4 adds opcode metadata, bytecode growth and big-endian branch patching, constant/name tables, and module/function-body emission. Phase 5 adds linear instruction boundaries, operand/index checks, branch targets, CFG traversal, stack-depth consistency, maximum-stack calculation, and malformed-bytecode rejection. The VM slice adds verified scalar dispatch, explicit value/frame stacks, checked integer operations, and cleanup on runtime errors. Runtime-value tests cover tracked reference-counted strings and lists, aliases, indexed replacement, bounds errors, cycle rejection, release, and shutdown cleanup. Host sanitizer validation uses AddressSanitizer and UndefinedBehaviorSanitizer when supported; the full `make test` suite (all unit tests plus `pythonami -V`/`--help`) has been run under `-fsanitize=address,undefined` with zero leaks and zero UB findings.

`tests/unit/test_compiler.c` covers `break`/`continue` bytecode generation and jump patching: `break` inside a `while` body, `continue` inside a `for` body (verified via executed VM state), and `while`-`else` compilation for both the normal-completion path (else runs) and the `break`-exits-early path (else is skipped), each checked with `allocator.stats.current_bytes == 0` after teardown. It also covers source-level compilation and VM execution of `def` functions: parameter binding, recursion (`fibonacci`), local-variable shadowing of globals, `UNBOUND` local read errors, rejection of nested `def` statements, and rejection of duplicate parameter names.

## Language-level advanced fixtures

`make test` now also runs `make language-test`, which executes Python68K scripts under `tests/language/` and `examples/` through `build/host/pythonami` and diffs stdout against fixtures in `tests/fixtures/` via `tests/integration/host/run_language_check.sh`. Coverage added:

- `tests/language/operators/` — Python floor-div/mod for negatives, precedence, integer comparisons, short-circuit `and`/`or` with empty list/string falsiness, and unary `not` (including empty containers).
- `tests/language/control_flow/` — `elif` ladders, `while`-`else` (complete and break-skip), `for`-`else`, `continue` in `for`/`while`, nested `for` loops.
- `tests/language/functions/` — recursion (fibonacci/factorial/deep sum), local shadowing, reading globals from functions, `continue` inside functions, augassign on locals, multi-path `return`, implicit `None` return.
- `tests/language/lists/` — concat, `list_append`/`list_pop`, nested index, list-building helpers, empty/`step`/`reverse` `range`, list slices with explicit bounds.
- `tests/language/strings/` — concat, index/slice, falsy empty string, `str`/`int` conversions.
- `tests/language/test_advanced_suite.py` — single cross-cutting differential fixture combining the above.
- `examples/test_features.py` sections 13–18 extend the incremental demo with the same advanced behaviors.

A host bug in `OP_NOT` (truthiness evaluated after overwriting the value type, and missing release of object operands) was fixed so `not []` / `not ""` match Language Level 0.1 falsiness rules; language fixtures assert the corrected behavior.

Subsequent host fixes covered by `tests/language/test_bugfix_suite.py` and `examples/test_features.py` sections 19–21: for-`break` emits `OP_POP` to discard the range iterator before joining the exit path; subscript assignment parses `INDEX` targets and executes `OP_STORE_INDEX`; `None`/`bool` equality follows D-0011. Decisions D-0009 through D-0011 record the designs. `tests/unit/test_compiler.c` also executes a for-`break` total accumulation case through verify+VM.

## Cross-target fixture comparison (host vs. Amiga emulator/hardware)

The host build (Intel/Linux, this environment) can run and be verified directly. The Amiga Hunk build (`vbcc +aos68k`, produced by `Makefile.amiga`) cannot execute in this sandboxed environment — it requires an AmigaDOS runtime, a 680x0 emulator (e.g. WinUAE/FS-UAE), or real hardware, and must be tested by the project owner. Every increment therefore builds and tests **both** targets:

- Host: `make -f Makefile.host test` runs the full unit-test suite, smoke-checks `-V`/`--help`, and runs `language-test` stdout diffs on Linux. `build/host/pythonami <script.py>` executes example/feature scripts directly. `make language-test` runs only the language fixture diffs.
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
=== 13. Floor/Mod Negatives ===
-3
2
-3
-2
=== 14. Truthiness & not ===
[]
2
x
5
True
False
True
=== 15. Elif & while-else ===
two
0
1
while-done
=== 16. Nested Loops & continue ===
66
=== 17. Recursion & list build ===
[0, 1, 4, 9]
120
=== 18. Range reverse ===
21
=== 19. for-break ===
10
=== 20. Index Assignment ===
9
2
7
5
6
=== 21. Bool/None Equality ===
True
False
True
True
True
=== Feature Test Complete ===
```

Host exit code for `examples/test_features.py` is `0`. CLI `-c` is also supported (`pythonami -c 'print(1+2)'`). If the Amiga run produces different output or a non-zero exit code, that is a real target-specific bug to report (not a host/logic bug, since the host build already exercises the identical bytecode/VM path).
