// GIN: wrapper DAW-POL (ginSetKit / ginNoteOn / ginRenderNative, przygotowanie w locie) kontra referencja gin_fw.c
// (slownik fabryczny). Ten sam scenariusz (gin_events.h).
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "synth/gin_dsp.h"
#include "../tests/gin_events.h"

int main(int argc, char** argv) {
    FILE* f = fopen(argv[1], "rb");
    if (!f) { printf("brak referencji\n"); return 1; }
    std::vector<int32_t> ref;
    int32_t blk[128];
    while (fread(blk, sizeof blk, 1, f) == 1) ref.insert(ref.end(), blk, blk + 128);
    fclose(f);
    GinCore* g = ginCreate();
    std::vector<int32_t> got;
    static GinSound s[24];
    for (int kit = 0; kit < GE_KITS; kit++) {
        for (int i = 0; i < 24; i++) ginFactorySound(kit, i, &s[i]);
        ginSetKit(g, (kit + 1) % GE_KITS, s);  // wymus przeladowanie (jak gin_fw_factory przy kazdym zestawie)
        ginSetKit(g, kit, s);
        for (int b = 0; b < GE_BLOCKS; b++) {
            for (int w = 0; w < 2; w++) { const int key = ge_key(kit, b, w); if (key >= 0) ginNoteOn(g, 36 + key, 127); }
            ginRenderNative(g, blk);
            got.insert(got.end(), blk, blk + 128);
        }
    }
    long diff = 0; int32_t maxd = 0; long nz = 0;
    for (size_t i = 0; i < got.size() && i < ref.size(); i++) {
        int32_t d = abs(got[i] - ref[i]);
        if (d) diff++;
        if (d > maxd) maxd = d;
        if (ref[i]) nz++;
    }
    printf("probek: %zu (ref %zu, niezerowych %ld), roznych: %ld, max roznica: %d\n", got.size(), ref.size(), nz, diff, maxd);
    return diff || got.size() != ref.size();
}
