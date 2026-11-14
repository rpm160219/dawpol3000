# Wspolne dla skryptow testowych: obiekty silnikow GIN, PARALLAX, LOUNGE, FELT i GRAND w "$T" (gin_*.o). Flagi GIN jak w CMakeLists.txt (plugin.json).
GV=third_party/kennel_gin/gin/vendor
GIN_DEFS="-DGIN_FACTORY_BANK -DGIN_FAST_FIXED=1 -DGIN_AUDIO_OBSERVATION=0 -DGIN_WORKING_SLOTS=1 -DGIN_PROFILE_ENABLED=0 -DGIN_PARAMS_ONLY=0 -DGIN_FW_OUTPUT_SHIFT=14 -DGIN_FW_OUTPUT_LEVEL_Q12=4596 -DGIN_SINE_QUARTER=0 -DGIN_SYNTH_VOICE_LIMIT=4 -DGIN_RATE_2X=1 -DGIN_BLOCK_STATE=1 -DGIN_FACTORY_PACKED_INDEX=1 -fno-tree-loop-distribute-patterns -ffp-contract=off"
GIN_INC="-I$GV/sdk/synth_sdk -I$GV/sdk/synth_sdk/plugins/gin -I$GV/src/tonic/sdk_drum -I$GV/src/tonic/ui -I$GV/src/tonic/ui_native -I$GV/src/tonic/integer -I$GV/src/tonic -I$GV/src/tonic/presets -I$GV/third_party/libpo32/core/include -I$GV/third_party/libpo32/core/src"
for s in $GV/sdk/synth_sdk/plugins/gin/gin_adapter.c $GV/sdk/synth_sdk/plugins/gin/gin_dsp.c $GV/src/tonic/sdk_drum/gin_resample.c $GV/src/tonic/integer/gin_prepare.c $GV/src/tonic/tonic_voice.c $GV/third_party/libpo32/core/src/po32_synth.c $GV/src/tonic/presets/gin_factory.c $GV/src/tonic/factory_bank/gin_factory_bank.c core/synth/gin_dsp.c; do
    gcc -std=gnu11 -O2 -w $GIN_DEFS $GIN_INC -Icore -c "$s" -o "$T/gin_$(echo "$s" | tr / _).o"
done
# PARALLAX (wlasny DSP) — linkowany przez silnik glosow tak jak GIN
gcc -std=gnu11 -O3 -ffast-math -Icore -c core/synth/parallax_dsp.c -o "$T/gin_parallax.o"
gcc -std=gnu11 -O3 -ffast-math -Icore -c core/synth/lounge_dsp.c -o "$T/gin_lounge.o"
gcc -std=gnu11 -O3 -ffast-math -Icore -c core/synth/felt_dsp.c -o "$T/gin_felt.o"
gcc -std=gnu11 -O3 -ffast-math -Icore -c core/synth/grand_dsp.c -o "$T/gin_grand.o"
# VELOUR (oryginal PicoFaceSM, tryb hosta) — wavetable_synth.cpp i project.cpp go wolaja
for s in core/synth/velour_dsp.cpp third_party/pico_solina/src/*.cpp; do
    g++ -std=c++17 -O2 -w -DSOLINA_HOST_BUILD -Icore -Ithird_party/pico_solina/include -c "$s" -o "$T/gin_vl_$(basename "$s" .cpp).o"
done
# LINEA (silnik PicoFaceD5, naglowki) — wavetable_synth.cpp go wola; ROM-y niepotrzebne do linkowania
g++ -std=c++17 -O2 -w -Icore -Ithird_party/pico_d50/include -c core/synth/linea_dsp.cpp -o "$T/gin_linea.o"

# project.cpp now initializes the independent Polivoks preset bank as well.
gcc -std=c11 -O3 -Icore -c core/synth/polivoks_dsp.c -o "$T/gin_polivoks_dsp.o"
gcc -std=c11 -O3 -Icore -c core/synth/polivoks_presets.c -o "$T/gin_polivoks_presets.o"

gcc -std=c11 -O3 -Icore -c core/synth/prophet_dsp.c -o "$T/gin_prophet_dsp.o"
gcc -std=c11 -O3 -Icore -c core/synth/prophet_presets.c -o "$T/gin_prophet_presets.o"
