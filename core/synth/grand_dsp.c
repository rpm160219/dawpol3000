/* GRAND: DSP (wlasny, zob. grand_dsp.h). */
#include "synth/grand_dsp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#ifdef GRAND_DEBUG
#include <stdio.h>
#endif

#define VOICES 6
#define BLOCK 128
#define NLEN 1024
#define BLEN 2048
#define NAP 8
#ifndef CLICK_K
#define CLICK_K 55.f
#endif
#ifndef CLICK_Z
#define CLICK_Z .3f
#endif
#ifndef KNEE
#define KNEE .8f
#endif
#ifndef SHELF_F
#define SHELF_F 260.f
#endif
#ifndef SHELF_B
#define SHELF_B .4f
#endif

typedef struct {
    float nut[NLEN], br[BLEN];
    int nw, bw;
    float lp, apx[NAP], apy[NAP];
} GStr;

typedef struct {
    GStr s[3];
    int ns, note, strike, held, damped;
    float xh, vh, ys, m, K, pw, tcont, Z; int contact;
    float apC; int apN, apNote;   /* sztywnosc: wspolczynnik filtrow wszechprzepustowych dobrany do nuty */
    float knock, kLp, shLp;
    float click[64]; int clickPos;   /* ostre klikniecie powierzchni filcu (szerokopasmowe pobudzenie strun) */       /* pukniecie mlotka w plyte, polka niskich (plyta slabo oddaje podstawe basu) */
    float fdn[4][2048]; int fw; float fdlp[4], fdHp;
    float grHp, grX, lidLp, dcx, dcy, level, last, squash;
} GVoice;

struct GrandCore { GVoice v[VOICES]; uint32_t strikes; float squash; int lastNs; };

/* plyta: dlugie, wzglednie pierwsze linie = gesto rozlozone, lagodne rezonanse (krotkie linie dawaly co ~60 Hz
 * wyrazne piki ponizej C4 — buczenie) */
static const int FDL[4] = {557, 761, 1031, 1307};

struct GrandCore* grandCreate(void) {
    struct GrandCore* p = (struct GrandCore*)calloc(1, sizeof(struct GrandCore));
    if (p) for (int i = 0; i < VOICES; i++) p->v[i].note = -1;
    return p;
}
void grandNoteOn(struct GrandCore* p, int voice) {
    if (!p || voice < 0 || voice >= VOICES) return;
    p->v[voice].strike = 1; p->v[voice].held = 1;
    p->strikes++;
}
void grandNoteOff(struct GrandCore* p, int voice) {
    if (!p || voice < 0 || voice >= VOICES) return;
    if (p->v[voice].held) { p->v[voice].held = 0; p->v[voice].damped = 1; }
}
float grandDecaySeconds(int knob) { return 2.f * powf(10.f, (knob < 0 ? 0 : knob) / 32767.f * 1.2f); }  /* 2 .. 32 s */
float grandStrikeRatio(int knob) { return 1.f / 16.f + (1.f / 5.f - 1.f / 16.f) * (knob < 0 ? 0 : knob) / 32767.f; }

static float lpDelay(float b, float w) { return atan2f(b * sinf(w), 1.f - b * cosf(w)) / w; }
static float apDelay(float a, float w) {
    const float ph = atan2f(-sinf(w), a + cosf(w)) - atan2f(-a * sinf(w), 1.f + a * cosf(w));
    float d = -ph / w;
    while (d < 0) d += 6.2831853f / w;
    return d;
}

void grandRender(struct GrandCore* p, int voice, int16_t* out, const int16_t* knobs, uint32_t inc, float rate, int vel) {
    if (!p || voice < 0 || voice >= VOICES) return;
    GVoice* v = &p->v[voice];
    const float fs = rate * .5f, dt = 1.f / fs;
    const int n = BLOCK / 2;
    float f0 = (float)inc * rate / 4294967296.f;
    if (f0 < 26.f) f0 = 26.f;
    if (f0 > 4300.f) f0 = 4300.f;
    const float hard = (knobs[0] < 0 ? 0 : knobs[0]) / 32767.f, lid = (knobs[3] < 0 ? 0 : knobs[3]) / 32767.f;
    const float unison = (knobs[5] < 0 ? 0 : knobs[5]) / 32767.f, growl = (knobs[6] < 0 ? 0 : knobs[6]) / 32767.f;
    const float board = (knobs[7] < 0 ? 0 : knobs[7]) / 32767.f;
    const int nt = (int)lrintf(69.f + 12.f * log2f(f0 / 440.f));
    const float key = (nt - 21) / 87.f;  /* 0 = A0 .. 1 = C8 */
    if (v->strike) {
        v->strike = 0;
        if (nt != v->note || v->damped) {  /* inna nuta albo struny wytlumione: od cichych strun */
            for (int s = 0; s < 3; s++) { memset(&v->s[s], 0, sizeof(GStr)); }
            v->ys = 0;
        }
        v->note = nt;
        v->ns = nt < 31 ? 1 : (nt < 41 ? 2 : 3);
        v->damped = 0;
        const float vv = (vel < 1 ? 1 : vel > 127 ? 127 : vel) / 127.f;
        v->m = (.0115f - .0075f * key) * (1.35f - .7f * hard);        /* masa mlotka (kg): lzejsze w gorze; HAMMER: twardy = lzejszy, krotszy kontakt */
        /* impedancja struny sqrt(T mu): ciezkie struny owijane w basie (~8), lekkie w gorze (~0,8) — krotki kontakt w basie */
        v->Z = .8f + 7.f * (1.f - key) * (1.f - key);
        v->pw = 2.2f + .8f * hard + .4f * key;                         /* wykladnik filcu */
        v->K = 3e9f * powf(10.f, 1.6f * hard - .8f + 1.2f * key);      /* sztywnosc filcu */
        v->vh = .3f + 5.2f * powf(vv, 1.6f);                           /* predkosc mlotka (m/s) */
        v->xh = v->ys;
        v->contact = 1; v->tcont = 0;
        v->knock = (.15f + .5f * vv) * vv * (1.2f - .6f * key);
        {  /* klik: impuls przez filtr dolnoprzepustowy (miekki filc ciemniej), bez skladowej stalej */
            const float A = CLICK_K * v->vh * (.3f + .7f * vv) * (.35f + .9f * hard);
            const float fc = (1500.f + 4000.f * hard) * (.6f + .4f * vv);
            const float k1 = 1.f - expf(-6.2831853f * fc / fs);
            /* krotki szelest filcu (~1,3 ms): ta sama barwa co impuls, ale niski wspolczynnik szczytu */
            uint32_t r = 0x9e3779b9u * (uint32_t)(nt + 7);
            float lp = 0, sum = 0;
            for (int i = 0; i < 64; i++) {
                r = r * 1664525u + 1013904223u;
                lp += ((float)(int32_t)r * (1.f / 2147483648.f) - lp) * k1;
                v->click[i] = lp * expf(-(float)i / 18.f);
                sum += v->click[i];
            }
            for (int i = 0; i < 64; i++) v->click[i] = (v->click[i] - sum / 64.f) * A;
            v->clickPos = 0;
        }  /* drewniane pukniecie w plyte (mocniejsze w basie) */
        v->squash = 0;
        p->lastNs = v->ns;
    }
    /* parametry strun na blok */
    float T = grandDecaySeconds(knobs[2]) * powf(110.f / f0, .5f) * 1.6f;
    if (T > 40.f) T = 40.f;
    if (v->damped && nt < 89) T = .12f + .25f * (1.f - key);           /* tlumik (od E6 w gore go nie ma) */
    /* strata wysokich alikwotow: biegun filtra dobrany tak, by 10. alikwot gasl ~3,5x szybciej niz podstawa
     * (w fortepianie wysokie alikwoty brzmia 1-3 s) — przy stalym biegunie srodek i gora tracily je w ulamku sekundy */
    float b;
    {
        const float w10 = 6.2831853f * f0 * 10.f / fs > 1.5f ? 1.5f : 6.2831853f * f0 * 10.f / fs;
        const float hNeed = powf(.001f, 1.f / (T * .28f * f0)) / powf(.001f, 1.f / (T * f0));  /* strata wzgledna na obieg */
        float lo = 0.f, hi = .9f;
        for (int it = 0; it < 20; it++) {
            const float mid = .5f * (lo + hi);
            const float h = (1.f - mid) / sqrtf(1.f - 2.f * mid * cosf(w10) + mid * mid);
            if (h < hNeed) hi = mid; else lo = mid;
        }
        b = .5f * (lo + hi);
    }
    /* niealikwotowosc B jak w fortepianie: duza w skrajnym basie (krotkie, grube struny owijane) i w gorze, najmniejsza
     * w okolicy C3-C4; filtry wszechprzepustowe dobrane tak, by alikwot K mial czestotliwosc K f0 sqrt(1 + B K^2) */
    if (v->apNote != nt + 1) {
        const float B = 2e-4f * powf(2.f, (nt - 60) / 9.f) + 3e-4f * powf(2.f, (33 - nt) / 12.f);
        const int M = nt < 40 ? 8 : (nt < 60 ? 4 : 2);
        int K = (int)(fs * .2f / f0);
        if (K > (nt < 72 ? 12 : 20)) K = nt < 72 ? 12 : 20;
        float a = 0.f;
        if (K >= 2) {
            const float P = fs / f0, w0 = 6.2831853f * f0 / fs, wk = w0 * K;
            const float need = P * (1.f - 1.f / sqrtf(1.f + B * K * K));  /* roznica opoznien potrzebna miedzy f0 a K f0 */
            float lo = -.95f, hi = 0.f;
            for (int it = 0; it < 24; it++) {
                const float mid = .5f * (lo + hi);
                const float got = (float)M * (apDelay(mid, w0) - apDelay(mid, wk));
                if (got > need) lo = mid; else hi = mid;
            }
            a = .5f * (lo + hi);
        }
        v->apC = a; v->apN = M; v->apNote = nt + 1;
    }
    const float ap = v->apC;
    const int nap = v->apN;
    const float beta = grandStrikeRatio(knobs[1]);
    const float cents = (.15f + 4.f * unison * unison);
    static const float sdet[3] = {0.f, 1.f, -.85f};
    int Nn[3]; float Lb[3], g[3];
    for (int s = 0; s < v->ns; s++) {
        const float f = f0 * powf(2.f, sdet[s] * cents / 1200.f), w = 6.2831853f * f / fs;
        const float P = fs / f;
        int a2 = (int)lrintf(beta * P);
        if (a2 < 1) a2 = 1;
        if (a2 > NLEN - 1) a2 = NLEN - 1;
        float lb = P - (float)a2 - lpDelay(b, w) - (float)nap * apDelay(ap, w);
        if (lb < 1.5f) lb = 1.5f;
        if (lb > BLEN - 3) lb = BLEN - 3;
        Nn[s] = a2; Lb[s] = lb;
        g[s] = powf(.001f, 1.f / (T * f));
    }
    /* mostek: drganie wspolne strun (w fazie) oddaje energie do plyty szybciej — wybrzmienie dwuetapowe: wspolne ~0,4 T,
     * rozstrojone (UNISON) zostaje dluzej */
    const float Tp = (v->damped && nt < 89 ? T : T * .4f);
    /* pojedyncza struna (skrajny bas): bez dodatkowego szybkiego etapu — dlugie wybrzmienie */
    const float cb = v->ns > 1 ? .5f * (1.f - powf(.001f, 1.f / (Tp * f0))) : 0.f;
    const float lidK = 1.f - expf(-6.2831853f * (1800.f + 16000.f * lid * lid) / fs);
    const float grK = 1.f - expf(-6.2831853f * 60.f / fs);
    const float shK = 1.f - expf(-6.2831853f * (SHELF_F * (1.f - key) + 90.f * key) / fs);  /* plyta slabo oddaje dol: w basie polka wyzej */
    const float grAmt = growl * .8f * (1.f - key) * (1.f - key);
    /* plyta: zanik linii FDN */
    const float Tb = .04f + .16f * board;
    float fg[4];
    for (int k = 0; k < 4; k++) fg[k] = powf(.001f, (float)FDL[k] / (Tb * fs));
    /* wyrownanie poziomu po klawiaturze (zmierzone szczyty przy vel 100, cel ~0,3 skali) */
    static const float CT[8] = {2.531f, 0.931f, 0.245f, 0.223f, 0.207f, 0.216f, 0.748f, 0.551f};
    float comp;
    {
        float q = (nt - 21) / 12.f;
        if (q < 0) q = 0;
        if (q > 7) q = 7;
        const int qi = q >= 7 ? 6 : (int)q;
        comp = (CT[qi] + (CT[qi + 1] - CT[qi]) * (q - qi)) * (2300.f / 32767.f) * powf(2.2f, 1.f - 2.f * hard)  /* HAMMER: twardy jasniejszy, ale tylko troche glosniejszy */;
    }
    float y[BLOCK / 2], lvl = 0;
    for (int i = 0; i < n; i++) {
        float r1[3], r2[3], u[3], usum = 0;
        for (int s = 0; s < v->ns; s++) {
            GStr* st = &v->s[s];
            r1[s] = -st->nut[(st->nw - Nn[s]) & (NLEN - 1)];
            const float rp = (float)st->bw - Lb[s];
            const int i0 = (int)floorf(rp);
            const float fr = rp - (float)i0;
            const float x0 = st->br[i0 & (BLEN - 1)], x1 = st->br[(i0 + 1) & (BLEN - 1)];
            float x = (x0 + (x1 - x0) * fr) * g[s];
            st->lp = (1.f - b) * x + b * st->lp;
            x = st->lp;
            for (int k = 0; k < nap; k++) {
                const float yk = ap * x + st->apx[k] - ap * st->apy[k];
                st->apx[k] = x; st->apy[k] = yk; x = yk;
            }
            u[s] = x;
            usum += x;
        }
        const float hb = 2.f * cb / (float)v->ns;
        for (int s = 0; s < v->ns; s++) r2[s] = -(u[s] - hb * usum);   /* odbicie na mostku ze sprzezeniem strun */
        /* mlotek */
        float fi = 0;
        if (v->contact) {
            float vin = 0;
            for (int s = 0; s < v->ns; s++) vin += r1[s] + r2[s];
            vin /= (float)v->ns;
            const float h = dt * .25f;
            float Fsum = 0, cmax = 0;
            for (int k = 0; k < 4; k++) {
                const float c = v->xh - v->ys;
                const float F = c > 0.f ? v->K * powf(c, v->pw) : 0.f;
                const float vs = vin + F / (2.f * v->Z * (float)v->ns);
                v->ys += vs * h;
                v->vh -= F / v->m * h;
                v->xh += v->vh * h;
                Fsum += F;
                if (c > cmax) cmax = c;
            }
            fi = Fsum * .25f / (2.f * v->Z * (float)v->ns);

            if (cmax * 2000.f > v->squash) v->squash = cmax * 2000.f > 1.f ? 1.f : cmax * 2000.f;
            v->tcont += dt;
            if ((v->xh < v->ys && v->vh < 0.f && v->tcont > .0003f) || v->tcont > .03f) {
                v->contact = 0;  /* odskok mlotka */
#ifdef GRAND_DEBUG
                fprintf(stderr, "nuta %d: kontakt %.2f ms, vh %.2f, xh-ys %.2e\n", nt, v->tcont * 1000.f, v->vh, v->xh - v->ys);
#endif
            }
        } else {
            float vs = 0;
            for (int s = 0; s < v->ns; s++) vs += r1[s] + r2[s];
            v->ys += vs / (float)v->ns * dt;
            v->ys *= .9995f;
        }
        if (v->clickPos < 64) fi += v->click[v->clickPos++] * CLICK_Z;  /* niezaleznie od impedancji: jasny atak takze w basie */
        for (int s = 0; s < v->ns; s++) {
            GStr* st = &v->s[s];
            st->nut[st->nw & (NLEN - 1)] = r2[s] + fi; st->nw++;
            st->br[st->bw & (BLEN - 1)] = r1[s] + fi; st->bw++;
        }
        /* sila na mostku -> pomruk (nieliniowosc napiecia) -> plyta FDN */
        float bridge = usum;
        if (grAmt > 0.f) {
            const float sq = bridge * bridge;
            v->grX += (sq - v->grX) * grK;
            bridge += (sq - v->grX) * grAmt;
        }
        float kin = 0;
        if (v->knock > 1e-5f) {
            static uint32_t rs = 22222u;
            rs = rs * 1664525u + 1013904223u;
            v->kLp += ((float)(int32_t)rs * (1.f / 2147483648.f) - v->kLp) * .12f;
            kin = v->kLp * v->knock;
            v->knock *= .9975f;
        }
        bridge += kin;
        v->fdHp += (bridge - v->fdHp) * .012f;  /* plyta bez dudnienia ponizej ~90 Hz */
        const float bin = bridge - v->fdHp;
        float o[4], hs;
        for (int k = 0; k < 4; k++) o[k] = v->fdn[k][(v->fw - FDL[k]) & 2047];
        hs = (o[0] + o[1] + o[2] + o[3]) * .5f;
        for (int k = 0; k < 4; k++) {
            float x = (o[k] - hs) * fg[k];
            v->fdlp[k] += (x - v->fdlp[k]) * .55f;
            v->fdn[k][v->fw & 2047] = v->fdlp[k] + bin * .25f;
        }
        v->fw = (v->fw + 1) & 2047;
        float out1 = bridge * .75f + (o[0] - o[1] + o[2] - o[3]) * (.1f + .35f * board);
        v->shLp += (out1 - v->shLp) * shK;                   /* plyta slabo oddaje podstawe basu: polka -7 dB ponizej ~90 Hz */
        out1 -= (.55f + SHELF_B * (1.f - key)) * v->shLp;
        v->lidLp += (out1 - v->lidLp) * lidK;                /* pokrywa: barwa */
        out1 = v->lidLp * (.6f + .4f * lid);
        const float yd = out1 - v->dcx + .999f * v->dcy;     /* filtr DC */
        v->dcx = out1; v->dcy = yd;
        float yy = yd * comp;  /* skala: 1 = pelna skala */
        const float ay0 = yy < 0 ? -yy : yy;
        if (ay0 > KNEE) { const float e = ay0 - KNEE, kk = KNEE + e / (1.f + e / (.98f - KNEE)); yy = yy < 0 ? -kk : kk; }  /* kolano < 0,98 */
        y[i] = yy;
        if (ay0 > lvl) lvl = ay0;
    }
    p->squash = v->squash;
    v->squash *= .9f;
    float prev = v->last;
    for (int i = 0; i < n; i++) {
        const float a = (prev + y[i]) * .5f * 32767.f, c = y[i] * 32767.f;
        out[2 * i] = (int16_t)(a > 32767.f ? 32767 : (a < -32767.f ? -32767 : a));
        out[2 * i + 1] = (int16_t)(c > 32767.f ? 32767 : (c < -32767.f ? -32767 : c));
        prev = y[i];
    }
    v->last = prev;
    v->level += (lvl * 2.5f - v->level) * .3f;
}

float grandMotion(const struct GrandCore* p) {
    float m = 0;
    if (p) for (int i = 0; i < VOICES; i++) if (p->v[i].level > m) m = p->v[i].level;
    return m > 1.f ? 1.f : m;
}
uint32_t grandStrikes(const struct GrandCore* p) { return p ? p->strikes : 0; }
float grandSquash(const struct GrandCore* p) { return p ? p->squash : 0.f; }
int grandHeld(const struct GrandCore* p) { int h = 0; if (p) for (int i = 0; i < VOICES; i++) h += p->v[i].held; return h; }
int grandLastStrings(const struct GrandCore* p) { return p && p->lastNs ? p->lastNs : 3; }
