# Host Build

Linux is the primary host development environment. The default compiler is
`gcc`; Clang is also supported through the `HOST_CC` override:

```text
make -f Makefile.host test
make -f Makefile.host test HOST_CC=clang
```

The host makefile remains POSIX-oriented and keeps the Amiga compiler setup
separate. Windows-specific environment handling is guarded in the host source;
it does not change the Linux build or the Amiga target.

```text
make host              # debug → build/host/pythonami
make host MODE=release # optimized host build
make test              # unit + language fixtures
make clean
```

Aliases `make debug` / `make release` still forward to `Makefile.host`. The executable is written to `build/host/pythonami`.

The host compiler is selected with `HOST_CC`; the Amiga compiler settings are
unchanged. On Windows with LLVM-MinGW installed by WinGet, use the compiler
path from the local installation, for example:

```powershell
$env:HOST_CC = "C:\Users\prozentreter\AppData\Local\Microsoft\WinGet\Packages\MartinStorsjo.LLVM-MinGW.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\llvm-mingw-20260616-ucrt-x86_64\bin\clang.exe"
make -f Makefile.host test HOST_CC=$env:HOST_CC
```

This is a host-only compiler override and does not affect `Makefile.amiga`.

Local development environment note (2026-09-14): LLVM-MinGW Clang 22.1.8 is
installed through WinGet for Windows host builds. Visual Studio 2026 is also
installed on that machine, but its `cl.exe` is not the configured host
compiler; use a Visual Studio Developer PowerShell before evaluating it.
