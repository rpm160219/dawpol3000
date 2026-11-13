/* ENTE (op1r, pcppcp): ente.c bez zmian — kompresor sidechain (kaczka); zrodla T1..T4 / TAPE = sciezki DAW-POL,
 * BEAT = zegar tempa, IN / INST = cisza (fx_common.c). Stan per egzemplarz (fxw.h). */
#include "core/fx/fxw.h"
#define st (*fxw_st)
#define FX_ente fxw_FX_ente_unused
#define SCREEN_ente fxw_SCREEN_ente_unused
#include "ente/ente.c"
#define ARR(X) X(st)
typedef struct { ARR(FXW_FIELD) } Store;
typedef struct { Store s; } Inst;
typedef struct { ARR(FXW_TFIELD) } Tel;
_Static_assert(sizeof(Tel) <= FX_TELEMETRY_BYTES, "telemetria ENTE");
static void bind(Inst* x) {
    ARR(FXW_BINDX)
}
static void* create(void) {
    Inst* x = (Inst*)calloc(1, sizeof(Inst));
    if (!x) return 0;
    bind(x);
    ente_init(0);
    return x;
}
static void stereo(void* inst, int16_t* l, int16_t* r, const int16_t* w) { bind((Inst*)inst); FXW_BEAT(); ente_render_stereo(0, l, r, w); }
static void mono(void* inst, int16_t* io, const int16_t* w) { bind((Inst*)inst); FXW_BEAT(); ente_render(0, io, w); }
static void tel(void* inst, void* out) {
    bind((Inst*)inst);
    ARR(FXW_TCOPYX)
}
/* zrodlo sidechain wg slowa 3 (lista): 0 = wlasne wejscie / BEAT, 1..4 = T1..T4, 5 = TAPE, 6 = IN, 7 = INST */
static int scsrc(const int16_t* w) {
    const int src = ent_src[ent_of(op1_fx_param_q15(w, 3))];
    return src >= S_T1 && src <= S_IN ? src : (src == S_INST ? 0 : 0);
}
static void destroy(void* inst) { Inst* x = (Inst*)inst; if (!x) return; free(x); }
const FxImpl FXI_ente = {1, create, mono, stereo, tel, scsrc, destroy};
