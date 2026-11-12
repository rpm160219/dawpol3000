/* Ekran DDDD: dddd.c skompilowany drugi raz (przedrostek dds_, osobne zmienne ekranu i osobna "sciana" — nieuzywana),
 * z funkcja zwrotna linii. Licznik skokow "rak" i poziom kopiowane z instancji sciezki przed kazda klatka. Dolny rzad
 * (kreski i napisy galek) pominiety — dublowalby galki DAW-POL. */
#define DD_P(n) dds_##n
#include <string.h>
#include "synth/dddd_dsp.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "../../third_party/op1r_dddd/plugins/dddd/dddd.c"
#pragma GCC diagnostic pop

const int32_t* dds_pitch_ptr;
int dds_vel;
int16_t dds_env[128];
const void* dds_base;
volatile uint8_t dds_eng_page[4];
const int16_t* dds_knobs;
const int16_t* dds_live;

void ddddCopyForScreen(const struct DdddCore* p, void* mem, uint32_t* jumps, int32_t* lev);

static uint32_t s_mem[OP1_VOICES][OP1_VOICE_SIZE / 4];
static void* cb_user;
static DdwLineFn cb_line;
void dds_screen_draw(void* scr, int dt) { (void)scr; (void)dt; }  /* dddd.svg jest pusty */
void* dds_raster(void) { return cb_user; }
void dds_raster_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* c) {
    if (a->y == OP1_Q10(110) && b->y == OP1_Q10(110) && w == 0x800) return;  /* dolny rzad */
    cb_line(r, a->x, a->y, b->x, b->y, w, c);
}

void ddddScreenUpdate(void* user, DdwLineFn line_fn, const struct DdddCore* core, const int16_t knobs[8],
                      const int16_t live[4], int page2, int dt_ms) {
    cb_user = user; cb_line = line_fn;
    if (core) {
        uint32_t j; int32_t lv;
        ddddCopyForScreen(core, s_mem, &j, &lv);
        dd.jumps = j; level = lv;
    } else { level = 0; }
    dds_base = s_mem;
    dds_knobs = knobs;
    dds_live = live;
    for (int i = 0; i < 3; i++) dds_eng_page[i] = (uint8_t)(page2 != 0);
    dds_screen.update(0, dt_ms);
}
