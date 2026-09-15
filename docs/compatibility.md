# Compatibility

Python68K is a restricted Python-compatible language, not CPython. Language Levels 0.1–0.6 implement the documented subset in `docs/language-reference.md`. No CPython `.pyc` compatibility is claimed. Differential behavior against desktop Python is intended only for the documented subset (floor division, bool/int equality, true divide to float, exception kind names, sibling-module imports, comprehension target leakage matching `for` rather than Python 3 nested scopes).

Import compatibility is limited to single-level `.py` modules. The importing
script directory has precedence over `sys.path`, and `sys.path` is searched in
list order. Host paths use `/`; the platform path abstraction also accepts
Amiga-style `:` and `\\` separators when supplied by a script. Packages,
dotted names, relative imports, and native ABI **imports** are not supported.

On Amiga only, `load_library(path)` loads a relocatable `*.py68k` LoadSeg plugin
and returns a module of native exports (D-0027). This is not an `import` path
and is unavailable on the host build. See `docs/amiga-extensions.md`.

Command execution is currently limited to synchronous `os.system(command)`.
The command must be a non-empty string without an embedded NUL, is executed
with inherited standard handles, and returns the platform command status
directly. `subprocess`, output capture, per-child directories/environments,
timeouts, and `Popen` are not implemented yet.
