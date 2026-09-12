# Host Build

The Phase 0 host build uses GCC or a compatible C compiler with strict warnings:

```text
make debug
make test
make release
```

The executable is written to `build/host/pythonami`. Phase 0 supports `-V` and `--help` only; source execution is intentionally not implemented yet.
