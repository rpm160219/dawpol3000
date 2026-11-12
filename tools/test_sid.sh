#!/bin/sh
# Test zgodnosci portu SID z oryginalem op1r (bit w bit): oryginalny harness op1r vs nasz silnik glosow, glos 88,2 kHz.
set -e
cd "$(dirname "$0")/.."
SRC=${OP1R_SRC:-$HOME/src_ext/op1r-src}
[ -d "$SRC" ] || git clone --depth 1 https://github.com/pcppcp/op1r-src.git "$SRC"
T=$(mktemp -d)
P=$SRC/synth_sdk/plugins/sid
(cd "$P" && cc -O1 -DOP1_HOST -DENGINE=ENGINE_sid -I ../.. ../../harness/audio_harness.c sid.c sid_dsp.c -lm -o "$T/ref")
"$T/ref" 6656 16384 32136 8192 > "$T/ref.raw" 2>/dev/null
for c in eva mbira reactor oxbow oxbow_plus; do gcc -std=gnu11 -O2 -Icore -c core/synth/${c}_dsp.c -o "$T/$c.o"; done
gcc -std=gnu11 -O2 -Icore -c core/synth/sid_dsp_wrap.c -o "$T/sidw.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/monolit_dsp.c -o "$T/mono.o"
gcc -std=gnu11 -O2 -c third_party/op1r_sid/sid_dsp.c -o "$T/sidd.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/vapora_dsp.c -o "$T/vapw.o"
gcc -std=gnu11 -O2 -c third_party/op1r_vapora/vapora_dsp.c -o "$T/vapd.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/supersaw_dsp.c -o "$T/ssw.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/mixtur_dsp.c -o "$T/mxw.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/brass_dsp.c -o "$T/brw.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/hollowsaint_dsp.c -o "$T/hsw.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/bloodmoon_dsp.c -o "$T/bmw.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/blot_dsp.c -o "$T/blw.o"
g++ -std=c++17 -O2 -Icore tests/sid_match.cpp core/synth/wavetable_synth.cpp core/synth/taffy_dsp.cpp core/synth/pew_dsp.cpp \
    core/synth/elg_dsp.cpp "$T/eva.o" "$T/mbira.o" "$T/reactor.o" "$T/oxbow.o" "$T/oxbow_plus.o" "$T/sidw.o" "$T/sidd.o" "$T/mono.o" \
    "$T/vapw.o" "$T/vapd.o" "$T/ssw.o" "$T/mxw.o" "$T/brw.o" "$T/hsw.o" "$T/bmw.o" "$T/blw.o" core/seq/project.cpp core/seq/pattern.cpp -o "$T/match" -lm
"$T/match" "$T/ref.raw"
rm -rf "$T"
