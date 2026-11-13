// Wrapper efektow DAW-POL (dwa egzemplarze przetwarzane na zmiane) kontra referencja oryginalu.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "fx/op1r_fx.h"
#include "../tests/fx_events.h"
int main(int argc, char** argv) {
    const bool sister = argc > 2 && !strcmp(argv[2], "sister");
    void* a = sister ? fxSisterCreate() : fxJuniChorusCreate();
    void* b = sister ? fxSisterCreate() : fxJuniChorusCreate();
    FILE* f = fopen(argv[1], "rb");
    std::vector<int16_t> ref(FXE_BLOCKS * 128);
    if (!f || fread(ref.data(), 2, ref.size(), f) != ref.size()) { printf("brak referencji\n"); return 1; }
    fclose(f);
    long diff = 0, nz = 0; int maxd = 0;
    for (int k = 0; k < FXE_BLOCKS; k++) {
        int16_t l[64], r[64], w[8], l2[64], r2[64], w2[8];
        fxe_block(k, 0, l, r, w);
        fxe_block(k, 1, l2, r2, w2);
        if (sister) { fxSisterProcess(a, l, r, w); fxSisterProcess(b, l2, r2, w2); }
        else { fxJuniChorusProcess(a, l, r, w); fxJuniChorusProcess(b, l2, r2, w2); }
        for (int i = 0; i < 64; i++) {
            const int d1 = abs(l[i] - ref[k * 128 + i]), d2 = abs(r[i] - ref[k * 128 + 64 + i]);
            if (d1 || d2) diff++;
            if (d1 > maxd) maxd = d1;
            if (d2 > maxd) maxd = d2;
            if (ref[k * 128 + i]) nz++;
        }
    }
    printf("%s: probek: %d (niezerowych %ld), roznych: %ld, max roznica: %d\n", sister ? "SISTER" : "JUNI CHORUS", FXE_BLOCKS * 128, nz, diff, maxd);
    return diff != 0;
}
