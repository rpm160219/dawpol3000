// Wrappery DAW-POL efektow op1r (dwa egzemplarze na zmiane, core/fx/fxw.h) kontra referencja oryginalu (fx7_ref.c).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "fx/op1r_fx.h"
#include "../tests/fx7_events.h"
int main(int argc, char** argv) {
    const int id = atoi(argv[2]);
    const char* name = argv[3];
    const FxImpl* fi = fxImpl(id);
    if (!fi) { printf("brak efektu %d\n", id); return 1; }
    const bool mono = !fi->stereo, tapeSync = !strcmp(name, "TAPE");
    void* a = fi->create();
    void* b = fi->create();
    FILE* f = fopen(argv[1], "rb");
    const int per = mono ? 64 : 128;
    std::vector<int16_t> ref((size_t)FXE_BLOCKS * per);
    if (!f || fread(ref.data(), 2, ref.size(), f) != ref.size()) { printf("brak referencji\n"); return 1; }
    fclose(f);
    long diff = 0, nz = 0; int maxd = 0;
    int16_t sc[64];
    for (int k = 0; k < FXE_BLOCKS; k++) {
        int16_t l[64], r[64], w[8], l2[64], r2[64], w2[8];
        fx7_block(k, 0, tapeSync, l, r, w);
        fx7_block(k, 1, tapeSync, l2, r2, w2);
        int32_t len, next;
        fx7_beat(k, &len, &next);
        fxSetBeat(len, next, 1);
        fx7_sc(k, sc);
        fxSetSidechain(sc);
        if (mono) { fi->mono(a, l, w); fi->mono(b, l2, w2); }
        else { fi->stereoFn(a, l, r, w); fi->stereoFn(b, l2, r2, w2); }
        for (int i = 0; i < per; i++) {
            const int v = i < 64 ? l[i] : r[i - 64], rv = ref[(size_t)k * per + i];
            const int d = abs(v - rv);
            if (d) diff++;
            if (d > maxd) maxd = d;
            if (rv) nz++;
        }
    }
    printf("%s: probek: %d (niezerowych %ld), roznych: %ld, max roznica: %d\n", name, FXE_BLOCKS * per, nz, diff, maxd);
    fi->destroy(a); fi->destroy(b);
    return diff != 0;
}
