/* Opakowanie mixtur.c (C, bez zmian; petle C z mixtur_dsp.h jak w buildach hosta — jadra Blackfin .S pominiete).
 * Pamiec glosu oryginalu jest pod "self" (slot 0x24 bajtow jak w firmware) — u nas w instancji sciezki. Wspolna tablica
 * sygnatur own[6] (rozpoznaje glos nadpisany przez inny silnik) podmieniana per instancja przed / po wywolaniu.
 * Bufory robocze bloku (mx_b1, mx_b2, mx_xbuf) wypelniane od nowa w kazdym wywolaniu — wspolne. */
#include "synth/mixtur_dsp.h"
#include <stdlib.h>
#include <string.h>

const int32_t* mxw_pitch_ptr = 0;
int mxw_vel = 100;
const void* mxw_base = 0;
const void* mxw_view = 0;
uint8_t mxw_eng_page[4];
const int16_t* mxw_knobs = 0;
const int16_t* mxw_live = 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "../../third_party/op1r_mixtur/mixtur.c"
#pragma GCC diagnostic pop

struct MixturCore {
    uint32_t mem[OP1_VOICES][OP1_VOICE_SIZE / 4];  /* pamiec glosow (wyrownana jak w firmware) */
    uint32_t own[OP1_VOICES];
};

static void enter(struct MixturCore* p) { mxw_base = p->mem; memcpy(own, p->own, sizeof own); }
static void leave(struct MixturCore* p) { memcpy(p->own, own, sizeof own); }

struct MixturCore* mixturCreate(void) { return (struct MixturCore*)calloc(1, sizeof(struct MixturCore)); }
void mixturNoteOn(struct MixturCore* p, int voice, const int16_t* knobs) {
    enter(p);
    ENGINE_mixtur.note_on(p->mem[voice], knobs);
    leave(p);
}
void mixturNoteOff(struct MixturCore* p, int voice) {
    if (!(((const mx_voice_t*)p->mem[voice])->n[0] & GATE)) return;
    ENGINE_mixtur.note_off(p->mem[voice]);
}
void mixturRender(struct MixturCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel) {
    enter(p);
    mxw_pitch_ptr = (const int32_t*)pitch;
    mxw_vel = vel;
    ENGINE_mixtur.render(p->mem[voice], out, knobs);
    leave(p);
}

int mixturKnobN(int16_t k) { return knob_n(k); }
static int16_t pw(int16_t w) { int16_t r[8] = {0}; r[5] = w; return (int16_t)page_word(r, 5); }
int mixturF1Hz(int16_t w) { return curve(F1_HZ, pw(w)); }
int mixturF2Hz(int16_t w) { return curve(F2_HZ, pw(w)); }
int mixturResQ10(int16_t w) { return curve(RES_Q10, pw(w)); }

/* ---- ekran ---- */
static void* cb_user;
static MxwLineFn cb_line;
static MxwArtFn cb_art;
static const uint32_t idle_mem[OP1_VOICES][OP1_VOICE_SIZE / 4];
void mxw_line(void* r, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, const uint8_t* c) { cb_line(r, x0, y0, x1, y1, w, c); }
void mxw_screen_draw(void* scr, int dt) { (void)scr; (void)dt; cb_art(cb_user); }
void* mxw_raster(void) { return cb_user; }

void mixturScreenUpdate(void* user, MxwLineFn line_fn, MxwArtFn art_fn, const struct MixturCore* core,
                        const int16_t knobs[8], const int16_t live[4], int page2, int dt_ms) {
    cb_user = user; cb_line = line_fn; cb_art = art_fn;
    mxw_view = core ? (const void*)core->mem : (const void*)idle_mem;
    mxw_knobs = knobs;
    mxw_live = live;
    for (int i = 0; i < 3; i++) mxw_eng_page[i] = (uint8_t)(page2 != 0);
    mxw_eng_page[3] = 0;
    SCREEN_mixtur.update(0, dt_ms);
}
