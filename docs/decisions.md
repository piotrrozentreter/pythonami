# Decisions

## D-0001: Host 32-bit language integer typedef

- Context: The brief requires `signed long` for `Py68I32`, while modern 64-bit hosts commonly define `long` as 64 bits.
- Decision: Use `signed int` and `unsigned int` for host builds and `signed long` and `unsigned long` for Amiga builds, with compile-time four-byte assertions in both configurations.
- Alternatives considered: Force the host compiler into an LLP32 data model, or reject common 64-bit host compilers.
- Consequences: The language width is explicit and portable on the supported host and Amiga targets; serialized formats must continue to use `Py68U8` byte encoding rather than C type layout.

## D-0002: Cross-target validation strategy

- Context: Python68K must be checked both on the Motorola 68000 target and on a current Linux Intel host without confusing compilation evidence with execution evidence.
- Decision: Use `vbccm68k` for 68000 compiler/object checks, `vbcci386` or GCC for Intel-host compatibility checks, and Musashi for 680x0 emulation when the repository and harness are available.
- Alternatives considered: Treat the Amiga Hunk build as sufficient, or rely only on GCC and desktop tests.
- Consequences: Host sanitizers remain fast and authoritative for portable-core defects; target and emulator results are reported separately, and no hardware compatibility claim is made without actual execution evidence.

## D-0003: Workbench startup ownership

- Context: The sibling assembler project contains `wbstartup.s`, while the Python68K Amiga build uses vbcc's `+aos68k` C runtime.
- Decision: Keep vbcc `startup.o` as the sole Workbench startup/exit owner. Do not link or call the sibling `wbstartup.s` from C code.
- Alternatives considered: Add `WBStartup`/`WBExit` calls around `main`, or replace vbcc startup with a custom assembly entry.
- Consequences: The current build follows the vbcc/NDK Workbench handshake contract; a second message receive/reply must not be introduced. Workbench retesting must use a freshly rebuilt NDK-linked binary.

## D-0004: vbcc-native Amiga runtime linkage

- Context: The NDK `amiga.lib` and vbcc target libraries are separate ABI/runtime families.
- Decision: Link the Python68K Amiga C program through the vbcc `+aos68k` configuration and its target `startup.o`/`vc.lib`; use the NDK only as an optional reference/source of headers, not as a library mixed into this link.
- Alternatives considered: Append NDK `lib/amiga.lib` to the vbcc link command.
- Consequences: The C runtime, Workbench startup handshake, DOS inline calls, and vbcc ABI remain coherent. Amiga runtime execution must be retested with freshly rebuilt artifacts.

## D-0005: Release-build stack safety flags

- Context: The debug build succeeds while the release binary crashes in Amiga startup or early execution, a classic sign of a 68000 optimization or delayed-pop issue rather than a source-logic error.
- Decision: Keep `-use-framepointer` and `-no-delayed-popping` in both debug and release builds to preserve a stable stack frame and avoid release-only Guru Meditation behavior.
- Alternatives considered: Keep the release build at `-O=2` alone, or add a custom assembly startup wrapper.
- Consequences: The release artifact follows the same stable ABI assumptions as the debug build; any remaining emulator or hardware crash will be treated as a true runtime issue, not a compiler flag mismatch.

## D-0006: Content-based global/builtin name lookup

- Context: `Py68GlobalEntry` and the builtin table originally stored a `name_index` into a code object's constant/name table. Each `Py68Code` (module or function body) owns its own independently-numbered name table, so the same numeric index can denote different identifiers in different code objects. Once function bodies could read/write module globals and call builtins, this caused genuine cross-code-object name collisions (a function's local index 2 could collide with the module's index 2 for an unrelated name).
- Decision: Store `(const Py68U8 *name, Py68U16 name_length)` in both `Py68GlobalEntry` and the builtin registry, and resolve by byte-content comparison instead of index equality. `py68_global_set_copy`/`get_copy` and `py68_builtin_set_copy`/`get_copy` take name bytes and length directly.
- Alternatives considered: Give every code object a shared/global name table (larger refactor, touches the compiler's per-function name emission); intern all names into one process-wide table with stable indices.
- Consequences: Global/builtin lookup is a linear byte comparison rather than an index compare, which is acceptable at Language Level 0.1's expected program sizes on a 68000. A future increment may revisit interning with FNV-1a hashing (already implemented for `Py68String`) if lookup cost becomes a concern for larger programs.

## D-0007: UNBOUND sentinel for uninitialized locals

- Context: Python raises `UnboundLocalError` when a function reads a local variable before any assignment reaches it on the executed path (e.g. `if False: x = 1` then `return x`). The frame previously initialized every local slot to `None`, silently masking this class of bug and diverging from Python's documented semantics.
- Decision: Add `PY68_VALUE_UNBOUND` as a distinct `Py68ValueType` and initialize every non-parameter local slot to it when a frame is set up. `OP_LOAD_LOCAL` checks for this sentinel and raises a runtime error instead of returning it as a usable value; `OP_STORE_LOCAL` overwrites it normally.
- Alternatives considered: Track "assigned" state via a separate bitmask per frame; perform a static "definitely assigned" data-flow analysis at compile time.
## D-0008: Short-circuit and/or via JUMP_*_OR_POP

- Context: Python `and`/`or` must not evaluate the right-hand operand when the left-hand value already decides the result, and must return the deciding operand rather than a coerced boolean.
- Decision: Compile `and`/`or` with `OP_JUMP_IF_FALSE_OR_POP` / `OP_JUMP_IF_TRUE_OR_POP`. Verifier treats the jump path as keeping TOS and the fall-through path as popping TOS (stack effect -1).
- Alternatives considered: Always evaluate both sides into booleans with `OP_AND`/`OP_OR` opcodes.
- Consequences: Matches Python value-preserving short-circuit semantics; empty strings/lists are falsy via updated truthiness rules.

## D-0009: for-break pops the range iterator

- Context: `for` compilation leaves a range object on the value stack for `OP_RANGE_NEXT`. Exhausted iteration pops it before joining the exit/else path, but `break` previously jumped to the same join with the iterator still on the stack, so the verifier reported inconsistent stack depth.
- Decision: Mark for-loop contexts with `pop_on_break` and emit `OP_POP` immediately before each for-`break` jump. `OP_POP` releases the popped value. While-loops leave `pop_on_break` clear.
- Alternatives considered: Dedicated break-cleanup label after the loop, or changing `OP_RANGE_NEXT` metadata so break could jump through a shared pop block only.
- Consequences: for-`break` verifies and runs; else clauses remain skipped on break; continue is unchanged (jumps back to `RANGE_NEXT` with the iterator still under the body).

## D-0010: Subscript assignment via OP_STORE_INDEX

- Context: Language Level 0.1 documents list item assignment and `OP_STORE_INDEX` existed, but the statement parser only accepted bare-name targets, so `L[1] = 99` failed before codegen.
- Decision: After parsing a leading expression, if `=` follows and the expression is an `INDEX` node, emit an assignment whose `target` is that index. Compile as container, index, value, then `OP_STORE_INDEX`. Symbol analysis treats the target as a use (not a new binding). String item assignment remains a TypeError.
- Alternatives considered: Restrict targets to `NAME[index]` only, or invent a separate AST kind.
- Consequences: Nested stores such as `G[1][1] = 40` work because the outer index container may itself be an index expression.

## D-0011: Equality for None and bool/int

- Context: Comparisons required both operands to be `INT`, so `None == None` and `True == 1` raised TypeError despite Language Level 0.1 scalar equality rules.
- Decision: Handle `OP_EQUAL`/`OP_NOT_EQUAL` for `None` first (`None` equals only `None`). Accept `BOOL` alongside `INT` for equality and ordering by using the stored 0/1 integer payload (Python numeric policy for booleans).
- Alternatives considered: Coerce bool to int at load time only, or reject bool/int mixed comparisons.
- Consequences: `True == 1`, `False == 0`, and `None == None` match Python; unrelated types still TypeError on arithmetic/order paths that do not special-case them.
