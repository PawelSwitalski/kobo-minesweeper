#!/bin/sh
# Fetches (pinned) and cross-builds FBInk as a static library for the Kobo
# flavor (T023). Run inside WSL2 with koxtoolchain on PATH or at the default
# ~/x-tools location. Produces third_party/FBInk/Release/libfbink.a.
set -e

# No FBInk tag has been cut since v1.25.0 (Dec 2022), but device-ID support
# for the Kobo Clara/Libra Colour line (device code 390) landed later, in
# commit 0341c0f (2024-04-20, "ID the Clara BW/Colour & Libra Colour and
# their Tolino counterparts"), on master only. Pinning to v1.25.0 built a
# binary that can't identify these devices and fails to refresh the screen
# ("[FBInk] Unidentified Kobo device code (390)!"). Pin to a commit instead.
FBINK_TAG="${FBINK_TAG:-83110d3d278cf9cd44cc1d16237e284a89f72633}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DIR="$ROOT/third_party/FBInk"
TC_TRIPLE=arm-kobo-linux-gnueabihf
TC_ROOT="${CROSS_TC_ROOT:-$HOME/x-tools/$TC_TRIPLE}"

if [ ! -d "$DIR/.git" ]; then
    echo ">> cloning FBInk $FBINK_TAG"
    git init -q "$DIR"
    git -C "$DIR" remote add origin https://github.com/NiLuJe/FBInk.git
    git -C "$DIR" fetch --depth 1 origin "$FBINK_TAG"
    git -C "$DIR" checkout -q FETCH_HEAD
    git -C "$DIR" submodule update --init --recursive
fi

export PATH="$TC_ROOT/bin:$PATH"
command -v "$TC_TRIPLE-gcc" >/dev/null 2>&1 || {
    echo "!! $TC_TRIPLE-gcc not found — install koxtoolchain first (see cmake/kobo-toolchain.cmake)"
    exit 1
}

echo ">> building libfbink.a (Kobo, minimal + image for fbink_print_raw_data)"
make -C "$DIR" clean
CROSS_TC="$TC_TRIPLE" make -C "$DIR" staticlib KOBO=true MINIMAL=1 IMAGE=1

ls -l "$DIR/Release/libfbink.a"
echo ">> done"
