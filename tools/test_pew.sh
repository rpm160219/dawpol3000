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
gcc -std=gnu11 -O2 -Icore -c core/synth/mbira_dsp.c -o "$T/mbira.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/reactor_dsp.c -o "$T/reactor.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/oxbow_dsp.c -o "$T/oxbow.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/oxbow_plus_dsp.c -o "$T/oxbow_plus.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/sid_dsp_wrap.c -o "$T/sidw.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/monolit_dsp.c -o "$T/mono.o"
gcc -std=gnu11 -O2 -c third_party/op1r_sid/sid_dsp.c -o "$T/sidd.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/vapora_dsp.c -o "$T/vapw.o"
gcc -std=gnu11 -O2 -c third_party/op1r_vapora/vapora_dsp.c -o "$T/vapd.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/supersaw_dsp.c -o "$T/ssw.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/mixtur_dsp.c -o "$T/mxw.o"
g++ -std=c++17 -O2 -Icore tests/pew_match.cpp core/synth/wavetable_synth.cpp core/synth/taffy_dsp.cpp core/synth/pew_dsp.cpp core/synth/elg_dsp.cpp "$T/eva.o" "$T/mbira.o" "$T/reactor.o" "$T/oxbow.o" "$T/oxbow_plus.o" "$T/sidw.o" "$T/sidd.o" "$T/mono.o" \
    "$T/vapw.o" "$T/vapd.o" "$T/ssw.o" "$T/mxw.o" core/seq/project.cpp core/seq/pattern.cpp -o "$T/match" -lm
"$T/match" "$T/ref.raw"
rm -rf "$T"
