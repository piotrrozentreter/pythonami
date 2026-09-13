# Language Reference

Language Level 0.1 syntax and semantics are specified by the project brief. The parser accepts the documented expression and statement forms at the AST boundary. Symbol analysis classifies names using local, module-global, builtin, and undefined lookup order; parameters occupy the first local slots and later assignment targets use deterministic source order.

Currently executable: scalar integer/boolean/None values and arithmetic (`+ - * // %` with checked overflow and Python-compatible floor division/modulo for negative operands), comparisons, `if`/`elif`/`else`, `while` and `for ... in range(...)` loops with `break`/`continue`, list literals/indexing/assignment, augmented assignment (`+= -= *= //= %=`) to locals and globals, `def` function statements with parameters, locals, recursion, `return` (explicit and implicit `None`), and the builtins `print`, `len`, `range`, `list_pop`, and `list_append`. Referencing a local variable before it has been assigned within a function is a runtime error rather than yielding `None`.

Not yet implemented: `and`/`or` short-circuit logic, string concatenation/indexing/slicing, the `int`/`str`/`bool`/`abs`/`min`/`max`/`exit` builtins, and tracebacks.
