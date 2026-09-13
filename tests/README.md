# Tests

Phase-specific unit, language, negative, memory, differential, integration, and fixture directories live here.

- `tests/unit/` — host GCC C unit tests (allocator through builtins).
- `tests/language/` — advanced Language Level 0.1 scripts executed by `pythonami`, checked against `tests/fixtures/language/*.expected.txt` via `make language-test`.
- `tests/fixtures/` — expected stdout for language/example scripts (`print_values`, `test_features`, language suites).
- `tests/integration/host/run_language_check.sh` — runs a script and `diff`s stdout to its fixture.
- `tests/negative/`, `tests/differential/`, `tests/integration/amiga/` — reserved for rejected-input, CPython-diff, and Amiga emulator checks.
