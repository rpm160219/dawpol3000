/* Ekran FATHER: father_screen.c (op1r, Hundekraft) bez zmian — pokoj z postacia (bramka) i twarza; stan z telemetrii. */
#include "core/fx/fxs.h"
#include "op1_plugin.h"
volatile uint32_t father_blocks[3];
volatile int16_t father_live[3][4];
volatile int32_t father_peak[3];
volatile int16_t father_gate[3];
#include "father/father_screen.c"
#include "ui/fx_screen_c.h"
typedef struct { FXS_TFIELD(father_blocks) FXS_TFIELD(father_live) FXS_TFIELD(father_peak) FXS_TFIELD(father_gate) } Tel;
void fxScreenFather(const void* tel, int reset, int dt) {
    const Tel* t = (const Tel*)tel;
    FXS_TRESTORE(t, father_blocks) FXS_TRESTORE(t, father_live) FXS_TRESTORE(t, father_peak) FXS_TRESTORE(t, father_gate)
    if (reset) SCREEN_father.init(0);
    SCREEN_father.update(0, dt);
}
