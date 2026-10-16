#!/bin/sh
# Builds the Haiku/RenkuOS package (run on Haiku/RenkuOS after "make").
# Layout (Haiku conventions):
#   apps/MazeOfGalious                               executable
#   data/MazeOfGalious/{graphics,sound,rooms}        game data
#   data/deskbar/menu/Applications/Maze of Galious   Deskbar entry (symlink)
#   documentation/packages/mazeofgalious/            licence and read-me files
# The executable finds its data through find_directory(B_SYSTEM_DATA_DIRECTORY).
set -e
ROOT=$(dirname "$(readlink -f "$0")")/..
cd "$ROOT"
VERSION=0.63.3-3
PKG=obj/pkg
PACKAGE=${PKGTOOL:-package}
rm -rf "$PKG"
mkdir -p "$PKG/apps" "$PKG/data/MazeOfGalious" \
	"$PKG/data/deskbar/menu/Applications" \
	"$PKG/documentation/packages/mazeofgalious"
cp MazeOfGalious "$PKG/apps/MazeOfGalious"
chmod 755 "$PKG/apps/MazeOfGalious"
cp -r data/graphics data/sound data/rooms "$PKG/data/MazeOfGalious/"
cp LICENSE readme.txt leeme.txt MoG-FAQ.txt src/third_party/LICENSE.stb_vorbis.txt \
	"$PKG/documentation/packages/mazeofgalious/"
ln -s "../../../../apps/MazeOfGalious" \
	"$PKG/data/deskbar/menu/Applications/Maze of Galious"
sed "s/@VERSION@/$VERSION/g" build/haiku/PackageInfo.in > "$PKG/.PackageInfo"
rm -f mazeofgalious-*-x86_64.hpkg
"$PACKAGE" create -C "$PKG" "mazeofgalious-$VERSION-x86_64.hpkg"
echo "created mazeofgalious-$VERSION-x86_64.hpkg"
echo "install with: pkgman install ./mazeofgalious-$VERSION-x86_64.hpkg"

# Extra graphic and sound sets: a separate package. packagefs merges its
# data/MazeOfGalious/ with the one of the main package.
EXTRAS=obj/pkg-extras
rm -rf "$EXTRAS"
mkdir -p "$EXTRAS/data/MazeOfGalious"
cp -r data-extras/graphics data-extras/sound "$EXTRAS/data/MazeOfGalious/"
sed "s/@VERSION@/$VERSION/g" build/haiku/PackageInfo-extras.in > "$EXTRAS/.PackageInfo"
rm -f mazeofgalious_extras-*-any.hpkg
"$PACKAGE" create -C "$EXTRAS" "mazeofgalious_extras-$VERSION-any.hpkg"
echo "created mazeofgalious_extras-$VERSION-any.hpkg (optional extra sets)"
