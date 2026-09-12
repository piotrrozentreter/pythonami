# Amiga Build

The checked-in `Makefile.amiga` uses vbcc through `vc`, targets `+aos68k`, and explicitly passes `-cpu=68000 -fpu=0` for both debug and release builds.

Release builds keep the same safe frame-pointer and no-delayed-popping flags as the debug run. This avoids release-only 68000 stack-layout failures that can surface as `80000008` Guru Meditation on real Amiga or emulated startup even when the debug build succeeds. The configured local frontend is `/home/piotr/local/vbcc/bin/vc`. The Amiga build uses the vbcc-native target tree at `/home/piotr/local/vbcc/targets/m68k-amigaos` for the C runtime, `startup.o`, and `vc.lib`. NDK 3.2 `Include_H` supplies missing Amiga system headers, but its `lib/amiga.lib` is not mixed into the vbcc link. Build with `make amiga-debug VBCC=/home/piotr/local/vbcc NDK=/run/media/piotr/BACKUP/Rozen/Programy/Amiga/NDK3.2` or the corresponding release target. Emulator and real-hardware execution remain unverified.

AmigaDOS console output uses `Output()` for standard output and `ErrorOutput()` for standard error, then writes raw length-delimited data with `Write()`. This preserves CLI redirection and avoids hosted `stdio` assumptions. `platform/amiga/amiga_compat.h` contains only the minimal ABI declarations needed by the platform layer; final linking requires the AmigaOS NDK and DOS startup configuration.

The native `print` builtin uses the same platform writer, so output is sent to the AmigaDOS CLI standard output handle and participates in normal shell redirection.

## Workbench startup

The vbcc `+aos68k` configuration links its own `startup.o`, which handles the AmigaOS `WBenchMsg` handshake and normal process exit. Do not add the sibling assembler project's `wbstartup.s` to this `vc` link: that wrapper is for programs with a custom assembly entry point and would risk receiving or replying to the Workbench message twice. Rebuild `pythonami` with the NDK path above before retesting a Workbench launch.
