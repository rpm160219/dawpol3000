#!/bin/sh
# Test zgodnosci portu OUTATIME z oryginalem (bit w bit): harness op1r z oryginalnym outatime.c + ota_dsp.c z paczki
# zrodel Hundekraft 3.1.4 vs wrapper DAW-POL, glos 88,2 kHz.
set -e
cd "$(dirname "$0")/.."
SRC=${OP1R_SRC:-$HOME/src_ext/op1r-src}
HK=${HK_SRC:-$HOME/src_ext/hundekraft-3.1.4/src/hundekraft-3.1.4-source}
T=$(mktemp -d)
P=$HK/ext/kennel/synth_sdk/plugins/outatime
cp "$P/outatime.c" "$P/ota_dsp.c" "$P/ota_dsp.h" "$P/ota_tables.h" "$T/"
(cd "$T" && cc -O1 -DOP1_HOST -DENGINE=ENGINE_outatime -I "$SRC/synth_sdk" "$SRC/synth_sdk/harness/audio_harness.c" outatime.c ota_dsp.c -lm -o ref)
"$T/ref" 16549 6620 8274 11584 > "$T/ref.raw" 2>/dev/null
gcc -std=gnu11 -O2 -Icore -c core/synth/outatime_dsp.c -o "$T/otw.o"
g++ -std=c++17 -O2 -Icore tests/outatime_match.cpp "$T/otw.o" -o "$T/match" -lm
"$T/match" "$T/ref.raw"
rm -rf "$T"
