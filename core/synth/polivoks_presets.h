#ifndef POLIVOKS_PRESETS_H
#define POLIVOKS_PRESETS_H
#include "polivoks_dsp.h"
#ifdef __cplusplus
extern "C" {
#endif

enum PolivoksDemoStyle { PVX_DEMO_BASS, PVX_DEMO_LEAD, PVX_DEMO_DUO, PVX_DEMO_SLOW, PVX_DEMO_DRUM };
struct PolivoksPresetInfo {
    const char* id;
    const char* name;
    const char* category;
    const char* description;
    int demo_style;
    int extended;
};
int polivoksPresetCount(void);
const struct PolivoksPresetInfo* polivoksPresetInfo(int index);
/* Copies a complete current-version parameter array. Invalid index/pointer: returns 0,
 * leaving the destination untouched. No allocation or audio-engine changes. */
int polivoksLoadPreset(int index, int16_t params[POLIVOKS_NUM_PARAMS]);
#ifdef __cplusplus
}
#endif
#endif
