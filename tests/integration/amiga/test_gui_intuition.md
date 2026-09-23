# Amiga gui_intuition LoadSeg fixture

Owner-run on emulator or hardware after building:

```text
make amiga
make amiga-ext
```

Place `pythonami` and `ext/gui_intuition/gui_intuition.py68k` so the path in
the script resolves (or edit the path). From that drawer:

```text
pythonami tests/integration/amiga/test_gui_intuition.py >T:py68k-gui-out
echo $RC
type T:py68k-gui-out
```

**Smoke (non-interactive):** the script calls `init`, `begin_window`, adds a
label+button, `show`, then immediately `close_window` + `shutdown`. It does
not wait for user input. Expected `$RC` 0 and stdout:

```text
init_ok
shown
closed
```

**Interactive modal (optional):** see comments in the `.py` file for a
`gui_dialog.run_modal` path when verifying gadgets by hand.

Intuition execution is **owner-verified** only. Musashi cannot run Kickstart
Intuition. Compile/link of the plugin is verified with `make amiga-ext`.

See also `docs/amiga-extensions.md` and HAS `docs/GUI_PYTHONAMI_API.md`.
