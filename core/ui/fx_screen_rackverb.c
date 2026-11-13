/* Ekran RACKVERB: rackverb_screen.c (op1r, Hundekraft) bez zmian — front urzadzenia rackowego z wyswietlaczem
 * programu i miernikiem L / R; stan (jednostka, program, tryb, rozmiar, szczyty, nazwa programu) z telemetrii. */
#include "core/fx/fxs.h"
#include "op1_plugin.h"
volatile int16_t rv_live[3][6];
static const char* rv_name_live[3];
const char* rv_live_name(int p) { return rv_name_live[p % 3]; }
#include "rackverb/rackverb_screen.c"
#include "ui/fx_screen_c.h"
typedef struct { FXS_TFIELD(rv_live) FXS_TFIELD(rv_name_live) } Tel;
void fxScreenRackverb(const void* tel, int reset, int dt) {
    const Tel* t = (const Tel*)tel;
    FXS_TRESTORE(t, rv_live) FXS_TRESTORE(t, rv_name_live)
    if (reset) SCREEN_rackverb.init(0);
    SCREEN_rackverb.update(0, dt);
}
