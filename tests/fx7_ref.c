/* Referencja efektow op1r (oryginalny plugin, pozycja 0, jeden egzemplarz): -DFX7=tape|stutter|snowglobe|father|
 * rackverb|crush|ente. Wyjscie: bloki L (mono) albo L i R (stereo). */
#define OP1_HOST
#include "op1_plugin.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int16_t sc_buf[64];
int op1_sc_source(int src, int16_t* out) { (void)src; memcpy(out, sc_buf, sizeof sc_buf); return 1; }
#define T_tape 1
#define T_stutter 2
#define T_snowglobe 3
#define T_father 4
#define T_rackverb 5
#define T_crush 6
#define T_ente 7
#define CAT(a, b) a##b
#define XT(n) CAT(T_, n)
#if XT(FX7) == T_tape
#include "tape/tape.c"
#define INIT() FX_tape.init(0)
#define MONO 1
#define RUN(l, r, w) FX_tape.render(0, l, w)
#elif XT(FX7) == T_stutter
#include "stutter/stutter.c"
#define INIT() FX_stutter.init(0)
#define MONO 1
#define RUN(l, r, w) FX_stutter.render(0, l, w)
#elif XT(FX7) == T_snowglobe
#include "snowglobe/snowglobe_src_snowglobe.c"
#define INIT() snowglobe_fx_reset(0)
#define MONO 1
#define RUN(l, r, w) snowglobe_fx_process(0, l, 64, w)
#elif XT(FX7) == T_father
#include "father/father.c"
#define INIT() FX_father.mono.init(0)
#define MONO 0
#define RUN(l, r, w) FX_father.render_stereo(0, l, r, w)
#elif XT(FX7) == T_rackverb
#include "rackverb/rackverb.c"
#define INIT() FX_rackverb.mono.init(0)
#define MONO 0
#define RUN(l, r, w) FX_rackverb.render_stereo(0, l, r, w)
#elif XT(FX7) == T_crush
#include "crush/crush.c"
#define INIT() FX_crush.mono.init(0)
#define MONO 0
#define RUN(l, r, w) FX_crush.render_stereo(0, l, r, w)
#else
#include "ente/ente.c"
#define INIT() FX_ente.init(0)
#define MONO 0
#define RUN(l, r, w) FX_ente.render_stereo(0, l, r, w)
#endif
#include "fx7_events.h"
int main(int argc, char** argv) {
    FILE* f = fopen(argv[1], "wb");
    host_fx_scratch[0] = calloc(1, OP1_FX_SCRATCH_BYTES + 64);
    INIT();
    int16_t l[64], r[64], w[8];
    for (int b = 0; b < FXE_BLOCKS; b++) {
        fx7_block(b, 0, XT(FX7) == T_tape, l, r, w);
        int32_t len, next;
        fx7_beat(b, &len, &next);
        op1_beat_t* c = host_beat();
        c->len = len; c->next = next; c->step = 0x100000; c->tape_pos = 0; c->follows_tape = 0;
        fx7_sc(b, sc_buf);
        RUN(l, r, w);
        fwrite(l, 2, 64, f);
        if (!MONO) fwrite(r, 2, 64, f);
    }
    fclose(f);
    printf("%d\n", MONO);
    return 0;
}
