# Python68K Developer Guide

This guide is for contributors working on the portable ANSI C89 interpreter,
its host test harness, and the Amiga 68000 target. Keep public language claims
in [`language-reference.md`](language-reference.md), build details in
[`host-build.md`](host-build.md) and [`amiga-build.md`](amiga-build.md), and
design rationale in [`decisions.md`](decisions.md).

## Development loop

1. Build the host target with `make host`.
2. Add or update a focused C unit test under `tests/unit/` or a language fixture
   under `tests/language/` with its expected stdout fixture.
3. Run the narrowest relevant target, then run `make test`.
4. Run the release build with `make host MODE=release` when compiler warnings,
   optimization, or size-sensitive behavior is involved.
5. When Amiga code is affected, run `make amiga MODE=debug` and
   `make amiga` with the configured vbcc and NDK. Report emulator or hardware
   execution separately from host build evidence.

The host build uses GCC by default and accepts `HOST_CC=clang`. It is compiled
as strict C89 with warnings treated as errors. Do not assume host `long` has
the Amiga width; use the project typedefs and configuration headers.

## Source layout

| Area | Responsibility |
| --- | --- |
| `src/main.c` | CLI parsing, source/check execution, diagnostics, exit mapping |
| `src/token*.c` | Token definitions and indentation-aware tokenization |
| `src/parser_*.c` | Expression and statement parsing into the AST arena |
| `src/ast_arena.c` | Lifetime management for parse trees |
| `src/symbol.c` | Function prepass, name classification, and local slots |
| `src/compiler.c` | AST-to-bytecode emission and control-flow patching |
| `src/code.c`, `src/opcode.c` | Bytecode buffers, constants, names, and opcode metadata |
| `src/verify.c` | Instruction, operand, branch, CFG, and stack-depth validation |
| `src/vm.c` | Verified stack-machine execution, frames, calls, and unwinding |
| `src/object*.c` and type files | Refcounted runtime values and container behavior |
| `src/import.c`, `src/module.c` | Module lookup, compilation, cache, and `sys` |
| `src/platform_*.c` | Host/Amiga I/O, clocks, process calls, and target handles |
| `include/` | Public internal interfaces and fixed-width/configured types |
| `tests/unit/` | Host C-level tests with allocator teardown checks |
| `tests/language/` | Source-level behavior fixtures and expected stdout |
| `tests/integration/` | Host scripts and owner-run Amiga integration procedures |
| `ext/demo_add/` | Sample Amiga LoadSeg extension and its assembly header |

## Execution pipeline

```text
source -> tokenizer -> parser/AST -> symbol analysis -> compiler
       -> bytecode verifier -> stack VM
```

`src/main.c` uses the same compile and verify path for normal execution and
`--check`; check-only mode stops before builtin installation and VM execution.
Imported modules are compiled when normal execution reaches the import, then
cached by the module loader. The explicit allocator and reference-counted
objects make ownership and teardown part of the runtime contract, so new object
paths should include cleanup assertions in tests.

## Adding a language feature

1. Define or confirm the syntax in `docs/grammar.ebnf` and the tokenizer token
   rules.
2. Add parser and context validation behavior.
3. Extend symbol analysis before emitting bytecode that depends on names.
4. Add opcode metadata and verifier rules before VM dispatch if new bytecode is
   required.
5. Implement runtime ownership and platform behavior through the existing
   interfaces rather than embedding host or Amiga calls in the VM.
6. Add a focused unit test and a source-level fixture where the user-visible
   behavior warrants it.
7. Update the language reference, changelog, checklist, and a decision record
   only when the change is implemented and supported by executed evidence.

Unsupported syntax should produce a targeted diagnostic. Do not infer support
from a token or declaration alone: require parser/compiler/runtime behavior and
the relevant test or build evidence.

## Platform and extension work

Portable behavior belongs behind the interfaces in `include/py68k_platform.h`
and the matching host/Amiga source files. Keep the Amiga target at
`+aos68k`, `-cpu=68000`, and `-fpu=0`; do not introduce compiler or library
assumptions that require a 68020 or hardware floating-point unit.

The Amiga extension ABI is documented in [`amiga-extensions.md`](amiga-extensions.md)
and exposed by `include/py68k_ext.h`. `make amiga-ext` builds a pure LoadSeg
Hunk plugin without `startup.o`; the host intentionally has no `load_library()`
implementation. Extension lifetime is tied to the returned module, so native
function references must not outlive the loaded segment.

## Tests and evidence

`make test` is the normal regression gate. The focused Makefile targets are
listed in the [user guide](user-guide.md). Language fixtures compare normalized
stdout, while diagnostics and debug statistics are checked separately on
stderr. Run sanitizer builds when the local compiler supports them and record
the exact command and result in `docs/testing.md`.

Keep these distinctions in reports:

- **Host execution verified:** the host binary ran the test or fixture.
- **Compile/link verified:** a target build completed, but the target program
  did not run in that environment.
- **Owner verified:** the project owner ran the Amiga binary in an emulator or
  on real hardware.

## Documentation and review checklist

Before submitting a change, inspect `git diff`, preserve unrelated working-tree
changes, and check that claims agree with executable behavior. Update
`CHANGELOG.md` only for user-visible released or explicitly unreleased work;
update `docs/decisions.md` for material compatibility or architecture choices.
Do not mark emulator, hardware, or release-artifact verification complete
without that evidence.