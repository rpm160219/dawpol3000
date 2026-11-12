// Port czesci ekranowej mbira.c (op1r/mbira, Hundekraft). Arytmetyka i liczby jak w oryginale; rysowanie przez Canvas.
#include "ui/mbira_screen.h"
#include <math.h>

#include "../../third_party/op1r_mbira/mbira_art.h"
#include "../../third_party/op1r_mbira/mbira_geom.h"

namespace dp {

static float S = 2.f, OX = 0.f, OY = 44.f;  // ekran OP-1 -> nasz (ilustracja ustawia inne)
#define QP(v) ((int32_t)((v) * 1024))

static int16_t SIN_T[8192];
static void sinInit() {
    if (SIN_T[2048]) return;
    for (int i = 0; i < 8192; i++) SIN_T[i] = (int16_t)lrint(32767.0 * sin(2 * M_PI * i / 8192.0));
}
static inline Color hexc(unsigned v) { return rgb((v >> 16) & 255, (v >> 8) & 255, v & 255); }
static const Color M_COL[4] = {rgb(0x69, 0x8E, 0xFF), rgb(0x00, 0xED, 0x95), rgb(0xFF, 0xFF, 0xFF), rgb(0xFF, 0x3A, 0x5D)};
static const int32_t M_ROW[4][2] = {{MB_ROW0_Y0, MB_ROW0_Y1}, {MB_ROW1_Y0, MB_ROW1_Y1}, {MB_ROW2_Y0, MB_ROW2_Y1},
                                    {MB_ROW3_Y0, MB_ROW3_Y1}};

static void ln(Canvas& c, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, Color col) {
    c.line(OX + x0 / 1024.f * S, OY + y0 / 1024.f * S, OX + x1 / 1024.f * S, OY + y1 / 1024.f * S, w / 1024.f * S * 0.8f, col);
}
static void arcOf(Canvas& c, int32_t cx, int32_t cy, int32_t rad, int32_t a0, int32_t a1, int32_t w, Color col) {
    int32_t px = cx + ((rad * SIN_T[(a0 + 2048) & 8191]) >> 15), py = cy + ((rad * SIN_T[a0 & 8191]) >> 15);
    for (int32_t a = a0 + 512; a <= a1; a += 512) {
        const int32_t x = cx + ((rad * SIN_T[(a + 2048) & 8191]) >> 15), y = cy + ((rad * SIN_T[a & 8191]) >> 15);
        ln(c, px, py, x, y, w, col);
        px = x; py = y;
    }
}

// element SVG; vis[g] = widocznosc grupy (l0,p0,l1,p1,l2,p2)
static void drawItem(Canvas& c, const MBItem& it, const bool* vis) {
    if (it.kind == 1) {
        const MBStroke& l = MB_STROKES[it.idx];
        if (l.group >= 0 && !vis[l.group]) return;
        c.beginStroke();
        const int segs = l.closed ? l.count : l.count - 1;
        for (int i = 0; i < segs; i++) {
            const int a = l.first + i, b = l.first + (i + 1 == l.count ? 0 : i + 1);
            c.strokeSeg(OX + MB_PTS[a * 2] * S, OY + MB_PTS[a * 2 + 1] * S, OX + MB_PTS[b * 2] * S, OY + MB_PTS[b * 2 + 1] * S,
                        l.width * S * 0.8f);
        }
        c.endStroke(hexc(l.rgb));
    } else if (it.kind == 0) {
        const MBFill& f = MB_FILLS[it.idx];
        if (f.group >= 0 && !vis[f.group]) return;
        static float buf[2 * 1024];
        if (f.count > 1024) return;
        for (int i = 0; i < f.count; i++) { buf[2 * i] = OX + MB_PTS[(f.first + i) * 2] * S; buf[2 * i + 1] = OY + MB_PTS[(f.first + i) * 2 + 1] * S; }
        c.fillPolygon(buf, f.count, hexc(f.rgb));
    } else {
        const MBCircle& d = MB_CIRCLES[it.idx];
        if (d.group >= 0 && !vis[d.group]) return;
        if (d.filled) c.fillCircle(OX + d.x * S, OY + d.y * S, d.r * S, hexc(d.rgb));
        else c.circle(OX + d.x * S, OY + d.y * S, d.r * S, d.width * S * 0.8f, hexc(d.rgb));
    }
}
static int itemGroup(const MBItem& it) {
    return it.kind == 0 ? MB_FILLS[it.idx].group : it.kind == 1 ? MB_STROKES[it.idx].group : MB_CIRCLES[it.idx].group;
}

static void meters(Canvas& c, const int32_t v[4]) {
    for (int i = 0; i < 4; i++) {
        const int n = (v[i] * 12 + 16383) >> 15;
        for (int t = 0; t < n && t < 12; t++) {
            const int32_t x = MB_TICK_X0 + t * MB_TICK_DX;
            ln(c, x, M_ROW[i][0], x, M_ROW[i][1], 0x6CC, M_COL[i]);
        }
    }
}

void MbiraScreen::draw(Canvas& c, const int16_t kn[4], const int16_t words[3], bool page2, const MbiraView& view) {
    const int NI = (int)(sizeof MB_ORDER / sizeof MB_ORDER[0]);
    if (!cache_) {
        sinInit();
        cache_ = new Canvas();
        cache_->clear(pal::bg);
        const bool none[6] = {};
        while (firstLive_ < NI && itemGroup(MB_ORDER[firstLive_]) < 0) drawItem(*cache_, MB_ORDER[firstLive_++], none);
    }
    int32_t v[4];
    bool vis[6];
    for (int i = 0; i < 4; i++) {
        const bool p2 = i < 3 && page2;
        v[i] = p2 ? words[i] : kn[i];
        if (v[i] < 0) v[i] = 0;
        if (i < 3) { vis[2 * i] = !p2; vis[2 * i + 1] = p2; }
    }
    c.copyRows(*cache_, (int)OY, (int)(OY + 160 * S));
    for (int i = firstLive_; i < NI; i++) drawItem(c, MB_ORDER[i], vis);
    frame_++;
    meters(c, v);

    // najglosniejszy glos: drgania, kregi, kapsle
    int32_t loud = 0;
    for (int vo = 0; vo < 6; vo++) if (view.level[vo] > loud) loud = view.level[vo];
    if (loud < 256) return;
    const int32_t lv = loud > 32767 ? 32767 : loud;
    const int32_t sh = (SIN_T[(frame_ * 2600) & 8191] * QP(1)) >> 15;
    for (int a = 0; a < 2; a++) {
        const int32_t rad = QP(4) + a * QP(3) + ((lv * QP(3)) >> 15) + sh;
        arcOf(c, MB_TIP_X, MB_TIP_Y, rad, 6144 + 512, 8192 + 1024, 0x600, M_COL[2]);
    }
    const int32_t ph = (int32_t)(frame_ % 20);
    arcOf(c, MB_HOLE_X, MB_HOLE_Y, QP(6) + ph * QP(1), 2048 + 1024, 4096 + 2048, 0x500, M_COL[2]);
    const int ticks = (kn[3] < 0 ? 0 : kn[3]) * 6 >> 15;
    for (int cp = 0; cp < 2; cp++) {
        const int32_t cx = cp ? MB_CAP1_X : MB_CAP0_X, cy = cp ? MB_CAP1_Y : MB_CAP0_Y;
        for (int t = 0; t < ticks; t++) {
            rng_ = rng_ * 1664525u + 1013904223u;
            const int32_t a = (int32_t)(t * 1365 + (rng_ >> 22)) & 8191;
            const int32_t s = SIN_T[a], co = SIN_T[(a + 2048) & 8191];
            ln(c, cx + ((QP(11) >> 5) * co >> 10), cy + ((QP(11) >> 5) * s >> 10), cx + ((QP(14) >> 5) * co >> 10),
               cy + ((QP(14) >> 5) * s >> 10), 0x500, M_COL[3]);
        }
    }
}

void drawMbiraIllustration(Canvas& c, float x, float y, float w, float h) {
    sinInit();
    const float s0 = S, x0 = OX, y0 = OY;
    S = fminf(w / 320.f, h / 160.f);
    OX = x + (w - 320 * S) * 0.5f;
    OY = y + (h - 160 * S) * 0.5f;
    const bool vis[6] = {true, false, true, false, true, false};
    for (const MBItem& it : MB_ORDER) drawItem(c, it, vis);
    static const int32_t def[4] = {14000, 16384, 12000, 0};
    meters(c, def);
    S = s0; OX = x0; OY = y0;
}

}  // namespace dp
