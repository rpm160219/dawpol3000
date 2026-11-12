#!/bin/sh
# Test zgodnosci portu BRASSOPHONE z oryginalem op1r (bit w bit): oryginalny harness op1r vs wrapper DAW-POL, glos 88,2 kHz.
set -e
cd "$(dirname "$0")/.."
SRC=${OP1R_SRC:-$HOME/src_ext/op1r-src}
[ -d "$SRC" ] || git clone --depth 1 https://github.com/pcppcp/op1r-src.git "$SRC"
T=$(mktemp -d)
P=$SRC/synth_sdk/plugins/brassophone
(cd "$P" && cc -O1 -DOP1_HOST -DENGINE=ENGINE_brass -I ../.. ../../harness/audio_harness.c brass.c -lm -o "$T/ref")
"$T/ref" 19660 16384 3932 7536 > "$T/ref.raw" 2>/dev/null
gcc -std=gnu11 -O2 -Icore -c core/synth/brass_dsp.c -o "$T/brw.o"
g++ -std=c++17 -O2 -Icore tests/brass_match.cpp "$T/brw.o" -o "$T/match" -lm
"$T/match" "$T/ref.raw"
rm -rf "$T"
