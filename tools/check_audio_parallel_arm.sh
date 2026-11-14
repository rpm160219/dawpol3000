#!/bin/sh
# Focused ARM regression against the approved serial orchestration + speed test.
set -eu
cd "$(dirname "$0")/.."
AUDIO_CHECK_LOCAL=$(mktemp -d)
trap 'rm -rf "$AUDIO_CHECK_LOCAL"' EXIT
./tools/build.sh arm
python3 - "$AUDIO_CHECK_LOCAL" <<'PY'
from pathlib import Path
import subprocess,sys
root=Path(sys.argv[1])/'approved'/'audio';root.mkdir(parents=True)
for file in ['engine.cpp','engine.h']:
    s=subprocess.check_output(['git','show','fdee48c:core/audio/'+file],text=True)
    if file=='engine.cpp':s=s.replace('AudioEngine::init(float sr)','AudioEngine::init(float sr,int)')
    else:s=s.replace('void init(float sampleRate);','void init(float sampleRate,int=1);\n    int renderThreads()const{return 1;}')
    (root/file).write_text(s)
PY
aarch64-linux-gnu-g++ -std=c++17 -O2 -mcpu=cortex-a53 -Icore tests/audio_workers.cpp core/audio/track_workers.cpp -pthread -lm -o "$AUDIO_CHECK_LOCAL/workers"
aarch64-linux-gnu-g++ -std=c++17 -O2 -mcpu=cortex-a53 -Icore tests/audio_parallel.cpp build-arm/libdpcore.a -pthread -lm -o "$AUDIO_CHECK_LOCAL/current"
aarch64-linux-gnu-g++ -std=c++17 -O2 -mcpu=cortex-a53 -I"$AUDIO_CHECK_LOCAL/approved" -Icore tests/audio_parallel.cpp "$AUDIO_CHECK_LOCAL/approved/audio/engine.cpp" build-arm/libdpcore.a -pthread -lm -o "$AUDIO_CHECK_LOCAL/approved-engine"
. ./tools/ssh_env.sh
trap 'rm -rf "$AUDIO_CHECK_LOCAL"; rm -f "$PWHELPER"' EXIT
AUDIO_CHECK_REMOTE=$(dssh 'mktemp -d /tmp/dawpol-audio.XXXXXX')
dscp "$AUDIO_CHECK_LOCAL/workers" "$AUDIO_CHECK_LOCAL/current" "$AUDIO_CHECK_LOCAL/approved-engine" "$USR@$IP:$AUDIO_CHECK_REMOTE/"
dssh "cd '$AUDIO_CHECK_REMOTE' && timeout 180 ./workers && timeout 180 ./approved-engine render 1 approved.raw && timeout 180 ./current render 1 serial.raw && cmp approved.raw serial.raw && timeout 180 ./current render 4 parallel.raw && cmp serial.raw parallel.raw && sha256sum approved.raw serial.raw parallel.raw && timeout 180 ./current bench 1 && timeout 180 ./current bench 4 && timeout 180 ./current pace 4"
dssh "rm -rf '$AUDIO_CHECK_REMOTE'"
rm -rf "$AUDIO_CHECK_LOCAL"
