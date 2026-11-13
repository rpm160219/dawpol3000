/* Ekran CRUSH: crush_screen.c (Maiko #250, rekonstrukcja Hundekraft) bez zmian; licznik blokow z telemetrii. */
#include "core/fx/fxs.h"
#include "op1_plugin.h"
volatile uint32_t crush_blocks[3];
#include "crush/crush_screen.c"
#include "ui/fx_screen_c.h"
/* punkty zaczepienia ekranu CRUSH na PC (w oryginale: harness kennel) -> host ekranow DAW-POL */
extern int16_t host_fx_knobs[3][8];
extern int16_t host_sine[8192];
void fxHostText(void* r, int x, int y, int size, const char* s, const uint8_t* c);
void crush_host_knobs(int16_t* q) { for (int i = 0; i < 4; i++) q[i] = host_fx_knobs[0][i] < 0 ? 0 : host_fx_knobs[0][i]; }
void* crush_host_raster(void) { return host_raster(); }
void crush_host_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* c) { host_raster_line(r, a, b, w, c); }
void crush_host_text(void* r, int x, int y, int size, const char* s, const uint8_t* c) { fxHostText(r, x, y, size, s, c); }
int16_t crush_host_sine(uint32_t ph) { return host_sine[(ph >> 19) & 8191]; }
typedef struct { FXS_TFIELD(crush_blocks) } Tel;
void fxScreenCrush(const void* tel, int reset, int dt) {
    const Tel* t = (const Tel*)tel;
    FXS_TRESTORE(t, crush_blocks)
    if (reset) SCREEN_crush.init(0);
    host_screen_draw(0, dt);  /* tlo (w trybie OP1_HOST kod ekranu go nie rysuje) */
    SCREEN_crush.update(0, dt);
}
