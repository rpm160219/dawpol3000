/* SNOWGLOBE (op1r, Hundekraft): snowglobe.c bez zmian — poglos shimmer (ogon wspina sie o oktawe). Galki = slowa Q15
 * podane wprost do fx_process (jak adapter op1r po przeskalowaniu zakresow galek OP-1). Stan per egzemplarz (fxw.h). */
#include "core/fx/fxw.h"
#define st (*fxw_st)
#define snowglobe_blocks (*fxw_snowglobe_blocks)
#define snowglobe_level (*fxw_snowglobe_level)
#define snowglobe_freeze (*fxw_snowglobe_freeze)
#define snowglobe_knobs (*fxw_snowglobe_knobs)
#define snowglobe_kblocks (*fxw_snowglobe_kblocks)
#define snowglobe_fx_screen_load sgw_unused_load
#define snowglobe_fx_screen_animate sgw_unused_animate
#define snowglobe_fx_screen_draw sgw_unused_draw
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include "snowglobe/snowglobe_src_snowglobe.c"
#pragma GCC diagnostic pop
#define ARR(X) X(st) X(snowglobe_knobs) X(snowglobe_kblocks)
#define ARR1(X) X(snowglobe_blocks) X(snowglobe_level) X(snowglobe_freeze)
typedef struct { ARR(FXW_FIELD) ARR1(FXW_FIELD1) } Store;
typedef struct { Store s; } Inst;
typedef struct { ARR1(FXW_TFIELD1) FXW_TFIELD(snowglobe_knobs) FXW_TFIELD(snowglobe_kblocks) } Tel;
static void bind(Inst* x) {
    ARR(FXW_BINDX) ARR1(FXW_BIND1X)
}
static void* create(void) {
    Inst* x = (Inst*)calloc(1, sizeof(Inst));
    if (!x) return 0;
    bind(x);
    snowglobe_fx_reset(0);
    return x;
}
static void mo(void* inst, int16_t* io, const int16_t* w) { bind((Inst*)inst); snowglobe_fx_process(0, io, OP1_FX_BLOCK, w); }
static void tel(void* inst, void* out) {
    bind((Inst*)inst);
    ARR1(FXW_TCOPY1X)
    FXW_TCOPYX(snowglobe_knobs) FXW_TCOPYX(snowglobe_kblocks)
}
static void destroy(void* inst) { Inst* x = (Inst*)inst; if (!x) return; free(x); }
const FxImpl FXI_snowglobe = {0, create, mo, 0, tel, 0, destroy};
