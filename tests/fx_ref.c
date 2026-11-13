/* Referencja efektow op1r: oryginalny plugin (pozycja 0, jak w testach op1r), szum deterministyczny, bloki 64. */
#define OP1_HOST
#define op1_fx_scratch unused_host_scratch
#define op1_fx_param_q15 unused_host_q15
#include "op1_plugin.h"
#undef op1_fx_scratch
#undef op1_fx_param_q15
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef FX_SISTER_REF
#include "sister/sister_dsp.h"
static si_state inst[3];
static void* op1_fx_scratch(int p) { return &inst[p]; }
#endif
static int32_t op1_fx_param_q15(const int16_t* p, int i) { return p[i] < 0 ? 0 : p[i]; }
#ifdef FX_SISTER_REF
#include "sister/sister.c"
#define FXV FX_sister
#else
#include "juni_chorus/juni_chorus.c"
#define FXV FX_juni_chorus
#endif
#include "fx_events.h"
int main(int argc, char** argv) {
    FILE* f = fopen(argv[1], "wb");
    FXV.mono.init(0);
    int16_t l[64], r[64], w[8];
    for (int b = 0; b < FXE_BLOCKS; b++) {
        fxe_block(b, 0, l, r, w);
        FXV.render_stereo(0, l, r, w);
        fwrite(l, 2, 64, f); fwrite(r, 2, 64, f);
    }
    fclose(f);
    return 0;
}
