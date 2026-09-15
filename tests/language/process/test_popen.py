import os

output = os.popen("echo PY68K_POPEN_OK")
print(output.strip())

try:
    os.popen(7)
except TypeError:
    print("type-error")
