#include "polivoks_presets.h"
#include <math.h>

static const struct PolivoksPresetInfo presets[] = {
    {"foundation", "FOUNDATION", "BASS", "Square/saw bass with a solid floor and moderate resonance.", PVX_DEMO_BASS, 0},
    {"iron_teeth", "IRON TEETH", "BASS", "Hot dual saws, more upper harmonics, driven low-pass.", PVX_DEMO_BASS, 0},
    {"acid_motor", "ACID MOTOR", "BASS", "Fast filter accents with resonance below continuous self-oscillation.", PVX_DEMO_BASS, 0},
    {"unison_wire", "UNISON WIRE", "LEAD", "Open saw/pulse lead with small detune and portamento.", PVX_DEMO_LEAD, 0},
    {"formant_snarl", "FORMANT SNARL", "LEAD", "Intentionally nasal band-pass lead with restrained filter modulation.", PVX_DEMO_LEAD, 0},
    {"repeat_body", "REPEAT BODY", "DUO", "Looping filter envelope above a raised cutoff floor; sustained amplifier.", PVX_DEMO_DUO, 0},
    {"metal_body", "METAL BODY", "FM", "Moderate exponential FM with audible oscillator body and open low-pass.", PVX_DEMO_LEAD, 0},
    {"metal_ring", "METAL RING", "FM", "METAL BODY with ring level added; all other parameters are identical.", PVX_DEMO_LEAD, 1},
    {"sample_factory", "SAMPLE FACTORY", "MOD", "Sample-and-hold movement, moderate depth, low-pass with retained body.", PVX_DEMO_LEAD, 0},
    {"slow_copper", "SLOW COPPER", "DUO", "Slow two-note texture, beating oscillators, no chorus or reverb.", PVX_DEMO_SLOW, 0},
    {"noise_press", "NOISE PRESS", "PERC", "Short filtered noise hits with a small low oscillator component.", PVX_DEMO_DRUM, 0},
    {"low_orbit", "LOW ORBIT", "SUB", "32-foot triangle plus 16-foot saw to keep the sub audible on smaller speakers.", PVX_DEMO_BASS, 0},
    {"red_hammer", "RED HAMMER", "BASS", "Punchy saw and sub-pulse bass with a fast filter strike.", PVX_DEMO_BASS, 0},
    {"pulse_engine", "PULSE ENGINE", "BASS", "Narrow pulse bass with slow motion in oscillator II's pulse width.", PVX_DEMO_BASS, 1},
    {"buried_copper", "BURIED COPPER", "SUB", "Deep 32-foot triangle/saw bass, low resonance and a long round tail.", PVX_DEMO_BASS, 0},
    {"razor_fifth", "RAZOR FIFTH", "LEAD", "High saw fifth above a pulse lead, with cross-mod bite and glide.", PVX_DEMO_LEAD, 0},
    {"glass_current", "GLASS CURRENT", "BELL", "Decaying triangle FM and ring bell with a falling amplifier contour.", PVX_DEMO_LEAD, 1},
    {"vowel_press", "VOWEL PRESS", "LEAD", "Band-pass pulse voice with an inverted envelope and a closing vowel.", PVX_DEMO_LEAD, 0},
    {"duo_brass", "DUO BRASS", "DUO", "Two-note square/saw brass with a rounded attack and independent vibrato.", PVX_DEMO_DUO, 0},
    {"pwm_horizon", "PWM HORIZON", "PAD", "Slow duophonic pulse-width texture with gentle filter breathing.", PVX_DEMO_SLOW, 1},
    {"radio_steps", "RADIO STEPS", "MOD", "Dusty stepped pitch/filter motion and restrained metallic cross-mod.", PVX_DEMO_LEAD, 1},
    {"geiger_heart", "GEIGER HEART", "FX", "Both envelopes repeat: short resonant noise pulses under a held key.", PVX_DEMO_DRUM, 0},
    {"steel_clap", "STEEL CLAP", "PERC", "Tight band-pass noise hit with a small pitched metallic edge.", PVX_DEMO_DRUM, 1},
    {"snow_circuit", "SNOW CIRCUIT", "PAD", "Slow noisy duophonic band-pass texture with irregular filter modulation.", PVX_DEMO_SLOW, 0}
};
static int16_t knob(float x) {
    if (x < 0) x = 0;
    if (x > 1) x = 1;
    return (int16_t)lroundf(x * 32767);
}
static int16_t centered(float x) {
    long value = lroundf(16384 + 16384*x);
    return (int16_t)(value < 0 ? 0 : value > 32767 ? 32767 : value);
}
static int16_t timeKnob(float seconds, float minimum, float maximum) {
    float x = (seconds-minimum)/(maximum-minimum);
    return knob(cbrtf(x < 0 ? 0 : x));
}
static void cutoff(int16_t* p, float hz) { p[PVX_FILT_CUTOFF] = knob(log2f(hz/18)/9.965784f); }
static void filterEnv(int16_t* p, float octaves, float a, float d, float s, float r) {
    p[PVX_FILT_ENV_AMT] = centered(octaves/7.5f);
    p[PVX_FENV_ATTACK] = timeKnob(a,0.001f,3);
    p[PVX_FENV_DECAY] = timeKnob(d,0.002f,12);
    p[PVX_FENV_SUSTAIN] = knob(s);
    p[PVX_FENV_RELEASE] = timeKnob(r,0.002f,12);
}
static void ampEnv(int16_t* p, float a, float d, float s, float r) {
    p[PVX_AENV_ATTACK] = timeKnob(a,0.001f,3);
    p[PVX_AENV_DECAY] = timeKnob(d,0.002f,12);
    p[PVX_AENV_SUSTAIN] = knob(s);
    p[PVX_AENV_RELEASE] = timeKnob(r,0.002f,12);
}
static void oscillators(int16_t* p, int register1, int wave1, int register2, int wave2, float level1, float level2, float cents) {
    p[PVX_OSC1_OCT] = polivoksChoice(register1,5);
    p[PVX_OSC2_OCT] = polivoksChoice(register2,5);
    p[PVX_OSC1_WAVE] = polivoksChoice(wave1,5);
    p[PVX_OSC2_WAVE] = polivoksChoice(wave2,5);
    p[PVX_MIX_OSC1] = knob(level1); p[PVX_MIX_OSC2] = knob(level2);
    p[PVX_OSC2_FINE] = centered(cents/50);
}
static void lfo(int16_t* p, int wave, float hz, float cents1, float cents2, float filterOctaves) {
    p[PVX_LFO_WAVE] = polivoksChoice(wave,4);
    p[PVX_LFO_RATE] = knob(logf(hz/0.1f)/logf(350));
    p[PVX_LFO_TO_PITCH] = knob(cents1/1800);
    p[PVX_LFO_TO_OSC2] = knob(cents2/1800);
    p[PVX_LFO_TO_FILT] = knob(filterOctaves/4.5f);
}
static void glide(int16_t* p, float seconds) {
    p[PVX_GLIDE] = seconds <= 0 ? 0 : knob(sqrtf(fmaxf(0,seconds-0.001f)/1.5f));
}
int polivoksPresetCount(void) { return (int)(sizeof(presets)/sizeof(presets[0])); }
const struct PolivoksPresetInfo* polivoksPresetInfo(int index) {
    return index >= 0 && index < polivoksPresetCount() ? &presets[index] : 0;
}
int polivoksLoadPreset(int index, int16_t p[POLIVOKS_NUM_PARAMS]) {
    if (!p || !polivoksPresetInfo(index)) return 0;
    polivoksDefaultParams(p);
    p[PVX_MASTER_LEVEL] = knob(0.85f);
    p[PVX_MIX_NOISE] = 0;
    p[PVX_FILT_KEYTRACK] = knob(0.35f);
    p[PVX_FILT_RES] = knob(0.35f);
    filterEnv(p,2,0.003f,0.3f,0.3f,0.2f);
    ampEnv(p,0.004f,0.4f,0.85f,0.2f);
    switch (index) {
    case 0:
        oscillators(p,PVX_16,PVX_SQUARE,PVX_16,PVX_SAW,0.72f,0.55f,5);
        cutoff(p,180); p[PVX_FILT_RES]=knob(0.3f);
        filterEnv(p,3,0.002f,0.26f,0.22f,0.18f);
        ampEnv(p,0.003f,0.4f,0.82f,0.18f);
        break;
    case 1:
        oscillators(p,PVX_16,PVX_SAW,PVX_16,PVX_SAW,0.92f,0.8f,-8);
        cutoff(p,330); p[PVX_FILT_RES]=knob(0.57f);
        filterEnv(p,3.3f,0.003f,0.32f,0.3f,0.2f);
        break;
    case 2:
        oscillators(p,PVX_16,PVX_SAW,PVX_8,PVX_SQUARE,0.8f,0.25f,3);
        cutoff(p,230); p[PVX_FILT_RES]=knob(0.76f);
        filterEnv(p,4,0.001f,0.18f,0.1f,0.1f);
        ampEnv(p,0.002f,0.25f,0.88f,0.1f);
        glide(p,0.025f);
        break;
    case 3:
        oscillators(p,PVX_8,PVX_SAW,PVX_8,PVX_PULSE25,0.7f,0.6f,7);
        cutoff(p,2100); p[PVX_FILT_RES]=knob(0.38f);
        filterEnv(p,1.5f,0.015f,0.5f,0.6f,0.32f);
        ampEnv(p,0.012f,0.5f,0.9f,0.32f);
        lfo(p,PVX_LFO_TRI,4.8f,4,3,0); glide(p,0.07f);
        break;
    case 4:
        oscillators(p,PVX_8,PVX_SQUARE,PVX_8,PVX_PULSE12,0.85f,0.6f,-12);
        cutoff(p,650); p[PVX_FILT_MODE]=32767; p[PVX_FILT_RES]=knob(0.66f);
        filterEnv(p,1.2f,0.008f,0.4f,0.45f,0.25f);
        lfo(p,PVX_LFO_TRI,5,0,0,0.3f); glide(p,0.035f);
        break;
    case 5:
        oscillators(p,PVX_8,PVX_SAW,PVX_8,PVX_SQUARE,0.72f,0.7f,4);
        cutoff(p,520); p[PVX_FILT_RES]=knob(0.5f); p[PVX_DUOPHONY]=32767;
        filterEnv(p,3,0.04f,0.32f,0,0.3f); p[PVX_FENV_REPEAT]=32767;
        ampEnv(p,0.008f,0.5f,1,0.45f); glide(p,0.05f);
        break;
    case 6: case 7:
        oscillators(p,PVX_8,PVX_SAW,PVX_8,PVX_TRI,0.78f,0.45f,0);
        p[PVX_OSC2_DETUNE]=32767; p[PVX_XMOD_DEPTH]=knob(0.13f);
        p[PVX_MIX_NOISE]=knob(0.008f);
        cutoff(p,2500); p[PVX_FILT_RES]=knob(0.45f);
        filterEnv(p,1.3f,0.005f,0.35f,0.5f,0.25f);
        if (index==7) p[PVX_MIX_RING]=knob(0.48f);
        break;
    case 8:
        oscillators(p,PVX_8,PVX_SQUARE,PVX_8,PVX_SAW,0.7f,0.65f,5);
        cutoff(p,850); p[PVX_FILT_RES]=knob(0.6f); p[PVX_XMOD_DEPTH]=knob(0.04f);
        filterEnv(p,0.8f,0.005f,0.35f,0.7f,0.3f);
        lfo(p,PVX_LFO_SH,7.5f,0,0,1.2f); glide(p,0.05f);
        break;
    case 9:
        oscillators(p,PVX_8,PVX_PULSE25,PVX_8,PVX_SAW,0.65f,0.7f,-6);
        cutoff(p,900); p[PVX_DUOPHONY]=32767;
        filterEnv(p,1.4f,1,1,0.7f,1);
        ampEnv(p,0.6f,1,1,1);
        lfo(p,PVX_LFO_TRI,0.4f,2,3,0.3f); glide(p,0.18f);
        break;
    case 10:
        oscillators(p,PVX_32,PVX_SQUARE,PVX_16,PVX_SAW,0.1f,0,0);
        p[PVX_MIX_NOISE]=knob(0.8f); p[PVX_FILT_MODE]=32767;
        cutoff(p,1000); p[PVX_FILT_RES]=knob(0.55f);
        filterEnv(p,1.5f,0.001f,0.28f,0,0.2f);
        ampEnv(p,0.004f,0.55f,0,0.22f);
        break;
    case 11:
        oscillators(p,PVX_32,PVX_TRI,PVX_16,PVX_SAW,0.8f,0.28f,-4);
        cutoff(p,480); p[PVX_FILT_RES]=knob(0.25f); p[PVX_XMOD_DEPTH]=knob(0.02f);
        filterEnv(p,2.2f,0.008f,0.45f,0.3f,0.3f);
        ampEnv(p,0.006f,0.4f,0.9f,0.3f);
        break;
    case 12:
        oscillators(p,PVX_16,PVX_SAW,PVX_32,PVX_PULSE25,.82f,.4f,-3);
        cutoff(p,150); p[PVX_FILT_RES]=knob(.5f);
        filterEnv(p,3.8f,.001f,.2f,.18f,.14f); ampEnv(p,.002f,.25f,.83f,.16f);
        break;
    case 13:
        oscillators(p,PVX_16,PVX_PULSE12,PVX_16,PVX_SQUARE,.72f,.55f,-4);
        cutoff(p,400); p[PVX_FILT_RES]=knob(.58f);
        filterEnv(p,2.7f,.003f,.38f,.25f,.24f); ampEnv(p,.004f,.4f,.88f,.24f);
        lfo(p,PVX_LFO_TRI,.7f,0,0,.08f);p[PVX_OSC2_PWM]=knob(.35f);glide(p,.02f);
        break;
    case 14:
        oscillators(p,PVX_32,PVX_TRI,PVX_32,PVX_SAW,.78f,.3f,2);
        cutoff(p,400);p[PVX_FILT_RES]=knob(.18f);
        filterEnv(p,1.3f,.008f,.5f,.3f,.45f);ampEnv(p,.01f,.7f,.9f,.5f);
        break;
    case 15:
        oscillators(p,PVX_8,PVX_PULSE25,PVX_4,PVX_SAW,.7f,.4f,5);
        p[PVX_OSC2_DETUNE]=32767;p[PVX_XMOD_DEPTH]=knob(.025f);
        cutoff(p,1800);p[PVX_FILT_RES]=knob(.43f);
        filterEnv(p,1.5f,.004f,.4f,.55f,.3f);ampEnv(p,.004f,.4f,.88f,.3f);
        lfo(p,PVX_LFO_TRI,4.6f,4,3,0);glide(p,.1f);
        break;
    case 16:
        oscillators(p,PVX_4,PVX_TRI,PVX_4,PVX_TRI,.62f,.32f,0);
        p[PVX_OSC2_DETUNE]=32767;p[PVX_XMOD_DEPTH]=knob(.2f);p[PVX_MIX_RING]=knob(.25f);
        cutoff(p,4300);p[PVX_FILT_RES]=knob(.3f);
        filterEnv(p,-1,.002f,1.1f,0,.8f);ampEnv(p,.002f,1.3f,0,.9f);
        break;
    case 17:
        oscillators(p,PVX_8,PVX_SQUARE,PVX_4,PVX_PULSE12,.75f,.4f,-5);
        p[PVX_OSC2_DETUNE]=centered(-5.f/7);p[PVX_XMOD_DEPTH]=knob(.03f);
        cutoff(p,1200);p[PVX_FILT_MODE]=32767;p[PVX_FILT_RES]=knob(.72f);
        filterEnv(p,-1.8f,.02f,.55f,.1f,.3f);ampEnv(p,.004f,.5f,.9f,.3f);
        lfo(p,PVX_LFO_TRI,2.3f,5,0,.15f);
        break;
    case 18:
        oscillators(p,PVX_8,PVX_SQUARE,PVX_8,PVX_SAW,.7f,.65f,9);
        cutoff(p,750);p[PVX_FILT_RES]=knob(.3f);p[PVX_DUOPHONY]=32767;
        filterEnv(p,2.4f,.12f,.65f,.3f,.45f);ampEnv(p,.045f,.5f,.9f,.55f);
        lfo(p,PVX_LFO_TRI,5.5f,2,4,.05f);glide(p,.08f);
        break;
    case 19:
        oscillators(p,PVX_8,PVX_SAW,PVX_8,PVX_SQUARE,.62f,.68f,-8);
        cutoff(p,1000);p[PVX_FILT_RES]=knob(.4f);p[PVX_DUOPHONY]=32767;
        filterEnv(p,1.6f,.85f,1.2f,.65f,1.7f);ampEnv(p,.55f,1.5f,.9f,1.6f);
        lfo(p,PVX_LFO_TRI,.18f,3,2,.22f);p[PVX_OSC2_PWM]=knob(.7f);
        break;
    case 20:
        oscillators(p,PVX_8,PVX_PULSE25,PVX_4,PVX_TRI,.65f,.24f,2);
        cutoff(p,1250);p[PVX_FILT_RES]=knob(.65f);
        p[PVX_XMOD_DEPTH]=knob(.08f);p[PVX_MIX_RING]=knob(.15f);p[PVX_MIX_NOISE]=knob(.08f);
        filterEnv(p,.6f,.007f,.6f,.7f,.4f);ampEnv(p,.005f,.6f,.88f,.4f);
        lfo(p,PVX_LFO_SH,3.8f,120,30,1);
        break;
    case 21:
        oscillators(p,PVX_32,PVX_SQUARE,PVX_16,PVX_TRI,.25f,.1f,0);
        p[PVX_MIX_NOISE]=knob(.55f);cutoff(p,850);p[PVX_FILT_RES]=knob(.8f);
        filterEnv(p,4,.003f,.085f,0,.1f);ampEnv(p,.003f,.16f,0,.1f);
        p[PVX_FENV_REPEAT]=p[PVX_AENV_REPEAT]=32767;
        break;
    case 22:
        oscillators(p,PVX_32,PVX_TRI,PVX_4,PVX_SQUARE,.1f,.12f,0);
        p[PVX_MIX_NOISE]=knob(.9f);p[PVX_MIX_RING]=knob(.08f);
        cutoff(p,1600);p[PVX_FILT_MODE]=32767;p[PVX_FILT_RES]=knob(.3f);
        filterEnv(p,2.1f,.001f,.09f,0,.08f);ampEnv(p,.001f,.12f,0,.09f);
        break;
    case 23:
        oscillators(p,PVX_16,PVX_PULSE25,PVX_8,PVX_TRI,.22f,.12f,3);
        p[PVX_MIX_NOISE]=knob(.45f);p[PVX_DUOPHONY]=32767;
        cutoff(p,900);p[PVX_FILT_MODE]=32767;p[PVX_FILT_RES]=knob(.72f);
        filterEnv(p,.4f,1.4f,2,.85f,1.4f);ampEnv(p,.6f,1.5f,.8f,1.8f);
        lfo(p,PVX_LFO_NOISE,.8f,0,0,.6f);
        break;
    }
    return 1;
}
