/* FATHER (op1r, Hundekraft): father.c + father_dsp.c (wersje C kerneli) bez zmian — poglos ambience z bramka;
 * stan per egzemplarz (fxw.h), linia stanu w pamieci roboczej egzemplarza. */
#include "core/fx/fxw.h"
#include "father/father_dsp.h"
#define father_blocks (*fxw_father_blocks)
#define father_live (*fxw_father_live)
#define father_peak (*fxw_father_peak)
#define father_gate (*fxw_father_gate)
#define raw_params (*fxw_raw_params)
#define normalized (*fxw_normalized)
#define params_valid (*fxw_params_valid)
#define spread (*fxw_spread)
#define fields_due (*fxw_fields_due)
#include "father/father.c"
#define ARR(X) X(father_blocks) X(father_live) X(father_peak) X(father_gate) X(raw_params) X(normalized) X(params_valid) X(spread) X(fields_due)
#define TEL(X) X(father_blocks) X(father_live) X(father_peak) X(father_gate)
typedef struct { ARR(FXW_FIELD) } Store;
typedef struct { Store s; void* scr; } Inst;
typedef struct { TEL(FXW_TFIELD) } Tel;
static void bind(Inst* x) {
    ARR(FXW_BINDX)
    fxw_scratch = x->scr;
}
static void* create(void) {
    Inst* x = (Inst*)calloc(1, sizeof(Inst));
    if (!x) return 0;
    x->scr = calloc(1, FXW_SCRATCH_BYTES);
    bind(x);
    init(0);
    return x;
}
static void st2(void* inst, int16_t* l, int16_t* r, const int16_t* w) { bind((Inst*)inst); stereo(0, l, r, w); }
static void mo(void* inst, int16_t* io, const int16_t* w) { bind((Inst*)inst); mono(0, io, w); }
static void tel(void* inst, void* out) {
    bind((Inst*)inst);
    TEL(FXW_TCOPYX)
}
static void destroy(void* inst) { Inst* x = (Inst*)inst; if (!x) return; free(x->scr); free(x); }
const FxImpl FXI_father = {1, create, mo, st2, tel, 0, destroy};
