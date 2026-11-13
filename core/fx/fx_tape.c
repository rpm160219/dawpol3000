/* TAPE (op1r, pcppcp): tape.c bez zmian — echo tasmowe; tablice stanu przekierowane na pamiec egzemplarza (fxw.h). */
#include "core/fx/fxw.h"
#define st (*fxw_st)
#define in_level (*fxw_in_level)
#define fb_level (*fxw_fb_level)
#define samples (*fxw_samples)
#define FX_tape fxw_FX_tape_unused
#define SCREEN_tape fxw_SCREEN_tape_unused
#include "tape/tape.c"
#define ARR(X) X(st) X(in_level) X(fb_level) X(samples)
typedef struct { ARR(FXW_FIELD) } Store;
typedef struct { Store s; void* scr; } Inst;
typedef struct { ARR(FXW_TFIELD) } Tel;
_Static_assert(sizeof(Tel) <= FX_TELEMETRY_BYTES, "telemetria TAPE");
static void bind(Inst* x) {
    ARR(FXW_BINDX)
    fxw_scratch = x->scr;
}
static void* create(void) {
    Inst* x = (Inst*)calloc(1, sizeof(Inst));
    if (!x) return 0;
    x->scr = calloc(1, FXW_SCRATCH_BYTES);
    bind(x);
    tape_init(0);
    return x;
}
static void mono(void* inst, int16_t* io, const int16_t* w) { bind((Inst*)inst); FXW_BEAT(); tape_render(0, io, w); }
static void tel(void* inst, void* out) {
    bind((Inst*)inst);
    ARR(FXW_TCOPYX)
}
static void destroy(void* inst) { Inst* x = (Inst*)inst; if (!x) return; free(x->scr); free(x); }
const FxImpl FXI_tape = {0, create, mono, 0, tel, 0, destroy};
