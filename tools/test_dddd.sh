#!/bin/sh
# Test zgodnosci portu DDDD z oryginalem op1r (bit w bit): oryginalny harness op1r vs wrapper DAW-POL (zastepcza
# obwiednia harnessa), glos 88,2 kHz.
set -e
cd "$(dirname "$0")/.."
SRC=${OP1R_SRC:-$HOME/src_ext/op1r-src}
[ -d "$SRC" ] || git clone --depth 1 https://github.com/pcppcp/op1r-src.git "$SRC"
T=$(mktemp -d)
P=$SRC/synth_sdk/plugins/dddd
(cd "$P" && cc -O1 -DOP1_HOST -DENGINE=ENGINE_dddd -I ../.. ../../harness/audio_harness.c dddd.c ../../dddd/dd_core.c -lm -o "$T/ref")
"$T/ref" 0 24000 20000 8000 > "$T/ref.raw" 2>/dev/null
gcc -std=gnu11 -O2 -Icore -c core/synth/dddd_dsp.c -o "$T/ddw.o"
gcc -std=gnu11 -O3 -c third_party/op1r_dddd/dddd/dd_core.c -o "$T/ddc.o"
g++ -std=c++17 -O2 -Icore tests/dddd_match.cpp "$T/ddw.o" "$T/ddc.o" -o "$T/match" -lm
"$T/match" "$T/ref.raw"
rm -rf "$T"
