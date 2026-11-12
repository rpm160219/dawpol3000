#!/bin/sh
# Test zgodnosci portu ELG z oryginalem op1r (bit w bit): oryginalny harness op1r vs nasz silnik glosow, glos 88,2 kHz.
set -e
cd "$(dirname "$0")/.."
SRC=${OP1R_SRC:-$HOME/src_ext/op1r-src}
[ -d "$SRC" ] || git clone --depth 1 https://github.com/pcppcp/op1r-src.git "$SRC"
T=$(mktemp -d)
P=$SRC/synth_sdk/plugins/elg
(cd "$P" && cc -O1 -DOP1_HOST -DENGINE=ENGINE_elg -I ../.. ../../harness/audio_harness.c elg_plugin.c \
    elg_src_screen.c elg_src_elg.c -lm -o "$T/ref")
"$T/ref" 16384 8192 20000 8192 > "$T/ref.raw" 2>/dev/null
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
gcc -std=gnu11 -O2 -Icore -c core/synth/brass_dsp.c -o "$T/brw.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/hollowsaint_dsp.c -o "$T/hsw.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/bloodmoon_dsp.c -o "$T/bmw.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/blot_dsp.c -o "$T/blw.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/aether_dsp.c -o "$T/aew.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/arc_dsp.c -o "$T/arw.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/dddd_dsp.c -o "$T/ddw.o"
gcc -std=gnu11 -O3 -c third_party/op1r_dddd/dddd/dd_core.c -o "$T/ddc.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/juni_dsp.c -o "$T/jnw.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/outatime_dsp.c -o "$T/otw.o"
. tools/gin_objs.sh
g++ -std=c++17 -O2 -Icore tests/elg_match.cpp core/synth/wavetable_synth.cpp core/synth/taffy_dsp.cpp core/synth/pew_dsp.cpp core/synth/elg_dsp.cpp "$T/eva.o" "$T/mbira.o" "$T/reactor.o" "$T/oxbow.o" "$T/oxbow_plus.o" "$T/sidw.o" "$T/sidd.o" "$T/mono.o" \
    "$T/vapw.o" "$T/vapd.o" "$T/ssw.o" "$T/mxw.o" "$T/brw.o" "$T/hsw.o" "$T/bmw.o" "$T/blw.o" "$T/aew.o" "$T/arw.o" "$T/ddw.o" "$T/ddc.o" "$T/jnw.o" "$T/otw.o" "$T"/gin_*.o core/seq/project.cpp core/seq/pattern.cpp -o "$T/match" -lm
"$T/match" "$T/ref.raw"
rm -rf "$T"
