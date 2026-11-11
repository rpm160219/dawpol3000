# Wspolne ustawienia SSH do konsoli (dolaczane przez inne skrypty).
# Knulli trzyma /userdata na exFAT -> Dropbear odrzuca klucze, wiec haslo z tools/device.conf.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
[ -f "$ROOT/tools/device.conf" ] && . "$ROOT/tools/device.conf"
IP=${IP:-$DEVICE_IP}; USR=${DEVICE_USER:-root}
[ -z "$IP" ] && { echo "Brak IP konsoli: ./tools/deploy.sh 192.168.x.x"; exit 1; }
PWHELPER="${TMPDIR:-/tmp}/dawpol_askpass_$$"
printf '#!/bin/sh\necho "%s"\n' "${DEVICE_PASS:-linux}" > "$PWHELPER"; chmod 700 "$PWHELPER"
trap 'rm -f "$PWHELPER"' EXIT
export SSH_ASKPASS="$PWHELPER" SSH_ASKPASS_REQUIRE=force
SSHOPT="-o ConnectTimeout=8 -o PubkeyAuthentication=no -o ControlMaster=auto -o ControlPath=${TMPDIR:-/tmp}/dawpol_ssh_%h -o ControlPersist=60"
dssh() { ssh $SSHOPT "$USR@$IP" "$@"; }
dscp() { scp $SSHOPT "$@"; }
