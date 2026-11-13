#!/bin/bash
# Efekty miksera (op1r JUNI CHORUS, SISTER): zgodnosc bit w bit wrappera DAW-POL (2 egzemplarze na zmiane) z oryginalem.
set -e
cd "$(dirname "$0")/.."
T=$(mktemp -d)
V=third_party/op1r_fx
I="-DOP1_HOST -I. -I$V -I$V/sister -Itests -Icore"
gcc -std=gnu11 -O2 -w $I tests/fx_ref.c -o "$T/refj" && "$T/refj" "$T/j.raw"
gcc -std=gnu11 -O2 -w $I -DFX_SISTER_REF tests/fx_ref.c $V/sister/sister_dsp.c $V/sister/sister_kern.c -o "$T/refs" && "$T/refs" "$T/s.raw"
for s in core/fx/fx_juni_chorus.c core/fx/fx_sister.c $V/sister/sister_dsp.c $V/sister/sister_kern.c; do gcc -std=gnu11 -O2 -w $I -c $s -o "$T/$(basename $s).o"; done
g++ -std=c++17 -O2 -Icore tests/fx_match.cpp "$T"/*.o -o "$T/match"
"$T/match" "$T/j.raw" juni
"$T/match" "$T/s.raw" sister
rm -rf "$T"
