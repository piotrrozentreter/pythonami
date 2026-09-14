# Language Reference

Python68K is a restricted Python-compatible language. Language Levels **0.1** (core), **0.2.0** (file/env I/O), **0.3** (types and limited attributes), **0.4** (exceptions and `with`), **0.5** (import/modules), and **0.6** (comprehensions) are executable on the host and Amiga builds.

Symbol analysis classifies names using local, module-global, builtin, and undefined lookup order; parameters occupy the first local slots and later assignment targets use deterministic source order. Referencing a local before assignment is a runtime `NameError`. Empty strings, lists, tuples, dicts, and sets are falsy. `input([prompt])` writes an optional prompt, reads one line, and returns it without the trailing newline; EOF raises an I/O error.

## Values

- Scalars: signed 32-bit `int` (checked overflow), `bool`, `None`, IEEE-754 binary32 `float` (no NaN/Inf; no 68881; integer-only software in `src/float.c`).
- `str`: 8-bit strings (not Unicode).
- `list`, `tuple` (parenthesized only: `()`, `(a,)`, `(a, b)`), `dict`, `set`.
- Hashable keys: `None`, `bool`, `int`, `str`, and tuples of hashable items. Lists, dicts, sets, files, functions, and modules are unhashable.
- Cyclic list/dict insertion is rejected (`ValueError`).
- `/` is true division and yields `float`. `//` is integer floor division. Mixed int/float arithmetic promotes to float.

## Operators and comparisons

Arithmetic `+ - * / // %`, unary `+ - not`, comparisons `== != < <= > >=` (bool compares as 0/1; `None == None`; strings/lists/tuples compare when types match). Short-circuit `and` / `or` are value-preserving.

## Control flow

- `if` / `elif` / `else`
- `while` … `else`, `for … in iterable` … `else` (`range`, list, tuple, dict keys, set)
- List, set, and dict comprehensions: `[elt for x in iterable if cond]`, nested `for`, `{elt for ...}`, `{k: v for ...}`. Loop targets bind in the enclosing function or module, matching `for` (D-0026). Generator expressions are not supported.
- `break` / `continue` / `return` / `pass`
- `try` / `except` / `except TypeError` / `except TypeError as e` / `finally`
- `raise` and `raise TypeError("msg")`
- `with EXPR as NAME` (file handles from `fopen` implement `__enter__` / `__exit__`)

Catchable runtime kinds: `TypeError`, `ValueError`, `IndexError`, `KeyError`, `ZeroDivisionError`, `OverflowError`, `NameError`, `IOError`, `RecursionError`, `ImportError`. Token, syntax, bytecode, memory, and internal errors are not catchable. Matching is by kind name, not a class hierarchy.

## Attributes

Limited attribute access: `obj.name` loads a bound method from a per-type table, or a module export. Not a user object system. `list.append` / `list.pop` exist alongside `list_append` / `list_pop`. Dict: `get`, `keys`, `values`, `items`, `pop`. Set: `add`, `remove`, `discard`. Strings: ASCII/8-bit methods including case (`upper`/`lower`/`capitalize`/`swapcase`/`title`/`casefold`), search (`find`/`rfind`/`index`/`rindex`/`count`/`startswith`/`endswith`), trim (`strip`/`lstrip`/`rstrip`/`removeprefix`/`removesuffix`), split/join (`split`/`rsplit`/`splitlines`/`partition`/`rpartition`/`join`), `replace`, align (`center`/`ljust`/`rjust`/`zfill`), `expandtabs`, `translate`, and classifiers (`isalnum`…`isupper`). Methods are positional-only (no kwargs).

## Imports (0.5)

- `import name`, `import name as alias`
- `from name import a, b`, `from name import a as b`
- Search: directory of the importing source, then entries in `sys.path` (starts with `.`)
- A successfully loaded module is cached and its top-level code runs once per runtime.
- A module that is currently loading is rejected with `ImportError: import cycle detected`.
- Failed imports are removed from the cache; their partial globals are not published.
- No relative imports, no `from x import *`, no multi-level packages

`sys` is a builtin module: `sys.path` (list), `sys.modules`, `sys.argv`.

## Builtins

`print`, `input`, `len`, `range`, `list`, `tuple`, `dict`, `set`, `list_pop`, `list_append`, `int`, `float`, `str`, `bool`, `abs`, `min`, `max`, `ord`, `chr`, `repr`, `ascii`, `all`, `any`, `format`, `maketrans`, `exit`, plus 0.2.0 file builtins `fopen`/`fclose`/`fread`/`freadline`/`fwrite`/`exists`/`remove`/`rename` (modes `r`/`w`/`a`/`rb`/`wb`/`ab`). Host: `getenv`/`setenv`/`unsetenv`. Amiga: `assign_get`/`assign_add`/`assign_remove`.

`ord`/`chr` operate on one byte (`0..255`). `format` supports a minimal int subset (`''`, `d`, width, zero-pad such as `04d`). `maketrans` builds a translation `dict` for `str.translate`. `ascii` escapes bytes `>= 128` as `\xHH`.

## Still not implemented

Classes and instances, Unicode, bytes/bytearray/`encode`, generator expressions, closures, nested `def`, async, `match`, `*args`/`**kwargs`, relative imports, `from x import *`, AmigaDOS `ENV:` GetVar/SetVar, file seek, encodings, full `str.format`/`format_map`, `eval`/`exec`/`compile`, general iterator protocol builtins (`iter`/`next`/`enumerate`/`reversed`/`sorted`), and Language Level freeze after owner emulator/hardware verification.
