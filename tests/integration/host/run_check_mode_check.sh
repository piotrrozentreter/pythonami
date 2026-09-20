#!/bin/sh

set -eu

pythonami=$1
tmpdir=${TMPDIR:-/tmp}/pythonami-check-mode-$$
trap 'rm -rf "$tmpdir"' EXIT
mkdir "$tmpdir"

set +e
"$pythonami" --check examples/hello.py >"$tmpdir/valid.out" 2>"$tmpdir/valid.err"
valid_status=$?
"$pythonami" --check -c 'if:' >"$tmpdir/invalid.out" 2>"$tmpdir/invalid.err"
invalid_status=$?
set -e

test "$valid_status" -eq 0
test ! -s "$tmpdir/valid.out"
test ! -s "$tmpdir/valid.err"
test "$invalid_status" -eq 10
test ! -s "$tmpdir/invalid.out"
grep '^SyntaxError:' "$tmpdir/invalid.err" >/dev/null

echo "PASS: --check CLI"