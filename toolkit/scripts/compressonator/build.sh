#!/bin/bash
# Builds a portable, self-contained compressonatorcli for Linux in a container.
#
# The result only depends on glibc >= 2.28. See README.md for details.
#
# Usage: build.sh [--install]
#   --install   copy the binary to bin/linux/compressonatorcli in this repository
#
# Environment:
#   WORK_DIR    where sources and build files go (default ~/.cache/nebula-compressonatorcli)
#   DOCKER      container tool (default docker, podman works too)
set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
REPO_ROOT=$(cd "$HERE/../../.." && pwd)
WORK_DIR=${WORK_DIR:-${XDG_CACHE_HOME:-$HOME/.cache}/nebula-compressonatorcli}
DOCKER=${DOCKER:-docker}
IMAGE=nebula-compressonatorcli-build
SOURCES=$WORK_DIR/sources

INSTALL=0
for arg in "$@"; do
    case "$arg" in
        --install) INSTALL=1 ;;
        *) echo "usage: $0 [--install]" >&2; exit 1 ;;
    esac
done

# name                          url                                                    tag       commit
DEPS=(
    "compressonator             https://github.com/GPUOpen-Tools/compressonator        V4.6.0    095a143ad8cfbf9ed5afa4ea63c735bfd0fd04dd"
    "imath                      https://github.com/AcademySoftwareFoundation/Imath     v3.2.2    1e480d11cb98b032a2dece9b9a8730512effc7f6"
    "openexr                    https://github.com/AcademySoftwareFoundation/openexr   v3.4.16   37d012e2faa04dbf00ca169682ee1eb6d9e8603e"
    "zlib                       https://github.com/madler/zlib                         v1.3.2    da607da739fa6047df13e66a2af6b8bec7c2a498"
    "libtiff                    https://gitlab.com/libtiff/libtiff.git                 v4.7.2    d01a94be176f5f6a87f7ee1c0b32e65416aa2b4d"
    "common/lib/ext/glm         https://github.com/g-truc/glm.git                      0.9.8.0   454d480ceb4ac10ca76ac24b49836b7d5a35f786"
    "common/lib/ext/rapidxml    https://github.com/discord/rapidxml.git                -         2ae4b2888165a393dfb6382168825fddf00c27b9"
)

mkdir -p "$SOURCES"
for dep in "${DEPS[@]}"; do
    read -r name url tag commit <<< "$dep"
    dir=$SOURCES/$name
    if [ ! -d "$dir" ]; then
        mkdir -p "$(dirname "$dir")"
        if [ "$tag" = "-" ]; then
            git clone -q "$url" "$dir"
            git -C "$dir" -c advice.detachedHead=false checkout -q "$commit"
        else
            git -c advice.detachedHead=false clone -q --depth 1 --branch "$tag" "$url" "$dir"
        fi
    fi
    actual=$(git -C "$dir" rev-parse HEAD)
    if [ "$actual" != "$commit" ]; then
        echo "error: $name is at $actual, expected $commit" >&2
        exit 1
    fi
done

"$DOCKER" build -t "$IMAGE" "$HERE"
"$DOCKER" run --rm --user "$(id -u):$(id -g)" -e HOME=/work/container-build \
    -v "$WORK_DIR:/work" -v "$HERE:/scripts:ro" -w /work "$IMAGE" \
    /scripts/build_compressonatorcli.sh /work/sources /work/container-build /work/out/compressonatorcli

# licenses of everything linked into the binary
{
    echo "compressonatorcli is built from the following projects. Their license texts follow."
    echo "The TIFF plugin and the build patches in toolkit/scripts/compressonator are part of Nebula."
    for f in \
        "Compressonator (MIT):compressonator/license/clilicense.txt" \
        "Compressonator Core (MIT):compressonator/license/corelicense.txt" \
        "Compressonator Framework (MIT):compressonator/license/frameworklicense.txt" \
        "stb_image / stb_image_write (MIT or public domain):compressonator/external/stb/LICENSE" \
        "OpenEXR (BSD-3-Clause):openexr/LICENSE.md" \
        "Imath (BSD-3-Clause):imath/LICENSE.md" \
        "libdeflate, vendored by OpenEXR (MIT):openexr/external/deflate/COPYING" \
        "OpenJPH, vendored by OpenEXR (BSD-2-Clause):openexr/external/OpenJPH/LICENSE" \
        "libtiff (libtiff license):libtiff/LICENSE.md" \
        "zlib (zlib license):zlib/LICENSE" \
        "GLM (MIT):common/lib/ext/glm/copying.txt" \
        "RapidXml (Boost or MIT):common/lib/ext/rapidxml/license.txt"; do
        title=${f%%:*}
        path=$SOURCES/${f#*:}
        printf '\n\n==============================================================================\n%s\n==============================================================================\n\n' "$title"
        cat "$path"
    done
} > "$WORK_DIR/out/THIRD_PARTY_LICENSES.txt"
cp "$WORK_DIR/out/THIRD_PARTY_LICENSES.txt" "$HERE/THIRD_PARTY_LICENSES.txt"

if [ "$INSTALL" = 1 ]; then
    mkdir -p "$REPO_ROOT/bin/linux"
    cp "$WORK_DIR/out/compressonatorcli" "$REPO_ROOT/bin/linux/compressonatorcli"
    echo "installed $REPO_ROOT/bin/linux/compressonatorcli"
fi

echo
echo "done: $WORK_DIR/out/compressonatorcli"
