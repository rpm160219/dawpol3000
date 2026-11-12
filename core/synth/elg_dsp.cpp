// Opakowanie elg.c: oryginalny kod wklejony do ciala klasy (zmienne pliku staja sie polami), zeby kazda
// sciezka miala wlasne glosy i wlasne wartosci dla ekranu. Kod elg.c bez zmian; "#define static" tylko na czas wklejenia.
#include "synth/elg_dsp.h"
#include "../../third_party/op1r_elg/hk/engine.h"

namespace dp {

struct ElgCore {
    const uint32_t* pitch_ = nullptr;
    int vel_ = 100;
#define static
#define engine_note_on note_on
#define engine_note_off note_off
#define engine_render render
#include "../../third_party/op1r_elg/elg.c"
#undef engine_note_on
#undef engine_note_off
#undef engine_render
#undef static
};

ElgCore* elgCreate() { return new ElgCore(); }

void elgNoteOn(ElgCore* p, int voice) {
    static const int16_t k[8] = {0};
    p->note_on((void*)(uintptr_t)voice, k);
}

void elgRender(ElgCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel) {
    p->pitch_ = pitch;
    p->vel_ = vel;
    p->render((void*)(uintptr_t)voice, out, knobs);
}

void elgView(const ElgCore* p, ElgView* v) {
    v->blocks = p->elg_blocks;
    v->level = p->elg_level;
    v->lfo = p->elg_lfo;
    for (int i = 0; i < 4; i++) v->knobs[i] = p->elg_knobs[i];
}

int elgInterval(int32_t value) {
    static ElgCore* c = new ElgCore();  // funkcja bez stanu; instancja tylko jako "przestrzen nazw"
    return c->elg_interval(value);
}

}  // namespace dp
