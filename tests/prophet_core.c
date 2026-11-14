/* Behavioral checks for the new independent engine; no reference audio files. */
#include "synth/prophet_dsp.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
static void block(ProphetCore *c, const int16_t *p, int frames, float *out) {
    prophetRender(c, out, frames, p, 48000, 120);
    for (int i = 0; i < frames; i++)
        assert(isfinite(out[i]) && fabsf(out[i]) < 8);
}
int main(void) {
    int16_t p[PROPHET_PARAMS];
    float out[128];
    ProphetState s;
    double worst = 0;
    clock_t started = clock();
    double seconds = 0;
    for (int model = 0; model < 2; model++)
        for (int patch = 0; patch < prophetPresetCount(); patch++) {
            ProphetCore *c = prophetCreate();
            assert(c);
            prophetPreset(patch, p);
            p[P5_REVISION] = model ? 32767 : 0;
            for (int n = 0; n < 5; n++)
                prophetKeyDown(c, 48 + 3 * n, 100, p);
            clock_t patchStart = clock();
            double energy = 0;
            for (int b = 0; b < 188; b++) {
                block(c, p, 128, out);
                for (int i = 0; i < 128; i++)
                    energy += out[i] * out[i];
            }
            assert(energy > 1e-6);
            seconds += 188 * 128 / 48000.;
            double usage = (clock() - patchStart) / (double)CLOCKS_PER_SEC / (188 * 128 / 48000.);
            if (usage > worst)
                worst = usage;
            printf("REV %d %s RMS %.5f\n", model ? 2 : 3, prophetPresetName(patch),
                   sqrt(energy / (188 * 128)));
            prophetDestroy(c);
        }
    ProphetCore *c = prophetCreate();
    prophetDefault(p);
    p[P5_UNISON] = 32767;
    p[P5_GLIDE] = 20000;
    prophetKeyDown(c, 60, 100, p);
    prophetKeyDown(c, 72, 100, p);
    block(c, p, 128, out);
    prophetGetState(c, &s);
    for (int i = 0; i < 5; i++)
        assert(s.notes[i] == 60);
    prophetKeyUp(c, 60, p);
    block(c, p, 1, out);
    prophetGetState(c, &s);
    for (int i = 0; i < 5; i++)
        assert(s.notes[i] == 72);
    // Release switch really closes VCA; silent oscillator does not leak.
    p[P5_RELEASE] = 0;
    prophetKeyUp(c, 72, p);
    for (int b = 0; b < 100; b++)
        block(c, p, 128, out);
    prophetGetState(c, &s);
    for (int i = 0; i < 5; i++)
        assert(s.amp[i] == 0);
    // Common LFO advances through silence and zero wheel position.
    prophetPanic(c);
    p[P5_LFOSYNC] = 32767;
    p[P5_LFODIV] = prophetChoice(21, 24);
    assert(fabsf(prophetLfoHz(p, 120) - 64) < .001);
    p[P5_LFODIV] = prophetChoice(6, 24);
    prophetResetLfo(c);
    for (int b = 0; b < 93; b++)
        block(c, p, 128, out);
    block(c, p, 96, out);
    prophetGetState(c, &s);
    assert(fabsf(s.lfoPhase - .5f) < .001);
    // Five-note polyphony, stealing and returning from UNISON with held keys.
    prophetPanic(c);
    p[P5_UNISON] = 0;
    for (int n = 60; n < 66; n++)
        prophetKeyDown(c, n, 100, p);
    block(c, p, 128, out);
    prophetGetState(c, &s);
    for (int i = 0; i < 5; i++)
        assert(s.notes[i] >= 61 && s.notes[i] <= 65);
    // Very low/high notes, simultaneous waves, audio-rate Poly-Mod and resonance.
    for (int test = 0; test < 12; test++) {
        prophetPanic(c);
        prophetDefault(p);
        p[P5_REVISION] = test % 2 ? 32767 : 0;
        p[P5_RESONANCE] = 32767;
        p[P5_AWAVE] = 32767;
        p[P5_BWAVE] = 32767;
        p[P5_PMB] = 32767;
        p[P5_PMFREQ] = p[P5_PMFILTER] = p[P5_PMPW] = 32767;
        p[P5_CUTOFF] = test * 2978;
        p[P5_BFREQ] = 32767;
        prophetKeyDown(c, test % 2 ? 0 : 127, 127, p);
        for (int b = 0; b < 40; b++)
            block(c, p, (b % 127) + 1, out);
    }
    // Switching the revision while holding a note keeps the voice/envelopes.
    prophetPanic(c);
    prophetDefault(p);
    prophetKeyDown(c, 60, 100, p);
    for (int k = 0; k < 120; k++) {
        p[P5_REVISION] = k < 40 || k >= 80 ? 0 : 32767;
        block(c, p, 128, out);
    }
    prophetGetState(c, &s);
    assert(s.held == 1 && s.amp[0] > 0);
    prophetDestroy(c);
    printf("PASS; core CPU / rendered seconds = %.3f; worst %.3f\n",
           (clock() - started) / (double)CLOCKS_PER_SEC / seconds, worst);
    return 0;
}
