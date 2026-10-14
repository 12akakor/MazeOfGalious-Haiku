#!/bin/sh
# usage: check-golden.sh <binary> [scenario]
# Runs a scripted scenario and compares every frame hash with the golden reference.
set -e
ROOT=$(dirname "$(readlink -f "$0")")/..
BIN=$1; SC=${2:-basic}
OUT=$(mktemp -d)
sh "$ROOT/tools/run-test.sh" "$BIN" "$ROOT/tests/scripts/$SC.txt" "$OUT" 1400 ""
if cmp -s "$OUT/hashes.txt" "$ROOT/tests/golden/$SC/hashes.txt"; then
	echo "PASS $SC: all $(wc -l < "$OUT/hashes.txt") frames identical"
	rm -rf "$OUT"
else
	echo "FAIL $SC: first difference:"
	diff "$OUT/hashes.txt" "$ROOT/tests/golden/$SC/hashes.txt" | head -4
	echo "(output kept in $OUT)"
	exit 1
fi
