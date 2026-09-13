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
- Consequences: Reading an unbound local is now a runtime error with a clear diagnostic, matching Python's runtime (not compile-time) detection of this condition. The sentinel must never be observable by user code as a first-class value (it is not `None`, not printable, and cannot be stored back into another variable).
