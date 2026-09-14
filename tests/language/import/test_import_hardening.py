import sys
sys.path.append("tests/language/import/path_second")
sys.path.append("tests/language/import/path_first")
sys.path.append("tests/language/import/path_shadow")
import import_cache_once
import import_cache_once as cached_alias
from import_cache_once import value as cached_value
import import_script_precedence
import import_path_order
print(import_cache_once.value)
print(cached_alias.value)
print(cached_value)
print(import_script_precedence.value)
print(import_path_order.value)
print(sys.argv)