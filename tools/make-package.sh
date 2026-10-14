#!/bin/sh
# Builds the Haiku/RenkuOS package (run on Haiku/RenkuOS after "make").
# Layout:
#   apps/MazeOfGalious/MazeOfGalious        executable
#   apps/MazeOfGalious/{graphics,sound,rooms}  game data (found next to the binary)
#   apps/MazeOfGalious/docs/                licence and upstream read-me files
#   data/deskbar/menu/Applications/Maze of Galious  -> Deskbar entry
set -e
ROOT=$(dirname "$(readlink -f "$0")")/..
cd "$ROOT"
VERSION=0.63.1-1
PKG=obj/pkg
rm -rf "$PKG"
mkdir -p "$PKG/apps/MazeOfGalious/docs" "$PKG/data/deskbar/menu/Applications"
cp MazeOfGalious "$PKG/apps/MazeOfGalious/"
cp -r data/graphics data/sound data/rooms "$PKG/apps/MazeOfGalious/"
cp LICENSE readme.txt leeme.txt MoG-FAQ.txt src/third_party/LICENSE.stb_vorbis.txt \
	"$PKG/apps/MazeOfGalious/docs/"
ln -s "../../../../apps/MazeOfGalious/MazeOfGalious" \
	"$PKG/data/deskbar/menu/Applications/Maze of Galious"
sed "s/@VERSION@/$VERSION/g" build/haiku/PackageInfo.in > "$PKG/.PackageInfo"
rm -f "mazeofgalious-$VERSION-x86_64.hpkg"
package create -C "$PKG" "mazeofgalious-$VERSION-x86_64.hpkg"
echo "created mazeofgalious-$VERSION-x86_64.hpkg"
echo "install with: pkgman install ./mazeofgalious-$VERSION-x86_64.hpkg"
