#!/bin/sh
# Paczka PortMastera do pokazania / instalacji na innej konsoli: ./tools/package_portmaster.sh
# Wynik: dist/DAW-POL-3000_portmaster_<data>.zip (rozpakowac do folderu ports na karcie, np. /roms/ports).
set -e
cd "$(dirname "$0")/.."
./tools/build.sh arm >/dev/null
VER=$(date +%Y-%m-%d)
T=$(mktemp -d)
G="$T/dawpol3000"
mkdir -p "$G/licenses"
cp build-arm/dawpol3000 "$G/"
cp "tools/portmaster/DAW-POL 3000.sh" "$T/"
chmod +x "$G/dawpol3000" "$T/DAW-POL 3000.sh"
cp LICENSE "$G/licenses/LICENSE-DAWPOL3000.txt"
cp third_party/LICENSES.md "$G/licenses/THIRD_PARTY.md"
for d in third_party/*/; do
  n=$(basename "$d")
  [ -f "$d/LICENSE" ] && cp "$d/LICENSE" "$G/licenses/$n-LICENSE.txt"
  [ -f "$d/NOTICE" ] && cp "$d/NOTICE" "$G/licenses/$n-NOTICE.txt"
done
cat > "$G/README.txt" <<TXT
DAW-POL 3000 ($VER) — pocket sketchbook DAW for Anbernic RG35XX Pro (640x480, aarch64), OP-1 style.

INSTALL (PortMaster)
  Unzip into the ports folder on the SD card (e.g. /roms/ports or /userdata/roms/ports),
  so you get "DAW-POL 3000.sh" and the folder "dawpol3000" next to it. Refresh the game list,
  then run DAW-POL 3000 from Ports. Needs PortMaster installed.

CONTROLS
  L2 / R2      tabs: PROJECT - ARRANGE - PIANO ROLL - SYNTH
  Start        play / stop
  Select       on a track in ARRANGE: track window (R2 = ENGINE card); in SYNTH: next knob page
  Sticks       4 encoders (blue, green, white, red)
  L3 / R3      tools bar / select mode (and extra knob layers in SYNTH)
  Menu+Select  quit
  A demo song "ALL ENGINES" appears in PROJECT on first start.

ENGINES
  WAVETABLE, TAFFY, PEW, ELG, EVA, MBIRA, REACTOR: ports of op1r engines by Hundekraft (GPL-3.0-or-later).
  OXBOW: Hundekraft's port of zvenson's OberVave (GPL-3.0-only). WAVETABLE+ / OXBOW+: extended versions.
  See licenses/ for all licences and notices. Source code available on request (GPL).

Built for RG35XX Pro (Allwinner H700). Other aarch64 handhelds with 640x480 may work, button mapping may differ.
Log: dawpol3000/log.txt
TXT
cat > "$G/port.json" <<JSON
{
  "version": 2,
  "name": "dawpol3000.zip",
  "items": ["DAW-POL 3000.sh", "dawpol3000"],
  "items_opt": [],
  "attr": {
    "title": "DAW-POL 3000",
    "porter": ["Patryk"],
    "desc": "Pocket sketchbook DAW in the spirit of OP-1: patterns, arrange, piano roll and synth engines.",
    "inst": "Unzip into the ports folder. Requires PortMaster.",
    "genres": ["other"],
    "image": {},
    "rtr": true,
    "exp": false,
    "runtime": null,
    "reqs": [],
    "arch": ["aarch64"]
  }
}
JSON
mkdir -p dist
OUT="$PWD/dist/DAW-POL-3000_portmaster_$VER.zip"
rm -f "$OUT"
(cd "$T" && python3 -c "
import zipfile, os, sys
out = sys.argv[1]
with zipfile.ZipFile(out, 'w', zipfile.ZIP_DEFLATED) as z:
    for root, dirs, files in os.walk('.'):
        for f in sorted(files):
            p = os.path.join(root, f)
            info = zipfile.ZipInfo(os.path.relpath(p, '.'))
            st = os.stat(p)
            info.external_attr = (st.st_mode & 0o777 | 0o100000) << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            with open(p, 'rb') as fh: z.writestr(info, fh.read())
" "$OUT")
rm -rf "$T"
echo "Paczka: $OUT"
