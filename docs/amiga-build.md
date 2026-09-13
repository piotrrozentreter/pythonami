# Amiga Build

```text
make amiga                 # release Hunk → ./pythonami
make amiga MODE=debug      # → ./pythonami-debug
```

Defaults (override as needed):

```text
VBCC=/home/piotr/local/vbcc
NDK=/run/media/piotr/BACKUP/Rozen/Programy/Amiga/NDK3.2
```

Example with explicit paths:

```text
make amiga VBCC=/home/piotr/local/vbcc NDK=/run/media/piotr/BACKUP/Rozen/Programy/Amiga/NDK3.2
```

`Makefile.amiga` uses vbcc through `vc`, targets `+aos68k`, and passes `-cpu=68000 -fpu=0`. Release and debug both keep `-use-framepointer -no-delayed-popping` to avoid 68000 stack-layout failures. The Amiga build uses the vbcc-native target tree for the C runtime, `startup.o`, and `vc.lib`. NDK 3.2 `Include_H` supplies Amiga system headers; its `lib/amiga.lib` is not mixed into the vbcc link. Emulator and real-hardware execution remain owner-verified.

AmigaDOS console output uses `Output()` for standard output and `ErrorOutput()` for standard error, then writes raw length-delimited data with `Write()`. This preserves CLI redirection and avoids hosted `stdio` assumptions. `platform/amiga/amiga_compat.h` contains only the minimal ABI declarations needed by the platform layer.

## Workbench startup

The vbcc `+aos68k` configuration links its own `startup.o`, which handles the AmigaOS `WBenchMsg` handshake and normal process exit. Do not add the sibling assembler project's `wbstartup.s` to this `vc` link: that wrapper is for programs with a custom assembly entry point and would risk receiving or replying to the Workbench message twice. Rebuild `pythonami` before retesting a Workbench launch.
