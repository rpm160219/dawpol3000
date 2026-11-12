#!/bin/bash
# GIN: zgodnosc bit w bit z oryginalem (gin_fw.c kennel + slownik wspolczynnikow z build_factory_data.py).
set -e
cd "$(dirname "$0")/.."
T=$(mktemp -d)
V=third_party/kennel_gin/gin/vendor
python3 -c "import sys;sys.path.insert(0,'$V/tools/tonic/sdk-drum');import build_factory_data as b;b.ROOT=__import__('pathlib').Path('$V').resolve();b.build('$T/factory')" >/dev/null
python3 - "$T" <<'PY'
import sys,struct
t=sys.argv[1]
d=open(t+'/factory/factory.dictionary.bin','rb').read();x=open(t+'/factory/factory.index12.bin','rb').read()
w=struct.unpack('<%dI'%(len(d)//4),d)
with open(t+'/gin_factory_data.h','w') as f:
  f.write('const uint32_t gin_factory_prepared_dictionary[]={'+','.join('%uu'%v for v in w)+'};\n')
  f.write('const uint8_t gin_factory_prepared_index[]={'+','.join(str(b) for b in x)+'};\n')
PY
DEFS="-DGIN_FACTORY_BANK -DGIN_FAST_FIXED=1 -DGIN_AUDIO_OBSERVATION=0 -DGIN_WORKING_SLOTS=1 -DGIN_PROFILE_ENABLED=0 -DGIN_PARAMS_ONLY=0 -DGIN_FW_OUTPUT_SHIFT=14 -DGIN_FW_OUTPUT_LEVEL_Q12=4596 -DGIN_SINE_QUARTER=0 -DGIN_SYNTH_VOICE_LIMIT=4 -DGIN_RATE_2X=1 -DGIN_BLOCK_STATE=1 -DGIN_FACTORY_PACKED_INDEX=1 -fno-tree-loop-distribute-patterns -ffp-contract=off"
INC="-I$V/sdk/synth_sdk -I$V/sdk/synth_sdk/plugins/gin -I$V/src/tonic/sdk_drum -I$V/src/tonic/ui -I$V/src/tonic/ui_native -I$V/src/tonic/integer -I$V/src/tonic -I$V/src/tonic/presets -I$V/third_party/libpo32/core/include -I$V/third_party/libpo32/core/src"
SRC="$V/sdk/synth_sdk/plugins/gin/gin_adapter.c $V/sdk/synth_sdk/plugins/gin/gin_dsp.c $V/src/tonic/sdk_drum/gin_resample.c $V/src/tonic/integer/gin_prepare.c $V/src/tonic/tonic_voice.c $V/third_party/libpo32/core/src/po32_synth.c $V/src/tonic/presets/gin_factory.c $V/src/tonic/factory_bank/gin_factory_bank.c"
# referencja: oryginalny gin_fw.c (O2 jak w kennel)
gcc -std=gnu11 -O2 -w -DOP1_HOST $DEFS $INC -I"$T" tests/gin_ref.c $V/src/tonic/sdk_drum/gin_fw.c $SRC -lm -o "$T/ref"
"$T/ref" "$T/ref.raw"
# DAW-POL: wrapper z przygotowaniem w locie
for s in $SRC core/synth/gin_dsp.c; do gcc -std=gnu11 -O2 -w $DEFS $INC -Icore -c $s -o "$T/$(echo $s | tr / _).o"; done
g++ -std=c++17 -O2 -Icore tests/gin_match.cpp "$T"/*.o -lm -o "$T/match"
"$T/match" "$T/ref.raw"
rm -rf "$T"
