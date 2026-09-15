# Changelog

## Unreleased

- Fixed-count unpacking / multiple assignment: `a, b = (1, 2)`, `a, b = [1, 2]`,
  `a, b = 1, 2`, `for a, b in pairs:`, and string unpack `a, b = "ab"`.
  `OP_UNPACK` (0x0F). Length mismatch is `ValueError`; non-sequence is
  `TypeError`. Assignment RHS is an expression list (D-0038). Starred and
  nested unpacking remain unsupported. D-0015 still requires parentheses for
  tuples outside assignment (`return a, b` is still a syntax error).

- Identity operators `is` / `is not` (`OP_IS` / `OP_IS_NOT`). Immediate
  None/bool/int/float values compare by type and payload; heap objects compare
  by pointer (D-0037). `is not` is one comparison operator, not `is` plus unary
  `not`.
- Membership operators `in` / `not in` (`OP_CONTAINS` / `OP_NOT_CONTAINS`) for
  str substring, list/tuple equality scan, dict keys, and set members.
- `for` / comprehensions iterate strings as successive one-character strings.
- Example `examples/wordcount.py` fixed for Level APIs (`IOError`, `fread`).
- `fread` allocates from remaining file size (Seek/ftell), not the full count,
  so large “read all” counts work on Amiga without multi-GiB pre-allocation.

- Amiga LoadSeg extensions: `load_library(path)` loads `*.py68k` plugins with a
  public export ABI (`include/py68k_ext.h`); sample `ext/demo_add` (vbcc C +
  vasm) via `make amiga-ext` (D-0027).

## 0.6.0

- Language Level 0.6: list, set, and dict comprehensions with nested `for`
  clauses and `if` filters. Targets bind in the enclosing function or module
  scope, matching `for` (D-0026). Generator expressions remain rejected.
- Comprehension bytecode uses `OP_LIST_APPEND` / `OP_SET_ADD` / `OP_MAP_ADD`
  with existing `RANGE_INIT`/`RANGE_NEXT` loops; no nested functions.
- Amiga: avoid calling `ErrorOutput()` on dos.library < V47 (fixes `--debug` /
  stderr Guru on Kickstart 2.x–3.1); use `pr_CES` or `Output()` instead.
- String methods (ASCII/8-bit): case, search, trim, split/join, replace, align,
  expandtabs, translate, classifiers; bound via `attr.c`.
- Text builtins: `ord`, `chr`, `repr`, `ascii`, `all`, `any`, `format` (minimal
  int specs), `maketrans` (returns dict). Literal escapes `\\ \' \" \n \r \t \xHH`
  decoded at load. See D-0024 / D-0025.
- Version string `Python68K 0.6.0`.

## 0.5.0

- Language Level 0.3: tuple, dict, set, limited attributes, bound methods, value equality/hash, IEEE-754 binary32 float, `/` true divide vs `//` floor divide.
- Language Level 0.4: catchable exception objects, `try`/`except`/`finally`/`raise`, `with fopen(...) as f`.
- Language Level 0.5: `import` / `from` / `as`, module objects, loader cache, builtin `sys` (`path`, `modules`, `argv`).
- Targeted diagnostics for remaining unsupported keywords (`class`, `lambda`, …).
- Version string `Python68K 0.5.0`.

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
