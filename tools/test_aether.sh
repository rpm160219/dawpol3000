#!/bin/sh
# Test zgodnosci portu AETHER z oryginalem op1r (bit w bit): oryginalny harness op1r (czujnik poziomo) vs wrapper DAW-POL
# bez przechylenia, glos 88,2 kHz.
set -e
cd "$(dirname "$0")/.."
SRC=${OP1R_SRC:-$HOME/src_ext/op1r-src}
[ -d "$SRC" ] || git clone --depth 1 https://github.com/pcppcp/op1r-src.git "$SRC"
T=$(mktemp -d)
P=$SRC/synth_sdk/plugins/aether
(cd "$P" && cc -O1 -DOP1_HOST -DENGINE=ENGINE_aether -I ../.. ../../harness/audio_harness.c aether.c -lm -o "$T/ref")
"$T/ref" 15671 11469 16384 32767 > "$T/ref.raw" 2>/dev/null
gcc -std=gnu11 -O2 -Icore -c core/synth/aether_dsp.c -o "$T/aew.o"
g++ -std=c++17 -O2 -Icore tests/aether_match.cpp "$T/aew.o" -o "$T/match" -lm
"$T/match" "$T/ref.raw"
rm -rf "$T"
