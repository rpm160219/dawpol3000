// Port czesci ekranowej reactor.c (op1r/reactor, Hundekraft). Arytmetyka i liczby jak w oryginale; rysowanie przez
// nasz Canvas, odczyty nasza czcionka (oryginal: seq_text 4x6 przy 0,8 px).
#include "ui/reactor_screen.h"
#include <math.h>
#include <stdio.h>
#include "gfx/font.h"

#include "../../third_party/op1r_reactor/plugins/reactor/reactor_art.h"
#include "../../third_party/op1r_reactor/plugins/reactor/reactor_geom.h"

namespace dp {

static float S = 2.f, OX = 0.f, OY = 44.f;  // ekran OP-1 -> nasz (ilustracja ustawia inne)

#define T_ONE 65536
#define SK_ONE 65536
#define SK_GAP 172
#define SK_QUIET 40
#define SK_REST 1300
#define SK_NPART 7
#define CH_X (96 << 10)
#define CH_Y (69 << 10)
#define P_DESK 0
#define P_TOWER 4
#define P_VESSEL 6
enum { G_S0 = 0, G_ROD1 = 8, G_CAP = 12, NG = 13 };

static int16_t SIN_T[8192];
static void sinInit() {
    if (SIN_T[2048]) return;
    for (int i = 0; i < 8192; i++) SIN_T[i] = (int16_t)lrint(32767.0 * sin(2 * M_PI * i / 8192.0));
}
static inline int32_t tsin(int32_t a) { return SIN_T[a & 8191]; }
static inline int32_t tcos(int32_t a) { return tsin(a + 2048); }
static inline int32_t clampi(int32_t x, int32_t lo, int32_t hi) { return x < lo ? lo : x > hi ? hi : x; }
static inline Color hexc(unsigned v) { return rgb((v >> 16) & 255, (v >> 8) & 255, v & 255); }
static const Color C_WHITE = rgb(255, 255, 255), C_BLUE = rgb(105, 142, 255), C_GREEN = rgb(0, 237, 149),
                   C_RED = rgb(255, 58, 93);

// ---- stan ekranu (jak zmienne pliku w oryginale; jeden ekran naraz) ----
static int32_t g_disp[4], rod_disp[3];
static uint32_t frame;
static struct { int32_t amt; uint32_t tick, blocks, quiet, t; uint32_t lfsr; uint8_t init; } sk;
static uint32_t guiTick;
static float tickFrac;
static int32_t ox[SK_NPART], oy[SK_NPART], rdx[4], rdy[4], dial_a[4], dial_x[4], gauge_a[4], ch_s, ch_c = 32767, kick;

static void rodTargets(int32_t rods, int32_t t[3]) {
    const int32_t x = rods * 3;
    for (int i = 0; i < 3; i++) t[i] = clampi(x - i * 32767, 0, 32767);
}
static int32_t skTarget(int32_t tmax, int crit, int32_t flare) {
    if (!crit) {
        const int32_t b = clampi(((tmax - 39322) * 5) >> 1, 0, 65536);
        return b ? 7864 + (int32_t)(((int64_t)b * 13107) >> 16) : 0;
    }
    const int32_t deep = clampi(tmax - T_ONE, 0, T_ONE);
    return clampi(36045 + ((deep * 29491) >> 16) + ((flare * 22938) >> 15), 0, SK_ONE);
}
static void skStep(uint32_t el, int32_t target, int hot) {
    if (el > SK_GAP && !hot) { sk.amt = 0; return; }
    if (el > 69) el = 69;
    const int32_t k = clampi((int32_t)el * (target > sk.amt ? 128 : 20), 0, 4096);
    sk.amt += (int32_t)(((int64_t)(target - sk.amt) * k) >> 12);
    if (!target && sk.amt < SK_REST) sk.amt = 0;
    sk.amt = clampi(sk.amt, 0, SK_ONE);
}
static int32_t jolt() {
    sk.lfsr ^= sk.lfsr << 13; sk.lfsr ^= sk.lfsr >> 17; sk.lfsr ^= sk.lfsr << 5;
    return (int32_t)(sk.lfsr >> 16) - 32768;
}
static int32_t wobble(int k, int32_t hz) {
    const int32_t ph = (int32_t)(((uint32_t)(((uint64_t)sk.t * (uint32_t)hz * 1189u) >> 10) + (uint32_t)k * 1777u) & 8191u);
    return (tsin(ph) * 5 + jolt() * 3) >> 3;
}
static void skOffsets() {
    static const int16_t AX[SK_NPART] = {500, 700, 2600, 0, 2600, 0, 1200};
    static const int16_t AY[SK_NPART] = {400, 500, 1800, 0, 1600, 0, 1300};
    static const int16_t HZ[SK_NPART] = {1130, 1370, 870, 610, 1010, 1730, 1490};
    const int32_t a0 = sk.amt >> 1;
    const int32_t a = (a0 * (65536 - a0)) >> 15;
    if (a0 < 13107 && (frame % 3)) return;
    const int32_t ga = (a * (32768 + kick)) >> 15;
    int32_t gx = (ga * ((2500 * wobble(11, 790)) >> 15)) >> 15, gy = (ga * ((1600 * wobble(12, 930)) >> 15)) >> 15;
    if (a0 > 22938 && (sk.lfsr & 0x700) == 0) gx += (gx < 0 ? -1 : 1) * ((a * 5120) >> 15);
    for (int p = 0; p < SK_NPART; p++) {
        ox[p] = gx + ((a * ((AX[p] * wobble(p, HZ[p])) >> 15)) >> 15);
        const int32_t w = wobble(p + 20, HZ[p] + 230);
        oy[p] = gy + ((a * ((AY[p] * (p >= 5 ? (w < 0 ? -w : w) : w)) >> 15)) >> 15);
        oy[p] = clampi(oy[p], p >= 5 ? 0 : -1300, p == 0 ? 900 : 1300);
    }
    ox[5] = ox[6]; oy[5] = oy[6];
    {
        const int32_t ang = (a * ((137 * wobble(3, HZ[3])) >> 15)) >> 15;
        ch_s = tsin(ang); ch_c = tcos(ang);
        ox[3] = ox[1]; oy[3] = oy[1];
    }
    for (int i = 0; i < 4; i++) {
        const int32_t j = jolt();
        rdx[i] = (a * ((500 * wobble(30 + i, 1610 + i * 170)) >> 15)) >> 15;
        rdy[i] = -((a * ((2000 * (j < 0 ? -j : j)) >> 15)) >> 15);
        dial_a[i] = (a * ((228 * wobble(40 + i, 1900 + i * 110)) >> 15)) >> 15;
        dial_x[i] = (a * ((500 * wobble(44 + i, 2100)) >> 15)) >> 15;
        if (frame & 1) gauge_a[i] = (a * ((137 * wobble(50 + i, 1300 + i * 140)) >> 15)) >> 15;
    }
}
static void skZero() {
    for (int p = 0; p < SK_NPART; p++) ox[p] = oy[p] = 0;
    for (int i = 0; i < 4; i++) rdx[i] = rdy[i] = dial_a[i] = dial_x[i] = gauge_a[i] = 0;
    ch_s = 0; ch_c = 32767;
}
static void skMove(int p, int32_t* x, int32_t* y) {
    if (p == 3 && ch_s) {
        const int32_t dx = *x - CH_X, dy = *y - CH_Y;
        *x = CH_X + (int32_t)(((int64_t)dx * ch_c - (int64_t)dy * ch_s) >> 15);
        *y = CH_Y + (int32_t)(((int64_t)dx * ch_s + (int64_t)dy * ch_c) >> 15);
    }
    *x += ox[p]; *y += oy[p];
}
static void ln(Canvas& c, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, Color col) {
    c.line(OX + x0 / 1024.f * S, OY + y0 / 1024.f * S, OX + x1 / 1024.f * S, OY + y1 / 1024.f * S, w / 1024.f * S * 0.8f, col);
}
static void segp(Canvas& c, int p, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, Color col) {
    skMove(p, &x0, &y0); skMove(p, &x1, &y1);
    ln(c, x0, y0, x1, y1, w, col);
}

// element SVG z przeksztalceniem grupy (Q10): czesci elektrowni (skMove), prety (przesuniecie), przycisk
static int32_t rodTx[4], rodTy[4];
static bool capVisible = true;
static void xform(int g, float x, float y, float* ox_, float* oy_) {
    int32_t X = (int32_t)lrintf(x * 1024.f), Y = (int32_t)lrintf(y * 1024.f);
    if (g >= G_S0 && g < G_ROD1) skMove(RS_GROUP_PART[g], &X, &Y);
    else if (g >= G_ROD1 && g < G_CAP) { X += rodTx[g - G_ROD1]; Y += rodTy[g - G_ROD1]; }
    else if (g == G_CAP) { X += ox[P_DESK]; Y += oy[P_DESK]; }
    *ox_ = OX + X / 1024.f * S;
    *oy_ = OY + Y / 1024.f * S;
}
static void drawItem(Canvas& c, const RXItem& it) {
    static float buf[2 * 512];
    if (it.kind == 0) {
        const RXFill& f = RX_FILLS[it.idx];
        if ((f.group == G_CAP && !capVisible) || f.count > 512 || f.group < -1) return;
        for (int i = 0; i < f.count; i++) xform(f.group, RX_PTS[(f.first + i) * 2], RX_PTS[(f.first + i) * 2 + 1], &buf[2 * i], &buf[2 * i + 1]);
        c.fillPolygon(buf, f.count, hexc(f.rgb));
    } else if (it.kind == 1) {
        const RXStroke& l = RX_STROKES[it.idx];
        if ((l.group == G_CAP && !capVisible) || l.group < -1) return;
        c.beginStroke();
        const int segs = l.closed ? l.count : l.count - 1;
        for (int i = 0; i < segs; i++) {
            const int a = l.first + i, b = l.first + (i + 1 == l.count ? 0 : i + 1);
            float x0, y0, x1, y1;
            xform(l.group, RX_PTS[a * 2], RX_PTS[a * 2 + 1], &x0, &y0);
            xform(l.group, RX_PTS[b * 2], RX_PTS[b * 2 + 1], &x1, &y1);
            c.strokeSeg(x0, y0, x1, y1, l.width * S * 0.8f);
        }
        c.endStroke(hexc(l.rgb));
    } else {
        const RXCircle& d = RX_CIRCLES[it.idx];
        if ((d.group == G_CAP && !capVisible) || d.group < -1) return;
        float x, y;
        xform(d.group, d.x, d.y, &x, &y);
        if (d.filled) c.fillCircle(x, y, (d.r + d.width * 0.5f) * S, hexc(d.rgb));
        else c.circle(x, y, d.r * S, d.width * S * 0.8f, hexc(d.rgb));
    }
}

// rysowanie sceny i elementow na zywo (k = galki, tmax/crit/clk/blink = z glosow)
static void drawScene(Canvas& c, const int32_t k[4], int32_t tmax, int blink) {
    int32_t top[4];
    top[0] = RG_ROD1_TOP;
    for (int i = 0; i < 3; i++) top[i + 1] = RG_ROD_IN - (int32_t)(((int64_t)(RG_ROD_IN - RG_ROD_OUT) * rod_disp[i]) >> 15);
    for (int i = 0; i < 4; i++) {
        rodTx[i] = ox[P_VESSEL] + rdx[i];
        rodTy[i] = top[i] - (i ? RG_ROD_IN : RG_ROD1_TOP) + oy[P_VESSEL] + rdy[i];
    }
    capVisible = !blink;
    for (const RXItem& it : RX_ORDER) drawItem(c, it);

    for (int i = 0; i < 4; i++) {  // trzonki pretow i poprzeczki T
        const int32_t x = RG_ROD_X[i] + rdx[i], y = top[i] - RG_T_GAP + rdy[i];
        segp(c, P_VESSEL, RG_ROD_X[i], RG_STEM_Y0, x, y, 0x3C0, C_WHITE);
        segp(c, P_VESSEL, x - RG_T_HALF, y, x + RG_T_HALF, y, 0x400, C_WHITE);
    }
    const Color kc[4] = {C_BLUE, C_GREEN, C_WHITE, C_RED};
    for (int i = 0; i < 4; i++) {  // wskazowki pokretel: -135..+135 stopni
        const int32_t a = ((k[i] * 6144) >> 15) - 3072 + dial_a[i], s = tsin(a), co = tcos(a), x = RG_DIAL_X[i] + dial_x[i];
        segp(c, P_DESK, x + ((RG_DIAL_R0 * s) >> 15), RG_DIAL_Y[i] - ((RG_DIAL_R0 * co) >> 15), x + ((RG_DIAL_R1 * s) >> 15),
             RG_DIAL_Y[i] - ((RG_DIAL_R1 * co) >> 15), RG_DIAL_W, kc[i]);
    }
    for (int i = 0; i < 4; i++) {  // luki i kreski wskaznikow
        int32_t px = RG_GAUGE_X[i] + ((RG_GAUGE_R * tcos(RG_ARC_A0)) >> 15), py = RG_GAUGE_Y[i] - ((RG_GAUGE_R * tsin(RG_ARC_A0)) >> 15);
        for (int j = 1; j <= 22; j++) {
            const int32_t a = RG_ARC_A0 + (RG_ARC_A1 - RG_ARC_A0) * j / 22;
            const int32_t x = RG_GAUGE_X[i] + ((RG_GAUGE_R * tcos(a)) >> 15), y = RG_GAUGE_Y[i] - ((RG_GAUGE_R * tsin(a)) >> 15);
            segp(c, P_DESK, px, py, x, y, 0x266, C_WHITE);
            px = x; py = y;
        }
        for (int j = 0; j < 5; j++) {
            const int32_t co = tcos(RG_TICK_A[j]), s = tsin(RG_TICK_A[j]), ri = RG_GAUGE_R - RG_GAUGE_TICK;
            segp(c, P_DESK, RG_GAUGE_X[i] + ((RG_GAUGE_R * co) >> 15), RG_GAUGE_Y[i] - ((RG_GAUGE_R * s) >> 15),
                 RG_GAUGE_X[i] + ((ri * co) >> 15), RG_GAUGE_Y[i] - ((ri * s) >> 15), 0x266, C_WHITE);
        }
    }
    for (int i = 0; i < 4; i++) {  // wskazowki wskaznikow
        const int32_t a = clampi(RG_GAUGE_A0 + (((RG_GAUGE_A1 - RG_GAUGE_A0) * g_disp[i]) >> 15) + gauge_a[i], RG_GAUGE_A1, RG_GAUGE_A0);
        segp(c, P_DESK, RG_GAUGE_X[i], RG_GAUGE_YMAX, RG_GAUGE_X[i] + ((RG_GAUGE_R1 * tcos(a)) >> 15),
             RG_GAUGE_Y[i] - ((RG_GAUGE_R1 * tsin(a)) >> 15), 0x500, kc[i]);
    }
    {  // para: wyzsza i szybsza, gdy goraco
        const int32_t h = clampi(tmax, 0, 3 * T_ONE / 2) * 2 / 3;
        const int32_t len = (RG_STEAM_LMAX >> 2) + (int32_t)(((int64_t)(RG_STEAM_LMAX - (RG_STEAM_LMAX >> 2)) * h) >> 16);
        const int32_t ph = (int32_t)(frame * (40 + (h >> 10)));
        for (int k2 = 0; k2 < 2; k2++) {
            int32_t px = RG_STEAM_X[k2], py = RG_STEAM_Y0;
            for (int j = 1; j <= 8; j++) {
                const int32_t y = RG_STEAM_Y0 - len * j / 8;
                const int32_t x = RG_STEAM_X[k2] + (((RG_STEAM_AMP * tsin(ph + j * 1100 + k2 * 4096)) >> 15) * j) / 8;
                segp(c, P_TOWER, px, py, x, y, 0x2C0, C_BLUE);
                px = x; py = y;
            }
        }
    }
    if (!blink)
        for (int i = 0; i < 2; i++) segp(c, 3, RG_BAND_X0, RG_BAND_Y[i], RG_BAND_X1, RG_BAND_Y[i], 0x480, C_RED);
    static const char* const names[4] = {"COOLING ", "RADIATION ", "RODS ", "HEAT "};
    for (int i = 0; i < 4; i++) {  // odczyty: nigdy sie nie trzesa
        char b[24];
        int v = (k[i] * 100 + 16384) >> 15;
        snprintf(b, sizeof b, "%s%d", names[i], v > 99 ? 99 : v);
        drawText(c, OX + RG_LABEL_X[i] / 1024.f * S, OY + RG_LABEL_Y / 1024.f * S, b, 4.8f * S, S * 0.6f, kc[i]);
    }
}

void ReactorScreen::draw(Canvas& c, const int16_t kn[4], const ReactorView& v, float dt) {
    sinInit();
    int32_t k[4];
    for (int i = 0; i < 4; i++) k[i] = kn[i] < 0 ? 0 : kn[i];
    frame++;
    tickFrac += dt * 689.06f;  // zegar GUI oryginalu: tyknięcie na blok audio
    const uint32_t dtk = (uint32_t)tickFrac;
    tickFrac -= dtk;
    guiTick += dtk;

    int32_t tmax = 0, clk = 0, crit = 0, flare = 0;
    const uint32_t now = v.blocks, tk = guiTick;
    const uint32_t el = sk.init ? tk - sk.tick : 0;
    if (el > SK_GAP) sk.quiet = now - sk.blocks < el ? SK_QUIET : 0;
    else sk.quiet = now != sk.blocks ? 0 : sk.quiet + el;
    sk.blocks = now; sk.tick = tk;
    if (!sk.init) { sk.lfsr = 0x2545F491u; sk.init = 1; }
    if (sk.quiet < SK_QUIET)
        for (int i = 0; i < 6; i++) {
            if (now - v.last[i] > 4 * 6) continue;
            if (v.temp[i] > tmax) tmax = v.temp[i];
            if (v.click[i] > clk) clk = v.click[i];
            crit |= v.crit[i];
            if (v.crit[i] && v.flare[i] > flare) flare = v.flare[i];
        }
    const int32_t target = skTarget(tmax, crit, flare);
    kick = (flare * 19661) >> 15;
    skStep(el, target, target > 0);
    sk.t += el;
    const int blink = crit && ((frame >> 3) & 1);

    int32_t g[4];
    g[0] = k[0];
    g[1] = ((k[1] * (13107 + ((clampi(tmax, 0, T_ONE) * 19660) >> 16))) >> 15) + (clk >> 1);
    g[2] = k[2];
    g[3] = ((tmax >> 2) * 13107) >> 13;
    for (int i = 0; i < 4; i++) {
        g[i] = clampi(g[i], 0, 32767);
        g_disp[i] += (g[i] - g_disp[i]) >> (i == 1 && g[i] > g_disp[i] ? 1 : 2);
    }
    int32_t rt[3];
    rodTargets(k[2], rt);
    for (int i = 0; i < 3; i++) rod_disp[i] += (rt[i] - rod_disp[i]) >> 2;
    if (sk.amt) skOffsets(); else skZero();
    drawScene(c, k, tmax, blink);
}

void drawReactorIllustration(Canvas& c, float x, float y, float w, float h) {
    sinInit();
    const float s0 = S, x0 = OX, y0 = OY;
    S = fminf(w / 320.f, h / 160.f);
    OX = x + (w - 320 * S) * 0.5f;
    OY = y + (h - 160 * S) * 0.5f;
    static const int32_t def[4] = {16384, 3000, 16384, 9000};
    int32_t saveG[4], saveR[3];
    for (int i = 0; i < 4; i++) { saveG[i] = g_disp[i]; g_disp[i] = def[i] == 3000 ? 2000 : def[i]; }
    int32_t rt[3];
    rodTargets(def[2], rt);
    for (int i = 0; i < 3; i++) { saveR[i] = rod_disp[i]; rod_disp[i] = rt[i]; }
    g_disp[3] = 6000;
    skZero();
    drawScene(c, def, 0, 0);
    for (int i = 0; i < 4; i++) g_disp[i] = saveG[i];
    for (int i = 0; i < 3; i++) rod_disp[i] = saveR[i];
    S = s0; OX = x0; OY = y0;
}

}  // namespace dp
