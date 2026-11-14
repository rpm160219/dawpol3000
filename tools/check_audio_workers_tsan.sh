#!/bin/sh
# Host-only concurrency/FPU/lifecycle checks; does not build the desktop app.
set -eu
cd "$(dirname "$0")/.."
AUDIO_TSAN_DIR=$(mktemp -d)
trap 'rm -rf "$AUDIO_TSAN_DIR"' EXIT
g++ -std=c++17 -O1 -g -fsanitize=thread -fno-omit-frame-pointer -no-pie -Icore core/audio/track_workers.cpp tests/audio_workers.cpp -pthread -lm -o "$AUDIO_TSAN_DIR/workers"
"$AUDIO_TSAN_DIR/workers"
