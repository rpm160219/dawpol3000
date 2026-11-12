/* Opakowanie eva.c (C, bez zmian). Stan silnika: hot_t pod wskaznikiem ev_h (oryginal sam go przelacza — pula L1 na
 * OP-1), plus dwie male struktury ev_q (sygnal Quindar, trzymane klawisze) i ev_m (wartosci dla ekranu). Kazda
 * instancja ma wlasne kopie; przed wywolaniem wstawiamy je, po wywolaniu zabieramy. Wywolania z jednego watku audio. */
#include "synth/eva_dsp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

const int32_t* eva_pitch_ptr = 0;
int eva_vel_now = 100;
const int16_t eva_sine_table[8192] = {0};  /* tylko ekran oryginalu (nieuzywany) */
uint8_t op1_eng_page[4];

#define ev_home ev_home_orig   /* wersja z pliku = wzor stanu poczatkowego */
#include "../../third_party/op1r_eva/eva.c"
#undef ev_home
#undef held  /* makra eva.c, ktore kolidowalyby z polami kopii */

struct EvaCore {
    hot_t hot;
    __typeof__(ev_q) q;
    __typeof__(ev_m) m;
};

static void enter(struct EvaCore* p) {
    ev_h = &p->hot;
    memcpy(&ev_q, &p->q, sizeof ev_q);
    memcpy((void*)&ev_m, (const void*)&p->m, sizeof ev_m);
}
static void leave(struct EvaCore* p) {
    memcpy(&p->q, &ev_q, sizeof ev_q);
    memcpy((void*)&p->m, (const void*)&ev_m, sizeof ev_m);
    ev_h = &ev_home_orig;
}

struct EvaCore* evaCreate(void) {
    static int init;
    static __typeof__(ev_q) q0;
    static __typeof__(ev_m) m0;
    if (!init) { memcpy(&q0, &ev_q, sizeof q0); memcpy((void*)&m0, (const void*)&ev_m, sizeof m0); init = 1; }
    struct EvaCore* p = (struct EvaCore*)aligned_alloc(32, (sizeof(struct EvaCore) + 31) & ~(size_t)31);
    memcpy(&p->hot, &ev_home_orig, sizeof p->hot);
    memcpy(&p->q, &q0, sizeof q0);
    memcpy((void*)&p->m, (const void*)&m0, sizeof m0);
    return p;
}

void evaNoteOn(struct EvaCore* p, int voice, const int16_t* knobs, int vel) {
    enter(p);
    eva_vel_now = vel;
    ENGINE_eva.note_on((void*)(uintptr_t)voice, knobs);
    leave(p);
}

void evaNoteOff(struct EvaCore* p, int voice) {
    if (!p->q.held[voice]) return;
    enter(p);
    ENGINE_eva.note_off((void*)(uintptr_t)voice);
    leave(p);
}

int evaHeld(const struct EvaCore* p, int voice) { return p->q.held[voice]; }

void evaRender(struct EvaCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel) {
    enter(p);
    eva_pitch_ptr = (const int32_t*)pitch;
    eva_vel_now = vel;
    ENGINE_eva.render((void*)(uintptr_t)voice, out, knobs);
    leave(p);
}

void evaView(const struct EvaCore* p, struct EvaView* v) {
    v->blocks = p->m.blocks;
    v->breath = p->m.breath;
    v->gate = p->m.gate;
    v->beep = p->m.beep;
}
