#!/bin/bash
# Builds portable texconv and texassemble for Linux in a container.
#
# The results only depend on glibc >= 2.28. See README.md for details.
#
# Usage: build.sh [--install]
#   --install   copy the binaries to bin/linux/ in this repository
#
# Environment:
#   WORK_DIR    where sources and build files go (default ~/.cache/nebula-directxtex)
#   DOCKER      container tool (default docker, podman works too)
set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
REPO_ROOT=$(cd "$HERE/../../.." && pwd)
WORK_DIR=${WORK_DIR:-${XDG_CACHE_HOME:-$HOME/.cache}/nebula-directxtex}
DOCKER=${DOCKER:-docker}
IMAGE=nebula-directxtex-build
SOURCES=$WORK_DIR/sources

INSTALL=0
for arg in "$@"; do
    case "$arg" in
        --install) INSTALL=1 ;;
        *) echo "usage: $0 [--install]" >&2; exit 1 ;;
    esac
done

# name                  url                                                  tag       commit
DEPS=(
    "texconv-custom-dll   https://github.com/matyalatte/Texconv-Custom-DLL     -         40b28a07e760a145e8677fb65b5e1ffb7e712e48"
    "directxtex           https://github.com/microsoft/DirectXTex              may2026   4feb3e11a020f35b796fc769a74216a555d4f5ef"
)

mkdir -p "$SOURCES"
for dep in "${DEPS[@]}"; do
    read -r name url tag commit <<< "$dep"
    dir=$SOURCES/$name
    if [ ! -d "$dir" ]; then
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
# DirectX-Headers, DirectXMath and safestringlib at the commits Texconv-Custom-DLL pins
git -C "$SOURCES/texconv-custom-dll" submodule update -q --init --recursive

"$DOCKER" build -t "$IMAGE" "$HERE"
"$DOCKER" run --rm --user "$(id -u):$(id -g)" -e HOME=/work/container-build \
    -v "$WORK_DIR:/work" -v "$HERE:/scripts:ro" -w /work "$IMAGE" \
    /scripts/build_directxtex.sh /work/sources /work/container-build /work/out

# licenses of everything linked into the binaries
{
    echo "texconv and texassemble are built from Texconv-Custom-DLL and DirectXTex."
    echo "The texassemble patch in toolkit/scripts/directxtex is part of Nebula."
    printf '\n\n==============================================================\nTexconv-Custom-DLL (MIT)\n==============================================================\n\n'
    cat "$SOURCES/texconv-custom-dll/LICENSE"
    printf '\n\n'
    cat "$SOURCES/texconv-custom-dll/THIRD_PARTY_LICENSES.txt"
} > "$WORK_DIR/out/THIRD_PARTY_LICENSES.txt"
cp "$WORK_DIR/out/THIRD_PARTY_LICENSES.txt" "$HERE/THIRD_PARTY_LICENSES.txt"

if [ "$INSTALL" = 1 ]; then
    mkdir -p "$REPO_ROOT/bin/linux"
    cp "$WORK_DIR/out/texconv" "$WORK_DIR/out/texassemble" "$REPO_ROOT/bin/linux/"
    echo "installed texconv and texassemble to $REPO_ROOT/bin/linux"
fi

echo
echo "done: $WORK_DIR/out/texconv $WORK_DIR/out/texassemble"
