# Host language fixture for file + environment builtins (0.2.0).
# Amiga uses assign_get/assign_add/assign_remove instead of getenv/setenv/unsetenv.
# Note: string escape decoding of \n in literals is not part of this suite;
# newline IO is covered by tests/unit/test_file_io.c and sample_lines.txt.

print("=== write/read text ===")
path = "build/host/lang_file_io.tmp"
f = fopen(path, "w")
fwrite(f, "hello")
fclose(f)
print(exists(path))
f = fopen(path, "r")
print(fread(f, 5))
fclose(f)

print("=== append ===")
f = fopen(path, "a")
fwrite(f, "WORLD")
fclose(f)
f = fopen(path, "r")
print(fread(f, 20))
fclose(f)

print("=== readline fixture ===")
f = fopen("tests/fixtures/sample_lines.txt", "r")
print(freadline(f))
print(freadline(f))
fclose(f)

print("=== binary ===")
bpath = "build/host/lang_file_io.bin"
f = fopen(bpath, "wb")
fwrite(f, "ABCDE")
fclose(f)
f = fopen(bpath, "rb")
data = fread(f, 5)
fclose(f)
print(len(data))
print(data[0])
print(data[4])

print("=== rename/remove ===")
npath = "build/host/lang_file_io_renamed.tmp"
rename(path, npath)
print(exists(path))
print(exists(npath))
remove(npath)
remove(bpath)
print(exists(npath))

print("=== env ===")
setenv("PY68K_LANG_ENV", "ok")
print(getenv("PY68K_LANG_ENV"))
unsetenv("PY68K_LANG_ENV")
print(getenv("PY68K_LANG_ENV"))

print("=== File IO Suite Complete ===")
