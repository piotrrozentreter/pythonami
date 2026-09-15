# Python68K User Guide

Python68K runs a deliberately restricted Python-compatible language on a
68000 Amiga or on a modern development host. The executable is named
`pythonami`. This guide lists the commands normally needed to build, test, and
run it. Language syntax and builtin semantics are specified in
[`language-reference.md`](language-reference.md).

## Command-line interface

The command form is:

```text
pythonami [--debug] [--check] [-V|--version|--help] [-c command | script.py [args...]]
```

Options are processed before the input. `--debug` may precede `-V`, `--help`,
`-c`, or a script path. `--check` compiles and verifies the selected input but
does not execute it; use it before `-c` or the script path.

| Command | Example | Description |
| --- | --- | --- |
| Show version | `pythonami -V` | Prints the current version, `Python68K 0.6.0`. `--version` is the long form. |
| Show help | `pythonami --help` | Prints the usage line. Running `pythonami` with no input does the same. |
| Run a script | `pythonami examples/hello.py` | Loads, compiles, verifies, and executes one `.py` source file. |
| Pass script arguments | `pythonami examples/test_features.py one two` | Makes the script path and following arguments available as `sys.argv`. |
| Execute source text | `pythonami -c "print(1 + 2)"` | Compiles and executes the command string; its diagnostic path is `<string>`. |
| Debug a script | `pythonami --debug examples/hello.py` | Runs the script and writes top-level memory, source, token, bytecode, and stack statistics to stderr. |
| Check a script | `pythonami --check examples/hello.py` | Compiles and verifies without producing script output or performing top-level execution. |
| Check source text | `pythonami --check -c "print(1 + 2)"` | Validates command text without running it. |
| Combine debug and check | `pythonami --debug --check examples/hello.py` | Reports compilation statistics while still skipping execution. |

`--debug -V` and `--debug --help` print only the requested version/help text;
they do not emit a statistics report. Script output goes to stdout. Errors and
debug statistics go to stderr where the platform supports separate streams.

The process exit code is `0` for success. `exit(value)` returns a value
clamped to the range `0..255`; runtime, source, and I/O failures return the
runtime status. A user break maps to `10` on AmigaDOS. A script can inspect
its arguments with:

```python
import sys
print(sys.argv)
```

## Build commands

Run these from the repository root. The top-level `Makefile` forwards host
work to `Makefile.host` and Amiga work to `Makefile.amiga`.

| Command | Example | Description |
| --- | --- | --- |
| Default make target | `make all` | Shows the top-level help because the default `all` target is intentionally informational. |
| List top-level targets | `make help` | Shows the supported top-level targets and configured Amiga paths. |
| Host debug build | `make host` | Builds `build/host/pythonami` with GCC and debug-friendly flags. |
| Host release build | `make host MODE=release` | Builds the optimized host executable. |
| Host debug alias | `make debug` | Forwards to the host debug build. |
| Host release alias | `make release` | Forwards to the host release build. |
| Amiga release build | `make amiga` | Builds the `pythonami` Amiga Hunk executable with vbcc. |
| Amiga debug build | `make amiga MODE=debug` | Builds `pythonami-debug` with debug flags. |
| Explicit Amiga release | `make amiga-release` | Directly invokes the Amiga release target. |
| Explicit Amiga debug | `make amiga-debug` | Directly invokes the Amiga debug target. |
| Build sample extension | `make amiga-ext` | Builds `ext/demo_add/demo_add.py68k` with vbcc, vasm, and vlink. |
| Override Amiga tools | `make amiga VBCC=/path/to/vbcc NDK=/path/to/NDK3.2` | Supplies the vbcc installation and NDK include tree. |
| Remove build products | `make clean` | Removes host, Amiga, and sample-extension products. |

The host compiler can be selected with `HOST_CC`, for example:

```text
make -f Makefile.host test HOST_CC=clang
```

## Test commands

| Command | Example | Description |
| --- | --- | --- |
| Full host suite | `make test` | Builds/runs C unit tests, language fixtures, CLI smoke checks, and focused integration checks. |
| Language fixtures only | `make language-test` | Runs scripts under `examples/` and `tests/language/` and diffs stdout fixtures. |
| Import slice | `make -f Makefile.host import-test` | Runs import unit tests, language fixtures, and import integration checks. |
| Import failures | `make -f Makefile.host import-failure-test` | Checks missing-module, syntax-error, runtime-error, and cycle failures. |
| Debug statistics | `make -f Makefile.host debug-stats-test` | Checks `--debug` output, determinism, stderr routing, and failure status preservation. |
| Stream redirection | `make -f Makefile.host stdio-redirection-test` | Checks stdout/stderr separation for success, failure, and debug runs. |
| Check-only mode | `make -f Makefile.host check-mode-test` | Checks that `--check` validates source without executing it. |

On Windows, use a POSIX-compatible `make` environment or WSL. A host compiler
such as LLVM-MinGW Clang may be passed through `HOST_CC`. Amiga builds that
complete on the host prove compilation and linking only; run the resulting
Hunk executable under an Amiga emulator or on hardware for execution evidence.

## Common script examples

```text
build/host/pythonami examples/hello.py
build/host/pythonami examples/fibonacci.py
build/host/pythonami --check examples/test_features.py
build/host/pythonami --debug -c 'print("hello from Python68K")'
```

Use [`examples/`](../examples/) for runnable samples. The language intentionally
does not implement full CPython; consult the unsupported-feature list in
[`language-reference.md`](language-reference.md) before porting a script.

The language-level command and builtin inventory is maintained in
[`language-reference.md`](language-reference.md), including `print`, `input`,
collection constructors, conversions, text and file operations, time
functions, imports, process calls, and the Amiga-only extension loader. The
user-facing command list in this guide is limited to the `pythonami` CLI and
repository build/test commands.

## Platform notes

Host file APIs use stdio and host environment variables. Amiga builds use
AmigaDOS file handles and assigns, and only Amiga provides `load_library()` for
`*.py68k` extensions. `os.system(command)` runs synchronously and
`os.popen(command)` returns captured combined output; subprocess handles,
timeouts, and asynchronous process APIs are not supported.