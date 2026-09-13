# Changelog

## 0.2.0

- File builtins: `fopen`/`fclose`/`fread`/`freadline`/`fwrite`/`exists`/`remove`/`rename`
  with modes `r`/`w`/`a`/`rb`/`wb`/`ab` (AmigaDOS `Open` on target; stdio on host).
- Console `input([prompt])` via stdin / AmigaDOS `Input()`.
- Host environment: `getenv`/`setenv`/`unsetenv`.
- Amiga DOS assigns: `assign_get`/`assign_add`/`assign_remove` (`AssignPath` /
  `AssignLock` / `Lock`+`NameFromLock`).
- `PY68_OBJECT_FILE` with close-on-release; host unit + language fixtures.
- Version string `Python68K 0.2.0`.

## 0.1.0

- Language Level 0.1 runtime: tokenize → parse → symbols → compile → verify → stack VM.
- Scalars, arithmetic, comparisons, control flow, lists/strings, functions, builtins.
- Host GCC build (`make host`) and Amiga vbcc build (`make amiga`).
- Host unit tests and language fixture suite (`make test`).
- Copyright preamble on all C/H sources: 2026 Piotr Rozentreter (Rozsoft).

## Unreleased
