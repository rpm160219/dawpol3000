#!/bin/sh
# Podglad logu z konsoli: ./tools/log.sh
cd "$(dirname "$0")/.."
. tools/device.conf
ssh "${DEVICE_USER:-root}@$DEVICE_IP" "cat /userdata/dawpol3000/log.txt"
