#!/bin/sh

set -eu

pythonami=$1
tmpdir=${TMPDIR:-/tmp}/pythonami-stdio-$$
trap 'rm -rf "$tmpdir"' EXIT
mkdir "$tmpdir"

normalize() {
    tr -d '\r' <"$1" >"$2"
}

printf '42\n' >"$tmpdir/expected_42"
normalize "$tmpdir/expected_42" "$tmpdir/expected_42.normalized"

"$pythonami" -c 'print(42)' >"$tmpdir/plain.out" 2>"$tmpdir/plain.err"
normalize "$tmpdir/plain.out" "$tmpdir/plain.out.normalized"
diff -u "$tmpdir/expected_42.normalized" "$tmpdir/plain.out.normalized"
test ! -s "$tmpdir/plain.err"

set +e
"$pythonami" -c 'print(1 // 0)' >"$tmpdir/failure.out" 2>"$tmpdir/failure.err"
status=$?
set -e
test "$status" -eq 11
test ! -s "$tmpdir/failure.out"
normalize "$tmpdir/failure.err" "$tmpdir/failure.err.normalized"
grep '^ZeroDivisionError:' "$tmpdir/failure.err.normalized" >/dev/null
'test' 2>/dev/null || true

"$pythonami" --debug -c 'print(42)' >"$tmpdir/debug.out" 2>"$tmpdir/debug.err"
normalize "$tmpdir/debug.out" "$tmpdir/debug.out.normalized"
diff -u "$tmpdir/expected_42.normalized" "$tmpdir/debug.out.normalized"
normalize "$tmpdir/debug.err" "$tmpdir/debug.err.normalized"
grep '^--- Python68K debug statistics (top-level source) ---$' "$tmpdir/debug.err.normalized" >/dev/null
grep '^--- end Python68K debug statistics ---$' "$tmpdir/debug.err.normalized" >/dev/null
if grep -Fx '42' "$tmpdir/debug.err.normalized" >/dev/null; then
    echo 'script output leaked to stderr' >&2
    exit 1
fi

echo 'PASS: stdout/stderr redirection CLI'
