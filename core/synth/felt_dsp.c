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
typedef struct { float b0, a1, a2, x1, x2, y1, y2; } Res;  /* pasmowy RBJ: stale 0 dB w szczycie */

typedef struct {
    Str s[3];
    float exc[ELEN]; int ePos, eLen;  /* impuls mlotka (z grzebieniem miejsca uderzenia) */
    float thump, thLp, thPh, damp, dampLp, swish, swLp1, swLp2;
    Res body[6];
    float wc[3], knock, kLp;      /* WORN: odchylenie strun tej nuty (centy); pukniecie plyty */
    int note;
    int strike, held, released;
    int damped;               /* tlumik na strunach (klawisz puszczony) */
    float level, last, out1, dcx, dcy;  /* filtr DC na wyjsciu */
    uint32_t rng;
} FtVoice;

struct FeltCore { FtVoice v[VOICES]; uint32_t strikes; float lastCents; };

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

static void resSet(Res* r, float f, float q, float fs) {  /* pasmowy (RBJ, 0 dB w szczycie) — bez dudnienia w basie */
    const float w = 6.2831853f * f / fs, al = sinf(w) / (2.f * q), n = 1.f / (1.f + al);
    r->b0 = al * n; r->a1 = 2.f * cosf(w) * n; r->a2 = -(1.f - al) * n;
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
    const float detune = (knobs[5] < 0 ? 0 : knobs[5]) / 32767.f, worn = (knobs[6] < 0 ? 0 : knobs[6]) / 32767.f;
    const float bodyK = (knobs[7] < 0 ? 0 : knobs[7]) / 32767.f;
    const float vv = (vel < 1 ? 1 : vel > 127 ? 127 : vel) / 127.f;
    /* struny: liczba i rozstrojenie z CORDA (plynnie: druga i trzecia wchodza ciszej) */
    const float g2 = corda < .33f ? 0.f : (corda - .33f) * 3.f > 1.f ? 1.f : (corda - .33f) * 3.f;
    const float g3 = corda < .66f ? 0.f : (corda - .66f) * 3.f > 1.f ? 1.f : (corda - .66f) * 3.f;
    const float cents = .1f + 14.f * detune * detune;  /* DETUNE: rozstrojenie strun jednej nuty */
    const float det[3] = {v->wc[0], cents + v->wc[1], -cents * .8f + v->wc[2]};
    const int ns = g3 > 0.f ? 3 : (g2 > 0.f ? 2 : 1);
    /* strata: T60 z DECAY, krotsze w gorze; jasnosc strat (b) */
    float T = feltDecaySeconds(knobs[2]) * powf(261.6f / f0, .55f);
    if (T > 40.f) T = 40.f;
    /* filc gasi tez gorne alikwoty szybciej; w gorze mniej strat na przejscie (wiecej przejsc na sekunde) */
    const float b = (.05f + .24f * felt) * (f0 > 262.f ? powf(262.f / f0, .7f) : 1.f);
    const float ap = -.12f - .25f * (f0 > 400.f ? 1.f : f0 / 400.f);  /* sztywnosc: wieksza niealikwotowosc w gorze */
    if (v->strike) {  /* uderzenie mlotka: impuls kontaktu + grzebien miejsca uderzenia + mechanika */
        v->strike = 0;
        {  /* ponowne uderzenie: mlotek pochlania czesc drgan (trzymana nuta), po puszczeniu tlumik juz je wygasil */
            const float keep = v->damped ? 0.f : .6f;
            for (int k = 0; k < 3; k++) {
                Str* st = &v->s[k];
                if (keep == 0.f) { memset(st->buf, 0, sizeof st->buf); st->lp = st->a1x = st->a1y = st->a2x = st->a2y = 0; }
                else for (int i = 0; i < SLEN; i++) st->buf[i] *= keep;
            }
        }
        const float tc = (.0004f + .003f * felt) * (1.4f - .75f * vv) * fs;  /* czas kontaktu (probki) */
        const float tmax = .45f * fs / f0 * (1.f + felt);  /* w gorze mlotek dotyka krocej (ulamek okresu) */
        const float tcl = tc < tmax ? tc : tmax;
        const int tci = tcl < 3.f ? 3 : (tcl > 300.f ? 300 : (int)tcl);
        const int beta = (int)(fs / f0 / 8.f);       /* miejsce uderzenia: 1/8 struny */
        const float A = (.06f + .94f * powf(vv, 1.8f)) * (.95f - .3f * felt);  /* szeroka dynamika jak w fortepianie */
        const float fc = (1300.f + 15000.f * vv * vv * vv) * (1.f - .7f * felt);  /* twardszy, jasniejszy mlotek przy forte */
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
        {  /* bez skladowej stalej: inaczej DC krazy w petli struny i narasta z kolejnymi uderzeniami */
            float sum = 0;
            for (int i = 0; i < len; i++) sum += v->exc[i];
            const float m = sum / (float)len;
            for (int i = 0; i < len; i++) v->exc[i] -= m;
        }
        v->ePos = 0; v->eLen = len;
        v->thump = noise * (.15f + .25f * vv);       /* stuk klawisza */
        v->knock = (.04f + .1f * vv) * (.3f + bodyK);  /* pukniecie plyty rezonansowej (czesc brzmienia fortepianu) */
        /* WORN: kazdy klawisz ma na stale swoje odchylenie (wiekszosc lekko, kilka wyraznie) */
        {
            const int nt = (int)lrintf(69.f + 12.f * log2f(f0 / 440.f));
            uint32_t h = (uint32_t)nt * 2654435761u ^ 0x9e3779b9u;
            float r[3];
            for (int k = 0; k < 3; k++) { h ^= h >> 15; h *= 2246822519u; h ^= h >> 13; r[k] = (float)(h & 0xffff) / 32767.5f - 1.f; }
            const float key = r[0] * r[0] * r[0] * r[0] * r[0] * 45.f * worn;    /* cala nuta */
            v->wc[0] = key;
            v->wc[1] = key + r[1] * 9.f * worn;                     /* struny nuty rozjezdzaja sie */
            v->wc[2] = key + r[2] * 9.f * worn;
            v->note = nt;
            p->lastCents = key;
        }
        v->swish = noise * (.05f + .08f * felt) * vv;  /* szelest filcu */
        v->thPh = 0;
        v->damped = 0;

    }
    if (v->released) { v->released = 0; v->damp = noise * .12f; v->damped = 1; }
    static const float bf[6] = {92.f, 185.f, 330.f, 610.f, 1150.f, 2400.f}, bq[6] = {3.5f, 4.f, 5.f, 5.f, 4.f, 3.f};
    for (int bb = 0; bb < 6; bb++) resSet(&v->body[bb], bf[bb], bq[bb], fs);
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
        /* plyta rezonansowa: struny + pukniecie mlotka w plyte */
        float kin = 0;
        if (v->knock > 1e-5f) { v->kLp += (rnd(&v->rng) - v->kLp) * .45f; kin = v->kLp * v->knock; v->knock *= .9965f; }
        float body = 0;
        for (int bb = 0; bb < 6; bb++) {
            Res* r = &v->body[bb];
            const float xin = bridge + kin * 3.f;
            const float o = r->b0 * (xin - r->x2) + r->a1 * r->y1 + r->a2 * r->y2;
            r->x2 = r->x1; r->x1 = xin;
            r->y2 = r->y1; r->y1 = o;
            body += o;
        }
        float o = bridge * .85f + body * (.6f + 1.6f * bodyK) + kin * .3f;
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
        {  /* filtr DC (~8 Hz) */
            const float yd = o - v->dcx + .999f * v->dcy;
            v->dcx = o; v->dcy = yd; o = yd;
        }
        {  /* lagodne kolano od 1,15, asymptota 1,6 (= pelna skala po * 20000): najglosniejsze uderzenia nie przesterowuja */
            const float ao = o < 0 ? -o : o;
            if (ao > 1.15f) { const float e = ao - 1.15f, k = 1.15f + e / (1.f + e / .45f); o = o < 0 ? -k : k; }
        }
        v->out1 = o;
        y[i] = o;
        const float ay = o < 0 ? -o : o;
        if (ay > lvl) lvl = ay;
    }
    float prev = v->last;
    for (int i = 0; i < n; i++) {
        const float a = (prev + y[i]) * .5f * 20000.f, bq2 = y[i] * 20000.f;
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
float feltLastCents(const struct FeltCore* p) { return p ? p->lastCents : 0.f; }
int feltHeld(const struct FeltCore* p) {
    int h = 0;
    if (p) for (int i = 0; i < VOICES; i++) h += p->v[i].held;
    return h;
}
