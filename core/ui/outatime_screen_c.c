/* Ekran OUTATIME: outatime_screen.c (Hundekraft / opus, bez zmian) z funkcjami zwrotnymi rysunku. Dane, ktore ekran
 * czyta z silnika (licznik blokow, trzymane klawisze, najnowszy glos, dryf i zaplon glosow), kopiowane z instancji
 * sciezki przed kazda klatka. */
#include <string.h>
#include "synth/outatime_dsp.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wbuiltin-declaration-mismatch"
#include "../../third_party/kennel_outatime/plugins/outatime/outatime_screen.c"
#pragma GCC diagnostic pop

volatile uint32_t outatime_blocks;
volatile int32_t outatime_held, outatime_newest = -1;
static struct ota_voice s_mir[OP1_VOICES];
const struct ota_voice* outatime_voice(int i) { return i >= 0 && i < OP1_VOICES ? &s_mir[i] : 0; }


void outatimeCopyForScreen(const struct OutatimeCore* p, uint32_t* blocks, int32_t* nheld, int32_t* newest,
                           struct ota_voice* mir);

static void* cb_user;
static OtwLineFn cb_line;
static OtwArtFn cb_art;
void ots_screen_draw_cached(void* scr, int dt, int flags) { (void)scr; (void)dt; (void)flags; cb_art(cb_user); }
void* ots_raster(void) { return cb_user; }
void ots_raster_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* c) { cb_line(r, a->x, a->y, b->x, b->y, w, c); }

void outatimeScreenUpdate(void* user, OtwLineFn line_fn, OtwArtFn art_fn, const struct OutatimeCore* core,
                          const int16_t knobs[8], const int16_t live[4], int page2, int dt_ms) {
    for (int i = 0; i < 3; i++) ots_eng_page[i] = (uint8_t)(page2 != 0);
    cb_user = user; cb_line = line_fn; cb_art = art_fn;
    if (core) {
        uint32_t b; int32_t h, n;
        outatimeCopyForScreen(core, &b, &h, &n, s_mir);
        outatime_blocks = b; outatime_held = h; outatime_newest = n;
    } else { outatime_held = 0; outatime_newest = -1; }
    static int inited;
    if (!inited) { SCREEN_outatime.init(0); inited = 1; }
    ots_knobs = knobs;
    ots_live = live;
    SCREEN_outatime.update(0, dt_ms);
}
