/* Opakowanie dddd.c + dd_core.c (C, bez zmian). Caly stan oryginalu w zmiennych pliku (sciana dd_t z buforem petli,
 * licznik wywolan, kolejnosc nut, trzymane klawisze, poziom dla ekranu) — kazda instancja (sciezka) ma wlasna kopie,
 * podmieniana leniwie przy zmianie instancji (lista DD_STATE). Ekran: druga kompilacja w core/ui/dddd_screen_c.c. */
#include "synth/dddd_dsp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

const int32_t* ddw_pitch_ptr = 0;
int ddw_vel = 100;
int16_t ddw_env[128];
int16_t dd_sine_tab[8192];
const void* ddw_base = 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "../../third_party/op1r_dddd/plugins/dddd/dddd.c"
#pragma GCC diagnostic pop

/* ekran tej kopii nieuzywany — zaslepki */
volatile uint8_t ddw_eng_page[4];
const int16_t* ddw_knobs;
const int16_t* ddw_live;
void ddw_screen_draw(void* scr, int dt) { (void)scr; (void)dt; }
void* ddw_raster(void) { return 0; }
void ddw_raster_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* c) { (void)r; (void)a; (void)b; (void)w; (void)c; }

#define DD_STATE X(dd) X(calls) X(last_call) X(order) X(order_n) X(voice_inc) X(taps_inc) X(taps_n) X(inited) \
    X(alive_seen) X(held) X(level)

struct DdddCore {
    uint32_t mem[OP1_VOICES][OP1_VOICE_SIZE / 4];
#define X(n) unsigned char s_##n[sizeof(n)];
    DD_STATE
#undef X
    volatile uint32_t jumps;   /* dla ekranu */
    volatile int32_t lev;
};
static struct DdddCore* cur;
static struct DdddCore fresh;
static int fresh_ready;
static void save(struct DdddCore* p) {
#define X(n) memcpy(p->s_##n, (const void*)&n, sizeof(n));
    DD_STATE
#undef X
}
static void load(const struct DdddCore* p) {
#define X(n) memcpy((void*)&n, p->s_##n, sizeof(n));
    DD_STATE
#undef X
}
static void enter(struct DdddCore* p) {
    if (cur != p) { if (cur) save(cur); load(p); cur = p; }
    ddw_base = p->mem;
}

static void sineInit(void) {
    if (dd_sine_tab[2048]) return;
    for (int i = 0; i < 8192; i++) dd_sine_tab[i] = (int16_t)lrint(32767.0 * sin(2 * M_PI * i / 8192.0));  /* jak OP-1 */
}

struct DdddCore* ddddCreate(void) {
    sineInit();
    if (!fresh_ready) { save(&fresh); fresh_ready = 1; }
    struct DdddCore* p = (struct DdddCore*)malloc(sizeof *p);
    if (p) { *p = fresh; p->jumps = 0; p->lev = 0; }
    return p;
}
void ddddNoteOn(struct DdddCore* p, int voice, const int16_t* knobs) {
    enter(p);
    ddw_engine.note_on(p->mem[voice], knobs);
}
void ddddNoteOff(struct DdddCore* p, int voice) {
    enter(p);
    if (held[voice]) ddw_engine.note_off(p->mem[voice]);
}
void ddddRender(struct DdddCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel, int env) {
    enter(p);
    ddw_pitch_ptr = (const int32_t*)pitch;
    ddw_vel = vel;
    const int16_t e = (int16_t)(env < 0 ? 0 : (env > 32767 ? 32767 : env));
    for (int i = 0; i < 128; i++) ddw_env[i] = e;
    ddw_engine.render(p->mem[voice], out, knobs);
    p->jumps = dd.jumps;
    p->lev = level;
}

void ddddCopyForScreen(const struct DdddCore* p, void* mem, uint32_t* jumps, int32_t* lev);
void ddddCopyForScreen(const struct DdddCore* p, void* mem, uint32_t* jumps, int32_t* lev) {
    memcpy(mem, p->mem, sizeof p->mem);
    *jumps = p->jumps;
    *lev = p->lev;
}
