path = "build/host/lang_with.tmp"
with fopen(path, "w") as f:
    fwrite(f, "hello")
print(exists(path))
with fopen(path, "r") as f:
    print(fread(f, 5))
remove(path)
print(exists(path))
