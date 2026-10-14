#!/bin/sh
# Compile check of the native build with a Haiku x86_64 cross compiler
# (no linking; the Haiku libraries are not available on the build host).
#
#   HAIKU_CROSS=/path/to/cross-tools-x86_64/bin/x86_64-unknown-haiku
#   HAIKU_HEADERS=/path/to/haiku-or-renkuos-source/headers
#   tools/cross-check.sh
#
# The cross compiler is built with the Haiku/RenkuOS source tree:
#   configure --cross-tools-source ../buildtools --build-cross-tools x86_64
set -e
ROOT=$(dirname "$(readlink -f "$0")")/..
: "${HAIKU_CROSS:?set HAIKU_CROSS}" "${HAIKU_HEADERS:?set HAIKU_HEADERS}"
H=$HAIKU_HEADERS
INC="-I$H -I$H/os -I$H/os/app -I$H/os/interface -I$H/os/kernel -I$H/os/storage \
-I$H/os/support -I$H/os/media -I$H/os/game -I$H/os/drivers -I$H/os/add-ons/graphics \
-I$H/posix -I$H/glibc"
OUT=$(mktemp -d)
cd "$ROOT"
n=0
for f in src/*.cpp src/platform/audio_mixer.cpp src/platform/platform_common.cpp \
		src/platform/video_scaler.cpp src/platform/keynames.cpp \
		src/platform/platform_haiku.cpp; do
	"$HAIKU_CROSS-g++" -std=gnu++17 -O2 -w -Wno-write-strings -Isrc $(echo $INC | sed 's/-I/-idirafter /g') \
		-c "$f" -o "$OUT/$(basename "$f" .cpp).o"
	n=$((n + 1))
done
# Test variant (make test): same sources with the deterministic test hooks.
for f in src/*.cpp src/platform/audio_mixer.cpp src/platform/platform_common.cpp \
		src/platform/video_scaler.cpp src/platform/keynames.cpp \
		src/platform/platform_haiku.cpp src/test/testhooks.cpp; do
	"$HAIKU_CROSS-g++" -std=gnu++17 -O2 -w -Wno-write-strings -Isrc $(echo $INC | sed 's/-I/-idirafter /g') \
		-DMOG_TESTHOOKS -include src/test/testhooks.h -c "$f" -o "$OUT/test_$(basename "$f" .cpp).o"
	n=$((n + 1))
done
"$HAIKU_CROSS-gcc" -O2 -w $(echo $INC | sed 's/-I/-idirafter /g') -c src/third_party/stb_vorbis.c -o "$OUT/stb_vorbis.o"
rm -rf "$OUT"
echo "OK: $((n + 1)) compilations for Haiku x86_64 (normal and test variant)"
