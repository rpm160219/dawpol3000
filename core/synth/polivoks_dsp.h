/* Polivoks-inspired virtual analog engine. See README.md for model limits.
 * Parameters are normalized int16_t controls, 0..32767. Use polivoksChoice()
 * for rotary switches; a literal 2 is NOT switch position 2.
 */
#ifndef POLIVOKS_DSP_H
#define POLIVOKS_DSP_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define POLIVOKS_VOICES 6
#define POLIVOKS_BLOCK 128
#define POLIVOKS_OVERSAMPLE 4
#define POLIVOKS_PARAM_VERSION 2

enum PolivoksParam {
    /* Existing parameter indices are retained. Octave switches now have FIVE
     * positions: 32',16',8',4',2'. Migrate old four-position preset values. */
    PVX_OSC1_OCT = 0,
    PVX_OSC1_WAVE,       /* TRI, SAW, SQUARE, PULSE25, PULSE12 */
    PVX_XMOD_DEPTH,      /* Exponential audio-rate FM, oscillator II -> I */
    PVX_OSC2_OCT,
    PVX_OSC2_WAVE,
    PVX_OSC2_DETUNE,     /* Bipolar +/-7 semitones, center 16384 */
    PVX_OSC2_FINE,       /* Bipolar +/-50 cents, center 16384 */
    PVX_MIX_OSC1,
    PVX_MIX_OSC2,
    PVX_MIX_NOISE,
    PVX_MIX_RING,        /* Software extension; absent from original instrument */
    PVX_FILT_MODE,       /* <16384 LP12; otherwise BP6 */
    PVX_FILT_CUTOFF,     /* 18 Hz .. 18 kHz before modulation */
    PVX_FILT_RES,
    PVX_FILT_ENV_AMT,    /* Bipolar +/-7.5 octaves, center 16384 */
    PVX_FILT_KEYTRACK,
    PVX_FILT_AUTOOSC,    /* Legacy extension: thermal excitation, NOT an original switch */
    PVX_FENV_ATTACK,     /* 1 ms .. 3 s (actual attack duration) */
    PVX_FENV_DECAY,      /* 2 ms .. 12 s (to within 0.1% of sustain) */
    PVX_FENV_SUSTAIN,
    PVX_FENV_RELEASE,    /* 2 ms .. 12 s (to -60 dB) */
    PVX_FENV_REPEAT,     /* Repeat requires sustain=0; note-off always releases */
    PVX_AENV_ATTACK,
    PVX_AENV_DECAY,
    PVX_AENV_SUSTAIN,
    PVX_AENV_RELEASE,
    PVX_AENV_REPEAT,
    PVX_LFO_RATE,        /* 0.1 .. 35 Hz, free running */
    PVX_LFO_WAVE,        /* TRI, SQUARE, S&H, NOISE; four normalized positions */
    PVX_LFO_TO_PITCH,    /* Oscillator I modulation; II has its own depth below */
    PVX_LFO_TO_FILT,
    PVX_LFO_TO_AMP,
    PVX_GLIDE,           /* Oscillator I only, 0..1.5 s time constant */
    PVX_DUOPHONY,        /* Keyboard: highest note -> I, lowest -> II */
    /* v2 additions, appended to preserve the original 34 IDs. */
    PVX_MASTER_TUNE,     /* Bipolar +/-2 semitones, center 16384 */
    PVX_LFO_TO_OSC2,
    PVX_OSC2_PWM,        /* Software extension: LFO -> oscillator II pulse width */
    PVX_MIX_EXTERNAL,    /* Audio input level; input floats use nominal +/-1 */
    PVX_MASTER_LEVEL,
    PVX_VELOCITY,        /* Software extension. 0 = original, velocity insensitive */
    PVX_VCA_HOLD,        /* Open VCA without a held key (drone / external filter) */
    POLIVOKS_NUM_PARAMS
};
enum PolivoksWave { PVX_TRI, PVX_SAW, PVX_SQUARE, PVX_PULSE25, PVX_PULSE12 };
enum PolivoksLfoWave { PVX_LFO_TRI, PVX_LFO_SQUARE, PVX_LFO_SH, PVX_LFO_NOISE };
enum PolivoksRegister { PVX_32, PVX_16, PVX_8, PVX_4, PVX_2 };
struct PolivoksCore;

struct PolivoksCore* polivoksCreate(void);
void polivoksDestroy(struct PolivoksCore* p);
void polivoksDefaultParams(int16_t params[POLIVOKS_NUM_PARAMS]);
int16_t polivoksChoice(int position, int positions);
void polivoksMigrateV1(const int16_t oldParams[34], int16_t params[POLIVOKS_NUM_PARAMS]);

/* Low-level host-managed voices. inc = frequency/rate * 2^32, conventionally
 * the MIDI note's frequency. inc=0 uses the note set by NoteOn/keyboard.
 * Portamento is applied relative to that note, so inc must follow note events.
 * Velocity 0 means note-off. Oscillators remain free running after note-off. */
void polivoksNoteOn(struct PolivoksCore* p, int voice, int midiNote, int velocity);
void polivoksNoteOff(struct PolivoksCore* p, int voice);
void polivoksSetDuoNote(struct PolivoksCore* p, int voice, int midiNoteGen2);

/* Original mono/duo keyboard behavior on voice 0, including held-key fallback
 * and legato envelopes. Use inc=0 when rendering keyboard-managed notes. */
void polivoksKeyDown(struct PolivoksCore* p, int note, int velocity,
                    const int16_t params[POLIVOKS_NUM_PARAMS]);
void polivoksKeyUp(struct PolivoksCore* p, int note,
                  const int16_t params[POLIVOKS_NUM_PARAMS]);

/* Call once per HOST block BEFORE rendering any voices. Advances the common
 * LFO once and shares the exact modulation buffer, regardless of voice order
 * or voice count. Call even for silent blocks to keep the LFO running.
 * Legacy single-voice Render calls can omit this: repeating a voice starts
 * the next block automatically. Multi-voice hosts should use BeginBlock. */
void polivoksBeginBlock(struct PolivoksCore* p,
                       const int16_t params[POLIVOKS_NUM_PARAMS], float rate);
/* 128 actual host-rate samples; 4x oversampling with FIR decimation internally.
 * Supports 8..192 kHz. Bad pointers/voice/rate produce silence where possible.
 * Float API retains headroom for mixing. No allocations during render. */
void polivoksRenderFloat(struct PolivoksCore* p, int voice, float* out,
                        const float* external,
                        const int16_t params[POLIVOKS_NUM_PARAMS],
                        uint32_t inc, float rate);
void polivoksRender(struct PolivoksCore* p, int voice, int16_t* out,
                    const int16_t params[POLIVOKS_NUM_PARAMS],
                    uint32_t inc, float rate);
/* Host integration: exact 1..128 frame segments, with events between segments.
 * Prepare exactly the same frame count that RenderFrames consumes. The sample
 * equations and the legacy 128-frame API are unchanged. */
void polivoksBeginFrames(struct PolivoksCore* p, const int16_t params[POLIVOKS_NUM_PARAMS], float rate, int frames);
void polivoksRenderFrames(struct PolivoksCore* p, int voice, float* out, const float* external,
                         const int16_t params[POLIVOKS_NUM_PARAMS], uint32_t inc, float rate, int frames);
void polivoksPanic(struct PolivoksCore* p); /* immediate all-off, no allocation */
struct PolivoksState {
    float filter_env, amp_env, lfo, phase[2], note1, note2;
    int held_keys;
};
/* Read in the AUDIO thread; copy to a synchronized snapshot before UI access. */
void polivoksGetState(const struct PolivoksCore* p, int voice, struct PolivoksState* out);

float polivoksCutoffHz(int knob);
int polivoksWaveName(int knob, char* out, int n);
void polivoksScope(const int16_t params[POLIVOKS_NUM_PARAMS], float* out, int n, uint32_t seed);
#ifdef __cplusplus
}
#endif
#endif
