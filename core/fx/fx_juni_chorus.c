/* JUNI CHORUS (op1r, Hundekraft): juni_chorus.c bez zmian; stan pozycji 0 podmieniany na stan egzemplarza. */
#define OP1_HOST
#define op1_fx_param_q15 jcfx_unused_q15
#include "op1_plugin.h"
#undef op1_fx_param_q15
#include <stdlib.h>
#include <string.h>
static int32_t op1_fx_param_q15(const int16_t* p, int i) { return p[i] < 0 ? 0 : p[i]; }  /* host: slowa = Q15 */
#include "juni_chorus/juni_chorus.c"
#include "core/fx/op1r_fx.h"
typedef struct { state_t st; } JcInst;
void* fxJuniChorusCreate(void) {
    JcInst* x = (JcInst*)calloc(1, sizeof(JcInst));
    if (!x) return 0;
    init(0);
    x->st = st[0];
    return x;
}
void fxJuniChorusProcess(void* inst, int16_t* l, int16_t* r, const int16_t* w) {
    JcInst* x = (JcInst*)inst;
    if (!x) return;
    st[0] = x->st;
    stereo(0, l, r, w);
    x->st = st[0];
}
