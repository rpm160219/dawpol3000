/* Opakowanie monolit.c (C, bez zmian). Stan: glosy br_home.v[6] i prosby o wybrzmienie br_rel[6] w zmiennych pliku;
 * kazda instancja ma wlasne kopie, przed wywolaniem wstawiamy stan tego glosu (render i note_on dotykaja tylko swojego;
 * bufory robocze bloku sa wypelniane od nowa), po wywolaniu zabieramy. Pamiec podreczna galek (br_kc) zalezy tylko od
 * galek — wspolna. Wywolania z jednego watku audio. */
#include "synth/monolit_dsp.h"
#include <stdlib.h>
#include <string.h>

const int32_t* mnw_pitch_ptr = 0;
int mnw_vel = 100;
const int16_t mnw_sine[8192] = {0};  /* tylko ekran oryginalu (nieuzywany) */
uint8_t mnw_eng_page[4];

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "../../third_party/op1r_monolit/monolit.c"
#pragma GCC diagnostic pop

struct MonolitCore { br_voice v[OP1_VOICES]; int32_t rel[OP1_VOICES]; int32_t env; };

static void enter(struct MonolitCore* p, int vi) { br_home.v[vi] = p->v[vi]; br_rel[vi] = p->rel[vi]; }
static void leave(struct MonolitCore* p, int vi) { p->v[vi] = br_home.v[vi]; p->rel[vi] = br_rel[vi]; }

struct MonolitCore* monolitCreate(void) {
    struct MonolitCore* p;
    if (posix_memalign((void**)&p, 32, sizeof *p)) return 0;
    memset(p, 0, sizeof *p);  /* jak statyczne zmienne oryginalu */
    return p;
}

void monolitNoteOn(struct MonolitCore* p, int voice, const int16_t* knobs) {
    enter(p, voice);
    ENGINE_monolit.note_on((void*)(uintptr_t)voice, knobs);
    leave(p, voice);
}

void monolitNoteOff(struct MonolitCore* p, int voice) {
    if (p->rel[voice]) return;
    enter(p, voice);
    ENGINE_monolit.note_off((void*)(uintptr_t)voice);
    leave(p, voice);
}

void monolitRender(struct MonolitCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel) {
    enter(p, voice);
    mnw_pitch_ptr = (const int32_t*)pitch;
    mnw_vel = vel;
    ENGINE_monolit.render((void*)(uintptr_t)voice, out, knobs);
    p->env = monolit_env;
    leave(p, voice);
}

int32_t monolitEnv(const struct MonolitCore* p) { return p->env; }
