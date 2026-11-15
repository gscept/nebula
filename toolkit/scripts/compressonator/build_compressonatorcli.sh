#!/bin/bash
# Builds a self-contained compressonatorcli for Linux. Normally run by build.sh
# inside the build container.
#
# Everything except glibc is linked statically: OpenEXR/Imath, libtiff/zlib
# (built here), libstdc++ and libgcc. OpenCV and Qt are not used, Nebula only
# needs the encoders and the image loaders. The Compressonator sources are
# patched with patches/*.patch (OpenEXR 3, TIFF plugin, -OutputSRGB, fixes).
#
# Expects these source trees (checked out by build.sh):
#   $SRC_ROOT/compressonator           GPUOpen-Tools/compressonator
#   $SRC_ROOT/common/lib/ext/glm       g-truc/glm
#   $SRC_ROOT/common/lib/ext/rapidxml  discord/rapidxml
#   $SRC_ROOT/imath                    AcademySoftwareFoundation/Imath
#   $SRC_ROOT/openexr                  AcademySoftwareFoundation/openexr
#   $SRC_ROOT/zlib                     madler/zlib
#   $SRC_ROOT/libtiff                  libtiff/libtiff
#
# Usage: build_compressonatorcli.sh <src root> <build root> <output binary>
set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
SRC_ROOT=$(realpath "$1")
BUILD_ROOT=$(realpath -m "$2")
OUTPUT=$(realpath -m "$3")
PREFIX=$BUILD_ROOT/install
JOBS=$(nproc)
export CC=${CC:-gcc}
export CXX=${CXX:-g++}

mkdir -p "$BUILD_ROOT"

# --- static Imath / OpenEXR ---------------------------------------------------
cmake -S "$SRC_ROOT/imath" -B "$BUILD_ROOT/imath" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_INSTALL_LIBDIR=lib \
    -DBUILD_SHARED_LIBS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DBUILD_TESTING=OFF -DIMATH_INSTALL_PKG_CONFIG=ON
cmake --build "$BUILD_ROOT/imath" -j "$JOBS"
cmake --install "$BUILD_ROOT/imath"

cmake -S "$SRC_ROOT/openexr" -B "$BUILD_ROOT/openexr" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_PREFIX_PATH="$PREFIX" \
    -DBUILD_SHARED_LIBS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_TESTING=OFF \
    -DOPENEXR_BUILD_TOOLS=OFF -DOPENEXR_INSTALL_TOOLS=OFF -DOPENEXR_BUILD_EXAMPLES=OFF \
    -DOPENEXR_TEST_LIBRARIES=OFF -DOPENEXR_TEST_TOOLS=OFF -DOPENEXR_TEST_PYTHON=OFF \
    -DOPENEXR_FORCE_INTERNAL_DEFLATE=ON -DOPENEXR_FORCE_INTERNAL_OPENJPH=ON
cmake --build "$BUILD_ROOT/openexr" -j "$JOBS"
cmake --install "$BUILD_ROOT/openexr"

# --- static zlib / libtiff ----------------------------------------------------
# libtiff only gets the codecs that need no extra library (none, LZW, PackBits,
# CCITT, ...) plus zlib for Deflate and PixarLog. JPEG, WebP, ZSTD, LZMA, LERC
# and JBIG compressed TIFFs are rejected with an error.
cmake -S "$SRC_ROOT/zlib" -B "$BUILD_ROOT/zlib" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_INSTALL_LIBDIR=lib \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DZLIB_BUILD_SHARED=OFF -DZLIB_BUILD_STATIC=ON -DZLIB_BUILD_TESTING=OFF
cmake --build "$BUILD_ROOT/zlib" -j "$JOBS"
cmake --install "$BUILD_ROOT/zlib"

cmake -S "$SRC_ROOT/libtiff" -B "$BUILD_ROOT/libtiff" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_PREFIX_PATH="$PREFIX" \
    -DBUILD_SHARED_LIBS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DZLIB_USE_STATIC_LIBS=ON \
    -Dtiff-tools=OFF -Dtiff-tests=OFF -Dtiff-contrib=OFF -Dtiff-docs=OFF -Dtiff-cxx=OFF \
    -Dzlib=ON -Dpixarlog=ON -Dlibdeflate=OFF -Djpeg=OFF -Dold-jpeg=OFF -Djbig=OFF \
    -Dlerc=OFF -Dlzma=OFF -Dzstd=OFF -Dwebp=OFF
cmake --build "$BUILD_ROOT/libtiff" -j "$JOBS"
cmake --install "$BUILD_ROOT/libtiff"
grep -q "^#define ZIP_SUPPORT 1" "$PREFIX/include/tiffconf.h" || { echo "libtiff was built without zlib" >&2; exit 1; }

# --- patch a copy of compressonator ------------------------------------------
# Compressonator expects its dependencies in ../common relative to its root
WORK=$BUILD_ROOT/work
rm -rf "$WORK"
mkdir -p "$WORK/common/lib/ext"
cp -a "$SRC_ROOT/compressonator" "$WORK/compressonator"
cp -a "$SRC_ROOT/common/lib/ext/glm" "$SRC_ROOT/common/lib/ext/rapidxml" "$WORK/common/lib/ext/"
CMP=$WORK/compressonator

# see patches/ and README.md, generated with git format-patch against V4.6.0
for p in "$HERE"/patches/*.patch; do
    echo "applying $(basename "$p")"
    patch -d "$CMP" -p1 --forward --batch --quiet -i "$p"
done

# --- compressonatorcli --------------------------------------------------------
# glm 0.9.8.0 declares an ancient cmake_minimum_required that CMake 4 rejects
PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig" cmake -S "$CMP" -B "$BUILD_ROOT/cmp" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DCMAKE_PREFIX_PATH="$PREFIX" \
    -DCMAKE_DISABLE_FIND_PACKAGE_OpenCV=ON \
    -DZLIB_USE_STATIC_LIBS=ON \
    -DOPTION_ENABLE_ALL_APPS=OFF \
    -DOPTION_BUILD_APPS_CMP_CLI=ON \
    -DOPTION_CMP_OPENCV=OFF \
    -DOPTION_CMP_OPENGL=OFF \
    -DOPTION_BUILD_TIFF=ON \
    "-DCMAKE_EXE_LINKER_FLAGS=-static-libstdc++ -static-libgcc"
cmake --build "$BUILD_ROOT/cmp" --target CompressonatorCLI-bin -j "$JOBS"

mkdir -p "$(dirname "$OUTPUT")"
cp "$BUILD_ROOT/cmp/bin/compressonatorcli-bin" "$OUTPUT"
strip "$OUTPUT"
echo "built $OUTPUT"
