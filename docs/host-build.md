# Host Build

```text
make host              # debug → build/host/pythonami
make host MODE=release # optimized host build
make test              # unit + language fixtures
make clean
```

Aliases `make debug` / `make release` still forward to `Makefile.host`. The executable is written to `build/host/pythonami`.
