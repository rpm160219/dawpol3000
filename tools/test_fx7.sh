#!/bin/bash
# Efekty miksera z op1r 68 / hundekraft (TAPE, STUTTER, SNOWGLOBE, FATHER, RACKVERB, CRUSH, ENTE): zgodnosc bit w bit
# wrappera DAW-POL (2 egzemplarze na zmiane, core/fx/fxw.h) z oryginalnym pluginem (pozycja 0). Zegar tempa i sidechain
# podawane identycznie po obu stronach (tests/fx7_events.h).
set -e
cd "$(dirname "$0")/.."
T=$(mktemp -d)
V=third_party/op1r_fx
I="-DOP1_HOST -I. -I$V -I$V/sister -Itests -Icore -Icore/ui/fx_shim"
DSP="$V/crush/crush_dsp.c $V/father/father_dsp.c $V/rackverb/rv_mv.c $V/rackverb/rv_quad.c"
for s in core/fx/fx_common.c core/fx/fx_juni_chorus.c core/fx/fx_sister.c core/fx/fx_tape.c core/fx/fx_stutter.c core/fx/fx_ente.c \
         core/fx/fx_crush.c core/fx/fx_father.c core/fx/fx_rackverb.c core/fx/fx_snowglobe.c core/ui/fx_screen_host.c \
         $V/sister/sister_dsp.c $V/sister/sister_kern.c $DSP; do
    gcc -std=gnu11 -O2 -w $I -c $s -o "$T/$(basename $s).o"
done
g++ -std=c++17 -O2 -Icore tests/fx7_match.cpp "$T"/*.o -o "$T/match" -lm
ID=(x x x snowglobe rackverb tape father stutter crush ente)
for n in 3 4 5 6 7 8 9; do
    e=${ID[$n]}
    gcc -std=gnu11 -O2 -w $I -DFX7=$e tests/fx7_ref.c $DSP core/ui/fx_screen_host.c -lm -o "$T/ref_$e" 2>&1 | grep -v "^$" | head -5 || true
    "$T/ref_$e" "$T/$e.raw" > /dev/null
    "$T/match" "$T/$e.raw" $n $(echo $e | tr a-z A-Z)
done
rm -rf "$T"
