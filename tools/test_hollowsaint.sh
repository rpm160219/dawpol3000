#!/bin/sh
# Test zgodnosci portu HOLLOW SAINT z oryginalem op1r (bit w bit): oryginalny harness op1r (z flagami kennel z
# plugin.json) vs wrapper DAW-POL, glos 88,2 kHz.
set -e
cd "$(dirname "$0")/.."
SRC=${OP1R_SRC:-$HOME/src_ext/op1r-src}
[ -d "$SRC" ] || git clone --depth 1 https://github.com/pcppcp/op1r-src.git "$SRC"
T=$(mktemp -d)
P=$SRC/synth_sdk/plugins/hollowsaint
F="-DHS_SHARED_ROOM -DHS_SHARED_RELIC -DHS_CAT11K -DHS_RELIC_LITE -DHS_BOW_CHEAP -DHS_HIGH_HALF"
(cd "$P" && cc -O1 -DOP1_HOST $F -DENGINE=ENGINE_hollow_saint -I ../.. ../../harness/audio_harness.c hollow_saint.c -lm -o "$T/ref")
"$T/ref" 16384 16384 12000 12000 > "$T/ref.raw" 2>/dev/null
gcc -std=gnu11 -O2 -Icore -c core/synth/hollowsaint_dsp.c -o "$T/hsw.o"
g++ -std=c++17 -O2 -Icore tests/hollowsaint_match.cpp "$T/hsw.o" -o "$T/match" -lm
"$T/match" "$T/ref.raw"
rm -rf "$T"
