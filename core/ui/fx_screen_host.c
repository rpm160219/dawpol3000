/* Host ekranow efektow op1r (JUNI CHORUS, SISTER) w DAW-POL: funkcje harness/op1_host.h kierowane do rysowania DAW-POL
 * (linie, nieruchomy rysunek z SVG, napisy wlasna czcionka). Galki = slowa efektu z miksera, strona 2 = op1_fx_page. */
#include <math.h>
#include <string.h>
#include "op1_plugin.h"
#include "ui/fx_screen_c.h"

int16_t host_sine[8192];
int16_t host_fx_knobs[3][8];
int16_t host_fx_meter[3];
void* host_fx_scratch[3];
volatile uint8_t op1_fx_page[3][4];

static void* cb_user;
static FxLineFn cb_line;
static FxArtFn cb_art;
static FxTextFn cb_text;

void* host_raster(void) { return cb_user; }
void host_raster_line(void* r, const void* a, const void* b, int32_t w, const uint8_t* c) {
    const op1_pt* p = (const op1_pt*)a;
    const op1_pt* q = (const op1_pt*)b;
    cb_line(r, p->x, p->y, q->x, q->y, w, c);
}
void host_screen_draw(void* s, int dt) { (void)s; (void)dt; cb_art(cb_user); }
typedef struct { int32_t x, y, size; uint8_t c[4]; char s[32]; } HostText;
void* host_text_ctor(void* o, int32_t x, int32_t y, int32_t z, const uint8_t* c) {
    HostText* t = (HostText*)o;
    t->x = x; t->y = y; t->size = z; memcpy(t->c, c, 4); t->s[0] = 0;
    return o;
}
void host_text_set(void* o, const char* s) { HostText* t = (HostText*)o; strncpy(t->s, s, 31); t->s[31] = 0; }
void host_text_draw(void* o, void* r, const int32_t* m) { (void)m; HostText* t = (HostText*)o; cb_text(r, t->x, t->y, t->size, t->s, t->c); }

void fxScreenSetup(void* user, FxLineFn line, FxArtFn art, FxTextFn text, const int16_t w[8], const uint8_t page[4]) {
    static int sineReady;
    if (!sineReady) {
        for (int i = 0; i < 8192; i++) host_sine[i] = (int16_t)lrint(32767.0 * sin(2 * M_PI * i / 8192.0));
        sineReady = 1;
    }
    cb_user = user; cb_line = line; cb_art = art; cb_text = text;
    for (int i = 0; i < 8; i++) host_fx_knobs[0][i] = w[i];
    for (int i = 0; i < 4; i++) op1_fx_page[0][i] = page[i];
}
