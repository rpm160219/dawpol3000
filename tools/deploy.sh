#!/bin/sh
# Wysylka na konsole: ./tools/deploy.sh [IP]
# Kopiuje program do /userdata/dawpol3000 i dodaje wpis w menu Ports.
# IP zapamietywane w tools/device.conf.
set -e
cd "$(dirname "$0")/.."
[ -f tools/device.conf ] && . tools/device.conf
IP=${1:-$DEVICE_IP}; USR=${DEVICE_USER:-root}; DIR=${DEVICE_DIR:-/userdata/dawpol3000}
[ -z "$IP" ] && { echo "Podaj IP konsoli: ./tools/deploy.sh 192.168.x.x"; exit 1; }
[ -n "$1" ] && printf 'DEVICE_IP=%s\n' "$IP" > tools/device.conf
./tools/build.sh arm
ssh "$USR@$IP" "mkdir -p $DIR /userdata/roms/ports"
scp build-arm/dawpol3000 "$USR@$IP:$DIR/"
scp "tools/knulli/DAW-POL 3000.sh" "$USR@$IP:/userdata/roms/ports/"
ssh "$USR@$IP" "chmod +x $DIR/dawpol3000 '/userdata/roms/ports/DAW-POL 3000.sh'; uname -a; ldd --version | head -1"
echo "Gotowe. Na konsoli: Ports -> DAW-POL 3000 (Menu+Select = wyjscie)."
