/* STUTTER (op1r, Hundekraft): stutter.c bez zmian — stutter w rytm tempa; stan per egzemplarz (fxw.h). */
#include "core/fx/fxw.h"
#define st (*fxw_st)
#define sx_blocks (*fxw_sx_blocks)
#define FX_stutter fxw_FX_stutter_unused
#define SCREEN_stutter fxw_SCREEN_stutter_unused
#include "stutter/stutter.c"
#define ARR(X) X(st) X(sx_blocks)
#define TEL(X) X(st) X(sx_blocks)
typedef struct { ARR(FXW_FIELD) } Store;
typedef struct { Store s; void* scr; } Inst;
typedef struct { TEL(FXW_TFIELD) } Tel;
_Static_assert(sizeof(Tel) <= FX_TELEMETRY_BYTES, "telemetria STUTTER");
static void bind(Inst* x) {
    ARR(FXW_BINDX)
    fxw_scratch = x->scr;
}
static void* create(void) {
    Inst* x = (Inst*)calloc(1, sizeof(Inst));
    if (!x) return 0;
    x->scr = calloc(1, FXW_SCRATCH_BYTES);
    bind(x);
    sx_init(0);
    return x;
}
/* FREEZE: w OP-1 przelacznik pozycji (op1_fx_page[pos][3]), w DAW-POL slowo 4 slotu */
static void mono(void* inst, int16_t* io, const int16_t* w) { bind((Inst*)inst); FXW_BEAT(); op1_fx_page[0][3] = w[4] != 0; sx_render(0, io, w); }
static void tel(void* inst, void* out) {
    bind((Inst*)inst);
    TEL(FXW_TCOPYX)
}
static void destroy(void* inst) { Inst* x = (Inst*)inst; if (!x) return; free(x->scr); free(x); }
const FxImpl FXI_stutter = {0, create, mono, 0, tel, 0, destroy};
