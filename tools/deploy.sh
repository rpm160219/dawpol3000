#!/bin/sh
# Wysylka na konsole i uruchomienie: ./tools/deploy.sh [IP] [uzytkownik]
# IP i uzytkownik zapamietywane w tools/device.conf (uzupelnimy w etapie 0).
set -e
cd "$(dirname "$0")/.."
[ -f tools/device.conf ] && . tools/device.conf
IP=${1:-$DEVICE_IP}; USR=${2:-${DEVICE_USER:-root}}; DIR=${DEVICE_DIR:-/userdata/dawpol3000}
[ -z "$IP" ] && { echo "Podaj IP konsoli: ./tools/deploy.sh 192.168.x.x"; exit 1; }
./tools/build.sh arm
ssh "$USR@$IP" "mkdir -p $DIR"
scp build-arm/dawpol3000 "$USR@$IP:$DIR/"
echo "Uruchamiam (Menu+Select = wyjscie)..."
ssh -t "$USR@$IP" "cd $DIR && ./dawpol3000"
