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

## CLI stdout/stderr redirection fixture

The host equivalent is run with `make -f Makefile.host stdio-redirection-test`. To verify the Amiga handles in an AmigaDOS emulator or on hardware, run the commands below from the directory containing `pythonami` (or replace the executable with `pythonami-debug` for a debug build). `$RC` is the AmigaDOS return code; remove the temporary files after checking them.

```text
pythonami -c "print(42)" >T:py68k-stdout 2>T:py68k-stderr
echo $RC
```

Expected: `$RC` is `0`, `T:py68k-stdout` contains `42` followed by a newline, and `T:py68k-stderr` is empty.

```text
pythonami -c "print(1 // 0)" >T:py68k-stdout 2>T:py68k-stderr
echo $RC
```

Expected: `$RC` is `11`, `T:py68k-stdout` is empty, and `T:py68k-stderr` contains the `ZeroDivisionError:` diagnostic.

```text
pythonami --debug -c "print(42)" >T:py68k-stdout 2>T:py68k-stderr
echo $RC
```

Expected: `$RC` is `0`, `T:py68k-stdout` contains only `42` followed by a newline, and `T:py68k-stderr` contains the debug-statistics header and footer but no line containing the script output `42`. These commands require an AmigaDOS emulator or Amiga hardware and are not automatically run by the host or Amiga make targets.

AmigaDOS console output uses `Output()` for standard output and a V36-safe error-stream helper for standard error, then writes raw length-delimited data with `Write()`. On dos.library V47+ the helper calls `ErrorOutput()`; on older Kickstarts it uses `pr_CES` when set and otherwise falls back to `Output()`. This preserves CLI redirection on AmigaOS 3.2 while remaining safe on AmigaOS 2.x–3.1, and avoids hosted `stdio` assumptions. `platform/amiga/amiga_compat.h` contains only the minimal ABI declarations needed by the platform layer.

Note: Amiga Shell `2>` stderr redirection and `SelectError` are also V47-era. On older shells, diagnostics and `--debug` statistics typically appear on the same console stream as script output unless `pr_CES` was already set by the caller.

## Workbench startup

The vbcc `+aos68k` configuration links its own `startup.o`, which handles the AmigaOS `WBenchMsg` handshake and normal process exit. Do not add the sibling assembler project's `wbstartup.s` to this `vc` link: that wrapper is for programs with a custom assembly entry point and would risk receiving or replying to the Workbench message twice. Rebuild `pythonami` before retesting a Workbench launch.
