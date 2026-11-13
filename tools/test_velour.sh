#!/bin/bash
# VELOUR: zgodnosc bit w bit kopii DAW-POL (third_party/pico_solina: + poziomy rejestrow i odczyt klawiatury) z
# oryginalem PicoFaceSM (lokalna kopia repo PicoVintageSynthCollection). 8 programow, akordy, legato, ogony.
set -e
cd "$(dirname "$0")/.."
O=${SOLINA_ORIG:-$HOME/src_ext/PicoVintageSynthCollection/instruments/PicoFaceSM}
if [ ! -d "$O" ]; then echo "VELOUR: brak oryginalu ($O) - test pominiety"; exit 0; fi
T=$(mktemp -d)
F="-std=c++17 -O2 -DSOLINA_HOST_BUILD"
g++ $F -I$O/include tests/velour_ref.cpp $O/src/solina/*.cpp -o "$T/orig"
g++ $F -Ithird_party/pico_solina/include tests/velour_ref.cpp third_party/pico_solina/src/*.cpp -o "$T/ours"
"$T/orig" "$T/a.raw"; "$T/ours" "$T/b.raw"
python3 - "$T/a.raw" "$T/b.raw" <<'PY'
import sys, array
a = array.array('f'); a.frombytes(open(sys.argv[1], 'rb').read())
b = array.array('f'); b.frombytes(open(sys.argv[2], 'rb').read())
nz = sum(1 for x in a if x != 0.0)
diff = sum(1 for x, y in zip(a, b) if x != y)
print("VELOUR: probek: %d (niezerowych %d), roznych: %d, max roznica: %g" % (len(a), nz, diff, max((abs(x - y) for x, y in zip(a, b)), default=0)))
sys.exit(1 if diff or len(a) != len(b) else 0)
PY
rm -rf "$T"
