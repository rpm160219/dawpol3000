#!/bin/sh
# Test zgodnosci portu TAFFY z oryginalem op1r (bit w bit): oryginalny harness op1r vs nasz silnik glosow, glos 88,2 kHz.
set -e
cd "$(dirname "$0")/.."
SRC=${OP1R_SRC:-$HOME/src_ext/op1r-src}
[ -d "$SRC" ] || git clone --depth 1 https://github.com/pcppcp/op1r-src.git "$SRC"
T=$(mktemp -d)
P=$SRC/synth_sdk/plugins/taffy
(cd "$P" && cc -O1 -DOP1_HOST -DENGINE=ENGINE_taffy -I ../.. ../../harness/audio_harness.c taffy_plugin.c \
    taffy_src_screen.c taffy_src_taffy.c -lm -o "$T/ref")
"$T/ref" 16384 8192 0 8192 > "$T/ref.raw" 2>/dev/null
g++ -std=c++17 -O2 -Icore tests/taffy_match.cpp core/synth/wavetable_synth.cpp core/synth/taffy_dsp.cpp \
    core/seq/project.cpp core/seq/pattern.cpp -o "$T/match" -lm
"$T/match" "$T/ref.raw"
rm -rf "$T"
