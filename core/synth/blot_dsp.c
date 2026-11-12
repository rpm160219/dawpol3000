/* Opakowanie blot.c (C, bez zmian; petla C oscylatora — jadro Blackfin blot_dsp.S pominiete). Stan glosu pod "self"
 * (slot jak pamiec glosu OP-1) w instancji sciezki. Poziom obwiedni filtra dla ekranu (ac_level[6], wspolny w
 * oryginale) kopiowany do instancji po kazdym bloku; ekran wczytuje go z instancji ogladanej sciezki. */
#include "synth/blot_dsp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

const int32_t* blw_pitch_ptr = 0;
int blw_vel = 100;
int16_t blw_sine[8192];
const void* blw_base = 0;
const void* blw_view = 0;
const int16_t* blw_knobs = 0;
const int16_t* blw_live = 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "../../third_party/op1r_blot/blot.c"
#pragma GCC diagnostic pop

struct BlotCore { uint32_t mem[OP1_VOICES][OP1_VOICE_SIZE / 4]; volatile int16_t level[OP1_VOICES]; };

static void sineInit(void) {
    if (blw_sine[2048]) return;
    for (int i = 0; i < 8192; i++) blw_sine[i] = (int16_t)lrint(32767.0 * sin(2 * M_PI * i / 8192.0));  /* jak OP-1 */
}

struct BlotCore* blotCreate(void) {
    sineInit();
    return (struct BlotCore*)calloc(1, sizeof(struct BlotCore));
}
void blotNoteOn(struct BlotCore* p, int voice, const int16_t* knobs) { ENGINE_blot.note_on(p->mem[voice], knobs); }
void blotNoteOff(struct BlotCore* p, int voice) {
    if (((const ac_voice_t*)p->mem[voice])->gate) ENGINE_blot.note_off(p->mem[voice]);
}
void blotRender(struct BlotCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel) {
    blw_base = p->mem;
    blw_pitch_ptr = (const int32_t*)pitch;
    blw_vel = vel;
    ENGINE_blot.render(p->mem[voice], out, knobs);
    p->level[voice] = ac_level[voice];
}
int blotDecayMs(int16_t k) {
    const int32_t kd = k < 0 ? 0 : k >> 11, kf = k < 0 ? 0 : k & 2047;
    return ac_tau_ms[kd] + (((ac_tau_ms[kd + (kd < 16)] - ac_tau_ms[kd]) * kf) >> 11);
}

/* ---- ekran ---- */
static void* cb_user;
static BlwLineFn cb_line;
static const uint32_t idle_mem[OP1_VOICES][OP1_VOICE_SIZE / 4];
void blw_screen_draw(void* scr, int dt) { (void)scr; (void)dt; }  /* blot.svg jest pusty */
void* blw_raster(void) { return cb_user; }
void blw_raster_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* c) {
    /* dolny rzad oryginalu (kolorowe kreski nad napisami galek, y 110): pominiety jak napisy */
    if (a->y == OP1_Q10(110) && b->y == OP1_Q10(110) && w == 0x800) return;
    cb_line(r, a->x, a->y, b->x, b->y, w, c);
}

void blotScreenUpdate(void* user, BlwLineFn line_fn, const struct BlotCore* core, const int16_t knobs[8],
                      const int16_t live[4], int dt_ms) {
    sineInit();
    cb_user = user; cb_line = line_fn;
    blw_view = core ? (const void*)core->mem : (const void*)idle_mem;
    for (int i = 0; i < OP1_VOICES; i++) ac_level[i] = core ? core->level[i] : 0;
    blw_knobs = knobs;
    blw_live = live;
    SCREEN_blot.update(0, dt_ms);
}
