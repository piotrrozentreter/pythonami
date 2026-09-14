import sys
sys.path.append("tests/language/import/path_second")
sys.path.append("tests/language/import/path_first")
import import_path_order
print(import_path_order.value)