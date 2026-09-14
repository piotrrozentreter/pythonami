import sys
sys.path.append("tests/language/import/path_shadow")
sys.path.append("tests/language/import/path_second")
sys.path.append("tests/language/import/path_first")
import import_script_precedence
import import_path_order
print(import_script_precedence.value)
print(import_path_order.value)
print(sys.argv)