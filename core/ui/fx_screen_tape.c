/* Ekran TAPE: tape.c (op1r, pcppcp) bez zmian — echo tasmowe z glowicami, rolkami i kabestanem; stan z telemetrii. */
#include "core/fx/fxs.h"
#include "tape/tape.c"
#include "ui/fx_screen_c.h"
typedef struct { FXS_TFIELD(st) FXS_TFIELD(in_level) FXS_TFIELD(fb_level) FXS_TFIELD(samples) } Tel;
void fxScreenTape(const void* tel, int reset, int dt) {
    const Tel* t = (const Tel*)tel;
    FXS_TRESTORE(t, st) FXS_TRESTORE(t, in_level) FXS_TRESTORE(t, fb_level) FXS_TRESTORE(t, samples)
    if (reset) SCREEN_tape.init(0);
    SCREEN_tape.update(0, dt);
}
