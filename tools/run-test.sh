#!/bin/sh
# usage: run-test.sh <binary> <script> <outdir> <frames> <dumps>
# Runs the game deterministically, writing framebuffer dumps and hashes.
# The test always uses the "original" graphics set and the default sound set
# (jorito), whatever other sets are installed: it runs on a data directory
# that only links those.
set -e
BIN=$(readlink -f "$1"); SCRIPT=$(readlink -f "$2"); OUT=$3; FRAMES=$4; DUMPS=$5
ROOT=$(readlink -f "$(dirname "$(readlink -f "$0")")/..")
mkdir -p "$OUT"; OUT=$(readlink -f "$OUT"); rm -f "$OUT"/frame_*
TMPHOME=$(mktemp -d)
TESTDATA=$(mktemp -d)
mkdir "$TESTDATA/graphics"
ln -s "$ROOT/data/graphics/original" "$TESTDATA/graphics/original"
ln -s "$ROOT/data/sound" "$TESTDATA/sound"
ln -s "$ROOT/data/rooms" "$TESTDATA/rooms"
cd "$TESTDATA"
HOME=$TMPHOME MOG_SETTINGS=$TMPHOME MOG_DATA=$TESTDATA \
MOG_TEST_FRAMES=$FRAMES MOG_TEST_DUMPS=$DUMPS MOG_TEST_OUT=$OUT MOG_TEST_SCRIPT=$SCRIPT \
	"$BIN" --window > "$OUT/stdout.log" 2>&1
rm -rf "$TMPHOME" "$TESTDATA"
