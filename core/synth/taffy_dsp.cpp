// Osobna jednostka kompilacji: taffy.c ma wlasne statyczne nazwy (voices, cosq, setup...), tu nie koliduja z niczym.
#include "synth/taffy_dsp.h"

extern "C++" {
const uint32_t* tf_pitch_ptr = nullptr;
int tf_vel_now = 100;
}

#define ENGINE_HOST_TEST  // poziom glosu dla ekranu bez rejestrow firmware
#define engine_note_on taffy_note_on
#define engine_render taffy_render
#define engine_note_off taffy_note_off
#ifndef TF_DSP_SOURCE
#define TF_DSP_SOURCE "../../third_party/op1r_taffy/taffy.c"
#endif
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include TF_DSP_SOURCE
#pragma GCC diagnostic pop

namespace dp {

static int gNext = 0;

int taffyAllocVoices(int n) {
    int base = gNext;
    gNext += n;
    if (gNext > TAFFY_MAX_VOICES) { gNext = n; base = 0; }  // (nie powinno sie zdarzyc: 20 sciezek x 6 glosow)
    return base;
}

void taffyNoteOn(int voice) {
    static const int16_t k[8] = {0};
    taffy_note_on((void*)(uintptr_t)voice, k);
}

void taffyRender(int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel) {
    tf_pitch_ptr = pitch;
    tf_vel_now = vel;
    taffy_render((void*)(uintptr_t)voice, out, knobs);
}

int16_t taffyVoiceEnv(int voice) { return taffy_venv[voice]; }

void taffyWaveCycle(int16_t* y, int n, int32_t k0, int32_t dcw) { taffy_wave_cycle(y, n, k0, dcw); }

}  // namespace dp
