import mod_a
from mod_a import name as alias
print(mod_a.value)
print(alias)
import mod_a as again
print(again.value)
