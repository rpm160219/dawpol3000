/* LOUNGE: DSP (wlasny, zob. lounge_dsp.h). */
#include "synth/lounge_dsp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define VOICES 6
#define BLOCK 128
#define KS_LEN 2048

typedef struct {
    int type, strike;
    float zr[3], zi[3];      /* mody (oscylatory rekurencyjne) */
    float hammer, hlp;       /* uderzenie mlotka (krotki szum) */
    float dc, hp, lp, lp2;   /* filtry wyjscia */
    float ks[KS_LEN];        /* struna CLAV */
    int kw;
    float kprev, level, last;
    uint32_t rng;
} LgVoice;

struct LoungeCore { LgVoice v[VOICES]; uint32_t strikes; };

static inline float rnd(uint32_t* r) { *r = *r * 1664525u + 1013904223u; return (float)(int32_t)*r * (1.f / 2147483648.f); }
static inline float sat(float x) {
    if (x > 3.f) return 1.f;
    if (x < -3.f) return -1.f;
    const float x2 = x * x;
    return x * (27.f + x2) / (27.f + 9.f * x2);
}

struct LoungeCore* loungeCreate(void) {
    struct LoungeCore* p = (struct LoungeCore*)calloc(1, sizeof(struct LoungeCore));
    if (!p) return 0;
    for (int i = 0; i < VOICES; i++) p->v[i].rng = 0x51ed2701u * (uint32_t)(i + 3);
    return p;
}

void loungeNoteOn(struct LoungeCore* p, int voice) {
    if (!p || voice < 0 || voice >= VOICES) return;
    p->v[voice].strike = 1;
    p->strikes++;
}

int loungeType(int knob) { const int t = (knob < 0 ? 0 : knob) * 3 / 32768; return t > 2 ? 2 : t; }
float loungeDecaySeconds(int knob) { return .35f * powf(30.f, (knob < 0 ? 0 : knob) / 32767.f); }  /* 0,35 .. 10,5 s */

static const float kRatio[3][3] = {{1.f, 1.0023f, 7.11f}, {1.f, 2.f, 6.27f}, {1.f, 1.f, 1.f}};

void loungeRender(struct LoungeCore* p, int voice, int16_t* out, const int16_t* knobs, uint32_t inc, float rate, int vel) {
    if (!p || voice < 0 || voice >= VOICES) return;
    LgVoice* v = &p->v[voice];
    const float fs = rate * .5f;           /* glos przy 48 kHz */
    const int n = BLOCK / 2;
    const float f0 = (float)inc * rate / 4294967296.f;
    const float bark = (knobs[0] < 0 ? 0 : knobs[0]) / 32767.f, tone = (knobs[1] < 0 ? 0 : knobs[1]) / 32767.f;
    const float vv = (vel < 1 ? 1 : vel > 127 ? 127 : vel) / 127.f;
    float T = loungeDecaySeconds(knobs[2]) * sqrtf(261.6f / (f0 > 20.f ? f0 : 20.f));
    if (T > 14.f) T = 14.f;
    if (v->strike) {  /* uderzenie: typ zapamietany, mody od nowa (z ciaglosci: dodane do tego, co jeszcze drga) */
        v->strike = 0;
        v->type = loungeType(knobs[3]);
        const float a = .35f + .65f * vv;
        if (v->type == 0) {
            v->zi[0] += a; v->zi[1] += a * .22f; v->zi[2] += a * (.12f + .45f * bark) * vv;
        } else if (v->type == 1) {
            v->zi[0] += a; v->zi[1] += a * .1f; v->zi[2] += a * (.2f + .4f * bark);
        } else {  /* CLAV: struna wypelniona szarpnieciem (szum przefiltrowany jasnoscia BARK) */
            int L = (int)(fs / (f0 > 20.f ? f0 : 20.f));
            if (L > KS_LEN - 2) L = KS_LEN - 2;
            float lp = 0, k = .25f + .7f * bark * (.5f + .5f * vv);
            for (int i = 0; i < KS_LEN; i++) {
                lp += (rnd(&v->rng) - lp) * k;
                v->ks[i] = i < L ? v->ks[i] * .3f + lp * a * 1.6f : 0.f;
            }
            v->kw = L;  /* zapis za szarpnieciem: odczyt (kw - L) trafia na poczatek struny */
        }
        v->hammer = (.05f + .25f * bark) * a;
    }
    /* oscylatory modow: obrot na probke + zanik */
    float cr[3], ci[3];
    for (int m = 0; m < 3; m++) {
        float f = f0 * kRatio[v->type][m];
        if (f > fs * .45f) f = fs * .45f;
        const float Tm = m == 0 ? T : (m == 1 ? T * (v->type == 0 ? .9f : .4f) : T * (v->type == 0 ? .05f : .12f));
        const float r = expf(-6.9f / (Tm * fs));
        cr[m] = cosf(6.2831853f * f / fs) * r;
        ci[m] = sinf(6.2831853f * f / fs) * r;
    }
    const float hpA = 1.f - expf(-6.2831853f * 25.f / fs);
    float y[BLOCK / 2], lvl = 0;
    if (v->type == 2) {  /* CLAV: Karplus-Strong z ulamkowym opoznieniem, grzebien przetwornika */
        float Lf = fs / (f0 > 20.f ? f0 : 20.f) - .5f;  /* filtr petli opoznia o pol probki */
        if (Lf > KS_LEN - 2) Lf = KS_LEN - 2;
        if (Lf < 2.f) Lf = 2.f;
        const float g = powf(.001f, 1.f / (T * .6f * f0 + 1e-3f));  /* strata na okres -> T60 */
        const float pos = .06f + .4f * tone;
        const int L = (int)Lf;
        const float fr = Lf - (float)L;
        for (int i = 0; i < n; i++) {
            const int r0 = (v->kw - L + KS_LEN) & (KS_LEN - 1), r1 = (r0 - 1 + KS_LEN) & (KS_LEN - 1);
            const float d = v->ks[r0] * (1.f - fr) + v->ks[r1] * fr;
            const float s = (d + v->kprev) * .5f * g;  /* filtr petli: srednia dwoch probek */
            v->kprev = d;
            v->ks[v->kw] = s;
            v->kw = (v->kw + 1) & (KS_LEN - 1);
            const int rp = (v->kw - 1 - (int)(pos * Lf) + KS_LEN) & (KS_LEN - 1);
            float o = (s - v->ks[rp]) * (1.4f + bark);
            o = sat(o * (1.f + 1.5f * bark)) * .8f;
            y[i] = o;
        }
    } else {
        const float drive = .6f + 2.8f * bark * (.4f + .6f * vv);
        const float off = (tone - .5f) * 1.3f, off0 = off / (1.f + off * off);
        const float bright = 1.f - expf(-6.2831853f * (700.f + 9000.f * tone * tone) / fs);
        for (int i = 0; i < n; i++) {
            float x = 0;
            for (int m = 0; m < 3; m++) {
                const float zr = v->zr[m] * cr[m] - v->zi[m] * ci[m];
                const float zi = v->zr[m] * ci[m] + v->zi[m] * cr[m];
                v->zr[m] = zr; v->zi[m] = zi;
                x += zi;
            }
            if (v->hammer > 1e-5f) { v->hlp += (rnd(&v->rng) * v->hammer - v->hlp) * .3f; v->hammer *= .985f; x += v->hlp; }
            float o;
            if (v->type == 0) {  /* przetwornik magnetyczny: s / (1 + s^2) wokol przesuniecia */
                const float s = x * drive + off;
                o = (s / (1.f + s * s) - off0) * 1.5f;
            } else {             /* przetwornik elektrostatyczny: asymetria, potem jasnosc */
                o = sat(x * (.9f + 1.6f * bark) + (.25f + .6f * bark) * x * x) * .9f;
                v->lp2 += (o - v->lp2) * bright;
                o = v->lp2;
            }
            v->hp += (o - v->hp) * hpA;  /* usun skladowa stala */
            y[i] = o - v->hp;
        }
    }
    float prev = v->last;
    for (int i = 0; i < n; i++) {
        const float a = (prev + y[i]) * .5f * 15000.f, b = y[i] * 15000.f;
        out[2 * i] = (int16_t)(a > 32767.f ? 32767 : (a < -32767.f ? -32767 : a));
        out[2 * i + 1] = (int16_t)(b > 32767.f ? 32767 : (b < -32767.f ? -32767 : b));
        prev = y[i];
        const float ay = y[i] < 0 ? -y[i] : y[i];
        if (ay > lvl) lvl = ay;
    }
    v->last = prev;
    v->level += (lvl - v->level) * .3f;
}

float loungeMotion(const struct LoungeCore* p) {
    float m = 0;
    if (p) for (int i = 0; i < VOICES; i++) if (p->v[i].level > m) m = p->v[i].level;
    return m > 1.f ? 1.f : m;
}
uint32_t loungeStrikes(const struct LoungeCore* p) { return p ? p->strikes : 0; }
