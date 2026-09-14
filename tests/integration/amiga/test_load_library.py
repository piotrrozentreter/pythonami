# load_library Amiga fixture (owner-run)
# Requires demo_add.py68k beside the script or at the path below.
# Build: make amiga && make amiga-ext

lib = load_library("demo_add.py68k")
print(lib.add(2, 3))
print(lib.mul(2, 3))
print(lib.add(2, 3) == 5)
