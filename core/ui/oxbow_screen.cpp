// Port czesci ekranowej oxbow.c (kennel/Hundekraft). Arytmetyka i liczby jak w oryginale; rysowanie przez Canvas,
// odczyty nasza czcionka (oryginal: seq_text 4x6 przy 0,875 px).
#include "ui/oxbow_screen.h"
#include <math.h>
#include <stdio.h>
#include "gfx/font.h"

#include "../../third_party/kennel_oxbow/plugins/oxbow/oxbow_art.h"
#include "../../third_party/kennel_oxbow/plugins/oxbow/oxbow_geom.h"

namespace dp {

// ekran OP-1 -> nasz: 1,82x z marginesami — pasek odczytow oryginalu (y 142..148) konczy sie nad napisami strony
// (przy 2x wchodzil na nie); ilustracja ustawia inne
static float S = 1.82f, OX = 29.f, OY = 46.f;
#define ONE 1024
#define DEG 23
enum { G_HORNL, G_HORNR, G_BIRDL, G_BIRDR, G_BELL, G_BREATH, G_S0, NG = G_S0 + 6 };
static const int32_t KDEF[4] = {11869, 5677, 23738, 2560};

static int16_t SIN_T[8192];
static void sinInit() {
    if (SIN_T[2048]) return;
    for (int i = 0; i < 8192; i++) SIN_T[i] = (int16_t)lrint(32767.0 * sin(2 * M_PI * i / 8192.0));
}
static inline int32_t sin15(int32_t a) { return SIN_T[a & 8191]; }
static inline int32_t cos15(int32_t a) { return SIN_T[(a + 2048) & 8191]; }
static inline Color hexc(unsigned v) { return rgb((v >> 16) & 255, (v >> 8) & 255, v & 255); }

// macierz grupy (Q10 jak SVG_MATRIX: x' = a x + c y + tx), widocznosc
struct Xf { int32_t a = ONE, b = 0, c = 0, d = ONE, tx = 0, ty = 0; bool vis = true; };
static void setM(Xf& m, int32_t a, int32_t b, int32_t c, int32_t d, int32_t tx, int32_t ty) {
    m.a = a; m.b = b; m.c = c; m.d = d; m.tx = tx; m.ty = ty;
}

static inline void mapPt(const Xf* xf, int g, float x, float y, float* ox_, float* oy_) {
    float u = x, v = y;
    if (g >= 0 && g < NG) {
        const Xf& m = xf[g];
        u = (m.a * x + m.c * y) / 1024.f + m.tx / 1024.f;
        v = (m.b * x + m.d * y) / 1024.f + m.ty / 1024.f;
    }
    *ox_ = OX + u * S;
    *oy_ = OY + v * S;
}

static void drawItem(Canvas& c, const OBItem& it, const Xf* xf) {
    static float buf[2 * 1024];
    if (it.kind == 0) {
        const OBFill& f = OB_FILLS[it.idx];
        if (f.group < -1 || (f.group >= 0 && !xf[f.group].vis) || f.count > 1024) return;
        for (int i = 0; i < f.count; i++) mapPt(xf, f.group, OB_PTS[(f.first + i) * 2], OB_PTS[(f.first + i) * 2 + 1], &buf[2 * i], &buf[2 * i + 1]);
        c.fillPolygon(buf, f.count, hexc(f.rgb));
    } else if (it.kind == 1) {
        const OBStroke& l = OB_STROKES[it.idx];
        if (l.group < -1 || (l.group >= 0 && !xf[l.group].vis)) return;
        c.beginStroke();
        const int segs = l.closed ? l.count : l.count - 1;
        float px, py;
        mapPt(xf, l.group, OB_PTS[l.first * 2], OB_PTS[l.first * 2 + 1], &px, &py);
        for (int i = 0; i < segs; i++) {
            const int b = l.first + (i + 1 == l.count ? 0 : i + 1);
            float x, y;
            mapPt(xf, l.group, OB_PTS[b * 2], OB_PTS[b * 2 + 1], &x, &y);
            c.strokeSeg(px, py, x, y, l.width * S * 0.8f);
            px = x; py = y;
        }
        c.endStroke(hexc(l.rgb));
    } else {
        const OBCircle& d = OB_CIRCLES[it.idx];
        if (d.group < -1 || (d.group >= 0 && !xf[d.group].vis)) return;
        float x, y;
        mapPt(xf, d.group, d.x, d.y, &x, &y);
        if (d.filled) c.fillCircle(x, y, (d.r + d.width * 0.5f) * S, hexc(d.rgb));
        else c.circle(x, y, d.r * S, d.width * S * 0.8f, hexc(d.rgb));
    }
}
static int itemGroup(const OBItem& it) {
    return it.kind == 0 ? OB_FILLS[it.idx].group : it.kind == 1 ? OB_STROKES[it.idx].group : OB_CIRCLES[it.idx].group;
}

static inline int32_t around(int32_t k, int32_t d, int32_t lo, int32_t hi) {
    return k < d ? lo - (lo * k) / d : (hi * (k - d)) / (32767 - d);
}

// przeksztalcenia rogow, ptakow, dzwonka i oddechu (jak ox_screen_update); swing = faza kolysania, act, puff
static void poses(Xf* xf, const int32_t k[4], uint32_t swing, int32_t act, int32_t puff) {
    const int32_t sx = ONE + around(k[0], KDEF[0], -123, 123);
    const int32_t droop = k[0] < KDEF[0] ? -(4 * DEG * (KDEF[0] - k[0])) / KDEF[0] : 0;
    const int32_t bd = (k[3] * ((OX_BIRD_N - 1) * 1024)) >> 15;
    const int32_t b0 = bd >> 10 < OX_BIRD_N - 1 ? bd >> 10 : OX_BIRD_N - 2, bt = bd - (b0 << 10);
    for (int s = 0; s < 2; s++) {
        const int32_t a = s ? -droop : droop;
        const int32_t c = (cos15(a) + 16) >> 5, sn = (sin15(a) + 16) >> 5;
        const int32_t px = s ? OX_HORN_R_X : OX_HORN_L_X, py = s ? OX_HORN_R_Y : OX_HORN_L_Y;
        setM(xf[G_HORNL + s], (c * sx) >> 10, (sn * sx) >> 10, -sn, c, px - ((((c * sx) >> 10) * px - sn * py) >> 10),
             py - ((((sn * sx) >> 10) * px + c * py) >> 10));
        const int32_t(*path)[2] = s ? OX_BIRD_R : OX_BIRD_L;
        const int32_t fx = s ? OX_FEET_R_X : OX_FEET_L_X, fy = s ? OX_FEET_R_Y : OX_FEET_L_Y;
        int32_t qx = fx + path[b0][0] + (((path[b0 + 1][0] - path[b0][0]) * bt) >> 10);
        int32_t qy = fy + path[b0][1] + (((path[b0 + 1][1] - path[b0][1]) * bt) >> 10);
        {  // horn_map
            const int32_t dx = ((qx - px) * sx) >> 10, dy = qy - py;
            const int32_t nx = px + ((c * dx - sn * dy) >> 10), ny = py + ((sn * dx + c * dy) >> 10);
            qx = nx; qy = ny;
        }
        setM(xf[G_BIRDL + s], c, sn, -sn, c, qx - ((c * fx - sn * fy) >> 10), qy - ((sn * fx + c * fy) >> 10));
    }
    const int32_t amp = ((k[1] * 26 * DEG) >> 15) * (ONE + (act >> 1)) >> 10;
    const int32_t ba = (sin15((int32_t)(swing >> 2)) * amp) >> 15;
    const int32_t bc = (cos15(ba) + 16) >> 5, bs = (sin15(ba) + 16) >> 5;
    setM(xf[G_BELL], bc, bs, -bs, bc, OX_BELL_X - ((bc * OX_BELL_X - bs * OX_BELL_Y) >> 10),
         OX_BELL_Y - ((bs * OX_BELL_X + bc * OX_BELL_Y) >> 10));
    const int32_t amt = k[2] > 16384 ? k[2] - 16384 : 16384 - k[2], ad = KDEF[2] - 16384;
    int32_t sc = amt < ad ? 512 + (512 * amt) / ad : ONE + (123 * (amt - ad)) / (16383 - ad);
    sc = (sc * puff) >> 10;
    setM(xf[G_BREATH], sc, 0, 0, sc, OX_BREATH_X - ((sc * OX_BREATH_X) >> 10), OX_BREATH_Y - ((sc * OX_BREATH_Y) >> 10));
}

// odczyty na dole (ox_controls)
static void controls(Canvas& c, const int32_t k[4], const int16_t words[3], bool page2, const OxbowPlusShow* plus) {
    static const Color colors[4] = {rgb(105, 142, 255), rgb(0, 237, 149), rgb(245, 245, 245), rgb(255, 58, 93)};
    static const char* const waves[6] = {"SAW", "PULSE", "BOTH", "SYNC", "X-MOD", "FIFTH"};
    static const char* const filters[6] = {"LP12", "HP12", "BP12", "NOTCH", "LP24", "SEM"};
    static const char* const primary[4] = {"CUTOFF ", "RES ", "ENV ", "DETUNE "};
    for (int i = 0; i < 4; i++) {
        char b[24];
        int v = (k[i] * 100 + 16383) / 32767;
        if (plus && i == 2) snprintf(b, sizeof b, "FILTER %s", filters[plus->ftype % 6]);  // OXBOW+
        else if (plus && i == 3) snprintf(b, sizeof b, "ENV>PITCH %+d", (int)lrintf(plus->envPitch * 24.f / 64.f));
        else if (i < 3 && page2) {
            const int w = words[i] < 0 ? 0 : words[i];
            if (i == 0) snprintf(b, sizeof b, "WAVE %s", waves[(w * 6) >> 15]);
            else if (i == 1) snprintf(b, sizeof b, "FILTER %s", filters[((unsigned)w >> 9) % 6]);
            else snprintf(b, sizeof b, "DRIVE %d%%", (w * 100 + 16383) / 32767);
        } else if (i == 2) {
            v = ((k[i] - 16384) * 100) / 16384;
            snprintf(b, sizeof b, "%s%c%d%%", primary[i], v < 0 ? '-' : '+', v < 0 ? -v : v);
        } else snprintf(b, sizeof b, "%s%d%%", primary[i], v);
        drawText(c, OX + (14 + 80 * i) * S, OY + 142 * S, b, 5.25f * S, S * 0.6f, colors[i]);
    }
}

// Tlo (scena z rogami i ptakami) w cache, przerysowywane tylko, gdy zmienia sie poza rogow / ptakow (CUTOFF, DETUNE);
// co klatke na nim: gwiazdy glosow, dzwonek i oddech (w kolejnosci dokumentu). Oryginal rysuje calosc co klatke —
// tu ~12 ms na klatke na konsoli, stad cache (dzwonek i oddech leza nad tlem jak w dokumencie).
static bool dynamicGroup(int g) { return g == G_BELL || g == G_BREATH || (g >= G_S0 && g < NG); }

void OxbowScreen::draw(Canvas& c, const int16_t kn[4], const int16_t words[3], bool page2, const OxbowView& v,
                       const OxbowPlusShow* plus) {
    sinInit();
    int32_t k[4];
    for (int i = 0; i < 4; i++) k[i] = kn[i] < 0 ? 0 : kn[i];
    int32_t pk[4] = {k[0], k[1], k[2], k[3]};  // galki dla sceny
    if (plus) {  // OXBOW+: oddech z typu filtra (LP12 = najmniejszy .. SEM = najwiekszy), ptaki z ENV>PITCH (srodek = 0)
        pk[2] = 16384 + (plus->ftype % 6) * 16383 / 5;
        pk[3] = (plus->envPitch + 64) * 32767 / 127;
    }
    const uint32_t blocks = v.blocks;
    const int on = blocks != blocks_;
    blocks_ = blocks;
    frame_++;
    Xf xf[NG];
    int32_t env = 0, nv = 0;
    for (int i = 0; i < 6; i++) {
        const int lit = on && blocks - v.vblk[i] < 4u;
        if (lit) { nv++; if (v.venv[i] > env) env = v.venv[i]; }
        xf[G_S0 + i].vis = lit;
    }
    act_ += ((nv ? ONE : 0) - act_) >> 3;
    swing_ += 440;
    const int32_t puff = 780 + ((env * 260) >> 15);
    puff_ += ((on && nv ? puff : ONE) - puff_) >> 2;
    if (frame_ < 2) puff_ = ONE;
    poses(xf, pk, swing_, act_, puff_);
    const int32_t key[4] = {xf[G_HORNL].a, xf[G_HORNL].b, xf[G_BIRDL].tx, xf[G_BIRDL].ty};
    if (!cache_ || key[0] != bgKey_[0] || key[1] != bgKey_[1] || key[2] != bgKey_[2] || key[3] != bgKey_[3]) {
        if (!cache_) cache_ = new Canvas();
        cache_->clear(pal::bg);
        for (const OBItem& it : OB_ORDER)
            if (!dynamicGroup(itemGroup(it))) drawItem(*cache_, it, xf);
        for (int i = 0; i < 4; i++) bgKey_[i] = key[i];
    }
    c.copyRows(*cache_, (int)OY, (int)(OY + 160 * S) + 1);
    for (const OBItem& it : OB_ORDER)
        if (dynamicGroup(itemGroup(it))) drawItem(c, it, xf);
    controls(c, k, words, page2, plus);
}

void drawOxbowIllustration(Canvas& c, float x, float y, float w, float h) {
    sinInit();
    const float s0 = S, x0 = OX, y0 = OY;
    S = fminf(w / 320.f, h / 160.f);
    OX = x + (w - 320 * S) * 0.5f;
    OY = y + (h - 160 * S) * 0.5f;
    Xf xf[NG];
    for (int i = 0; i < 6; i++) xf[G_S0 + i].vis = i < 3;  // trzy gwiazdy: akord
    poses(xf, KDEF, 0, 0, ONE);
    for (const OBItem& it : OB_ORDER) drawItem(c, it, xf);
    S = s0; OX = x0; OY = y0;
}

}  // namespace dp
