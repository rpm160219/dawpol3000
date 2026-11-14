/* Independent five-card Rev 3 inspired analog model. See docs/PROPHET.md.
 * Controls 0..32767; choices use prophetChoice(), never raw enum values.
 * This is a circuit-informed approximation, not a measured component clone. */
#ifndef PROPHET_DSP_H
#define PROPHET_DSP_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define PROPHET_VOICES 5
#define PROPHET_PARAMS 50
#define PROPHET_VERSION 1
#define PROPHET_DIVS 24
enum ProphetParam {
    P5_AFREQ,
    P5_AWAVE,
    P5_APW,
    P5_SYNC,
    P5_MIXA,
    P5_MIXB,
    P5_NOISE,
    P5_VINTAGE,
    P5_BFREQ,
    P5_BFINE,
    P5_BWAVE,
    P5_BPW,
    P5_BLOW,
    P5_BKEY,
    P5_CUTOFF,
    P5_RESONANCE,
    P5_ENVAMT,
    P5_KEYTRACK,
    P5_FA,
    P5_FD,
    P5_FS,
    P5_FR,
    P5_PMENV,
    P5_PMB,
    P5_PMFREQ,
    P5_PMPW,
    P5_PMFILTER,
    P5_LFORATE,
    P5_LFOWAVE,
    P5_WHEEL,
    P5_WMIX,
    P5_WFREQA,
    P5_AA,
    P5_AD,
    P5_AS,
    P5_AR,
    P5_RELEASE,
    P5_UNISON,
    P5_GLIDE,
    P5_TUNE,
    P5_WFREQB,
    P5_WPWA,
    P5_WPWB,
    P5_WFILTER,
    P5_LFOSYNC,
    P5_LFODIV,
    P5_LEVEL,
    P5_VELOCITY,
    P5_BEND,
    P5_A440
};
typedef struct ProphetCore ProphetCore;
typedef struct ProphetState {
    float amp[5], env[5], phase[2], lfo, lfoPhase, lfoHz;
    int notes[5], held;
} ProphetState;
ProphetCore *prophetCreate(void);
void prophetDestroy(ProphetCore *p);
void prophetSeed(ProphetCore *p, uint32_t seed); /* call at initialization */
void prophetDefault(int16_t *words);
int16_t prophetChoice(int index, int count);
int prophetChoices(int param);
void prophetPanic(ProphetCore *p);
void prophetKeyDown(ProphetCore *, int note, int velocity, const int16_t *words);
void prophetKeyUp(ProphetCore *, int note, const int16_t *words);
/* Mode changes reconcile held notes, and chord capture lives in the preset. */
void prophetConfigure(ProphetCore *, const int16_t *words);
void prophetRender(ProphetCore *, float *mono, int frames, const int16_t *, float rate, float bpm);
void prophetGetState(const ProphetCore *, ProphetState *);
void prophetResetLfo(ProphetCore *);
void prophetTune(ProphetCore *);
float prophetCutoff(int word);
float prophetTime(int word, int attack);
float prophetLfoHz(const int16_t *words, float bpm);
const char *prophetDivision(int word);
void prophetValue(int param, int word, char *out, int size);
int prophetPresetCount(void);
const char *prophetPresetName(int index);
int prophetPresetCategory(int index);
void prophetPreset(int index, int16_t *words);
#ifdef __cplusplus
}
#endif
#endif
