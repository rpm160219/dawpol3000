#!/bin/sh
# Test OXBOW+ z ustawieniami domyslnymi wobec oryginalnego OXBOW (bit w bit): oryginalny oxbow.c (paczka zrodel Hundekraft 3.0.1) z hostowym
# harnessem op1r vs nasz silnik glosow, glos 88,2 kHz.
set -e
cd "$(dirname "$0")/.."
SRC=${OP1R_SRC:-$HOME/src_ext/op1r-src}
HK=${HK_SRC:-$HOME/src_ext/hundekraft-3.0.1/hundekraft-3.0.1-source}
[ -d "$SRC" ] || git clone --depth 1 https://github.com/pcppcp/op1r-src.git "$SRC"
[ -d "$HK" ] || { echo "Brak zrodel Hundekraft: $HK (README op1-workshop: source/hundekraft-3.0.1-source.tar.gz)"; exit 1; }
T=$(mktemp -d)
cp -r "$SRC/synth_sdk" "$T/sdk"
cp -r "$HK/ext/kennel/synth_sdk/plugins/oxbow" "$T/sdk/plugins/oxbow_k"
(cd "$T/sdk/plugins/oxbow_k" && cc -O1 -DOP1_HOST -DENGINE=ENGINE_oxbow -I ../.. ../../harness/audio_harness.c oxbow.c -lm -o "$T/ref")
"$T/ref" 11869 5677 23738 2560 > "$T/ref.raw" 2>/dev/null
for c in eva mbira reactor oxbow oxbow_plus; do gcc -std=gnu11 -O2 -Icore -c core/synth/${c}_dsp.c -o "$T/$c.o"; done
gcc -std=gnu11 -O2 -Icore -c core/synth/sid_dsp_wrap.c -o "$T/sidw.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/monolit_dsp.c -o "$T/mono.o"
gcc -std=gnu11 -O2 -c third_party/op1r_sid/sid_dsp.c -o "$T/sidd.o"
gcc -std=gnu11 -O2 -Icore -c core/synth/vapora_dsp.c -o "$T/vapw.o"
gcc -std=gnu11 -O2 -c third_party/op1r_vapora/vapora_dsp.c -o "$T/vapd.o"
g++ -std=c++17 -O2 -Icore tests/oxbowplus_match.cpp core/synth/wavetable_synth.cpp core/synth/taffy_dsp.cpp core/synth/pew_dsp.cpp \
    core/synth/elg_dsp.cpp "$T/eva.o" "$T/mbira.o" "$T/reactor.o" "$T/oxbow.o" "$T/oxbow_plus.o" "$T/sidw.o" "$T/sidd.o" "$T/mono.o" "$T/vapw.o" "$T/vapd.o" core/seq/project.cpp core/seq/pattern.cpp -o "$T/match" -lm
"$T/match" "$T/ref.raw"
rm -rf "$T"
