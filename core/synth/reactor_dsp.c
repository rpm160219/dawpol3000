/* Opakowanie reactor.c (C, bez zmian). Stan: tablica glosow rx_voice voices[6] i licznik rx_blocks w zmiennych pliku;
 * kazda instancja ma wlasne kopie, przed wywolaniem wstawiamy stan tego glosu (render i note_on dotykaja tylko
 * swojego), po wywolaniu zabieramy. Wywolania z jednego watku audio. */
#include "synth/reactor_dsp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

const int32_t* rxw_pitch_ptr = 0;
int rxw_vel = 100;
int16_t rxw_sine[8192];

#define OP1_HOST 1  /* sciezka hosta w kodzie ekranu oryginalu (nieuzywany) */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "../../third_party/op1r_reactor/plugins/reactor/reactor.c"
#pragma GCC diagnostic pop

struct ReactorCore {
    rx_voice v[OP1_VOICES];
    uint32_t blocks;
};

static void enter(struct ReactorCore* p, int vi) { voices[vi] = p->v[vi]; rx_blocks = p->blocks; }
static void leave(struct ReactorCore* p, int vi) { p->v[vi] = voices[vi]; p->blocks = rx_blocks; }

struct ReactorCore* reactorCreate(void) {
    if (!rxw_sine[2048])
        for (int i = 0; i < 8192; i++) rxw_sine[i] = (int16_t)lrint(32767.0 * sin(2 * M_PI * i / 8192.0));
    struct ReactorCore* p = (struct ReactorCore*)calloc(1, sizeof *p);  /* jak statyczne zmienne oryginalu */
    return p;
}

void reactorNoteOn(struct ReactorCore* p, int voice, const int16_t* knobs, int vel) {
    enter(p, voice);
    rxw_vel = vel;
    ENGINE_reactor.note_on((void*)(uintptr_t)voice, knobs);
    leave(p, voice);
}

void reactorNoteOff(struct ReactorCore* p, int voice) {
    if (!p->v[voice].gate) return;
    enter(p, voice);
    ENGINE_reactor.note_off((void*)(uintptr_t)voice);
    leave(p, voice);
}

void reactorRender(struct ReactorCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel) {
    enter(p, voice);
    rxw_pitch_ptr = (const int32_t*)pitch;
    rxw_vel = vel;
    ENGINE_reactor.render((void*)(uintptr_t)voice, out, knobs);
    leave(p, voice);
}

void reactorView(const struct ReactorCore* p, struct ReactorView* v) {
    v->blocks = p->blocks;
    for (int i = 0; i < OP1_VOICES; i++) {
        v->temp[i] = p->v[i].temp;
        v->crit[i] = p->v[i].crit;
        v->flare[i] = p->v[i].flare;
        v->click[i] = p->v[i].run.click;
        v->last[i] = p->v[i].last;
    }
}
