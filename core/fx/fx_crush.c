/* CRUSH (Maiko #250, rekonstrukcja Hundekraft, kennel 3.1.4): crush.c bez zmian — bitcrusher; stan per egzemplarz. */
#include "core/fx/fxw.h"
#define state (*fxw_state)
#define crush_blocks (*fxw_crush_blocks)
#include "crush/crush.c"
#define ARR(X) X(state) X(crush_blocks)
#define TEL(X) X(crush_blocks)
typedef struct { ARR(FXW_FIELD) } Store;
typedef struct { Store s; } Inst;
typedef struct { TEL(FXW_TFIELD) } Tel;
static void bind(Inst* x) {
    ARR(FXW_BINDX)
}
static void* create(void) {
    Inst* x = (Inst*)calloc(1, sizeof(Inst));
    if (!x) return 0;
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
static void destroy(void* inst) { Inst* x = (Inst*)inst; if (!x) return; free(x); }
const FxImpl FXI_crush = {1, create, mo, st2, tel, 0, destroy};
