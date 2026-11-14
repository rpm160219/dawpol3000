/* Polivoks-inspired behavioral model, not a transistor-level circuit replica.
 * Four-times oversampled VCO/mixer/nonlinear implicit SVF/VCA, then FIR.
 * No old 48k core / linear interpolation path, inaccurate pitch approximations,
 * or mandatory hard clipping. See research_filter.md for equations and sources.
 */
#include "polivoks_dsp.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PVX_PI 3.14159265358979323846
#define PVX_FIR 97
#define PVX_OS_BLOCK (POLIVOKS_BLOCK * POLIVOKS_OVERSAMPLE)
#define PVX_INV32 (1.0 / 4294967296.0)

typedef enum { ENV_IDLE, ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE } EnvStage;
typedef struct { EnvStage stage; float level; } Envelope;
typedef struct {
    double phase[2];
    uint32_t rng;
    float cur_oct1, target_oct1, target_oct2;
    int has_note, gate;
    Envelope fenv, aenv;
    float velocity;
    float trim[2], filter_trim;
    float drift[2], drift_target[2], drift_timer;
    float osc_lp[2], osc_dc[2];
    float bp, lp, s1, s2, filter_dc, output_dc, supply;
    float smooth[POLIVOKS_NUM_PARAMS];
    int initialized;
    float fir[PVX_FIR];
    unsigned fir_pos;
    float external_prev;
} Voice;
struct PolivoksCore {
    Voice v[POLIVOKS_VOICES];
    double lfo_phase;
    uint32_t lfo_rng;
    float lfo_sh, lfo_smooth;
    float lfo[PVX_OS_BLOCK];
    float fir_coeff[PVX_FIR];
    unsigned rendered;
    int block_ready;
    int block_frames;
    float block_rate;
    unsigned char keys[128];
};
static float clampf(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
static uint32_t random32(uint32_t* s) { *s = *s * 1664525u + 1013904223u; return *s; }
static float noise(uint32_t* s) { return (float)(random32(s) >> 8) * (1.0f / 8388608.0f) - 1.0f; }
static float norm(int x) { return clampf((float)x / 32767.0f, 0.0f, 1.0f); }
static float bipolar(float x) { return (x - 16384.0f) / 16384.0f; }
static int choice(int x, int n) { return (int)(clampf((float)x, 0, 32767) * n / 32768.0f); }
static int validRate(float rate) { return isfinite(rate) && rate >= 8000 && rate <= 192000; }
static float coefficient(float seconds, float fs) { return -expm1f(-1.0f / (seconds * fs)); }

/* Monotonic, unity-slope at zero, no DC at zero. Different rails yield even
 * harmonics without the previous x+0.06*x*x sign reversal at large negative x. */
static float saturate(float x, float positive, float negative, float* derivative) {
    float rail = x >= 0 ? positive : negative;
    float q = x / rail;
    float r = 1.0f / sqrtf(1.0f + q * q);
    if (derivative) *derivative = r * r * r;
    return x * r;
}
static float blep(float t, float dt) {
    if (t < dt) { float x = t / dt; return x + x - x*x - 1; }
    if (t > 1-dt) { float x = (t-1) / dt; return x*x + x + x + 1; }
    return 0;
}
static float blamp(float t, float dt) {
    if (t < dt) { float x = 1-t/dt; return dt*x*x*x/3; }
    if (t > 1-dt) { float x = 1+(t-1)/dt; return dt*x*x*x/3; }
    return 0;
}
static float wrap(float t) { return t - floorf(t); }
static float oscillator(int wave, double phase, float dt, float duty_error, float pwm) {
    float t = (float)phase;
    if (wave == PVX_TRI)
        return 1 - 4*fabsf(t-0.5f) + 4*(blamp(t,dt)-blamp(wrap(t+0.5f),dt));
    if (wave == PVX_SAW) return 2*t-1-blep(t,dt);
    float duty = wave == PVX_SQUARE ? 0.5f : (wave == PVX_PULSE25 ? 0.25f : 0.125f);
    duty = clampf(duty + duty_error + pwm, 0.03f, 0.97f);
    /* Correct DC for arbitrary pulse width BEFORE nonlinear FM/mixing. */
    return 2*((t < duty ? 1.0f : 0.0f)-duty) + blep(t,dt)-blep(wrap(t-duty),dt);
}
static float envelopeTime(float knob, float minimum, float maximum) {
    float k = norm((int)knob);
    return minimum + (maximum-minimum)*k*k*k;
}
/* Times describe measured stage durations, not unexplained RC time constants.
 * Attack targets 1.25; decay/release reach -60 dB in the specified time. */
static float stepEnvelope(Envelope* e, int gate, int repeat, float ac, float dc, float sustain, float rc) {
    if (!gate && e->stage != ENV_IDLE) e->stage = ENV_RELEASE;
    switch (e->stage) {
    case ENV_ATTACK:
        e->level += (1.25f-e->level)*ac;
        if (e->level >= 1) { e->level = 1; e->stage = ENV_DECAY; }
        break;
    case ENV_DECAY:
        e->level += (sustain-e->level)*dc;
        if (fabsf(e->level-sustain) <= 0.001f) {
            e->level = sustain;
            e->stage = repeat && gate && sustain == 0 ? ENV_ATTACK : ENV_SUSTAIN;
        }
        break;
    case ENV_SUSTAIN:
        e->level = sustain;
        if (repeat && gate && sustain == 0) e->stage = ENV_ATTACK;
        break;
    case ENV_RELEASE:
        e->level *= 1-rc;
        if (e->level <= 0.00001f) { e->level = 0; e->stage = ENV_IDLE; }
        break;
    default: e->level = 0; break;
    }
    return e->level;
}

/* Implicit trapezoidal two-integrator loop. Both current sample states appear
 * in the feedback equations, solved together using a 2x2 Newton iteration.
 * Saturated differential currents model finite slew; unequal rails and cubic
 * rail restoring currents stop negative damping from running away. Input level
 * changes the loop gain, so overdrive can suppress self-oscillation naturally.
 */
static void filterStep(Voice* v, float input, float g, float damping, float supply) {
    float b = v->bp, l = v->lp;
    float g1 = g*(1.0f+v->filter_trim), g2 = g*(1.0f-v->filter_trim);
    for (int j = 0; j < 6; ++j) {
        float d1, d2;
        float drive = saturate(input-l-damping*b, 2.65f*supply, 2.3f*supply, &d1);
        float second = saturate(b, 2.4f*supply, 2.7f*supply, &d2);
        float f1 = b-v->s1-g1*(drive-0.012f*b-0.065f*b*b*b);
        float f2 = l-v->s2-g2*(second-0.008f*l-0.045f*l*l*l);
        float a = 1+g1*(damping*d1+0.012f+0.195f*b*b);
        float c = g1*d1;
        float d = -g2*d2;
        float e = 1+g2*(0.008f+0.135f*l*l);
        float inv = 1/(a*e-c*d);
        float db = (e*f1-c*f2)*inv;
        float dl = (a*f2-d*f1)*inv;
        b -= db; l -= dl;
        if (fabsf(db)+fabsf(dl) < 1e-6f) break;
    }
    v->s1 = 2*b-v->s1;
    v->s2 = 2*l-v->s2;
    v->bp = b; v->lp = l;
}

int16_t polivoksChoice(int position, int positions) {
    if (positions <= 0 || positions > 32768) return 0;
    if (position < 0) position = 0;
    if (position >= positions) position = positions-1;
    return (int16_t)(((2*position+1)*32768)/(2*positions));
}
void polivoksDefaultParams(int16_t p[POLIVOKS_NUM_PARAMS]) {
    if (!p) return;
    memset(p,0,POLIVOKS_NUM_PARAMS*sizeof(*p));
    p[PVX_OSC1_OCT] = p[PVX_OSC2_OCT] = polivoksChoice(PVX_8,5);
    p[PVX_OSC1_WAVE] = polivoksChoice(PVX_SAW,5);
    p[PVX_OSC2_WAVE] = polivoksChoice(PVX_SQUARE,5);
    p[PVX_OSC2_DETUNE] = p[PVX_OSC2_FINE] = p[PVX_MASTER_TUNE] = 16384;
    p[PVX_MIX_OSC1] = 18500; p[PVX_MIX_OSC2] = 15000;
    p[PVX_FILT_CUTOFF] = 18000; p[PVX_FILT_RES] = 14000;
    p[PVX_FILT_ENV_AMT] = 22000; p[PVX_FILT_KEYTRACK] = 16384;
    p[PVX_FILT_AUTOOSC] = 32767;
    p[PVX_FENV_ATTACK] = 500; p[PVX_FENV_DECAY] = 10500;
    p[PVX_FENV_SUSTAIN] = 3500; p[PVX_FENV_RELEASE] = 8500;
    p[PVX_AENV_ATTACK] = 400; p[PVX_AENV_DECAY] = 13000;
    p[PVX_AENV_SUSTAIN] = 25000; p[PVX_AENV_RELEASE] = 9000;
    p[PVX_LFO_RATE] = 14000; p[PVX_MASTER_LEVEL] = 26000;
}
void polivoksMigrateV1(const int16_t old[34], int16_t p[POLIVOKS_NUM_PARAMS]) {
    if (!old || !p) return;
    int16_t saved[34]; memcpy(saved,old,sizeof(saved));
    polivoksDefaultParams(p); memcpy(p,saved,sizeof(saved));
    p[PVX_OSC1_OCT] = polivoksChoice(choice(saved[PVX_OSC1_OCT],4)+1,5);
    p[PVX_OSC2_OCT] = polivoksChoice(choice(saved[PVX_OSC2_OCT],4)+1,5);
    p[PVX_LFO_TO_OSC2] = saved[PVX_LFO_TO_PITCH];
    p[PVX_VELOCITY] = 32767;
}
struct PolivoksCore* polivoksCreate(void) {
    struct PolivoksCore* p = calloc(1,sizeof(*p));
    if (!p) return NULL;
    p->lfo_rng = 0x5a17c903u; p->lfo_sh = noise(&p->lfo_rng);
    /* 97-tap Blackman-windowed sinc, cutoff 0.1125 cycles/oversample.
     * Unity DC gain. Passband and latency are documented in README. */
    double sum = 0;
    for (int i=0; i<PVX_FIR; ++i) {
        double m = i-(PVX_FIR-1)*0.5;
        double sinc = m == 0 ? 0.225 : sin(2*PVX_PI*0.1125*m)/(PVX_PI*m);
        double window = 0.42-0.5*cos(2*PVX_PI*i/(PVX_FIR-1))+0.08*cos(4*PVX_PI*i/(PVX_FIR-1));
        p->fir_coeff[i] = (float)(sinc*window); sum += sinc*window;
    }
    for (int i=0; i<PVX_FIR; ++i) p->fir_coeff[i] /= (float)sum;
    for (int i=0; i<POLIVOKS_VOICES; ++i) {
        Voice* v = &p->v[i];
        v->rng = 0xc47b2911u*(uint32_t)(i+1);
        for (int j=0; j<2; ++j) {
            v->phase[j] = random32(&v->rng)*PVX_INV32;
            v->trim[j] = noise(&v->rng)*0.0017f; /* +/-2 cents, not +/-20 */
            v->drift_target[j] = noise(&v->rng)*0.0012f;
        }
        v->filter_trim = noise(&v->rng)*0.018f;
        v->velocity = 1; v->supply = 1;
        v->drift_timer = 0.1f;
    }
    return p;
}
void polivoksDestroy(struct PolivoksCore* p) { free(p); }
static void targets(Voice* v, int note1, int note2, int retrigger, int velocity) {
    v->target_oct1 = (clampf((float)note1,0,127)-60)/12;
    v->target_oct2 = (clampf((float)note2,0,127)-60)/12;
    if (!v->has_note) { v->cur_oct1 = v->target_oct1; v->has_note = 1; }
    v->velocity = clampf((float)velocity,1,127)/127;
    v->gate = 1;
    if (retrigger) { v->fenv.stage = ENV_ATTACK; v->aenv.stage = ENV_ATTACK; }
}
void polivoksNoteOn(struct PolivoksCore* p, int voice, int note, int velocity) {
    if (!p || voice < 0 || voice >= POLIVOKS_VOICES) return;
    if (velocity <= 0) { polivoksNoteOff(p,voice); return; }
    targets(&p->v[voice],note,note,1,velocity);
}
void polivoksNoteOff(struct PolivoksCore* p, int voice) {
    if (!p || voice < 0 || voice >= POLIVOKS_VOICES) return;
    p->v[voice].gate = 0;
}
void polivoksSetDuoNote(struct PolivoksCore* p, int voice, int note) {
    if (!p || voice < 0 || voice >= POLIVOKS_VOICES) return;
    p->v[voice].target_oct2 = (clampf((float)note,0,127)-60)/12;
}
static void updateKeyboard(struct PolivoksCore* p, const int16_t params[POLIVOKS_NUM_PARAMS]) {
    int lo=-1, hi=-1;
    for (int i=0; i<128; ++i) if (p->keys[i]) { if (lo<0) lo=i; hi=i; }
    if (lo<0) { polivoksNoteOff(p,0); return; }
    targets(&p->v[0],hi,params[PVX_DUOPHONY]>=16384 ? lo : hi,!p->v[0].gate,p->keys[hi]);
}
void polivoksKeyDown(struct PolivoksCore* p, int note, int velocity, const int16_t params[POLIVOKS_NUM_PARAMS]) {
    if (!p || !params || note<0 || note>127) return;
    if (velocity<=0) { polivoksKeyUp(p,note,params); return; }
    p->keys[note] = (unsigned char)(velocity>127 ? 127 : velocity);
    updateKeyboard(p,params);
}
void polivoksKeyUp(struct PolivoksCore* p, int note, const int16_t params[POLIVOKS_NUM_PARAMS]) {
    if (!p || !params || note<0 || note>127) return;
    p->keys[note] = 0; updateKeyboard(p,params);
}
void polivoksBeginFrames(struct PolivoksCore* p, const int16_t params[POLIVOKS_NUM_PARAMS], float rate, int frames) {
    if (!p || !params || !validRate(rate) || frames < 1 || frames > POLIVOKS_BLOCK) { if (p) p->block_ready=0; return; }
    for (int note=0; note<128; ++note) if (p->keys[note]) {
        updateKeyboard(p,params); break;
    }
    float fs = rate*POLIVOKS_OVERSAMPLE;
    float hz = 0.1f*powf(350.0f,norm(params[PVX_LFO_RATE]));
    int wave = choice(params[PVX_LFO_WAVE],4);
    float smooth = coefficient(0.00035f,fs);
    for (int i=0; i<frames*POLIVOKS_OVERSAMPLE; ++i) {
        p->lfo_phase += hz/fs;
        if (p->lfo_phase >= 1) { p->lfo_phase-=1; p->lfo_sh=noise(&p->lfo_rng); }
        float t = (float)p->lfo_phase;
        float value = wave == PVX_LFO_TRI ? 1-4*fabsf(t-0.5f) :
                      wave == PVX_LFO_SQUARE ? (t<0.5f ? 1 : -1) :
                      wave == PVX_LFO_SH ? p->lfo_sh : noise(&p->lfo_rng);
        p->lfo_smooth += (value-p->lfo_smooth)*smooth;
        p->lfo[i] = p->lfo_smooth;
    }
    p->block_rate=rate; p->rendered=0; p->block_ready=1; p->block_frames=frames;
}
static int continuous(int i) {
    switch (i) {
    case PVX_OSC1_OCT: case PVX_OSC2_OCT: case PVX_OSC1_WAVE: case PVX_OSC2_WAVE:
    case PVX_FILT_MODE: case PVX_FILT_AUTOOSC: case PVX_FENV_REPEAT:
    case PVX_AENV_REPEAT: case PVX_LFO_WAVE: case PVX_DUOPHONY: case PVX_VCA_HOLD: return 0;
    default: return 1;
    }
}
void polivoksRenderFrames(struct PolivoksCore* p, int voice, float* out, const float* external,
                        const int16_t params[POLIVOKS_NUM_PARAMS], uint32_t inc, float rate, int frames) {
    if (!out || frames < 1 || frames > POLIVOKS_BLOCK) return;
    if (!p || !params || voice<0 || voice>=POLIVOKS_VOICES || !validRate(rate)) {
        memset(out,0,(size_t)frames*sizeof(*out)); return;
    }
    if (!p->block_ready || (p->rendered & (1u<<voice)) || p->block_rate!=rate || p->block_frames!=frames)
        polivoksBeginFrames(p,params,rate,frames);
    p->rendered |= 1u<<voice;
    Voice* v=&p->v[voice];
    float target[POLIVOKS_NUM_PARAMS];
    for (int j=0; j<POLIVOKS_NUM_PARAMS; ++j) {
        target[j]=clampf((float)params[j],0,32767);
        if (!v->initialized || !continuous(j)) v->smooth[j]=target[j];
    }
    v->initialized=1;
    float fs=rate*POLIVOKS_OVERSAMPLE;
    float control_c=coefficient(0.002f,fs);
    float drift_c=coefficient(0.55f,fs);
    float osc_c=coefficient(1.0f/(2*(float)PVX_PI*32000),fs);
    float osc_dc_c=coefficient(1.0f/(2*(float)PVX_PI*3),fs);
    float dc_c=coefficient(1.0f/(2*(float)PVX_PI*4),fs);
    float sag_c=coefficient(0.035f,fs);
    float fa=coefficient(envelopeTime(target[PVX_FENV_ATTACK],0.001f,3)/logf(5),fs);
    float fd=coefficient(envelopeTime(target[PVX_FENV_DECAY],0.002f,12)/logf(1000),fs);
    float fr=coefficient(envelopeTime(target[PVX_FENV_RELEASE],0.002f,12)/logf(1000),fs);
    float aa=coefficient(envelopeTime(target[PVX_AENV_ATTACK],0.001f,3)/logf(5),fs);
    float ad=coefficient(envelopeTime(target[PVX_AENV_DECAY],0.002f,12)/logf(1000),fs);
    float ar=coefficient(envelopeTime(target[PVX_AENV_RELEASE],0.002f,12)/logf(1000),fs);
    int wave1=choice(params[PVX_OSC1_WAVE],5), wave2=choice(params[PVX_OSC2_WAVE],5);
    float reg1=(float)(choice(params[PVX_OSC1_OCT],5)-2);
    float reg2=(float)(choice(params[PVX_OSC2_OCT],5)-2);
    float ref_hz = inc ? (float)(inc*PVX_INV32*rate) : 261.6255653f*exp2f(v->target_oct1);
    for (int i=0; i<frames; ++i) {
        float ext=external && isfinite(external[i]) ? clampf(external[i],-16,16) : 0;
        for (int os=0; os<POLIVOKS_OVERSAMPLE; ++os) {
            for (int j=0; j<POLIVOKS_NUM_PARAMS; ++j)
                if (continuous(j)) v->smooth[j]+=(target[j]-v->smooth[j])*control_c;
            const float* q=v->smooth;
            float glide=norm((int)q[PVX_GLIDE]);
            float glide_c=glide<0.001f ? 1 : coefficient(0.001f+1.5f*glide*glide,fs);
            v->cur_oct1+=(v->target_oct1-v->cur_oct1)*glide_c;
            float envf=stepEnvelope(&v->fenv,v->gate,params[PVX_FENV_REPEAT]>=16384,
                                   fa,fd,norm((int)q[PVX_FENV_SUSTAIN]),fr);
            float enva=stepEnvelope(&v->aenv,v->gate,params[PVX_AENV_REPEAT]>=16384,
                                   aa,ad,norm((int)q[PVX_AENV_SUSTAIN]),ar);
            float lfo=p->lfo[i*POLIVOKS_OVERSAMPLE+os];
            v->drift_timer-=1/fs;
            if (v->drift_timer<=0) {
                v->drift_timer=0.13f+0.22f*(noise(&v->rng)+1)*0.5f;
                v->drift_target[0]=noise(&v->rng)*0.0014f;
                v->drift_target[1]=noise(&v->rng)*0.0014f;
            }
            for (int j=0;j<2;++j) v->drift[j]+=(v->drift_target[j]-v->drift[j])*drift_c;
            float tune=bipolar(q[PVX_MASTER_TUNE])/6;
            float duo=params[PVX_DUOPHONY]>=16384 ? v->target_oct2-v->target_oct1 : 0;
            float oct2=reg2+duo+(7*bipolar(q[PVX_OSC2_DETUNE])+0.5f*bipolar(q[PVX_OSC2_FINE]))/12
                       +tune+v->trim[1]+v->drift[1]+1.5f*lfo*norm((int)q[PVX_LFO_TO_OSC2]);
            float hz2=clampf(ref_hz*exp2f(clampf(oct2,-20,20)),0.01f,fs*0.45f);
            float dt2=hz2/fs;
            float raw2=oscillator(wave2,v->phase[1],dt2,v->trim[1]*0.4f,
                                  0.35f*lfo*norm((int)q[PVX_OSC2_PWM]));
            v->phase[1]+=dt2; v->phase[1]-=floor(v->phase[1]);
            v->osc_lp[1]+=(raw2-v->osc_lp[1])*osc_c;
            float osc2=saturate(v->osc_lp[1],4.1f,3.8f,NULL);
            v->osc_dc[1]+=(osc2-v->osc_dc[1])*osc_dc_c; osc2-=v->osc_dc[1];
            float oct1=reg1+v->cur_oct1-v->target_oct1+tune+v->trim[0]+v->drift[0]
                       +1.5f*lfo*norm((int)q[PVX_LFO_TO_PITCH])
                       +3.5f*osc2*norm((int)q[PVX_XMOD_DEPTH]);
            float hz1=clampf(ref_hz*exp2f(clampf(oct1,-20,20)),0.01f,fs*0.45f);
            float dt1=hz1/fs;
            float raw1=oscillator(wave1,v->phase[0],dt1,v->trim[0]*0.4f,0);
            v->phase[0]+=dt1; v->phase[0]-=floor(v->phase[0]);
            v->osc_lp[0]+=(raw1-v->osc_lp[0])*osc_c;
            float osc1=saturate(v->osc_lp[0],4.0f,3.65f,NULL);
            v->osc_dc[0]+=(osc1-v->osc_dc[0])*osc_dc_c; osc1-=v->osc_dc[0];
            float random=noise(&v->rng);
            float input_ext=v->external_prev+(ext-v->external_prev)*(os+1)/POLIVOKS_OVERSAMPLE;
            /* Mixer pots retain dynamics at low gain; hot mixes hit the finite
             * differential-current limit INSIDE the filter, not a hard limiter. */
            float mix=2.0f*(osc1*norm((int)q[PVX_MIX_OSC1])+osc2*norm((int)q[PVX_MIX_OSC2]))
                      +1.2f*random*norm((int)q[PVX_MIX_NOISE])
                      +1.7f*osc1*osc2*norm((int)q[PVX_MIX_RING])
                      +2*input_ext*norm((int)q[PVX_MIX_EXTERNAL]);
            if (params[PVX_FILT_AUTOOSC]>=16384) mix+=random*0.000008f;
            float sag_target=1-0.025f*clampf(fabsf(v->bp)+fabsf(v->lp),0,4);
            v->supply+=(sag_target-v->supply)*sag_c;
            float cut_oct=9.965784f*norm((int)q[PVX_FILT_CUTOFF])
                          +7.5f*bipolar(q[PVX_FILT_ENV_AMT])*envf
                          +v->cur_oct1*norm((int)q[PVX_FILT_KEYTRACK])
                          +4.5f*lfo*norm((int)q[PVX_LFO_TO_FILT])
                          +v->filter_trim+0.025f*saturate(v->bp,2,2,NULL);
            float cutoff=clampf(18*exp2f(clampf(cut_oct,-12,12)),8,fminf(22000,rate*0.45f));
            float g=tanf((float)PVX_PI*cutoff/fs);
            float res=norm((int)q[PVX_FILT_RES]);
            float damping=1.8f-2.12f*res;
            filterStep(v,mix,g,damping,v->supply);
            float filtered=params[PVX_FILT_MODE]>=16384 ? v->bp : v->lp;
            v->filter_dc+=(filtered-v->filter_dc)*dc_c;
            filtered-=v->filter_dc;
            float amp=params[PVX_VCA_HOLD]>=16384 ? 1 : enva;
            float velocity=1-norm((int)q[PVX_VELOCITY])*(1-v->velocity);
            float trem=1-norm((int)q[PVX_LFO_TO_AMP])*0.5f*(1-lfo);
            float value=saturate(filtered*amp*velocity*trem,3.8f,3.5f,NULL);
            v->output_dc+=(value-v->output_dc)*dc_c;
            value=(value-v->output_dc)*0.38f*norm((int)q[PVX_MASTER_LEVEL]);
            v->fir[v->fir_pos]=value;
            v->fir_pos=(v->fir_pos+1)%PVX_FIR;
        }
        float sample=0;
        unsigned pos=v->fir_pos;
        for (int k=0;k<PVX_FIR;++k) {
            pos=pos ? pos-1 : PVX_FIR-1;
            sample+=p->fir_coeff[k]*v->fir[pos];
        }
        out[i]=sample;
        v->external_prev=ext;
    }
}
void polivoksBeginBlock(struct PolivoksCore* p, const int16_t params[POLIVOKS_NUM_PARAMS], float rate) {
    polivoksBeginFrames(p,params,rate,POLIVOKS_BLOCK);
}
void polivoksRenderFloat(struct PolivoksCore* p,int voice,float* out,const float* external,
                        const int16_t params[POLIVOKS_NUM_PARAMS],uint32_t inc,float rate) {
    polivoksRenderFrames(p,voice,out,external,params,inc,rate,POLIVOKS_BLOCK);
}
void polivoksPanic(struct PolivoksCore* p) {
    if (!p) return;
    memset(p->keys,0,sizeof(p->keys));
    for (int i=0;i<POLIVOKS_VOICES;++i) {
        Voice* v=&p->v[i];
        v->gate=0; v->has_note=0;
        v->fenv.level=v->aenv.level=0;
        v->fenv.stage=v->aenv.stage=ENV_IDLE;
        v->bp=v->lp=v->s1=v->s2=v->filter_dc=v->output_dc=0;
        memset(v->fir,0,sizeof(v->fir));
        v->fir_pos=0; v->external_prev=0;
        /* Leave oscillator phase, calibration, drift and the LFO free running. */
    }
    p->block_ready=0;
}
void polivoksGetState(const struct PolivoksCore* p,int voice,struct PolivoksState* out) {
    if (!out) return;
    memset(out,0,sizeof(*out));
    if (!p || voice<0 || voice>=POLIVOKS_VOICES) return;
    const Voice* v=&p->v[voice];
    out->filter_env=v->fenv.level; out->amp_env=v->aenv.level;
    out->phase[0]=(float)v->phase[0]; out->phase[1]=(float)v->phase[1];
    out->lfo=p->block_ready ? p->lfo[p->block_frames*POLIVOKS_OVERSAMPLE-1] : 0;
    out->note1=60+12*v->target_oct1; out->note2=60+12*v->target_oct2;
    for (int i=0;i<128;++i) out->held_keys+=p->keys[i]!=0;
}
void polivoksRender(struct PolivoksCore* p,int voice,int16_t* out,const int16_t params[POLIVOKS_NUM_PARAMS],uint32_t inc,float rate) {
    if (!out) return;
    float buffer[POLIVOKS_BLOCK];
    polivoksRenderFloat(p,voice,buffer,NULL,params,inc,rate);
    for (int i=0;i<POLIVOKS_BLOCK;++i) out[i]=(int16_t)lrintf(clampf(buffer[i],-1,1)*32767);
}
float polivoksCutoffHz(int knob) { return 18*exp2f(norm(knob)*9.965784f); }
int polivoksWaveName(int knob,char* out,int n) {
    static const char* names[]={"TRI","SAW","SQR 50","PLS 25","PLS 12"};
    if (!out || n<=0) return 0;
    return snprintf(out,(size_t)n,"%s",names[choice(knob,5)]);
}
void polivoksScope(const int16_t params[POLIVOKS_NUM_PARAMS],float* out,int n,uint32_t seed) {
    if (!params || !out || n<=0) return;
    struct PolivoksCore* p=polivoksCreate();
    if (!p) { memset(out,0,(size_t)n*sizeof(*out)); return; }
    p->v[0].rng=seed|1u;
    p->v[0].phase[0]=0; p->v[0].phase[1]=0.137;
    polivoksNoteOn(p,0,57,127);
    p->v[0].fenv.stage=p->v[0].aenv.stage=ENV_SUSTAIN;
    float block[POLIVOKS_BLOCK], capture[512];
    for (int i=0;i<40;++i) polivoksRenderFloat(p,0,block,NULL,params,0,48000);
    for (int j=0;j<4;++j) {
        polivoksRenderFloat(p,0,block,NULL,params,0,48000);
        memcpy(capture+j*POLIVOKS_BLOCK,block,sizeof(block));
    }
    float peak=0.001f;
    for (int i=0;i<512;++i) peak=fmaxf(peak,fabsf(capture[i]));
    for (int i=0;i<n;++i) out[i]=capture[(size_t)i*512/(size_t)n]*0.92f/peak;
    polivoksDestroy(p);
}
