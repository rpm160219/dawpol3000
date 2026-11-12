/* Opakowanie oxbow_plus.c (OXBOW+, zmodyfikowana kopia oxbow.c; globalne oxw_* wspolne z oxbow_dsp.c). Stan silnika to zmienne pliku (glosy, LFO, wygladzanie galek, pamiec podreczna
 * glosow, wartosci dla ekranu). Kazda instancja ma ich kopie; wrapper trzyma "biezaca" instancje w zmiennych pliku
 * i zamienia je tylko przy przelaczeniu na inna sciezke (glosy sciezki renderuja sie po kolei w jednym bloku).
 * Bufory robocze bloku (tk1, tc1, bmix...) sa wypelniane od nowa przy kazdym renderze glosu — wspolne.
 * Wywolania z jednego watku audio. */
#include "synth/oxbow_plus_dsp.h"
#include "synth/oxbow_dsp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "../../third_party/kennel_oxbow/plugins/oxbow/oxbow_plus.c"
#pragma GCC diagnostic pop

/* zmienne pliku oxbow.c, ktore przenosza stan miedzy blokami */
#define OX_STATE(X) X(vc) X(last_voice) X(osc_mode) X(lfo_ph) X(lfo_rnd_a) X(lfo_rnd_b) X(rnd_state) X(t_lfo_ph) \
    X(t_rnd_a) X(t_rnd_b) X(t_cut) X(cut_sm) X(reso_sm) X(fenv_amt) X(det_fine8) X(drive_q12) X(reso_k) X(w1) X(w2) \
    X(ftype) X(sync_on) X(xmod_q) X(obv_blocks) X(obv_vblk) X(obv_venv) X(block_cache) X(block_key)

struct OxbowPlusCore {
#define OX_FIELD(n) __typeof__(n) n;
    OX_STATE(OX_FIELD)
#undef OX_FIELD
};

static struct OxbowPlusCore initial;  /* stan poczatkowy zmiennych pliku (przed pierwszym uzyciem) */
static int haveInitial;
static struct OxbowPlusCore* current;  /* instancja, ktorej stan jest teraz w zmiennych pliku */

static void save(struct OxbowPlusCore* p) {
#define OX_SAVE(n) memcpy((void*)&p->n, (const void*)&n, sizeof n);
    OX_STATE(OX_SAVE)
#undef OX_SAVE
}
static void load(const struct OxbowPlusCore* p) {
#define OX_LOAD(n) memcpy((void*)&n, (const void*)&p->n, sizeof n);
    OX_STATE(OX_LOAD)
#undef OX_LOAD
}
static void enter(struct OxbowPlusCore* p) {
    if (current == p) return;
    if (current) save(current);
    load(p);
    current = p;
}

struct OxbowPlusCore* oxbowPlusCreate(void) {
    if (!haveInitial) { save(&initial); haveInitial = 1; }
    struct OxbowPlusCore* p = (struct OxbowPlusCore*)malloc(sizeof *p);
    memcpy(p, &initial, sizeof *p);
    return p;
}

void oxbowPlusNoteOn(struct OxbowPlusCore* p, int voice, const int16_t* knobs, int vel, const obx_params* prm) {
    enter(p);
    obx = *prm;
    oxw_vel = vel;
    ENGINE_oxbow_plus.note_on((void*)(uintptr_t)voice, knobs);
}

void oxbowPlusNoteOff(struct OxbowPlusCore* p, int voice) {
    enter(p);
    if (vc[voice].gate) ENGINE_oxbow_plus.note_off((void*)(uintptr_t)voice);
}

void oxbowPlusRender(struct OxbowPlusCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel,
                     const obx_params* prm) {
    enter(p);
    obx = *prm;
    oxw_pitch_ptr = (const int32_t*)pitch;
    oxw_vel = vel;
    ENGINE_oxbow_plus.render((void*)(uintptr_t)voice, out, knobs);
}

void oxbowPlusView(const struct OxbowPlusCore* p, struct OxbowView* v) {
    /* odczyt z watku UI: biezaca instancja ma stan w zmiennych pliku, pozostale w swoich kopiach */
    if (p == current) {
        v->blocks = obv_blocks;
        for (int i = 0; i < 6; i++) { v->vblk[i] = obv_vblk[i]; v->venv[i] = obv_venv[i]; }
    } else {
        v->blocks = p->obv_blocks;
        for (int i = 0; i < 6; i++) { v->vblk[i] = p->obv_vblk[i]; v->venv[i] = p->obv_venv[i]; }
    }
}

/* czasy obwiedni filtra: krok obwiedni 2 razy na tick sterowania, 4 ticki na blok, ~750 blokow/s przy 96 kHz */
#define OBX_STEPS_PER_S 6000.0
static double stepsTo(double k, double frac) { return k <= 0 ? 1e9 : log(frac) / log(1.0 - k / 16777216.0); }
uint32_t oxbowPlusAttackIndex(float seconds) {
    uint32_t best = 0;
    double bd = 1e18;
    for (uint32_t i = 0; i < 128; i++) {  /* atak dazy do 1,375 i staje na 1,0: zostaje 0,375 / 1,375 */
        const double t = stepsTo(ENV_K_ATT[i], 0.375 / 1.375) / OBX_STEPS_PER_S, d = fabs(t - seconds);
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}
uint32_t oxbowPlusDecayIndex(float seconds) {
    uint32_t best = 0;
    double bd = 1e18;
    for (uint32_t i = 0; i < 128; i++) {  /* jak nasze ADSR: czas = 5 stalych czasowych (~ -43 dB) */
        const double t = stepsTo(ENV_K_DEC[i], 0.0067) / OBX_STEPS_PER_S, d = fabs(t - seconds);
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}
float oxbowPlusAttackSeconds(uint32_t idx) { return (float)(stepsTo(ENV_K_ATT[idx & 127], 0.375 / 1.375) / OBX_STEPS_PER_S); }
float oxbowPlusDecaySeconds(uint32_t idx) { return (float)(stepsTo(ENV_K_DEC[idx & 127], 0.0067) / OBX_STEPS_PER_S); }
