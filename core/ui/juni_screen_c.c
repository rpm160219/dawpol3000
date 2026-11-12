/* Ekran JUNI: juni.c skompilowany drugi raz (przedrostek jns_), z funkcjami zwrotnymi rysunku. Obwiednie glosow i
 * liczniki blokow kopiowane z instancji sciezki przed kazda klatka (ognisko i woda reaguja na granie). */
#define JN_P(n) jns_##n
#include <string.h>
#include "synth/juni_dsp.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wparentheses"
#include "../../third_party/op1r_juni/juni.c"
#pragma GCC diagnostic pop

const int32_t* jns_pitch_ptr;
int jns_vel;
int16_t jns_env[128];
const void* jns_base;
const int16_t* jns_knobs;
const int16_t* jns_live;

void juniCopyForScreen(const struct JuniCore* p, int32_t* env0, uint32_t* blocks, uint32_t* vblk);

static void* cb_user;
static JnwLineFn cb_line;
static JnwArtFn cb_art;
void jns_screen_draw_cached(void* scr, int dt, int flags) { (void)scr; (void)dt; (void)flags; cb_art(cb_user); }
void* jns_raster(void) { return cb_user; }
void jns_raster_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* c) { cb_line(r, a->x, a->y, b->x, b->y, w, c); }

void juniScreenUpdate(void* user, JnwLineFn line_fn, JnwArtFn art_fn, const struct JuniCore* core, const int16_t knobs[8],
                      const int16_t live[4], int dt_ms) {
    cb_user = user; cb_line = line_fn; cb_art = art_fn;
    if (core) {
        int32_t e[OP1_VOICES]; uint32_t b, vb[OP1_VOICES];
        juniCopyForScreen(core, e, &b, vb);
        for (int i = 0; i < OP1_VOICES; i++) { vc[i].env0 = e[i]; jn_vblk[i] = vb[i]; }
        jn_blocks = b;
    } else {
        for (int i = 0; i < OP1_VOICES; i++) vc[i].env0 = 0;
    }
    jns_knobs = knobs;
    jns_live = live;
    jns_screen.update(0, dt_ms);
}
