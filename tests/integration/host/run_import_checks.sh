#!/bin/sh
set -eu

pythonami=$1
root=$2
import_dir="$root/tests/language/import"

check_failure() {
    script=$1
    expected_status=$2
    expected_text=$3
    output=$(mktemp)
    errors=$(mktemp)
    trap 'rm -f "$output" "$errors"' EXIT
    set +e
    "$pythonami" "$import_dir/$script" >"$output" 2>"$errors"
    status=$?
    set -e
    if [ "$status" -ne "$expected_status" ]; then
        echo "unexpected exit status for $script: $status" >&2
        exit 1
    fi
    if ! grep -F "$expected_text" "$errors" >/dev/null; then
        echo "missing diagnostic for $script" >&2
        cat "$errors" >&2
        exit 1
    fi
    if [ -s "$output" ]; then
        echo "unexpected stdout for $script" >&2
        cat "$output" >&2
        exit 1
    fi
    rm -f "$output" "$errors"
    trap - EXIT
    echo "PASS: $script (status $status)"
}

check_failure test_import_errors.py 11 "module not found"
check_failure test_import_syntax_error.py 10 "SyntaxError"
check_failure test_import_runtime_error.py 11 "division by zero"
check_failure test_import_cycle.py 11 "import cycle detected"