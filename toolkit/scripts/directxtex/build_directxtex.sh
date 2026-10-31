#!/bin/bash
# Builds portable texconv and texassemble for Linux. Normally run by build.sh
# inside the build container.
#
# The tools come from Texconv-Custom-DLL (Unix ports of DirectXTex's texconv
# and texassemble), built against the DirectXTex release the Windows binaries
# in bin/win32 are from, with patches/*.patch applied to Texconv-Custom-DLL and
# patches/directxtex/*.patch to DirectXTex. libjpeg-turbo, libpng,
# zlib and OpenEXR are built and linked statically by Texconv-Custom-DLL's CMake
# (fetched from GitHub during the build), libstdc++, libgcc and libgomp are
# linked statically here. Only glibc stays dynamic.
#
# Expects these source trees (checked out by build.sh):
#   $SRC_ROOT/texconv-custom-dll   matyalatte/Texconv-Custom-DLL, with submodules
#   $SRC_ROOT/directxtex           microsoft/DirectXTex
#
# Usage: build_directxtex.sh <src root> <build root> <output dir>
set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
SRC_ROOT=$(realpath "$1")
BUILD_ROOT=$(realpath -m "$2")
OUTPUT_DIR=$(realpath -m "$3")
JOBS=$(nproc)
export CC=${CC:-gcc}
export CXX=${CXX:-g++}

WORK=$BUILD_ROOT/work
rm -rf "$WORK"
mkdir -p "$WORK"
cp -a "$SRC_ROOT/texconv-custom-dll" "$WORK/texconv-custom-dll"
SRC=$WORK/texconv-custom-dll

# use the DirectXTex release of the Windows binaries instead of the fork's submodule
rm -rf "$SRC/DirectXTex"
cp -a "$SRC_ROOT/directxtex" "$SRC/DirectXTex"

for p in "$HERE"/patches/*.patch; do
    echo "applying $(basename "$p")"
    patch -d "$SRC" -p1 --forward --batch --quiet -i "$p"
done
# git apply follows DirectXTex's .gitattributes, its sources are checked out with CRLF
for p in "$HERE"/patches/directxtex/*.patch; do
    echo "applying directxtex/$(basename "$p")"
    git -C "$SRC/DirectXTex" apply "$p"
done

# DirectXTex uses OpenMP for the CPU BC6H/BC7 codecs, link GCC's runtime statically
GOMP=$("$CC" -print-file-name=libgomp.a)
if [ ! -f "$GOMP" ]; then
    echo "static libgomp not found" >&2
    exit 1
fi

cmake -S "$SRC" -B "$BUILD_ROOT/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DTEXCONV_BUILD_AS_EXE=ON \
    -DTEXCONV_USE_TEXASSEMBLE=ON \
    -DTEXCONV_USE_STATIC_LINK=ON \
    -DENABLE_LIBJPEG_SUPPORT=ON \
    -DENABLE_LIBPNG_SUPPORT=ON \
    -DENABLE_OPENEXR_SUPPORT=ON \
    -DOpenMP_gomp_LIBRARY="$GOMP" \
    "-DCMAKE_EXE_LINKER_FLAGS=-static-libstdc++ -static-libgcc"
cmake --build "$BUILD_ROOT/build" --target texconv texassemble -j "$JOBS"

mkdir -p "$OUTPUT_DIR"
for tool in texconv texassemble; do
    cp "$BUILD_ROOT/build/bin/$tool" "$OUTPUT_DIR/$tool"
    strip "$OUTPUT_DIR/$tool"
done
echo "built $OUTPUT_DIR/texconv $OUTPUT_DIR/texassemble"
