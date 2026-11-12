#!/bin/sh
# Test zgodnosci portu EVA z oryginalem op1r (bit w bit): oryginalny harness op1r vs nasz silnik glosow, glos 88,2 kHz.
set -e
cd "$(dirname "$0")/.."
SRC=${OP1R_SRC:-$HOME/src_ext/op1r-src}
[ -d "$SRC" ] || git clone --depth 1 https://github.com/pcppcp/op1r-src.git "$SRC"
T=$(mktemp -d)
P=$SRC/synth_sdk/plugins/eva
(cd "$P" && cc -O1 -DOP1_HOST -DENGINE=ENGINE_eva -I ../.. ../../harness/audio_harness.c eva.c -lm -o "$T/ref")
"$T/ref" 20000 16384 12000 3000 > "$T/ref.raw" 2>/dev/null
gcc -std=gnu11 -O2 -Icore -c core/synth/eva_dsp.c -o "$T/eva.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/mbira_dsp.c -o "$T/mbira.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/reactor_dsp.c -o "$T/reactor.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/oxbow_dsp.c -o "$T/oxbow.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/oxbow_plus_dsp.c -o "$T/oxbow_plus.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/sid_dsp_wrap.c -o "$T/sidw.o"
gcc -std=gnu11 -O2 -c third_party/op1r_sid/sid_dsp.c -o "$T/sidd.o"
g++ -std=c++17 -O2 -Icore tests/eva_match.cpp core/synth/wavetable_synth.cpp core/synth/taffy_dsp.cpp core/synth/pew_dsp.cpp \
    core/synth/elg_dsp.cpp "$T/eva.o" "$T/mbira.o" "$T/reactor.o" "$T/oxbow.o" "$T/oxbow_plus.o" "$T/sidw.o" "$T/sidd.o" core/seq/project.cpp core/seq/pattern.cpp -o "$T/match" -lm
"$T/match" "$T/ref.raw"
rm -rf "$T"
