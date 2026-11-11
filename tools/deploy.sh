#!/bin/sh
# Wysylka na konsole: ./tools/deploy.sh [IP]
# Kopiuje program do /userdata/dawpol3000 i dodaje wpis w menu Ports.
set -e
IP=$1
. "$(dirname "$0")/ssh_env.sh"
cd "$ROOT"
DIR=${DEVICE_DIR:-/userdata/dawpol3000}
./tools/build.sh arm
dssh "mkdir -p $DIR /userdata/roms/ports"
dscp build-arm/dawpol3000 "$USR@$IP:$DIR/"
dscp "tools/knulli/DAW-POL 3000.sh" "$USR@$IP:/userdata/roms/ports/"
dssh "chmod +x $DIR/dawpol3000 '/userdata/roms/ports/DAW-POL 3000.sh' 2>/dev/null; echo OK na konsoli"
echo "Gotowe. Na konsoli: Ports -> DAW-POL 3000 (Menu+Select = wyjscie)."
