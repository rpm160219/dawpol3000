#include "prophet_dsp.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define OS 4
#define NT 97
#define PI 3.14159265358979323846
#include "prophet_tables.inc"
#if defined(__aarch64__)
#include <arm_neon.h>
#endif
/* 2048-point interpolation: sub-millicent VCO error, no old cubic pitch approximation. */
static float pitchExp(float x) {
    int octave = (int)floorf(x);
    float at = (x - octave) * 2048;
    int i = (int)at;
    union {
        uint32_t u;
        float f;
    } scale = {(uint32_t)(127 + octave) << 23};
    return scale.f * (p5Exp[i] + (p5Exp[i + 1] - p5Exp[i]) * (at - i));
}
static float poleG(float ratio) {
    float at = ratio * 10240;
    int i = (int)at;
    if (i >= 2048)
        return p5G[2048];
    return p5G[i] + (p5G[i + 1] - p5G[i]) * (at - i);
}
static float lim(float x, float a, float b) { return x < a ? a : x > b ? b : x; }
static float norm(int x) { return lim(x / 32767.f, 0, 1); }
static float bi(int x) { return (x - 16384) / 16384.f; }
static int on(const int16_t *p, int i) { return p[i] >= 16384; }
static int choice(int x, int n) { return x >= 32767 ? n - 1 : (int)(norm(x) * n); }
int16_t prophetChoice(int i, int n) { return n < 2 ? 0 : (int16_t)(lim(i, 0, n - 1) * 32767 / (n - 1)); }
int prophetChoices(int p) {
    if (p == P5_AFREQ || p == P5_BFREQ)
        return 49;
    if (p == P5_AWAVE)
        return 4;
    if (p == P5_BWAVE || p == P5_LFOWAVE)
        return 8;
    if (p == P5_LFODIV)
        return PROPHET_DIVS;
    if (p == P5_KEYTRACK)
        return 2; /* original Rev 3 KEYBOARD on/off, not Rev 4 HALF */
    switch (p) {
    case P5_SYNC:
    case P5_BLOW:
    case P5_BKEY:
    case P5_PMFREQ:
    case P5_PMPW:
    case P5_PMFILTER:
    case P5_WFREQA:
    case P5_WFREQB:
    case P5_WPWA:
    case P5_WPWB:
    case P5_WFILTER:
    case P5_RELEASE:
    case P5_UNISON:
    case P5_LFOSYNC:
    case P5_A440:
    case P5_REVISION:
        return 2;
    default:
        return 0;
    }
}
static const char *divs[] = {"1/1",   "1/1D",  "1/1T", "1/2",   "1/2D",  "1/2T",  "1/4",    "1/4D",
                             "1/4T",  "1/8",   "1/8D", "1/8T",  "1/16",  "1/16D", "1/16T",  "1/32",
                             "1/32D", "1/32T", "1/64", "1/64D", "1/64T", "1/128", "1/128D", "1/128T"};
static float beats(int i) { return 4.f / exp2f(i / 3) * (i % 3 == 1 ? 1.5f : i % 3 == 2 ? 2.f / 3 : 1); }
const char *prophetDivision(int w) { return divs[choice(w, PROPHET_DIVS)]; }
float prophetTime(int w, int attack) {
    (void)attack;
    float x = norm(w);
    return x <= .5f ? .001f * powf(500.f, 2 * x) : .5f * powf(60.f, 2 * x - 1);
}
float prophetCutoff(int w) { return 20 * exp2f(norm(w) * 9.9657843f); }
float prophetLfoHz(const int16_t *p, float bpm) {
    return on(p, P5_LFOSYNC) ? lim(bpm, 20, 400) / (60 * beats(choice(p[P5_LFODIV], PROPHET_DIVS)))
                             : .04f * exp2f(norm(p[P5_LFORATE]) * 8.9657843f);
}
typedef struct Env {
    float v;
    int stage;
} Env;
typedef struct Voice {
    double phase[2], increment[2];
    float pitch, target, filter[4], fe, ae, dcx, dcy, drift[2], driftTarget[2], calibration[3];
    Env f, a;
    int note, gate, velocity;
    uint64_t age;
} Voice;
struct ProphetCore {
    Voice v[5];
    int held[128], unison;
    uint64_t clock;
    uint32_t rng;
    double lfo, refPhase;
    float pink[3], lfoValue, lfoHz, fir[NT], history[NT], smooth[PROPHET_PARAMS];
    int hp, ready, driftClock;
};
static float random1(ProphetCore *p) {
    uint32_t x = p->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    p->rng = x;
    return (x >> 8) * (2.f / 16777216.f) - 1;
}
void prophetDefault(int16_t *p) {
    memset(p, 0, sizeof(int16_t) * PROPHET_PARAMS);
    p[P5_AFREQ] = p[P5_BFREQ] = p[P5_TUNE] = p[P5_BEND] = 16384;
    p[P5_AWAVE] = prophetChoice(1, 4);
    p[P5_BWAVE] = prophetChoice(1, 8);
    p[P5_APW] = p[P5_BPW] = 16384;
    p[P5_BFINE] = 2200;
    p[P5_MIXA] = 26000;
    p[P5_MIXB] = 22000;
    p[P5_VINTAGE] = 7000;
    p[P5_BKEY] = 32767;
    p[P5_CUTOFF] = 19200;
    p[P5_RESONANCE] = 6500;
    p[P5_ENVAMT] = 12000;
    p[P5_KEYTRACK] = 32767;
    p[P5_FA] = 5000;
    p[P5_FD] = 18500;
    p[P5_FS] = 10000;
    p[P5_FR] = 14000;
    p[P5_AA] = 3500;
    p[P5_AD] = 19000;
    p[P5_AS] = 27000;
    p[P5_AR] = 15500;
    p[P5_RELEASE] = 32767;
    p[P5_LFORATE] = 18000;
    p[P5_LFOWAVE] = prophetChoice(2, 8);
    p[P5_LFODIV] = prophetChoice(6, 24);
    p[P5_LEVEL] = 28000;
}
ProphetCore *prophetCreate(void) {
    ProphetCore *p = calloc(1, sizeof(*p));
    if (!p)
        return NULL;
    p->rng = 0x519ab071;
    for (int i = 0; i < 5; i++) {
        p->v[i].note = -1;
        p->v[i].pitch = p->v[i].target = 60;
        for (int j = 0; j < 2; j++)
            p->v[i].phase[j] = (random1(p) + 1) * .5;
    }
    double total = 0;
    for (int i = 0; i < NT; i++) {
        double x = i - (NT - 1) * .5, f = .1125;
        double h = x == 0 ? 2 * f : sin(2 * PI * f * x) / (PI * x);
        double window = .42 - .5 * cos(2 * PI * i / (NT - 1)) + .08 * cos(4 * PI * i / (NT - 1));
        p->fir[i] = h * window;
        total += p->fir[i];
    }
    for (int i = 0; i < NT; i++)
        p->fir[i] /= total;
    return p;
}
void prophetDestroy(ProphetCore *p) { free(p); }
void prophetSeed(ProphetCore *p, uint32_t seed) {
    if (!p)
        return;
    p->rng = seed ? seed : 0x519ab071u;
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 2; j++)
            p->v[i].phase[j] = (random1(p) + 1) * .5;
}
void prophetPanic(ProphetCore *p) {
    if (!p)
        return;
    for (int i = 0; i < 5; i++) {
        p->v[i].gate = 0;
        p->v[i].note = -1;
        p->v[i].a = (Env){0, 0};
        p->v[i].f = (Env){0, 0};
        memset(p->v[i].filter, 0, sizeof(p->v[i].filter));
        p->v[i].dcx = p->v[i].dcy = 0;
    }
    memset(p->held, 0, sizeof(p->held));
    memset(p->history, 0, sizeof(p->history));
}
static void trigger(ProphetCore *p, int i, int n, int vel, int legato) {
    Voice *v = &p->v[i];
    v->target = n;
    if (v->note < 0)
        v->pitch = n;
    v->note = n;
    v->velocity = vel;
    v->gate = 1;
    v->age = ++p->clock;
    if (!legato) {
        v->a.stage = v->f.stage = 1;
    }
}
static int lowest(ProphetCore *p) {
    for (int n = 0; n < 128; n++)
        if (p->held[n])
            return n;
    return -1;
}
static void uni(ProphetCore *p, const int16_t *w, int retrigger) {
    (void)w;
    int n = lowest(p);
    for (int i = 0; i < 5; i++) {
        if (n < 0) {
            p->v[i].gate = 0;
            continue;
        }
        trigger(p, i, n, p->held[n], !retrigger);
    }
}
void prophetConfigure(ProphetCore *p, const int16_t *w) {
    if (!p || !w)
        return;
    int u = on(w, P5_UNISON);
    if (p->unison != u) {
        for (int i = 0; i < 5; i++)
            p->v[i].gate = 0;
        p->unison = u;
        if (u)
            uni(p, w, 1);
        else
            for (int n = 0; n < 128; n++)
                if (p->held[n]) {
                    int best = 0;
                    for (int i = 1; i < 5; i++)
                        if (p->v[i].age < p->v[best].age)
                            best = i;
                    trigger(p, best, n, p->held[n], 0);
                }
    }
}
void prophetKeyDown(ProphetCore *p, int n, int vel, const int16_t *w) {
    if (!p || !w || n < 0 || n > 127)
        return;
    if (vel <= 0) {
        prophetKeyUp(p, n, w);
        return;
    }
    prophetConfigure(p, w);
    int old = lowest(p);
    p->held[n] = vel;
    if (p->unison) {
        uni(p, w, old < 0);
        return;
    }
    int best = -1;
    float score = 1e9;
    for (int i = 0; i < 5; i++) {
        Voice *v = &p->v[i];
        if (v->note == n) {
            best = i;
            break;
        }
        float s = v->gate ? 1000.f + (float)(v->age % 1000000) * .00001f : v->a.v;
        if (s < score) {
            score = s;
            best = i;
        }
    }
    trigger(p, best, n, vel, 0);
}
void prophetKeyUp(ProphetCore *p, int n, const int16_t *w) {
    if (!p || !w || n < 0 || n > 127)
        return;
    p->held[n] = 0;
    if (p->unison) {
        uni(p, w, 0);
        return;
    }
    for (int i = 0; i < 5; i++)
        if (p->v[i].note == n)
            p->v[i].gate = 0;
}
static float envelope(Env *e, int gate, float a, float d, float s, float r) {
    if (!gate && e->stage != 0)
        e->stage = 4;
    switch (e->stage) {
    case 1:
        e->v += (1.25f - e->v) * a;
        if (e->v >= 1) {
            e->v = 1;
            e->stage = 2;
        }
        break;
    case 2:
        e->v += (s - e->v) * d;
        if (fabsf(e->v - s) < .0001f) {
            e->v = s;
            e->stage = 3;
        }
        break;
    case 3:
        e->v += (s - e->v) * d;
        break;
    case 4:
        e->v -= e->v * r;
        if (e->v < .00001f) {
            e->v = 0;
            e->stage = 0;
        }
        break;
    default:
        e->v = 0;
    }
    return e->v;
}
static float blep(double t, double d) {
    if (t < d) {
        float x = t / d;
        return 2 * x - x * x - 1;
    }
    if (t > 1 - d) {
        float x = (t - 1) / d;
        return x * x + 2 * x + 1;
    }
    return 0;
}
/* Unit triangle: derivative jumps corrected with integrated BLEP (cubic). */
static float blamp(double t, double d) {
    if (t < d) {
        float x = t / d - 1;
        return -x * x * x / 3;
    }
    if (t > 1 - d) {
        float x = (t - 1) / d + 1;
        return x * x * x / 3;
    }
    return 0;
}
static float wave(double t, double dt, float pw, int bits, int b) {
    float y = 0;
    double u = t - pw;
    if (u < 0)
        u += 1;
    if (bits & 1)
        y += 2 * t - 1 - blep(t, dt);
    if (bits & (b ? 4 : 2))
        y += (t < pw ? 1 : -1) + blep(t, dt) - blep(u, dt) - (2 * pw - 1);
    if (b && (bits & 2)) {
        double h = t + .5;
        if (h >= 1)
            h -= 1;
        y += 1 - 4 * fabs(t - .5) + 4 * dt * (blamp(t, dt) - blamp(h, dt));
    }
    return y;
}
/* Tabulation of the same bounded rational OTA, max interpolation error
 * below 1e-6 full scale. Derivative is that of the interpolated function. */
static float ota(float x, float *derivative) {
    float at = fabsf(x) * (2048.f / 3);
    int i = (int)fminf(at, 2048.f);
    if (i == 2048) {
        if (derivative)
            *derivative = 0;
        return x < 0 ? -1 : 1;
    }
    float slope = p5Ota[i + 1] - p5Ota[i];
    if (derivative)
        *derivative = slope * (2048.f / 3);
    float value = p5Ota[i] + slope * (at - i);
    return x < 0 ? -value : value;
}
/* SSM2040 family: asymmetric input-cell saturation documented by its
 * original designer in SSI2140 data sheet, Unity-Gain Overdrive. This is
 * a deliberately approximate model, not coefficients extracted from ROM.
 * Normalize the small-signal slope and remove the bias DC offset. */
typedef struct FilterColor {
    float drive, bias, zero, scale;
} FilterColor;
static FilterColor filterColor(float rev) {
    FilterColor c = {1 + .32f * rev, .18f * rev, 0, 1};
    float q = c.bias * c.bias, inv = 1 / (27 + 9 * q);
    c.zero = c.bias * (27 + q) * inv;
    float slope = (729 - 162 * q + 9 * q * q) * inv * inv;
    c.scale = 1 / (c.drive * slope);
    return c;
}
/* Four OTA-C stages. The scalar feedback root is solved against the same
 * nonlinear cascade subsequently committed to state (no mismatched predictor).
 * Rational bounded OTA characteristic avoids the unstable output cubic. */
static float cascade(Voice *v, float input, float G, float K, const FilterColor *color) {
    float u = input - K * v->filter[3], y[4], deriv;
    for (int k = 0; k <= 4; k++) {
        float z = u;
        deriv = 1;
        for (int j = 0; j < 4; j++) {
            float slope;
            float value;
            if (j == 0 && color->bias > 0) {
                value = (ota(z * color->drive + color->bias, &slope) - color->zero) * color->scale;
                slope *= color->drive * color->scale;
            } else
                value = ota(z, &slope);
            deriv *= G * slope;
            y[j] = G * value + (1 - G) * v->filter[j];
            z = y[j];
        }
        float f = u + K * y[3] - input;
        if (fabsf(f) < 1e-6f || k == 4)
            break;
        u -= lim(f / (1 + K * deriv), -2, 2);
    }
    for (int j = 0; j < 4; j++)
        v->filter[j] = lim(2 * y[j] - v->filter[j], -4, 4);
    return y[3];
}
/* Four independent cards in SIMD; the fifth uses the scalar solver.
 * Identical four-pole model and convergence criterion, full 4x signal rate.
 * The vector OTA evaluates the rational characteristic directly; the scalar
 * table interpolates it to within 1e-6 full scale. No shared voice state. */
#if defined(__aarch64__)
static float32x4_t reciprocal(float32x4_t x) {
    float32x4_t r = vrecpeq_f32(x);
    r = vmulq_f32(r, vrecpsq_f32(x, r));
    return vmulq_f32(r, vrecpsq_f32(x, r));
}
static float32x4_t vectorOta(float32x4_t x, float32x4_t *derivative) {
    x = vmaxq_f32(vdupq_n_f32(-3), vminq_f32(vdupq_n_f32(3), x));
    float32x4_t q = vmulq_f32(x, x), inv = reciprocal(vaddq_f32(vdupq_n_f32(27), vmulq_n_f32(q, 9)));
    float32x4_t num =
        vaddq_f32(vsubq_f32(vdupq_n_f32(729), vmulq_n_f32(q, 162)), vmulq_n_f32(vmulq_f32(q, q), 9));
    *derivative = vmulq_f32(num, vmulq_f32(inv, inv));
    return vmulq_f32(vmulq_f32(x, vaddq_f32(vdupq_n_f32(27), q)), inv);
}
#endif
static void filterCards(Voice *voices, const float *input, const float *coeff, float k, float *output,
                        const FilterColor *color) {
#if defined(__aarch64__)
    float32x4_t state[4], y[4], in = vld1q_f32(input), G = vld1q_f32(coeff), K = vdupq_n_f32(k),
                                one = vdupq_n_f32(1);
    float32x4_t H = vsubq_f32(one, G);
    for (int j = 0; j < 4; j++) {
        float lanes[4];
        for (int i = 0; i < 4; i++)
            lanes[i] = voices[i].filter[j];
        state[j] = vld1q_f32(lanes);
    }
    float32x4_t u = vsubq_f32(in, vmulq_f32(K, state[3]));
    for (int it = 0; it <= 4; it++) {
        float32x4_t z = u, deriv = one;
        for (int j = 0; j < 4; j++) {
            float32x4_t slope, value;
            if (j == 0 && color->bias > 0) {
                float32x4_t biased = vaddq_f32(vmulq_n_f32(z, color->drive), vdupq_n_f32(color->bias));
                value =
                    vmulq_n_f32(vsubq_f32(vectorOta(biased, &slope), vdupq_n_f32(color->zero)), color->scale);
                slope = vmulq_n_f32(slope, color->drive * color->scale);
            } else
                value = vectorOta(z, &slope);
            deriv = vmulq_f32(deriv, vmulq_f32(G, slope));
            y[j] = vaddq_f32(vmulq_f32(G, value), vmulq_f32(H, state[j]));
            z = y[j];
        }
        float32x4_t f = vsubq_f32(vaddq_f32(u, vmulq_f32(K, y[3])), in);
        if (vmaxvq_f32(vabsq_f32(f)) < 1e-6f || it == 4)
            break;
        float32x4_t step = vmulq_f32(f, reciprocal(vaddq_f32(one, vmulq_f32(K, deriv))));
        step = vmaxq_f32(vdupq_n_f32(-2), vminq_f32(vdupq_n_f32(2), step));
        u = vsubq_f32(u, step);
    }
    for (int j = 0; j < 4; j++) {
        float lanes[4];
        vst1q_f32(lanes, vsubq_f32(vmulq_n_f32(y[j], 2), state[j]));
        for (int i = 0; i < 4; i++)
            voices[i].filter[j] = lim(lanes[i], -4, 4);
    }
    vst1q_f32(output, y[3]);
    output[4] = cascade(&voices[4], input[4], coeff[4], k, color);
#else
    for (int i = 0; i < 5; i++)
        output[i] = cascade(&voices[i], input[i], coeff[i], k, color);
#endif
}
void prophetTune(ProphetCore *p) {
    if (!p)
        return;
    const float card[5] = {-.61f, .29f, .78f, -.32f, .07f};
    for (int i = 0; i < 5; i++) {
        Voice *v = &p->v[i];
        v->calibration[0] = card[i] * .09f + v->drift[0] * .035f;
        v->calibration[1] = v->drift[1] * .04f;
        v->calibration[2] = card[i] * .08f;
    }
}
void prophetResetLfo(ProphetCore *p) {
    if (p)
        p->lfo = 0;
}
void prophetRender(ProphetCore *restrict p, float *restrict out, int frames, const int16_t *restrict w,
                   float rate, float bpm) {
    if (!out || frames <= 0)
        return;
    memset(out, 0, frames * sizeof(float));
    if (!p || !w || !isfinite(rate) || rate < 8000 || rate > 192000)
        return;
    prophetConfigure(p, w);
    const float sr = rate * OS, delta = 1 / sr;
    float q[PROPHET_PARAMS];
    for (int j = 0; j < PROPHET_PARAMS; j++) {
        q[j] = norm(w[j]);
        if (!p->ready)
            p->smooth[j] = q[j];
    }
    p->ready = 1;
    float smoothing = 1 - expf(-1 / (.003f * sr));
    float ac[2], dc[2], rc[2];
    for (int e = 0; e < 2; e++) {
        int b = e ? P5_AA : P5_FA;
        ac[e] = 1 - expf(-logf(5.f) / (prophetTime(w[b], 1) * sr));
        dc[e] = 1 - expf(-6.907755f / (prophetTime(w[b + 1], 0) * sr));
        float rt = on(w, P5_RELEASE) ? prophetTime(w[b + 3], 0) : .006f;
        rc[e] = 1 - expf(-11.512925f / (rt * sr));
    }
    float glide = norm(w[P5_GLIDE]);
    float glideCoeff = glide < .001f ? 1 : 1 - expf(-delta / (.001f + 3 * glide * glide));
    int aw = choice(w[P5_AWAVE], 4), bw = choice(w[P5_BWAVE], 8), lw = choice(w[P5_LFOWAVE], 8);
    p->lfoHz = prophetLfoHz(w, bpm);
    float dcCoeff = expf(-2 * PI * 4 / sr);
    float lowPitch = 69 + 12 * log2f(.4f * powf(75.f, norm(w[P5_BFREQ])) / 440);
    int af = choice(w[P5_AFREQ], 49) - 24, bf = choice(w[P5_BFREQ], 49) - 24;
    float fine = norm(w[P5_BFINE]);
    float tune = bi(w[P5_TUNE]) * 2 + bi(w[P5_BEND]) * 2;
    const float card[5] = {-.61f, .29f, .78f, -.32f, .07f};
    int idle = !on(w, P5_A440) && norm(w[P5_RESONANCE]) < .65f;
    for (int i = 0; i < 5; i++)
        if (p->v[i].gate || p->v[i].a.stage)
            idle = 0;
    float timing[5];
    for (int i = 0; i < 5; i++)
        timing[i] = 1 + norm(w[P5_VINTAGE]) * .18f * card[i];
    for (int n = 0; n < frames; n++) {
        for (int o = 0; o < OS; o++) {
            for (int j = 0; j < PROPHET_PARAMS; j++) {
                p->smooth[j] += (q[j] - p->smooth[j]) * smoothing;
            }
            float *s = p->smooth;
            p->lfo += p->lfoHz * delta;
            p->lfo -= floor(p->lfo);
            float l = 0;
            if (lw & 1)
                l += 2 * p->lfo - 1;
            if (lw & 2)
                l += 1 - 4 * fabs(p->lfo - .5);
            if (lw & 4)
                l += p->lfo < .5 ? 1 : -1;
            p->lfoValue = l;
            float white = random1(p);
            p->pink[0] = .99765f * p->pink[0] + white * .0990460f;
            p->pink[1] = .96300f * p->pink[1] + white * .2965164f;
            p->pink[2] = .57000f * p->pink[2] + white * 1.0526913f;
            float pink = .18f * (p->pink[0] + p->pink[1] + p->pink[2] + white * .1848f);
            float wheel = s[P5_WHEEL] * ((1 - s[P5_WMIX]) * l + s[P5_WMIX] * pink);
            float sum = 0;
            if (--p->driftClock <= 0) {
                p->driftClock = (int)(sr * .071f);
                for (int i = 0; i < 5; i++)
                    for (int j = 0; j < 2; j++)
                        p->v[i].driftTarget[j] = random1(p);
            }
            /* Closed VCAs: retain oscillator/LFO/noise clocks cheaply. The
             * self-oscillating filter is deliberately kept running at full rate.
             * This does not skip a release tail or any held/attacking voice. */
            if (idle) {
                for (int i = 0; i < 5; i++) {
                    Voice *v = &p->v[i];
                    v->fe =
                        envelope(&v->f, 0, ac[0] * timing[i], dc[0] * timing[i], s[P5_FS], rc[0] * timing[i]);
                    for (int j = 0; j < 2; j++) {
                        v->phase[j] += v->increment[j];
                        if (v->phase[j] >= 1)
                            v->phase[j] -= 1;
                    }
                    for (int j = 0; j < 4; j++)
                        v->filter[j] = fabsf(v->filter[j]) < 1e-15f ? 0 : v->filter[j] * .999f;
                    v->dcy = fabsf(v->dcy) < 1e-15f ? 0 : v->dcy * .999f;
                    v->dcx = fabsf(v->dcx) < 1e-15f ? 0 : v->dcx * .999f;
                }
                p->history[p->hp] = 0;
                p->hp = (p->hp + 1) % NT;
                continue;
            }
            float filterInput[5], filterCoeff[5], filtered[5];
            for (int i = 0; i < 5; i++) {
                Voice *v = &p->v[i];
                v->fe = envelope(&v->f, v->gate, ac[0] * timing[i], dc[0] * timing[i], s[P5_FS],
                                 rc[0] * timing[i]);
                v->ae = envelope(&v->a, v->gate, ac[1] * timing[i], dc[1] * timing[i], s[P5_AS],
                                 rc[1] * timing[i]);
                v->pitch += (v->target - v->pitch) * (p->unison ? glideCoeff : 1);
                for (int j = 0; j < 2; j++)
                    v->drift[j] += (v->driftTarget[j] - v->drift[j]) * (delta * .7f);
                float vint = s[P5_VINTAGE];
                float trim = vint * (card[i] * .09f + v->drift[0] * .035f - v->calibration[0]);
                if (s[P5_REVISION] > 0)
                    trim *= 1 + .45f * s[P5_REVISION];
                float bpitch = (on(w, P5_BKEY) ? v->pitch : 60) + bf + fine + tune + trim +
                               vint * (v->drift[1] * .04f - v->calibration[1]) +
                               (on(w, P5_WFREQB) ? wheel * 12 : 0);
                if (on(w, P5_BLOW))
                    bpitch = lowPitch + (on(w, P5_BKEY) ? v->pitch - 60 : 0) + fine + tune + trim +
                             (on(w, P5_WFREQB) ? wheel * 12 : 0);
                double db = lim(440 * pitchExp(lim((bpitch - 69) / 12, -15, 10)) / sr, .00000001f, .40f);
                v->increment[1] = db;
                double tb = v->phase[1];
                v->phase[1] += db;
                int reset = v->phase[1] >= 1;
                if (reset)
                    v->phase[1] -= 1;
                float bpw = lim(.01f + .98f * s[P5_BPW] + (on(w, P5_WPWB) ? wheel * .45f : 0), .001f, .999f);
                float b = wave(tb, db, bpw, bw, 1);
                float pm = s[P5_PMENV] * v->fe + s[P5_PMB] * b;
                float apitch = v->pitch + af + tune + trim + (on(w, P5_PMFREQ) ? pm * 48 : 0) +
                               (on(w, P5_WFREQA) ? wheel * 12 : 0);
                double da = lim(440 * pitchExp(lim((apitch - 69) / 12, -15, 10)) / sr, .00000001f, .40f);
                float apw = lim(.01f + .98f * s[P5_APW] + (on(w, P5_PMPW) ? pm * .45f : 0) +
                                    (on(w, P5_WPWA) ? wheel * .45f : 0),
                                .001f, .999f);
                v->increment[0] = da;
                double ta = v->phase[0];
                float a = wave(ta, da, apw, aw, 0);
                v->phase[0] += da;
                v->phase[0] -= floor(v->phase[0]);
                /* Fractional B->A reset. Oversampling and area interpolation bound the
                 * reset error; not advertised as a fully bandlimited minBLEP sync. */
                if (reset && on(w, P5_SYNC)) {
                    double remaining = v->phase[1] / db;
                    double before = ta + da * (1 - remaining);
                    before -= floor(before);
                    float post = wave(0, da, apw, aw, 0);
                    a = (1 - remaining) * wave(before, da, apw, aw, 0) + remaining * post;
                    v->phase[0] = remaining * da;
                }
                float mix = .62f * (s[P5_MIXA] * a + s[P5_MIXB] * b + s[P5_NOISE] * white * .72f);
                float fc = 20 * pitchExp(lim(s[P5_CUTOFF] * 9.9657843f + s[P5_ENVAMT] * 7 * v->fe +
                                                 (on(w, P5_KEYTRACK) ? (v->pitch - 60) / 12 : 0) +
                                                 (on(w, P5_PMFILTER) ? pm * 6 : 0) +
                                                 (on(w, P5_WFILTER) ? wheel * 4 : 0) +
                                                 vint * (card[i] * .08f - v->calibration[2]),
                                             -2, 14));
                fc = lim(fc, 5, fminf(22000, sr * .20f));
                filterCoeff[i] = poleG(fc / sr);
                filterInput[i] = mix + white * .000002f;
            }
            FilterColor color = filterColor(s[P5_REVISION]);
            filterCards(p->v, filterInput, filterCoeff, 5.75f * s[P5_RESONANCE], filtered, &color);
            for (int i = 0; i < 5; i++) {
                Voice *v = &p->v[i];
                float vint = s[P5_VINTAGE];
                float dc = filtered[i] - v->dcx + dcCoeff * v->dcy;
                v->dcx = filtered[i];
                v->dcy = dc;
                float vel =
                    1 - s[P5_VELOCITY] + s[P5_VELOCITY] * (.25f + .75f * lim(v->velocity / 127.f, 0, 1));
                sum += dc * v->ae * vel * (1 + vint * card[i] * .025f);
            }
            if (on(w, P5_A440)) {
                p->refPhase += 440 * delta;
                p->refPhase -= floor(p->refPhase);
                sum += .25f * sinf(2 * PI * p->refPhase);
            }
            p->history[p->hp] = sum * .31f * s[P5_LEVEL];
            p->hp = (p->hp + 1) % NT;
        }
        float y = 0;
        int j = p->hp;
        for (int k = 0; k < NT; k++) {
            j = j ? j - 1 : NT - 1;
            y += p->history[j] * p->fir[k];
        }
        out[n] = y;
    }
}
void prophetGetState(const ProphetCore *p, ProphetState *s) {
    if (!p || !s)
        return;
    memset(s, 0, sizeof(*s));
    for (int i = 0; i < 5; i++) {
        s->amp[i] = p->v[i].ae;
        s->env[i] = p->v[i].fe;
        s->notes[i] = p->v[i].note;
    }
    for (int n = 0; n < 128; n++)
        s->held += p->held[n] > 0;
    for (int i = 0; i < 2; i++)
        s->phase[i] = p->v[0].phase[i];
    s->lfo = p->lfoValue;
    s->lfoPhase = p->lfo;
    s->lfoHz = p->lfoHz;
}
void prophetValue(int p, int w, char *out, int size) {
    float x = norm(w);
    int c = prophetChoices(p), i = c ? choice(w, c) : 0;
    if (p == P5_REVISION) {
        snprintf(out, size, "%s", i ? "REV 2 SSM" : "REV 3 CEM");
    } else if (p == P5_AWAVE) {
        const char *a[] = {"OFF", "SAW", "PULSE", "SAW+PLS"};
        snprintf(out, size, "%s", a[i]);
    } else if (p == P5_BWAVE || p == P5_LFOWAVE) {
        const char *a[] = {"OFF", "SAW", "TRI", "SAW+TRI", "PULSE", "SAW+PLS", "TRI+PLS", "ALL"};
        snprintf(out, size, "%s", a[i]);
    } else if (p == P5_LFODIV)
        snprintf(out, size, "%s", prophetDivision(w));
    else if (c == 2)
        snprintf(out, size, "%s", i ? "ON" : "OFF");
    else if (p == P5_CUTOFF)
        snprintf(out, size, "%.0f HZ", prophetCutoff(w));
    else if (p == P5_AFREQ || p == P5_BFREQ)
        snprintf(out, size, "%+.1f ST", (double)(choice(w, 49) - 24));
    else if (p == P5_BFINE || p == P5_TUNE || p == P5_BEND)
        snprintf(out, size, "%+.1f %s", (p == P5_BFINE ? norm(w) * 100 : bi(w) * 2),
                 p == P5_BFINE ? "C" : "ST");
    else if (p == P5_LFORATE)
        snprintf(out, size, "%.2f HZ", .04f * exp2f(x * 8.9657843f));
    else if ((p >= P5_FA && p <= P5_FR && p != P5_FS) || (p >= P5_AA && p <= P5_AR && p != P5_AS))
        snprintf(out, size, "%.3f S", prophetTime(w, p == P5_FA || p == P5_AA));
    else if (p == P5_GLIDE)
        snprintf(out, size, w < 33 ? "OFF" : "%.2f S", .001 + 3 * x * x);
    else
        snprintf(out, size, "%.0f%%", 100 * x);
}
