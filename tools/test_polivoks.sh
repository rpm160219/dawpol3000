#!/bin/sh
# Native Polivoks path: sample equality, event boundaries, parameters and lifecycle.
set -eu
cd "$(dirname "$0")/.."
POLIVOKS_TEST_DIR=$(mktemp -d)
trap 'rm -rf "$POLIVOKS_TEST_DIR"' EXIT
POLIVOKS_TEST_BUILD=${DAWPOL_TEST_BUILD:-build-polivoks}
cmake -S . -B "$POLIVOKS_TEST_BUILD" -DCMAKE_BUILD_TYPE=Release
cmake --build "$POLIVOKS_TEST_BUILD" -j 4
cc -O3 -std=c11 -Icore/synth core/synth/polivoks_dsp.c core/synth/polivoks_presets.c tests/polivoks_reference.c -lm -o "$POLIVOKS_TEST_DIR/reference"
"$POLIVOKS_TEST_DIR/reference" "$POLIVOKS_TEST_DIR/golden.raw"
g++ -O3 -std=c++17 -Icore tests/polivoks_integration.cpp "$POLIVOKS_TEST_BUILD/libdpcore.a" -pthread -lm -o "$POLIVOKS_TEST_DIR/test"
mkdir "$POLIVOKS_TEST_DIR/data"
"$POLIVOKS_TEST_DIR/test" "$POLIVOKS_TEST_DIR/golden.raw" "$POLIVOKS_TEST_DIR/data"
