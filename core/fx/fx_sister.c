/* SISTER (op1r, Hundekraft): sister.c / sister_dsp.c / sister_kern.c bez zmian (wersje C kerneli); pamiec robocza
 * pozycji = pamiec egzemplarza, linia sister_pos[0] podmieniana. */
#define OP1_HOST
#define op1_fx_scratch sifx_unused_scratch
#define op1_fx_param_q15 sifx_unused_q15
#include "op1_plugin.h"
#undef op1_fx_scratch
#undef op1_fx_param_q15
#include <stdlib.h>
#include <string.h>
#include "sister/sister_dsp.h"
static void* cur_scratch;
static void* op1_fx_scratch(int p) { (void)p; return cur_scratch; }
static int32_t op1_fx_param_q15(const int16_t* p, int i) { return p[i] < 0 ? 0 : p[i]; }
#include "sister/sister.c"
#include "core/fx/op1r_fx.h"
typedef struct { si_state s; si_pos_t pos; } SiInst;
void* fxSisterCreate(void) {
    SiInst* x = (SiInst*)calloc(1, sizeof(SiInst));
    if (!x) return 0;
    cur_scratch = &x->s;
    init(0);
    x->pos = sister_pos[0];
    return x;
}
void fxSisterProcess(void* inst, int16_t* l, int16_t* r, const int16_t* w) {
    SiInst* x = (SiInst*)inst;
    if (!x) return;
    cur_scratch = &x->s;
    sister_pos[0] = x->pos;
    stereo(0, l, r, w);
    x->pos = sister_pos[0];
}
