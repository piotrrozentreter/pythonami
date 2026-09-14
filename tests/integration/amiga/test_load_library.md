# Amiga LoadSeg library fixture

Owner-run on emulator or hardware after building:

```text
make amiga
make amiga-ext
```

Copy `pythonami` and `ext/demo_add/demo_add.py68k` into the same drawer (or
adjust the path in the script). From that drawer:

```text
pythonami tests/integration/amiga/test_load_library.py >T:py68k-ext-out
echo $RC
type T:py68k-ext-out
```

Or with an absolute/assign path, edit the script’s `load_library(...)` argument
to match (e.g. `PROGDIR:demo_add.py68k` if the `.py68k` sits beside the binary).

Expected: `$RC` is `0` and stdout is:

```text
5
6
True
```

(`add(2,3)`, `mul(2,3)`, and a type check).

See also `docs/amiga-extensions.md`.
