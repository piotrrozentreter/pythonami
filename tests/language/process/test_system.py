import os

print(os.system("echo PY68K_SYSTEM_OK"))
import os
print(os.system("echo PY68K_SYSTEM_AGAIN"))
print(os.system("echo PY68K_SYSTEM_THIRD"))

try:
    os.system(7)
except TypeError:
    print("type-error")
