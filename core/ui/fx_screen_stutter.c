/* Ekran STUTTER: stutter.c (op1r, Hundekraft) bez zmian — szpula i pierscienie plastrow; stan z telemetrii. */
#include "core/fx/fxs.h"
#include "stutter/stutter.c"
#include "ui/fx_screen_c.h"
typedef struct { FXS_TFIELD(st) FXS_TFIELD(sx_blocks) } Tel;
void fxScreenStutter(const void* tel, int reset, int dt) {
    const Tel* t = (const Tel*)tel;
    FXS_TRESTORE(t, st) FXS_TRESTORE(t, sx_blocks)
    if (reset) SCREEN_stutter.init(0);
    SCREEN_stutter.update(0, dt);
}
