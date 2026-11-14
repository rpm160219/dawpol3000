#include "prophet_dsp.h"
#include <string.h>
#include "prophet_patches.inc"
int prophetPresetCount(void) { return sizeof(patches) / sizeof(patches[0]); }
const char *prophetPresetName(int i) { return i >= 0 && i < prophetPresetCount() ? patches[i].name : "INIT"; }
int prophetPresetCategory(int i) { return i >= 0 && i < prophetPresetCount() ? patches[i].category : 8; }
void prophetPreset(int i, int16_t *w) {
    if (!w)
        return;
    if (i < 0 || i >= prophetPresetCount()) {
        prophetDefault(w);
        return;
    }
    memcpy(w, patches[i].words, sizeof(patches[i].words));
}
