# Python68K

**Version 0.1.0** — Copyright © 2026 Piotr Rozentreter (Rozsoft)

Python68K is a deliberately restricted, Python-compatible language and runtime for classic **Motorola 68000** Amiga systems (AmigaOS 2.x+), with a modern **Linux/host** build for development and testing.

Source is compiled to a custom bytecode format and executed on an explicit stack VM. The implementation is portable **ANSI C89**, cross-compiled with **vbcc** (`+aos68k`) for Amiga Hunk executables and with **GCC** on the host.

Executable name: `pythonami`

---

## Features (Language Level 0.1)

### Values and operators
- Scalars: integers (signed 32-bit, checked overflow), `True` / `False`, `None`
- Arithmetic: `+ - * // %` with Python floor-division and modulo for negatives
- Comparisons: `== != < <= > >=` (bool compares as 0/1; `None == None`)
- Unary: `+ - not`
- Short-circuit `and` / `or` (value-preserving; empty string/list are falsy)

### Control flow
- `if` / `elif` / `else`
- `while` … `else`, `for … in range(...)` … `else`
- `break` / `continue` (including `break` inside `for`)

### Data
- Lists: literals, index get/set (including nested), concat, slice, `list_append` / `list_pop`
- Strings: literals, concat, index, slice, `len`
- `range(stop)`, `range(start, stop)`, `range(start, stop, step)` (including negative step)

### Functions
- `def` with parameters and locals, recursion, explicit/`None` return
- Local shadowing of globals; unbound local reads raise errors
- Augmented assignment: `+= -= *= //= %=`

### Builtins
`print`, `len`, `range`, `list_append`, `list_pop`, `int`, `str`, `bool`, `abs`, `min`, `max`, `exit`

### Tooling
- CLI: `pythonami script.py`, `pythonami -c "..."`, `-V` / `--help`
- Pipeline: tokenize → parse (AST) → symbol analysis → compile → verify → VM execute
- Host unit tests and language fixture diffs (`make test`)
- Error reporting with frame traceback

### Not in 0.1.0
Classes, imports, exceptions as objects, floats, Unicode, dicts, comprehensions, closures, nested `def`, AmigaDOS file/environment APIs, and frozen cross-target differential sign-off on emulator/hardware.

---

## Quick start

```bash
make help                 # list targets
make host                 # → build/host/pythonami (debug)
make host MODE=release
make amiga                # → ./pythonami (Amiga release Hunk)
make amiga MODE=debug     # → ./pythonami-debug
make test                 # host unit + language tests
make clean
```

```bash
./build/host/pythonami -V
./build/host/pythonami examples/hello.py
./build/host/pythonami examples/test_features.py
./build/host/pythonami -c 'print(1 + 2 * 3)'
```

Amiga toolchain defaults (override if needed):

```text
VBCC=/home/piotr/local/vbcc
NDK=/run/media/piotr/BACKUP/Rozen/Programy/Amiga/NDK3.2
```

```bash
make amiga VBCC=/path/to/vbcc NDK=/path/to/NDK3.2
```

---

## Architecture

```text
source → tokenizer → parser (AST) → symbols → compiler → verifier → stack VM
```

| Area | Role |
|------|------|
| Host | GCC, sanitizers, fast unit/language tests |
| Amiga | vbcc `+aos68k`, `-cpu=68000 -fpu=0`, AmigaDOS I/O via platform layer |
| Memory | Tracked allocator, refcounted objects, explicit ownership |
| Bytecode | Stable opcodes, big-endian operands, verified before execution |

Details: `docs/architecture.md`, `docs/bytecode.md`, `docs/language-reference.md`, `docs/memory-model.md`.

---

## Repository layout

```text
src/           Implementation (.c)
include/       Public headers (py68k_*.h)
platform/      Amiga compatibility shims
config/        Host / Amiga config headers
examples/      Sample scripts (hello, fibonacci, test_features, …)
tests/unit/    Host C unit tests
tests/language/  Python68K scripts + expected stdout fixtures
docs/          Architecture, builds, testing, decisions
Makefile       Easy entry: host / amiga / test
Makefile.host  GCC host build and tests
Makefile.amiga vbcc Amiga build
```

---

## Documentation

| Document | Contents |
|----------|----------|
| `docs/language-reference.md` | What is executable in 0.1 |
| `docs/host-build.md` | Host GCC build |
| `docs/amiga-build.md` | vbcc / AmigaOS build and Workbench notes |
| `docs/testing.md` | Test coverage narrative |
| `docs/decisions.md` | Design decisions (D-0001…) |
| `docs/architecture.md` | Pipeline and module overview |
| `CHANGELOG.md` | Release history |

Normative design briefs (for implementers): `Python68K_Full_Agent_Implementation_Brief.md`, `Python68K_Final_Implementation_Checklist.md`.

---

## License / copyright

Copyright © 2026 Piotr Rozentreter (Rozsoft). All rights reserved unless otherwise noted in the repository.
