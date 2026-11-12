/* Ekran ARC: arc.c skompilowany drugi raz (przedrostek ars_), z funkcjami zwrotnymi rysunku. Pamiec glosow, portret
 * fazowy, poziomy i glos sledzony kopiowane z instancji sciezki przed kazda klatka. Tarcze z dolu oryginalu (wskazowki,
 * pierscienie strony 2) i napis proporcji pominiete — dublowalyby galki DAW-POL. */
#define AR_P(n) ars_##n
#include <string.h>
#include "synth/arc_dsp.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "../../third_party/op1r_arc/arc.c"
#pragma GCC diagnostic pop

const int32_t* ars_pitch_ptr;
int ars_vel;
const void* ars_base;
volatile uint8_t ars_eng_page[4];
const int16_t* ars_knobs;
const int16_t* ars_live;

void arcCopyForScreen(const struct ArcCore* p, void* mem, void* trace, uint16_t* tpos, int16_t* level, uint8_t* tvoice);

static uint32_t s_mem[OP1_VOICES][OP1_VOICE_SIZE / 4];
static int16_t s_trace[ARC_TN][4] __attribute__((aligned(8)));
static uint16_t s_tpos;
static int16_t s_level[OP1_VOICES], s_lseen[OP1_VOICES];
static void* cb_user;
static ArwLineFn cb_line;
static ArwArtFn cb_art;
static int screen_ready;

void* ars_find_node(void* scr, const char* id) { (void)scr; (void)id; return 0; }  /* wskazowki tarcz: pominiete */
void ars_screen_draw(void* scr, int dt) { (void)scr; (void)dt; cb_art(cb_user); }
void* ars_raster(void) { return cb_user; }
void ars_raster_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* c) {
    if (a->y > OP1_Q10(120) && b->y > OP1_Q10(120)) return;  /* pierscienie tarcz strony 2 (y 140) */
    cb_line(r, a->x, a->y, b->x, b->y, w, c);
}

void arcScreenUpdate(void* user, ArwLineFn line_fn, ArwArtFn art_fn, const struct ArcCore* core, const int16_t knobs[8],
                     const int16_t live[4], int page2, int dt_ms) {
    cb_user = user; cb_line = line_fn; cb_art = art_fn;
    static int16_t lv_in[OP1_VOICES];
    if (core) {
        uint8_t tv = 0xff;
        arcCopyForScreen(core, s_mem, s_trace, &s_tpos, lv_in, &tv);
        ars_tvoice = tv;
        /* poziom: oryginal zeruje arc_level po odczycie (render wpisuje nowy) — tu tylko zmiany od ostatniej klatki */
        for (int i = 0; i < OP1_VOICES; i++) { s_level[i] = lv_in[i] != s_lseen[i] ? lv_in[i] : 0; s_lseen[i] = lv_in[i]; }
    } else {
        memset(s_mem, 0, sizeof s_mem);
        for (int i = 0; i < OP1_VOICES; i++) s_level[i] = 0;
        ars_tvoice = 0xff;
    }
    ars_base = s_mem;
    ars_trace_ptr = &s_trace;
    ars_tpos_ptr = &s_tpos;
    ars_level_ptr = &s_level;
    ars_knobs = knobs;
    ars_live = live;
    for (int i = 0; i < 3; i++) ars_eng_page[i] = (uint8_t)(page2 != 0);
    if (!screen_ready) { ars_screen.init(0); screen_ready = 1; }
    ars_screen.update(0, dt_ms);
}
