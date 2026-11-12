#!/bin/sh
# Test zgodnosci portu WAVETABLE z oryginalem op1r (bit w bit).
# Buduje oryginalny harness op1r (fraza C3..E3, glos 88,2 kHz) i nasz port z oryginalna tabela 88,2 kHz, porownuje probki.
set -e
cd "$(dirname "$0")/.."
SRC=${OP1R_SRC:-$HOME/src_ext/op1r-src}
[ -d "$SRC" ] || git clone --depth 1 https://github.com/pcppcp/op1r-src.git "$SRC"
T=$(mktemp -d)
W=$SRC/synth_sdk/plugins/wavetable
(cd "$W" && cc -O1 -DOP1_HOST -DENGINE=ENGINE_wavetable -I ../.. ../../harness/audio_harness.c wavetable_plugin.c \
    wavetable_screen.c wavetable_src_wavetable.c -lm -o "$T/ref")
"$T/ref" 16384 8192 20000 8192 > "$T/ref.raw" 2>/dev/null
cp third_party/op1r_wavetable/wavetable_dsp.h third_party/op1r_wavetable/wavetable_level.h "$T/"
cp third_party/op1r_wavetable/orig_88k/wavetables.h "$T/"
gcc -std=gnu11 -O2 -Icore -c core/synth/eva_dsp.c -o "$T/eva.o"
g++ -std=c++17 -O2 -Icore "-DWT_DSP_HEADER=\"$T/wavetable_dsp.h\"" tests/wavetable_match.cpp core/synth/wavetable_synth.cpp \
    core/synth/taffy_dsp.cpp core/synth/pew_dsp.cpp core/synth/elg_dsp.cpp "$T/eva.o" core/seq/project.cpp core/seq/pattern.cpp -o "$T/match" -lm
"$T/match" "$T/ref.raw"
rm -rf "$T"
