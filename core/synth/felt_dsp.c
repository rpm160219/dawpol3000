/* FELT: DSP (wlasny, zob. felt_dsp.h). */
#include "synth/felt_dsp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define VOICES 6
#define BLOCK 128
#define SLEN 2048
#define ELEN 1024

typedef struct { float buf[SLEN]; int w; float lp, a1x, a1y, a2x, a2y; } Str;
typedef struct { float b0, a1, a2, y1, y2; } Res;

typedef struct {
    Str s[3];
    float exc[ELEN]; int ePos, eLen;  /* impuls mlotka (z grzebieniem miejsca uderzenia) */
    float thump, thLp, thPh, damp, dampLp, swish, swLp1, swLp2;
    Res body[4];
    int strike, held, released;
    int damped;               /* tlumik na strunach (klawisz puszczony) */
    float level, last, out1;
    uint32_t rng;
} FtVoice;

struct FeltCore { FtVoice v[VOICES]; uint32_t strikes; };

static inline float rnd(uint32_t* r) { *r = *r * 1664525u + 1013904223u; return (float)(int32_t)*r * (1.f / 2147483648.f); }

struct FeltCore* feltCreate(void) {
    struct FeltCore* p = (struct FeltCore*)calloc(1, sizeof(struct FeltCore));
    if (!p) return 0;
    for (int i = 0; i < VOICES; i++) p->v[i].rng = 0x2545f491u * (uint32_t)(i + 7);
    return p;
}

void feltNoteOn(struct FeltCore* p, int voice) {
    if (!p || voice < 0 || voice >= VOICES) return;
    p->v[voice].strike = 1;
    p->v[voice].held = 1;
    p->v[voice].released = 0;
    p->strikes++;
}

void feltNoteOff(struct FeltCore* p, int voice) {
    if (!p || voice < 0 || voice >= VOICES) return;
    FtVoice* v = &p->v[voice];
    if (v->held) { v->held = 0; v->released = 1; }
}

float feltDecaySeconds(int knob) { return 1.5f * powf(16.f, (knob < 0 ? 0 : knob) / 32767.f); }  /* 1,5 .. 24 s */
int feltStrings(int knob) { const int n = 1 + (knob < 0 ? 0 : knob) * 3 / 32768; return n > 3 ? 3 : n; }

/* opoznienie fazowe przy czestotliwosci w (rad/probke) */
static float lpDelay(float b, float w) { return atan2f(b * sinf(w), 1.f - b * cosf(w)) / w; }
static float apDelay(float a, float w) {  /* H = (a + z^-1) / (1 + a z^-1) */
    const float nr = a + cosf(w), ni = -sinf(w), dr = 1.f + a * cosf(w), di = -a * sinf(w);
    const float ph = atan2f(ni, nr) - atan2f(di, dr);
    float d = -ph / w;
    while (d < 0) d += 6.2831853f / w;
    return d;
}

static void resSet(Res* r, float f, float q, float fs) {  /* rezonator 2-biegunowy (wzmocnienie ~1 w szczycie) */
    const float w = 6.2831853f * f / fs, rr = expf(-w / (2.f * q));
    r->a1 = 2.f * rr * cosf(w); r->a2 = -rr * rr; r->b0 = (1.f - rr * rr) * .5f;
}

void feltRender(struct FeltCore* p, int voice, int16_t* out, const int16_t* knobs, uint32_t inc, float rate, int vel) {
    if (!p || voice < 0 || voice >= VOICES) return;
    FtVoice* v = &p->v[voice];
    const float fs = rate * .5f;
    const int n = BLOCK / 2;
    float f0 = (float)inc * rate / 4294967296.f;
    if (f0 < 26.f) f0 = 26.f;
    if (f0 > 4200.f) f0 = 4200.f;
    const float felt = (knobs[0] < 0 ? 0 : knobs[0]) / 32767.f, corda = (knobs[1] < 0 ? 0 : knobs[1]) / 32767.f;
    const float noise = (knobs[3] < 0 ? 0 : knobs[3]) / 32767.f;
    const float vv = (vel < 1 ? 1 : vel > 127 ? 127 : vel) / 127.f;
    /* struny: liczba i rozstrojenie z CORDA (plynnie: druga i trzecia wchodza ciszej) */
    const float g2 = corda < .33f ? 0.f : (corda - .33f) * 3.f > 1.f ? 1.f : (corda - .33f) * 3.f;
    const float g3 = corda < .66f ? 0.f : (corda - .66f) * 3.f > 1.f ? 1.f : (corda - .66f) * 3.f;
    const float cents = .25f + 3.f * corda * corda;
    const float det[3] = {0.f, cents, -cents * .8f};
    const int ns = g3 > 0.f ? 3 : (g2 > 0.f ? 2 : 1);
    /* strata: T60 z DECAY, krotsze w gorze; jasnosc strat (b) */
    float T = feltDecaySeconds(knobs[2]) * powf(261.6f / f0, .55f);
    if (T > 40.f) T = 40.f;
    /* filc gasi tez gorne alikwoty szybciej; w gorze mniej strat na przejscie (wiecej przejsc na sekunde) */
    const float b = (.18f + .25f * felt) * (f0 > 262.f ? powf(262.f / f0, .7f) : 1.f);
    const float ap = -.12f - .25f * (f0 > 400.f ? 1.f : f0 / 400.f);  /* sztywnosc: wieksza niealikwotowosc w gorze */
    if (v->strike) {  /* uderzenie mlotka: impuls kontaktu + grzebien miejsca uderzenia + mechanika */
        v->strike = 0;
        const float tc = (.0006f + .0032f * felt) * (1.35f - .6f * vv) * fs;  /* czas kontaktu (probki) */
        const float tmax = .45f * fs / f0 * (1.f + felt);  /* w gorze mlotek dotyka krocej (ulamek okresu) */
        const float tcl = tc < tmax ? tc : tmax;
        const int tci = tcl < 3.f ? 3 : (tcl > 300.f ? 300 : (int)tcl);
        const int beta = (int)(fs / f0 / 8.f);       /* miejsce uderzenia: 1/8 struny */
        const float A = (.25f + .75f * powf(vv, 1.4f)) * (.9f - .3f * felt);
        const float fc = (1200.f + 9000.f * vv * vv) * (1.f - .75f * felt);
        const float k = 1.f - expf(-6.2831853f * fc / fs);
        float lp = 0, lp2 = 0;
        int len = tci + beta + 64;
        if (len > ELEN) len = ELEN;
        memset(v->exc, 0, sizeof v->exc);
        for (int i = 0; i < len; i++) {
            const float x = i < tci ? sinf(3.14159265f * i / tci) : 0.f;
            lp += (x * x - lp) * k; lp2 += (lp - lp2) * k;
            v->exc[i] += lp2 * A;
            if (i + beta < ELEN) v->exc[i + beta] -= lp2 * A * .8f;
        }
        v->ePos = 0; v->eLen = len;
        v->thump = noise * (.15f + .25f * vv);       /* stuk klawisza */
        v->swish = noise * (.05f + .08f * felt) * vv;  /* szelest filcu */
        v->thPh = 0;
        v->damped = 0;
        for (int bb = 0; bb < 4; bb++) { v->body[bb].y1 = v->body[bb].y2 = 0; }
    }
    if (v->released) { v->released = 0; v->damp = noise * .12f; v->damped = 1; }
    static const float bf[4] = {98.f, 230.f, 540.f, 1350.f}, bq[4] = {4.f, 5.f, 6.f, 5.f};
    for (int bb = 0; bb < 4; bb++) resSet(&v->body[bb], bf[bb], bq[bb], fs);
    /* parametry strun na blok */
    float D[3], g[3];
    for (int s = 0; s < ns; s++) {
        const float f = f0 * powf(2.f, det[s] / 1200.f), w = 6.2831853f * f / fs;
        const float P = fs / f;
        float d = P - lpDelay(b, w) - 2.f * apDelay(ap, w);
        if (d < 2.f) d = 2.f;
        if (d > SLEN - 4) d = SLEN - 4;
        D[s] = d;
        /* boczne struny gasna nieco szybciej: dwa etapy; tlumik = wybrzmienie ~0,4 s */
        g[s] = powf(.001f, 1.f / ((v->damped ? (T < .4f ? T : .4f) : T) * f * (s ? .85f : 1.f)));
    }
    const float sg[3] = {1.f, g2 * .9f, g3 * .8f};
    const float norm = 1.f / (1.f + .9f * g2 + .8f * g3);
    float y[BLOCK / 2], lvl = 0;
    const float thK = 1.f - expf(-6.2831853f * 180.f / fs), thW = 6.2831853f * 70.f / fs;
    for (int i = 0; i < n; i++) {
        const float e = v->ePos < v->eLen ? v->exc[v->ePos++] : 0.f;
        float bridge = 0;
        for (int s = 0; s < ns; s++) {
            Str* st = &v->s[s];
            const float rp = (float)st->w - D[s];
            int i0 = (int)floorf(rp);
            const float fr = rp - (float)i0;
            const float a = st->buf[i0 & (SLEN - 1)], c = st->buf[(i0 + 1) & (SLEN - 1)];
            /* odczyt: opoznienie D = i0 + fr probek wstecz od zapisu */
            float x = a + (c - a) * fr;
            x *= g[s];
            st->lp = (1.f - b) * x + b * st->lp;
            const float y1 = ap * st->lp + st->a1x - ap * st->a1y; st->a1x = st->lp; st->a1y = y1;
            const float y2 = ap * y1 + st->a2x - ap * st->a2y; st->a2x = y1; st->a2y = y2;
            st->buf[st->w & (SLEN - 1)] = y2 + e;
            st->w++;
            bridge += y2 * sg[s];
        }
        bridge *= norm;
        /* plyta rezonansowa */
        float body = 0;
        for (int bb = 0; bb < 4; bb++) {
            Res* r = &v->body[bb];
            const float o = r->b0 * bridge + r->a1 * r->y1 + r->a2 * r->y2;
            r->y2 = r->y1; r->y1 = o;
            body += o;
        }
        float o = bridge * .8f + body * .35f;
        /* mechanika */
        if (v->thump > 1e-5f) {
            v->thLp += (rnd(&v->rng) - v->thLp) * thK;
            v->thPh += thW;
            o += (v->thLp * 1.5f + sinf(v->thPh) * .6f) * v->thump;
            v->thump *= .9975f;
        }
        if (v->swish > 1e-5f) {
            const float x = rnd(&v->rng);
            v->swLp1 += (x - v->swLp1) * .5f; v->swLp2 += (v->swLp1 - v->swLp2) * .5f;
            o += (v->swLp1 - v->swLp2) * v->swish;
            v->swish *= .997f;
        }
        if (v->damp > 1e-5f) {
            v->dampLp += (rnd(&v->rng) - v->dampLp) * .08f;
            o += v->dampLp * v->damp;
            v->damp *= .998f;
        }
        {  /* lagodne kolano powyzej 1,5 (ok. -5 dB pod pelna skala): kolejne uderzenia w brzmiace struny */
            const float ao = o < 0 ? -o : o;
            if (ao > 1.5f) { const float e = ao - 1.5f, k = 1.5f + e / (1.f + e); o = o < 0 ? -k : k; }
        }
        v->out1 = o;
        y[i] = o;
        const float ay = o < 0 ? -o : o;
        if (ay > lvl) lvl = ay;
    }
    float prev = v->last;
    for (int i = 0; i < n; i++) {
        const float a = (prev + y[i]) * .5f * 13000.f, bq2 = y[i] * 13000.f;
        out[2 * i] = (int16_t)(a > 32767.f ? 32767 : (a < -32767.f ? -32767 : a));
        out[2 * i + 1] = (int16_t)(bq2 > 32767.f ? 32767 : (bq2 < -32767.f ? -32767 : bq2));
        prev = y[i];
    }
    v->last = prev;
    v->level += (lvl * 2.f - v->level) * .3f;
}

float feltMotion(const struct FeltCore* p) {
    float m = 0;
    if (p) for (int i = 0; i < VOICES; i++) if (p->v[i].level > m) m = p->v[i].level;
    return m > 1.f ? 1.f : m;
}
uint32_t feltStrikes(const struct FeltCore* p) { return p ? p->strikes : 0; }
int feltHeld(const struct FeltCore* p) {
    int h = 0;
    if (p) for (int i = 0; i < VOICES; i++) h += p->v[i].held;
    return h;
}
