#!/bin/sh
# Test zgodnosci portu PEW z oryginalem op1r (bit w bit): oryginalny harness op1r vs nasz silnik glosow, glos 88,2 kHz.
set -e
cd "$(dirname "$0")/.."
SRC=${OP1R_SRC:-$HOME/src_ext/op1r-src}
[ -d "$SRC" ] || git clone --depth 1 https://github.com/pcppcp/op1r-src.git "$SRC"
T=$(mktemp -d)
P=$SRC/synth_sdk/plugins/pew
(cd "$P" && cc -O1 -DOP1_HOST -DENGINE=ENGINE_pew -I ../.. ../../harness/audio_harness.c pew_plugin.c \
    pew_src_screen.c pew_src_pew.c -lm -o "$T/ref")
"$T/ref" 16384 12000 8000 16384 > "$T/ref.raw" 2>/dev/null
gcc -std=gnu11 -O2 -Icore -c core/synth/eva_dsp.c -o "$T/eva.o"
g++ -std=c++17 -O2 -Icore tests/pew_match.cpp core/synth/wavetable_synth.cpp core/synth/taffy_dsp.cpp core/synth/pew_dsp.cpp core/synth/elg_dsp.cpp "$T/eva.o" \
    core/seq/project.cpp core/seq/pattern.cpp -o "$T/match" -lm
"$T/match" "$T/ref.raw"
rm -rf "$T"
