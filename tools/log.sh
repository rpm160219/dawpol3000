#!/bin/sh
# Podglad logu z konsoli: ./tools/log.sh
. "$(dirname "$0")/ssh_env.sh"
dssh "cat /userdata/dawpol3000/log.txt"
