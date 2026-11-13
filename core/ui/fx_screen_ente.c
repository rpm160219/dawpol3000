/* Ekran ENTE: ente.c (op1r, pcppcp) bez zmian — gumowa kaczka sciskana przez sciszanie; stan z telemetrii. */
#include "core/fx/fxs.h"
#include "ente/ente.c"
#include "ui/fx_screen_c.h"
typedef struct { FXS_TFIELD(st) } Tel;
void fxScreenEnte(const void* tel, int reset, int dt) {
    const Tel* t = (const Tel*)tel;
    FXS_TRESTORE(t, st)
    if (reset) SCREEN_ente.init(0);
    SCREEN_ente.update(0, dt);
}
