#!/bin/sh
# Run a pythonami script and diff stdout against an expected fixture.
# Usage: run_language_check.sh <pythonami> <script.py> <expected.txt>

set -eu

pythonami=$1
script=$2
expected=$3
actual=${4:-}

if [ -z "$actual" ]; then
  actual=$(mktemp)
  trap 'rm -f "$actual"' EXIT
fi

"$pythonami" "$script" >"$actual"
diff -u "$expected" "$actual"
echo "PASS: $script"
