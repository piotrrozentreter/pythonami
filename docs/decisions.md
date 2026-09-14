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

## D-0012: Language Level 0.2 file and env/assign builtins

- Context: Phase 6 requires AmigaDOS file and environment access. Language Level 0.1 has no attribute access or `with`, so Python-style file objects with methods are unavailable. Amiga “environment” for this project means DOS assigns, not `ENV:` GetVar.
- Decision: Expose function builtins `fopen`/`fclose`/`fread`/`freadline`/`fwrite`/`exists`/`remove`/`rename` with modes `r`/`w`/`a`/`rb`/`wb`/`ab`. Binary and text both use string payloads (no `bytes` type). Host installs `getenv`/`setenv`/`unsetenv`; Amiga installs `assign_get`/`assign_add`/`assign_remove` on the same platform_var_* layer (`AssignPath`, `AssignLock(name,0)`, `Lock("name:")`+`NameFromLock`). Platform I/O stays in `file_host.c` / `file_amiga.c`; `PY68_OBJECT_FILE` closes on final release.
- Alternatives considered: method-style `open()`, Amiga `GetVar`/`SetVar`, full CPython mode matrix (`+`, `x`).
- Consequences: Scripts targeting Amiga should call `assign_*`. Host tests exercise file APIs and POSIX env. Emulator/hardware assign behavior remains owner-verified.

## D-0013: True divide `/` vs floor divide `//`

- Context: Language Level 0.1 tokenized both `/` and `//` as floor-divide. Level 0.3 adds binary32 float and Python-3 true division.
- Decision: `/` emits `OP_TRUE_DIVIDE` and always yields a finite `float`. `//` remains integer floor division. NaN and Inf results are rejected as `ValueError`. No 68881 and no Amiga IEEE library: binary32 add/mul/div/parse/print are integer-only software in `src/float.c` (`-cpu=68000 -fpu=0`).
- Alternatives considered: Keep `/` as floor until a later level; use binary64.
- Consequences: Existing scripts that used `/` for floor-int must switch to `//`. Decision is a documented language-level break from 0.2.0.

## D-0014: Limited attributes and bound methods

- Context: Level 0.1 had no attribute access; file I/O used function builtins. Dict methods, `with`, and imports need `obj.name`.
- Decision: `obj.name` compiles to `OP_LOAD_ATTR`. Each heap type has a static method table. Lookup builds a `PY68_OBJECT_BOUND_METHOD` `{self, native}` consumed by `OP_CALL`. Modules resolve attributes in their global table. `STORE_ATTR` is allowed only on module objects. No user-defined attributes or classes.
- Alternatives considered: Function-style `dict_get` only; full instance dictionaries.
- Consequences: `list.append` exists alongside `list_append`. `sys.path.append` works because `sys.path` is a list.

## D-0015: Parenthesized tuples only

- Context: `(` `)` already grouped expressions. Bare `a, b` would collide with call and assignment parsing.
- Decision: Accept only parenthesized tuples: `()`, `(a,)`, `(a, b)`. A single `(expr)` remains grouping.
- Alternatives considered: Full Python tuple display including unparenthesized targets.
- Consequences: `return a, b` is a syntax error; write `return (a, b)`.

## D-0016: Hashable keys and cyclic containers

- Context: Dict and set need a value hash/equality protocol. Level 0.1 already rejects cyclic lists.
- Decision: Hashable: `None`, `bool`, `int`, `str`, and tuples of hashable items. Unhashable keys raise `TypeError`. Inserting a value that would make a dict reachable from itself is `ValueError: cyclic containers are not supported`. `OP_EQUAL` uses the same equality helper (so strings and lists compare).
- Alternatives considered: Allow all objects as keys via identity; add a tracing GC instead of cycle rejection.
- Consequences: Deterministic FNV-1a (strings) plus identity-free scalar hashes; no randomized hashing.

## D-0017: Catchable exceptions without classes

- Context: Runtime errors were a single aborting `Py68Error`. Level 0.4 needs `try`/`except`/`finally`/`raise`.
- Decision: Token/syntax/bytecode/memory/internal errors stay uncatchable. Other `Py68ErrorKind` values become `PY68_OBJECT_EXCEPTION` objects. `OP_SETUP_TRY` / `OP_POP_TRY` record handler IP and stack depth per frame. Matching compares exception kind to a builtin exception-type native (`TypeError`, …), not a class MRO. `finally` bodies are compiled inline before `return`/`break`/`continue`.
- Alternatives considered: Full exception class hierarchy; CPython block stack with Why flags.
- Consequences: `except TypeError as e` works; user-defined exception types do not.

## D-0018: `with` as enter / try / finally / exit

- Context: File I/O had no context managers. Level 0.4 adds `with`.
- Decision: Compile `with EXPR as NAME` to keep the manager on the stack, call `__enter__`, bind the result, wrap the body in `SETUP_TRY`, and call `__exit__(None, None, None)` on both success and handler paths. First context manager: `PY68_OBJECT_FILE` (`__enter__` returns self, `__exit__` closes).
- Alternatives considered: Method-style `open()`; no `as` target only.
- Consequences: `with fopen(path, mode) as f:` is the supported form.

## D-0019: Single-directory import loader

- Context: Level 0.1 had no import system. “Module” meant the top-level script code object.
- Decision: `import` / `from` / `as` compile to `OP_IMPORT_NAME` / `OP_IMPORT_FROM`. The loader reads `name.py` from the importing source directory, then `sys.path` entries. Compiled modules are `PY68_OBJECT_MODULE` objects cached by resolved path. Relative imports and `import *` stay unsupported. `sys` is a builtin module (`path`, `modules`, `argv`).
- Alternatives considered: Full package/`__init__.py` trees; CPython `.pyc`.
- Consequences: Multi-file programs work for sibling `.py` files; no CPython bytecode compatibility.

## D-0020: `set` and `dict` are names, not keywords

- Context: The 0.1 tokenizer classified `set` and `dict` as unsupported keywords, unlike Python where they are builtins.
- Decision: Remove them from the keyword table so they tokenize as `PY68_TOKEN_NAME` and resolve to constructor builtins.
- Alternatives considered: Keep them as keywords that introduce literal syntax only.
- Consequences: `set = 1` is a legal (if unwise) assignment that shadows the builtin.
