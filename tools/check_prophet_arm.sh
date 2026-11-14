#!/bin/sh
# Optional focused diagnostics on the actual H700. No desktop app/audio renders.
set -eu
cd "$(dirname "$0")/.."
PROPHET_CHECK_DIR=$(mktemp -d)
trap 'rm -rf "$PROPHET_CHECK_DIR"' EXIT
./tools/build.sh arm
aarch64-linux-gnu-gcc -std=c11 -O3 -mcpu=cortex-a53 -Icore tests/prophet_core.c core/synth/prophet_dsp.c core/synth/prophet_presets.c -lm -o "$PROPHET_CHECK_DIR/core"
aarch64-linux-gnu-g++ -std=c++17 -O3 -mcpu=cortex-a53 -Icore tests/prophet_host.cpp build-arm/libdpcore.a -pthread -lm -o "$PROPHET_CHECK_DIR/host"
# ssh_env uses the caller's tools/ path and keeps the password private.
. ./tools/ssh_env.sh
PROPHET_REMOTE_DIR=$(dssh 'mktemp -d /tmp/dawpol-prophet.XXXXXX')
dscp "$PROPHET_CHECK_DIR/core" "$PROPHET_CHECK_DIR/host" "$USR@$IP:$PROPHET_REMOTE_DIR/"
dssh "mkdir '$PROPHET_REMOTE_DIR/data'; '$PROPHET_REMOTE_DIR/core' && '$PROPHET_REMOTE_DIR/host' '$PROPHET_REMOTE_DIR/data'"
dssh "rm -rf '$PROPHET_REMOTE_DIR'"
rm -rf "$PROPHET_CHECK_DIR"
