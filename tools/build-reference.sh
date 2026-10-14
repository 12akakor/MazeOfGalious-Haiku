#!/bin/sh
# Builds the upstream SDL 1.2 reference (git tag reference-sdl) as ./mog-ref.
# Needs libsdl1.2-dev, libsdl-image1.2-dev, libsdl-mixer1.2-dev, libsdl-sound1.2-dev.
set -e
ROOT=$(dirname "$(readlink -f "$0")")/..
TMP=$(mktemp -d)
git -C "$ROOT" worktree add -q "$TMP/ref" reference-sdl
make -C "$TMP/ref" -f Makefile.linux ref -j2
cp "$TMP/ref/mog-ref" "$ROOT/mog-ref"
git -C "$ROOT" worktree remove --force "$TMP/ref"
rm -rf "$TMP"
echo "built $ROOT/mog-ref"
