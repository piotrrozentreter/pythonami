import os

print(os.system("echo PY68K_SYSTEM_OK"))
import os
print(os.system("echo PY68K_SYSTEM_AGAIN"))

try:
    os.system(7)
except TypeError:
    print("type-error")
