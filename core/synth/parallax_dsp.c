/* PARALLAX: DSP (wlasny, zob. parallax_dsp.h). Wszystko w float na glos, stan per instancja (sciezka). */
#include "synth/parallax_dsp.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VOICES 6
#define BLOCK 128

typedef struct {
    uint32_t ph[3], sub, rng;
    float s[4], hp;          /* stany drabinki i filtra w petli */
    float G, k;              /* wygladzone wspolczynniki */
    float last;              /* ostatnia probka 48 kHz (interpolacja do 96 kHz) */
    int fresh;
} PxVoice;

struct ParallaxCore { PxVoice v[VOICES]; };

static inline float blep(float t, float dt, float idt) {  /* PolyBLEP: t = faza 0..1, dt = przyrost, idt = 1/dt */
    if (t < dt) { t *= idt; return t + t - t * t - 1.f; }
    if (t > 1.f - dt) { t = (t - 1.f) * idt; return t * t + t + t + 1.f; }
    return 0.f;
}
static inline float frac1(float x) { return x - (float)(int)x; }
static inline float sat(float x) {  /* tanh (przyblizenie Pade), ograniczone */
    if (x > 3.f) return 1.f;
    if (x < -3.f) return -1.f;
    const float x2 = x * x;
    return x * (27.f + x2) / (27.f + 9.f * x2);
}
static inline uint32_t lcg(uint32_t* r) { *r = *r * 1664525u + 1013904223u; return *r; }

/* para ksztaltow w, w+1 (0 PULSE 25%, 1 SQUARE, 2 SAW, 3 SAW+SUB, 4 NOISE) z jednym PolyBLEP zbocza w t = 0 */
static inline float shapePair(int w, float f, float t, float dt, float idt, float ts, float dts, float idts, uint32_t* rng) {
    const float b0 = blep(t, dt, idt);
    float a = 0, c = 0;
    const int need1 = f > 0.f;
    for (int k = 0; k <= need1; k++) {
        float y;
        switch (w + k) {
        case 0: y = (t < .25f ? 1.5f : -.5f) + b0 - blep(frac1(t + .75f), dt, idt); break;
        case 1: y = (t < .5f ? 1.f : -1.f) + b0 - blep(frac1(t + .5f), dt, idt); break;
        case 2: y = 2.f * t - 1.f - b0; break;
        case 3: {
            const float sub = (ts < .5f ? 1.f : -1.f) + blep(ts, dts, idts) - blep(frac1(ts + .5f), dts, idts);
            y = .62f * (2.f * t - 1.f - b0) + .58f * sub;
            break;
        }
        default: y = (float)(int32_t)lcg(rng) * (1.f / 2147483648.f);
        }
        if (k == 0) a = y; else c = y;
    }
    return need1 ? a + (c - a) * f : a;
}

struct ParallaxCore* parallaxCreate(void) {
    struct ParallaxCore* p = (struct ParallaxCore*)calloc(1, sizeof(struct ParallaxCore));
    if (!p) return 0;
    for (int i = 0; i < VOICES; i++) { p->v[i].rng = 0x9e3779b9u * (uint32_t)(i + 1); p->v[i].fresh = 1; }
    return p;
}

void parallaxNoteOn(struct ParallaxCore* p, int voice) {
    if (!p || voice < 0 || voice >= VOICES) return;
    PxVoice* v = &p->v[voice];
    /* swobodne fazy jak w analogu (unison nie startuje w fazie) */
    v->ph[0] = lcg(&v->rng); v->ph[1] = lcg(&v->rng); v->ph[2] = lcg(&v->rng);
    v->sub = v->ph[0] >> 1;
    if (v->fresh) { memset(v->s, 0, sizeof v->s); v->hp = 0; v->fresh = 0; v->G = -1; }
}

float parallaxCutoffHz(int knob) { return 20.f * powf(1000.f, (knob < 0 ? 0 : knob) / 32767.f); }

int parallaxWaveName(int knob, char* out, int n) {
    static const char* N[5] = {"PULSE", "SQUARE", "SAW", "SAW+SUB", "NOISE"};
    const float p = (knob < 0 ? 0 : knob) * 4.f / 32767.f;
    int i = (int)p;
    float f = p - (float)i;
    if (i >= 4) { i = 4; f = 0; }
    if (f < .2f) return snprintf(out, (size_t)n, "%s", N[i]);
    if (f > .8f) return snprintf(out, (size_t)n, "%s", N[i + 1]);
    static const char* S[5] = {"PLS", "SQR", "SAW", "SUB", "NSE"};
    return snprintf(out, (size_t)n, "%s>%s", S[i], S[i + 1]);
}

/* wspolny rdzen: blok n probek glosu v przy czestotliwosci probkowania fs */
static void renderCore(PxVoice* v, float* y, int n, const int16_t* knobs, uint32_t inc, float fs, int unisonOn) {
    const float wp = (knobs[0] < 0 ? 0 : knobs[0]) * 4.f / 32767.f;
    int wi = (int)wp;
    float wf = wp - (float)wi;
    if (wi >= 4) { wi = 4; wf = 0; }
    const float u = (knobs[1] < 0 ? 0 : knobs[1]) / 32767.f;
    const float cents = unisonOn ? 4.f + 26.f * u * u : 0.f;
    const float side = unisonOn ? (u * 3.f > 1.f ? 1.f : u * 3.f) : 0.f;
    const float norm = 1.f / (1.f + 1.3f * side);
    const double r = pow(2.0, cents / 1200.0);
    const uint32_t incs[3] = {inc, (uint32_t)(inc * r), (uint32_t)(inc / r)};
    const float dts[3] = {incs[0] * (1.f / 4294967296.f), incs[1] * (1.f / 4294967296.f), incs[2] * (1.f / 4294967296.f)};
    const float idt[3] = {1.f / (dts[0] > 1e-9f ? dts[0] : 1e-9f), 1.f / (dts[1] > 1e-9f ? dts[1] : 1e-9f), 1.f / (dts[2] > 1e-9f ? dts[2] : 1e-9f)};
    /* filtr: cel na blok, wygladzony liniowo po probkach */
    float fc = parallaxCutoffHz(knobs[2]);
    if (fc > fs * .45f) fc = fs * .45f;
    const float g = tanf(3.14159265f * fc / fs), Gt = g / (1.f + g);
    const float res = (knobs[3] < 0 ? 0 : knobs[3]) / 32767.f;
    const float kt = 4.15f * powf(res, .85f);
    if (v->G < 0) { v->G = Gt; v->k = kt; }
    const float dG = (Gt - v->G) / (float)n, dk = (kt - v->k) / (float)n;
    const float hpA = 1.f - expf(-6.2831853f * 110.f / fs);  /* filtr gornoprzepustowy w petli (~110 Hz), jak w 303 */
    const int nOsc = side > 0.f ? 3 : 1;
    for (int i = 0; i < n; i++) {
        float x = 0;
        for (int o = 0; o < nOsc; o++) {
            const float t = v->ph[o] * (1.f / 4294967296.f), dt = dts[o];
            const float ts = v->sub * (1.f / 4294967296.f);
            const float s = shapePair(wi, wf, t, dt, idt[o], ts, dt * .5f, idt[o] * 2.f, &v->rng);
            x += o == 0 ? s : s * side;
            v->ph[o] += incs[o];
            if (o == 0) v->sub += incs[0] >> 1;
        }
        x *= norm * .85f;
        /* drabinka TPT: przewidywanie liniowe wyjscia, sprzezenie przez HP, nasycenie na wejsciu */
        v->G += dG; v->k += dk;
        const float G = v->G, k = v->k, H = 1.f - G;
        const float S = G * G * G * H * v->s[0] + G * G * H * v->s[1] + G * H * v->s[2] + H * v->s[3];
        const float G4 = G * G * G * G;
        const float y4 = (G4 * x + S) / (1.f + k * G4);
        v->hp += (y4 - v->hp) * hpA;
        float in = sat(x * (1.f + .35f * k) - k * (y4 - v->hp));
        for (int st = 0; st < 4; st++) {
            const float vv = G * (in - v->s[st]);
            const float o = vv + v->s[st];
            v->s[st] = o + vv;
            in = o;
        }
        y[i] = in;
    }
}

void parallaxRender(struct ParallaxCore* p, int voice, int16_t* out, const int16_t* knobs, uint32_t inc, float rate) {
    if (!p || voice < 0 || voice >= VOICES) return;
    /* glos liczony przy polowie czestotliwosci gospodarza (48 kHz), potem interpolacja liniowa do 96 kHz
     * (gospodarz i tak decymuje 2:1 filtrem FIR) — polowa kosztu przy tym samym brzmieniu do ~20 kHz */
    PxVoice* v = &p->v[voice];
    float y[BLOCK / 2];
    renderCore(v, y, BLOCK / 2, knobs, inc * 2u, rate * .5f, 1);
    float prev = v->last;
    for (int i = 0; i < BLOCK / 2; i++) {
        const float a = (prev + y[i]) * .5f * 21000.f, b = y[i] * 21000.f;
        out[2 * i] = (int16_t)(a > 32767.f ? 32767 : (a < -32767.f ? -32767 : a));
        out[2 * i + 1] = (int16_t)(b > 32767.f ? 32767 : (b < -32767.f ? -32767 : b));
        prev = y[i];
    }
    v->last = prev;
}

void parallaxScope(const int16_t knobs[4], float* out, int n, float detune, uint32_t seed) {
    PxVoice v;
    memset(&v, 0, sizeof v);
    v.G = -1;
    v.rng = seed | 1u;
    v.ph[0] = (uint32_t)(detune * 4294967296.0);
    const float fs = 96000.f;
    const int per = 436;  /* ~220 Hz */
    const uint32_t inc = (uint32_t)(4294967296.0 / per);
    float buf[436];
    for (int w = 0; w < 6; w++) renderCore(&v, buf, per, knobs, inc, fs, 0);  /* rozbieg filtra */
    renderCore(&v, buf, per, knobs, inc, fs, 0);
    float mx = 1e-3f;
    for (int i = 0; i < per; i++) mx = fabsf(buf[i]) > mx ? fabsf(buf[i]) : mx;
    const float sc = .92f / mx;  /* ekran: petla zawsze na pelna skale */
    for (int i = 0; i < n; i++) out[i] = buf[i * per / n] * sc;
}
