#!/bin/sh
# Test zgodnosci portu ARC z oryginalem op1r (bit w bit): oryginalny harness op1r vs wrapper DAW-POL, glos 88,2 kHz.
# Szum plazmy startuje z ziarna zaleznego od adresu pamieci glosu: harness bez PIE, adres czytany z tablicy symboli.
set -e
cd "$(dirname "$0")/.."
SRC=${OP1R_SRC:-$HOME/src_ext/op1r-src}
[ -d "$SRC" ] || git clone --depth 1 https://github.com/pcppcp/op1r-src.git "$SRC"
T=$(mktemp -d)
P=$SRC/synth_sdk/plugins/arc
(cd "$P" && cc -O1 -no-pie -DOP1_HOST -DENGINE=ENGINE_arc -I ../.. ../../harness/audio_harness.c arc.c -lm -o "$T/ref")
A=$(nm "$T/ref" | awk '$3 == "host_voice_mem" { print $1 }')
"$T/ref" 12000 6000 5000 9000 > "$T/ref.raw" 2>/dev/null
gcc -std=gnu11 -O2 -Icore -c core/synth/arc_dsp.c -o "$T/arw.o"
g++ -std=c++17 -O2 -Icore tests/arc_match.cpp "$T/arw.o" -o "$T/match" -lm
"$T/match" "$T/ref.raw" "$A"
rm -rf "$T"
