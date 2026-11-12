// Opakowanie pew.c: oryginalny kod wklejony do ciala klasy (statyczne zmienne pliku staja sie polami),
// zeby kazda sciezka miala wlasne organy i wlasny Leslie. Kod pew.c bez zmian; zmienia go tylko
// "#define static" na czas wklejenia. Tablice tylko do odczytu (pw_sin, rejestracje) sa polami const.
#include "synth/pew_dsp.h"
#include "../../third_party/op1r_pew/hk/engine.h"

namespace dp {

struct PewCore {
    const uint32_t* pitch_ = nullptr;
    int vel_ = 100;
#define static
#define engine_note_on note_on
#define engine_note_off note_off
#define engine_render render
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "../../third_party/op1r_pew/pew.c"
#pragma GCC diagnostic pop
#undef engine_note_on
#undef engine_note_off
#undef engine_render
#undef static
};

PewCore* pewCreate() { return new PewCore(); }

void pewNoteOn(PewCore* p, int voice) {
    static const int16_t k[8] = {0};
    p->note_on((void*)(uintptr_t)voice, k);
}

void pewNoteOff(PewCore* p, int voice) {
    static const int16_t k[8] = {0};
    if (p->voices[voice].gate) p->note_off((void*)(uintptr_t)voice, k);
}

void pewRender(PewCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel) {
    p->pitch_ = pitch;
    p->vel_ = vel;
    p->render((void*)(uintptr_t)voice, out, knobs);
}

void pewView(const PewCore* p, PewView* v) {
    v->blocks = p->pew_blocks;
    v->horn = p->pew_horn;
    v->drum = p->pew_drum;
    v->strikes = p->pew_strikes;
    v->speed = p->pew_speed;
    for (int i = 0; i < 6; i++) v->held[i] = p->pew_held[i];
}

}  // namespace dp
