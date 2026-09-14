# Amiga stdout/stderr redirection fixture

This is a manual integration specification for an AmigaDOS emulator or real Amiga hardware. It is not automatically run by `Makefile.host` or `Makefile.amiga`.

Run from the directory containing the release Hunk executable `pythonami` (use `pythonami-debug` for a debug build). AmigaDOS `$RC` is the process return code.

```text
pythonami -c "print(42)" >T:py68k-stdout 2>T:py68k-stderr
echo $RC
```

Expected: `$RC` is `0`; `T:py68k-stdout` contains exactly `42` followed by a newline; `T:py68k-stderr` is empty.

```text
pythonami -c "print(1 // 0)" >T:py68k-stdout 2>T:py68k-stderr
echo $RC
```

Expected: `$RC` is `11`; `T:py68k-stdout` is empty; `T:py68k-stderr` contains a line beginning with `ZeroDivisionError:`.

```text
pythonami --debug -c "print(42)" >T:py68k-stdout 2>T:py68k-stderr
echo $RC
```

Expected: `$RC` is `0`; `T:py68k-stdout` contains exactly `42` followed by a newline; `T:py68k-stderr` contains both `--- Python68K debug statistics (top-level source) ---` and `--- end Python68K debug statistics ---`, and does not contain a line consisting of `42`.

Inspect the three files after each command and remove them when finished:

```text
delete T:py68k-stdout
delete T:py68k-stderr
```

The host counterpart is `make -f Makefile.host stdio-redirection-test`.
