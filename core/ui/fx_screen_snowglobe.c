/* Ekran SNOWGLOBE: screen.c (op1r, Hundekraft, framework hk w trybie HK_SCREEN_HOST) bez zmian — kula ze sniegiem,
 * smuzka shimmer, aureola, kluczyk i dym; stan (bloki, poziom ogona, zamrozenie, galki) z telemetrii. */
#define HK_SCREEN_HOST  /* przed hk/fx.h: galki i egzemplarz z hosta, nie z adresow OP-1 */
#include "core/fx/fxs.h"
#include "op1_plugin.h"
#include "hk/fx.h"
volatile uint32_t snowglobe_blocks;
volatile int32_t snowglobe_level, snowglobe_freeze;
volatile int16_t snowglobe_knobs[FX_INSTANCES][4];
volatile uint32_t snowglobe_kblocks[FX_INSTANCES];
#include "snowglobe/snowglobe_src_screen.c"
#include "ui/fx_screen_c.h"
typedef struct { FXS_TFIELD1(snowglobe_blocks) FXS_TFIELD1(snowglobe_level) FXS_TFIELD1(snowglobe_freeze)
                 FXS_TFIELD(snowglobe_knobs) FXS_TFIELD(snowglobe_kblocks) } Tel;
void fxScreenSnowglobe(const void* tel, int reset, int dt) {
    const Tel* t = (const Tel*)tel;
    FXS_TRESTORE1(t, snowglobe_blocks) FXS_TRESTORE1(t, snowglobe_level) FXS_TRESTORE1(t, snowglobe_freeze)
    FXS_TRESTORE(t, snowglobe_knobs) FXS_TRESTORE(t, snowglobe_kblocks)
    if (reset) snowglobe_fx_screen_load(0);
    snowglobe_fx_screen_animate(0);
    op1_screen_draw(0, dt);
    snowglobe_fx_screen_draw(0, op1_raster());
}
