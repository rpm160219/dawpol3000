// Port czesci ekranowej monolit.c (op1r/monolit, Hundekraft). Arytmetyka i liczby jak w oryginale; rysowanie przez
// Canvas, napisy strony 2 nasza czcionka.
#include "ui/monolit_screen.h"
#include <math.h>
#include <stdio.h>
#include "gfx/font.h"

#include "../../third_party/op1r_monolit/monolit_art.h"

namespace dp {

static float S = 2.f, OX = 0.f, OY = 44.f;
#define QP(v) ((int32_t)((v) * 1024))
#define MO_HORIZON 104
#define MO_X 160
#define MO_FOOT 116
static const Color B_BLUE = rgb(0x69, 0x8E, 0xFF), B_GREEN = rgb(0x00, 0xED, 0x95), B_WHITE = rgb(255, 255, 255),
                   B_RED = rgb(0xFF, 0x3A, 0x5D);
static inline Color hexc(unsigned v) { return rgb((v >> 16) & 255, (v >> 8) & 255, v & 255); }

static int16_t SIN_T[8192];
static void sinInit() {
    if (SIN_T[2048]) return;
    for (int i = 0; i < 8192; i++) SIN_T[i] = (int16_t)lrint(32767.0 * sin(2 * M_PI * i / 8192.0));
}

static void art(Canvas& c) {
    static float buf[2 * 512];
    for (const MNItem& it : MN_ORDER) {
        if (it.kind == 0) {
            const MNFill& f = MN_FILLS[it.idx];
            if (f.group < -1 || f.count > 512) continue;
            for (int i = 0; i < f.count; i++) { buf[2 * i] = OX + MN_PTS[(f.first + i) * 2] * S; buf[2 * i + 1] = OY + MN_PTS[(f.first + i) * 2 + 1] * S; }
            c.fillPolygon(buf, f.count, hexc(f.rgb));
        } else if (it.kind == 1) {
            const MNStroke& l = MN_STROKES[it.idx];
            if (l.group < -1) continue;
            c.beginStroke();
            const int segs = l.closed ? l.count : l.count - 1;
            for (int i = 0; i < segs; i++) {
                const int a = l.first + i, b = l.first + (i + 1 == l.count ? 0 : i + 1);
                c.strokeSeg(OX + MN_PTS[a * 2] * S, OY + MN_PTS[a * 2 + 1] * S, OX + MN_PTS[b * 2] * S, OY + MN_PTS[b * 2 + 1] * S,
                            l.width * S * 0.8f);
            }
            c.endStroke(hexc(l.rgb));
        } else {
            const MNCircle& d = MN_CIRCLES[it.idx];
            if (d.group < -1) continue;
            if (d.filled) c.fillCircle(OX + d.x * S, OY + d.y * S, d.r * S, hexc(d.rgb));
            else c.circle(OX + d.x * S, OY + d.y * S, d.r * S, d.width * S * 0.8f, hexc(d.rgb));
        }
    }
}

static void ln(Canvas& c, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, Color col) {
    c.line(OX + x0 / 1024.f * S, OY + y0 / 1024.f * S, OX + x1 / 1024.f * S, OY + y1 / 1024.f * S, w / 1024.f * S * 0.8f, col);
}
// punkt (Q10) za monolitem (wierzcholek y 32, ramiona +-5 na y 42, stopa +-10 na y 116), z marginesem 2 px
static int behind(int32_t x, int32_t y) {
    x >>= 10; y >>= 10;
    if (y < 30 || y > MO_FOOT + 2) return 0;
    const int32_t d = y < 42 ? (y > 32 ? (y - 32) >> 1 : 0) : 5 + (y - 42) * 5 / 74;
    return x >= MO_X - d - 2 && x <= MO_X + d + 2;
}
static void seg(Canvas& c, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, Color col) {
    if (!behind(x0, y0) && !behind(x1, y1)) ln(c, x0, y0, x1, y1, w, col);
}

static void live(Canvas& c, const int32_t k[4], int32_t env, int32_t phase) {
    // niebieskie kregi wokol stopy (160, 116), splaszczone 7:1
    int n = 2 + ((k[0] * 6) >> 15);
    if (n > 7) n = 7;
    for (int i = 0; i < n; i++) {
        const int32_t rx = ((i * 32768 + phase) >> 5) * 140 / n;
        if (rx < QP(14)) continue;
        const int32_t ry = rx / 7;
        int32_t px = 0, py = 0;
        for (int j = 0; j <= 32; j++) {
            const int32_t t = j * 256;
            const int32_t x = QP(MO_X) + (((rx >> 5) * SIN_T[(t + 2048) & 8191]) >> 10);
            const int32_t y = QP(MO_FOOT) + (((ry >> 5) * SIN_T[t & 8191]) >> 10);
            if (j && py >= QP(MO_HORIZON + 2) && y >= QP(MO_HORIZON + 2)) seg(c, px, py, x, y, QP(1.5), B_BLUE);
            px = x; py = y;
        }
    }
    // zielony grzbiet; czubek czerwony z HEAT
    const int32_t w = 10 + ((k[1] * 56) >> 15), w2 = w * w * 64;
    const int32_t h = 72 + ((k[3] * 10) >> 15), tip = QP(2) + ((k[3] * 14) >> 5), top = QP(100 - h);
    for (int s = -1; s <= 1; s += 2) {
        int32_t px = 0, py = 0;
        for (int i = 0; i <= 36; i++) {
            const int32_t d8 = i * i * 146 * 8 / (36 * 36);
            const int32_t f = (w2 << 12) / (w2 + d8 * d8);
            const int32_t x = QP(MO_X) + s * (d8 << 7), y = QP(100) - (((h << 10) * f) >> 12);
            if (i) seg(c, px, py, x, y, QP(2.5), (py - top < tip && y - top < tip) ? B_RED : B_GREEN);
            px = x; py = y;
        }
    }
    // biala orbita CONTOUR i kropka
    if (k[2] >= 1000) {
        const int32_t span = 24 + ((k[2] * 110) >> 15), rise = 12 + ((k[2] * 70) >> 15);
        const int32_t x0 = QP(MO_X - span), cy = QP(100 - 2 * rise), x2 = QP(MO_X + span), y0 = QP(100);
        auto bez = [&](int32_t t, int32_t* ox, int32_t* oy) {
            const int32_t u = 4096 - t;
            *ox = ((u * u >> 12) * (x0 >> 4) + (2 * u * t >> 12) * (QP(MO_X) >> 4) + (t * t >> 12) * (x2 >> 4)) >> 8;
            *oy = ((u * u >> 12) * (y0 >> 4) + (2 * u * t >> 12) * (cy >> 4) + (t * t >> 12) * (y0 >> 4)) >> 8;
        };
        int32_t px = x0, py = y0;
        for (int i = 1; i <= 24; i++) {
            int32_t x, y;
            bez((i << 12) / 24, &x, &y);
            if (i & 1) seg(c, px, py, x, y, QP(1.5), B_WHITE);
            px = x; py = y;
        }
        if (env > 300) {
            int32_t x, y;
            bez(env >> 4, &x, &y);
            seg(c, x - QP(1), y, x + QP(1), y, QP(5), B_WHITE);
        }
    }
}

void MonolitScreen::draw(Canvas& c, const int16_t kn[4], const int16_t words[3], bool page2, int32_t env, bool on) {
    sinInit();
    if (!cache_) {
        cache_ = new Canvas();
        cache_->clear(pal::bg);
        art(*cache_);
    }
    int32_t k[4];
    for (int i = 0; i < 4; i++) k[i] = kn[i] < 0 ? 0 : kn[i];
    if (!on) env = 0;
    if (env < 0) env = 0;
    if (env > 2000) phase_ = (phase_ + (env >> 7)) & 32767;
    c.copyRows(*cache_, (int)OY, (int)(OY + 160 * S));
    live(c, k, env, phase_);
    if (page2) {  // BODY / VEL / AGE w prawym gornym rogu
        static const char* body[5] = {"SUB", "UNI", "5TH", "OCT", "SUB2"};
        static const char* name[3] = {"BODY", "VEL", "AGE"};
        const Color col[3] = {B_BLUE, B_GREEN, B_WHITE};
        for (int i = 0; i < 3; i++) {
            const int32_t v = words[i] < 0 ? 0 : words[i];
            char b[16];
            if (i == 0) { int s = (v * 5) >> 15; snprintf(b, sizeof b, "%s %s", name[i], body[s > 4 ? 4 : s]); }
            else snprintf(b, sizeof b, "%s %d", name[i], (v * 100 + 16383) >> 15);
            drawText(c, OX + 214 * S, OY + (14 + i * 14) * S, b, 9 * S * 0.6f, 1.3f, col[i]);
        }
    }
}

void drawMonolitIllustration(Canvas& c, float x, float y, float w, float h) {
    sinInit();
    const float s0 = S, x0 = OX, y0 = OY;
    S = fminf(w / 320.f, h / 160.f);
    OX = x + (w - 320 * S) * 0.5f;
    OY = y + (h - 160 * S) * 0.5f;
    art(c);
    static const int32_t def[4] = {16384, 14745, 11469, 9830};
    live(c, def, 0, 8000);
    S = s0; OX = x0; OY = y0;
}

}  // namespace dp
