#!/bin/sh
# usage: run-test.sh <binary> <script> <outdir> <frames> <dumps>
# Runs the game headless and deterministically, writing framebuffer dumps.
set -e
BIN=$(readlink -f "$1"); SCRIPT=$(readlink -f "$2"); OUT=$3; FRAMES=$4; DUMPS=$5
ROOT=$(dirname "$(readlink -f "$0")")/..
mkdir -p "$OUT"; OUT=$(readlink -f "$OUT"); rm -f "$OUT"/frame_*
TMPHOME=$(mktemp -d)
cd "$ROOT/data"
HOME=$TMPHOME MOG_SETTINGS=$TMPHOME MOG_DATA=$ROOT/data SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
MOG_TEST_FRAMES=$FRAMES MOG_TEST_DUMPS=$DUMPS MOG_TEST_OUT=$OUT MOG_TEST_SCRIPT=$SCRIPT \
	"$BIN" --window > "$OUT/stdout.log" 2>&1
rm -rf "$TMPHOME"
