#!/bin/bash
# LINEA: dekoder PCM ROM (C++) i parser / zapis SysEx kontra skrypty oryginalu PicoFaceD5 (d5_make_blob.py,
# d5_syx_to_patches.py). Wymaga lokalnych ROM-ow w roms/linea (poza repo) — bez nich test pominiety.
set -e
cd "$(dirname "$0")/.."
R=roms/linea
SYX=$(ls $R/*.syx 2>/dev/null | head -1)
if [ -z "$SYX" ] || ! ls $R/*.bin >/dev/null 2>&1; then echo "LINEA: brak ROM-ow / .syx w $R - test pominiety"; exit 0; fi
T=$(mktemp -d)
mkdir -p /tmp/claude-1000
python3 third_party/pico_d50/tools/d5_make_blob.py $R "$T/gen" > /dev/null
python3 third_party/pico_d50/tools/d5_syx_to_patches.py "$SYX" "$T/gen" > /dev/null
python3 - "$T/gen/d5_patch_data.h" "$T/patches.bin" <<'PY'
import re, sys
s = open(sys.argv[1]).read()
rows = re.findall(r'\{([0-9, ]+)\}', s.split('kPatchData')[1])
open(sys.argv[2], 'wb').write(bytes(int(x) for r in rows for x in r.split(',')))
PY
g++ -std=c++17 -O2 -Icore -Ithird_party/pico_d50/include tests/linea_rom_test.cpp core/synth/linea_dsp.cpp core/io/linea_banks.cpp -o "$T/t"
"$T/t" $R "$T/gen/d5_pcm.bin" "$SYX" "$T/patches.bin"
rm -rf "$T"
