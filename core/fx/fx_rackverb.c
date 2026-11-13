/* RACKVERB (op1r 68 / hundekraft 3.2.3, Hundekraft): rackverb.c + rv_mv.c + rv_quad.c (wersje C kerneli) bez zmian —
 * poglosy i efekty rackowe z konca lat 80. (VERB I / VERB II / FEX / QUAD). Stan per egzemplarz (fxw.h); stan QUAD
 * (host_big oryginalu) = osobny blok pamieci egzemplarza. */
#include "core/fx/fxw.h"
#include "rackverb/rv_mv.h"
#include "rackverb/rv_programs.h"
#include "rackverb/rv_tables.h"
#include "rackverb/rv_fbceil.h"
#include "rackverb/rv_quad.h"
#define l1_owner (*fxw_l1_owner)
#define rv_live (*fxw_rv_live)
#define peak_r (*fxw_peak_r)
#define rv_name_live (*fxw_rv_name_live)
#define spread (*fxw_spread)
#define phase (*fxw_phase)
#define raw (*fxw_raw)
#define q15 (*fxw_q15)
#define valid (*fxw_valid)
#define host_big (*fxw_host_big)
#define bigclr (*fxw_bigclr)
#define early (*fxw_early)
#define steady (*fxw_steady)
#define built (*fxw_built)
#define FX_rackverb fxw_FX_rackverb_unused
#define rv_live_name rvw_live_name_unused
#include "rackverb/rackverb.c"
#define ARR(X) X(l1_owner) X(rv_live) X(peak_r) X(rv_name_live) X(spread) X(phase) X(raw) X(q15) X(valid) X(bigclr) X(early) X(steady) X(built)
#define TEL(X) X(rv_live) X(rv_name_live)
typedef struct { ARR(FXW_FIELD) } Store;
typedef struct { Store s; void* scr; void* big; } Inst;
typedef struct { TEL(FXW_TFIELD) } Tel;
_Static_assert(sizeof(Tel) <= FX_TELEMETRY_BYTES, "telemetria RACKVERB");
static void bind(Inst* x) {
    ARR(FXW_BINDX)
    fxw_scratch = x->scr;
    fxw_host_big = (__typeof__(fxw_host_big))x->big;  /* uzywany tylko wiersz [0] (pozycja 0) */
}
static void* create(void) {
    Inst* x = (Inst*)calloc(1, sizeof(Inst));
    if (!x) return 0;
    x->scr = calloc(1, FXW_SCRATCH_BYTES);
    x->big = calloc(1, sizeof(qd_state) + 64);
    bind(x);
    init(0);
    return x;
}
static void st2(void* inst, int16_t* l, int16_t* r, const int16_t* w) { bind((Inst*)inst); render(0, l, r, w); }
static void mo(void* inst, int16_t* io, const int16_t* w) { bind((Inst*)inst); mono(0, io, w); }
static void tel(void* inst, void* out) {
    bind((Inst*)inst);
    TEL(FXW_TCOPYX)
}
static void destroy(void* inst) { Inst* x = (Inst*)inst; if (!x) return; free(x->scr); free(x->big); free(x); }
const FxImpl FXI_rackverb = {1, create, mo, st2, tel, 0, destroy};
