/* Opakowanie oxbow.c (C, bez zmian). Stan silnika to zmienne pliku (glosy, LFO, wygladzanie galek, pamiec podreczna
 * glosow, wartosci dla ekranu). Kazda instancja ma ich kopie; wrapper trzyma "biezaca" instancje w zmiennych pliku
 * i zamienia je tylko przy przelaczeniu na inna sciezke (glosy sciezki renderuja sie po kolei w jednym bloku).
 * Bufory robocze bloku (tk1, tc1, bmix...) sa wypelniane od nowa przy kazdym renderze glosu — wspolne.
 * Wywolania z jednego watku audio. */
#include "synth/oxbow_dsp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

const int32_t* oxw_pitch_ptr = 0;
int oxw_vel = 100;
const int16_t oxw_sine[8192] = {0};  /* tylko ekran oryginalu (nieuzywany) */
uint8_t oxw_eng_page[4];

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "../../third_party/kennel_oxbow/plugins/oxbow/oxbow.c"
#pragma GCC diagnostic pop

/* zmienne pliku oxbow.c, ktore przenosza stan miedzy blokami */
#define OX_STATE(X) X(vc) X(last_voice) X(osc_mode) X(lfo_ph) X(lfo_rnd_a) X(lfo_rnd_b) X(rnd_state) X(t_lfo_ph) \
    X(t_rnd_a) X(t_rnd_b) X(t_cut) X(cut_sm) X(reso_sm) X(fenv_amt) X(det_fine8) X(drive_q12) X(reso_k) X(w1) X(w2) \
    X(ftype) X(sync_on) X(xmod_q) X(obv_blocks) X(obv_vblk) X(obv_venv) X(block_cache) X(block_key)

struct OxbowCore {
#define OX_FIELD(n) __typeof__(n) n;
    OX_STATE(OX_FIELD)
#undef OX_FIELD
};

static struct OxbowCore initial;  /* stan poczatkowy zmiennych pliku (przed pierwszym uzyciem) */
static int haveInitial;
static struct OxbowCore* current;  /* instancja, ktorej stan jest teraz w zmiennych pliku */

static void save(struct OxbowCore* p) {
#define OX_SAVE(n) memcpy((void*)&p->n, (const void*)&n, sizeof n);
    OX_STATE(OX_SAVE)
#undef OX_SAVE
}
static void load(const struct OxbowCore* p) {
#define OX_LOAD(n) memcpy((void*)&n, (const void*)&p->n, sizeof n);
    OX_STATE(OX_LOAD)
#undef OX_LOAD
}
static void enter(struct OxbowCore* p) {
    if (current == p) return;
    if (current) save(current);
    load(p);
    current = p;
}

struct OxbowCore* oxbowCreate(void) {
    if (!haveInitial) { save(&initial); haveInitial = 1; }
    struct OxbowCore* p = (struct OxbowCore*)malloc(sizeof *p);
    memcpy(p, &initial, sizeof *p);
    return p;
}

void oxbowNoteOn(struct OxbowCore* p, int voice, const int16_t* knobs, int vel) {
    enter(p);
    oxw_vel = vel;
    ENGINE_oxbow.note_on((void*)(uintptr_t)voice, knobs);
}

void oxbowNoteOff(struct OxbowCore* p, int voice) {
    enter(p);
    if (vc[voice].gate) ENGINE_oxbow.note_off((void*)(uintptr_t)voice);
}

void oxbowRender(struct OxbowCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel) {
    enter(p);
    oxw_pitch_ptr = (const int32_t*)pitch;
    oxw_vel = vel;
    ENGINE_oxbow.render((void*)(uintptr_t)voice, out, knobs);
}

void oxbowView(const struct OxbowCore* p, struct OxbowView* v) {
    /* odczyt z watku UI: biezaca instancja ma stan w zmiennych pliku, pozostale w swoich kopiach */
    if (p == current) {
        v->blocks = obv_blocks;
        for (int i = 0; i < 6; i++) { v->vblk[i] = obv_vblk[i]; v->venv[i] = obv_venv[i]; }
    } else {
        v->blocks = p->obv_blocks;
        for (int i = 0; i < 6; i++) { v->vblk[i] = p->obv_vblk[i]; v->venv[i] = p->obv_venv[i]; }
    }
}
