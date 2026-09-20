#!/bin/sh
# Run a pythonami script and diff stdout against an expected fixture.
# Usage: run_language_check.sh <pythonami> <script.py> <expected.txt>

set -eu

pythonami=$1
script=$2
expected=$3
shift 3

actual=$(mktemp)
expected_normalized=$(mktemp)
actual_normalized=$(mktemp)
trap 'rm -f "$actual" "$expected_normalized" "$actual_normalized"' EXIT

"$pythonami" "$script" "$@" >"$actual"
tr -d '\r' <"$expected" >"$expected_normalized"
tr -d '\r' <"$actual" >"$actual_normalized"
diff -u "$expected_normalized" "$actual_normalized"
echo "PASS: $script"
