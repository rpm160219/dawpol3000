#!/bin/sh
# Test zgodnosci portu JUNI z oryginalem op1r (bit w bit): oryginalny harness op1r vs wrapper DAW-POL (zastepcza
# obwiednia harnessa), glos 88,2 kHz.
set -e
cd "$(dirname "$0")/.."
SRC=${OP1R_SRC:-$HOME/src_ext/op1r-src}
[ -d "$SRC" ] || git clone --depth 1 https://github.com/pcppcp/op1r-src.git "$SRC"
T=$(mktemp -d)
P=$SRC/synth_sdk/plugins/juni
(cd "$P" && cc -O1 -DOP1_HOST -DENGINE=ENGINE_juni -I ../.. ../../harness/audio_harness.c juni.c -lm -o "$T/ref")
"$T/ref" 18000 16384 11000 9800 > "$T/ref.raw" 2>/dev/null
gcc -std=gnu11 -O2 -Icore -c core/synth/juni_dsp.c -o "$T/jnw.o"
g++ -std=c++17 -O2 -Icore tests/juni_match.cpp "$T/jnw.o" -o "$T/match" -lm
"$T/match" "$T/ref.raw"
rm -rf "$T"
