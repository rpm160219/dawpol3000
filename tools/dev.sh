#!/bin/sh
# Dowolna komenda na konsoli: ./tools/dev.sh 'komenda'
. "$(dirname "$0")/ssh_env.sh"
dssh "$@"
