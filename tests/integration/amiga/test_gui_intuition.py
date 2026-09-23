# gui_intuition Amiga smoke fixture (owner-run)
# Build: make amiga && make amiga-ext
# Opens a Workbench window briefly, then closes it (no event wait).

gui = load_library("PROGDIR:ext/gui_intuition/gui_intuition.py68k")
status = gui.init()
if status != 0:
    print("init_fail")
    exit(1)
print("init_ok")

status = gui.begin_window("Py68K GUI", 40, 30, 280, 100, 0x0020026C, 0x0000100E)
if status != 0:
    print("begin_fail")
    gui.shutdown()
    exit(1)

gui.add_label(1, 12, 24, "Hello")
gui.add_button(2, 100, 50, 80, 18, "OK")
win = gui.show()
if win == 0 or win == -1:
    print("show_fail")
    gui.shutdown()
    exit(1)
print("shown")

gui.close_window()
gui.shutdown()
print("closed")
