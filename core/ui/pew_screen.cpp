// Port screen.c z op1r/pew (Hundekraft). Arytmetyka i liczby jak w oryginale; rysowanie przez nasz Canvas.
#include "ui/pew_screen.h"
#include <math.h>
#include <string.h>

#include "../../third_party/op1r_pew/pew_art.h"
#include "../../third_party/op1r_pew/pew_geom.h"
#include "../../third_party/op1r_pew/pew_macro.h"

namespace dp {

static const float S = 2.f, OX = 0.f, OY = 44.f;  // ekran OP-1 -> nasz
static inline float PX(float u) { return OX + u * S; }
static inline float PY(float u) { return OY + u * S; }
static inline float LW(float units) { return units * S * 0.8f; }

#define ONE 1024
#define Q(v) ((int32_t)((v) * 1024))

static const Color C_BLUE = rgb(0x69, 0x8E, 0xFF), C_GREEN = rgb(0x00, 0xED, 0x95);
static const Color C_WHITE = rgb(0xFF, 0xFF, 0xFF), C_RED = rgb(0xFF, 0x3A, 0x5D), C_SMOKE2 = rgb(0x9A, 0x9A, 0xA4);
static inline Color hexc(unsigned v) { return rgb((v >> 16) & 255, (v >> 8) & 255, v & 255); }

static int16_t SIN_T[8192];
static void sinInit() {
    if (SIN_T[2048]) return;
    for (int i = 0; i < 8192; i++) SIN_T[i] = (int16_t)lrint(32767.0 * sin(2 * M_PI * i / 8192.0));
}
static inline int32_t SVG_SIN(uint32_t a) { return SIN_T[a & 8191]; }

// linia w Q10 (piksele ekranu OP-1)
static void line(Canvas& c, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, Color col) {
    c.line(PX(x0 / 1024.f), PY(y0 / 1024.f), PX(x1 / 1024.f), PY(y1 / 1024.f), LW(w / 1024.f), col);
}

// Elementy SVG: grupa -1 do cache; nuty (grupy 0..2) z macierza skali i przesuniecia (Q10) jak SVG_MATRIX.
static void drawArt(Canvas& c, int group, const int32_t* m) {
    auto X = [&](float x) { return PX(m ? (x * m[0] + m[6]) / 1024.f : x); };
    auto Y = [&](float y) { return PY(m ? (y * m[4] + m[7]) / 1024.f : y); };
    const float sc = m ? m[0] / 1024.f : 1.f;
    static float buf[2 * 512];
    for (const PWFill& f : PW_FILLS) {
        if (f.group != group || f.count > 512) continue;
        for (int i = 0; i < f.count; i++) {
            buf[2 * i] = X(PW_PTS[(f.first + i) * 2]);
            buf[2 * i + 1] = Y(PW_PTS[(f.first + i) * 2 + 1]);
        }
        c.fillPolygon(buf, f.count, hexc(f.rgb));
    }
    for (const PWStroke& l : PW_STROKES) {
        if (l.group != group) continue;
        c.beginStroke();
        const int segs = l.closed ? l.count : l.count - 1;
        for (int i = 0; i < segs; i++) {
            const int a = l.first + i, b = l.first + (i + 1 == l.count ? 0 : i + 1);
            c.strokeSeg(X(PW_PTS[a * 2]), Y(PW_PTS[a * 2 + 1]), X(PW_PTS[b * 2]), Y(PW_PTS[b * 2 + 1]), LW(l.width * sc));
        }
        if (l.count == 1) c.strokeSeg(X(PW_PTS[l.first * 2]), Y(PW_PTS[l.first * 2 + 1]), X(PW_PTS[l.first * 2]) + 0.01f,
                                      Y(PW_PTS[l.first * 2 + 1]), LW(l.width * sc));
        c.endStroke(hexc(l.rgb));
    }
    for (const PWCircle& d : PW_CIRCLES) {
        if (d.group != group) continue;
        if (d.filled) c.fillCircle(X(d.x), Y(d.y), d.r * S * sc, hexc(d.rgb));
        else c.circle(X(d.x), Y(d.y), d.r * S * sc, LW(d.width * sc), hexc(d.rgb));
    }
}

void PewScreen::buildCache() {
    sinInit();
    cache_ = new Canvas();
    cache_->clear(pal::bg);
    drawArt(*cache_, -1, nullptr);
}

void PewScreen::animate(const int16_t k[4], const int16_t pushIn[3], const PewView& v) {
    const uint32_t blocks = v.blocks;
    const int on = blocks != lastBlocks_;
    lastBlocks_ = blocks;
    act_ += ((on ? ONE : 0) - act_) >> 3;
    t_++;
    if (first_) { lastStrikes_ = v.strikes; first_ = false; }

    // suwaki zmierzaja do rejestracji
    int16_t push[3];
    for (int g = 0; g < 3; g++) push[g] = pushIn[g] < 0 ? 0 : pushIn[g];
    int32_t lv[9];
    pew_registration(k[0], push, lv);
    for (int i = 0; i < 9; i++) barShown_[i] += (lv[i] - barShown_[i]) >> 2;

    // nuty: kazde uderzenie wysyla kolejna nute z organow w gore
    const int32_t perc = k[1] < 0 ? 0 : k[1];
    int n = (int)(v.strikes - lastStrikes_);
    lastStrikes_ = v.strikes;
    if (n > 3) n = 3;
    while (n-- > 0) {
        noteP_[nextNote_] = 0;
        notePop_[nextNote_] = (perc * 600) >> 15;
        nextNote_ = nextNote_ == 2 ? 0 : nextNote_ + 1;
    }
    for (int i = 0; i < 3; i++) {
        const int32_t step = 16 + ((perc * 48) >> 15);
        noteP_[i] += step;
        if (noteP_[i] > ONE) noteP_[i] = ONE;
        notePop_[i] -= notePop_[i] >> 3;
        const int32_t u = ONE - noteP_[i];
        const int32_t ease = (u * u) >> 10;
        const int32_t dy = (ease * Q(20)) >> 10;
        const int32_t bob = (SVG_SIN(t_ * 64 + (uint32_t)i * 2730) * 410) >> 15;
        const int32_t s = ONE + notePop_[i] - (((ONE - noteP_[i]) * 300) >> 10);
        const int32_t cx = pew_note_home[i][0], cy = pew_note_home[i][1];
        noteM_[i][0] = s;
        noteM_[i][1] = cx - ((s * (cx >> 4)) >> 6);
        noteM_[i][2] = cy + dy + bob - ((s * (cy >> 4)) >> 6);
    }
}

void PewScreen::notes(Canvas& c) {
    for (int i = 0; i < 3; i++) {
        const int32_t m[9] = {noteM_[i][0], 0, 0, 0, noteM_[i][0], 0, noteM_[i][1], noteM_[i][2], 1024};
        drawArt(c, i, m);
    }
}

void PewScreen::bars(Canvas& c) {
    for (int m = 0; m < 2; m++)
        for (int i = 0; i < PEW_BAR_N; i++) {
            const Color col = i < 3 ? C_BLUE : i < 6 ? C_GREEN : C_WHITE;
            const int32_t x = pew_bar_foot[m][i][0], y = pew_bar_foot[m][i][1];
            const int32_t top = y - Q(1.6) - ((barShown_[i] * PEW_BAR_LEN) >> 15);
            if (top < y - Q(1.7)) line(c, x, y, x, top, PEW_BAR_W, col);
            line(c, x, top, x, top - Q(1.0), PEW_BAR_W + Q(0.7), col);
        }
}

void PewScreen::smoke(Canvas& c, int32_t white) {
    const int32_t amp = Q(0.5) + ((white * Q(2.2)) >> 15);
    const int32_t w = Q(0.9) + ((white * Q(1.0)) >> 15);
    for (int s = 0; s < (white > 16384 ? 2 : 1); s++) {
        int32_t px = 0, py = 0;
        for (int i = 0; i < PEW_SMOKE_N; i++) {
            const int32_t h = (i * (ONE * 65536 / PEW_SMOKE_N)) >> 16;
            const int32_t dx = (((SVG_SIN((uint32_t)i * 300 - t_ * 70 + (uint32_t)s * 1500) * amp) >> 15) * h) >> 10;
            const int32_t x = pew_smoke[i][0] + dx + (s ? Q(1.3) + ((h * Q(1.5)) >> 10) : 0);
            const int32_t y = pew_smoke[i][1];
            if (i && !(s && i < PEW_SMOKE_N / 4)) line(c, px, py, x, y, s ? w - Q(0.3) : w, s ? C_SMOKE2 : C_WHITE);
            px = x;
            py = y;
        }
    }
}

void PewScreen::lamp(Canvas& c, int32_t white) {
    static const uint8_t lo[3] = {0x76, 0x59, 0x32}, hi[3] = {0xFF, 0xE6, 0xB8};
    uint8_t cc[3];
    for (int k = 0; k < 3; k++) cc[k] = (uint8_t)(lo[k] + (((hi[k] - lo[k]) * white) >> 15));
    const Color col = rgb(cc[0], cc[1], cc[2]);
    const int32_t len = Q(1.0) + ((white * Q(5.0)) >> 15) + ((act_ * Q(0.4) >> 10) * SVG_SIN(t_ * 400) >> 15);
    const int32_t r0 = pew_bulb[2] + Q(1.2);
    for (int i = 0; i < 5; i++) {
        const uint32_t a = 2048 - 1300 + (uint32_t)i * 650;
        const int32_t cs = SVG_SIN(a + 2048), sn = SVG_SIN(a);
        line(c, pew_bulb[0] + ((cs * (r0 >> 5)) >> 10), pew_bulb[1] + ((sn * (r0 >> 5)) >> 10),
             pew_bulb[0] + ((cs * ((r0 + len) >> 5)) >> 10), pew_bulb[1] + ((sn * ((r0 + len) >> 5)) >> 10), Q(0.8), col);
    }
}

static void ringAt(int r, uint32_t a, int32_t rs, int32_t* x, int32_t* y) {
    const int32_t cs = SVG_SIN(a + 2048), sn = SVG_SIN(a);
    const int32_t u = (pew_ring[r][2] * cs) >> 15, v = (pew_ring[r][3] * sn) >> 15;
    const int32_t st = pew_ring[r][4], ct = pew_ring[r][5];
    *x = pew_ring[r][0] + ((((u * ct) >> 15) - ((v * st) >> 15)) * rs >> 10);
    *y = pew_ring[r][1] + ((((u * st) >> 15) + ((v * ct) >> 15)) * rs >> 10);
}

void PewScreen::rotor(Canvas& c, const PewView& v, int32_t red) {
    const int32_t sp = act_ > 64 ? (v.speed < 0 ? 0 : v.speed) : pew_leslie_target(red);
    int32_t f;
    if (sp <= PEW_SLOW) f = ((sp >> 12) * (int32_t)((358 << 16) / (PEW_SLOW >> 12))) >> 16;
    else f = 358 + ((((sp - PEW_SLOW) >> 12) * (int32_t)((666 << 16) / ((PEW_FAST - PEW_SLOW) >> 12))) >> 16);
    if (f > ONE) f = ONE;
    if (f < 16) return;
    for (int k = 0; k < PEW_ARC_N; k++) {
        const int r = pew_arc[k][0];
        const uint32_t turn = (r == 2 ? v.drum : v.horn) >> 19;
        const int32_t sw = (pew_arc[k][2] * f) >> 10;
        const uint32_t a0 = (uint32_t)pew_arc[k][1] + turn + (uint32_t)((pew_arc[k][2] - sw) >> 1);
        enum { N = 8 };
        int32_t px, py, x, y;
        ringAt(r, a0, pew_arc[k][3], &px, &py);
        for (int i = 1; i <= N; i++) {
            ringAt(r, a0 + (uint32_t)((sw * i) >> 3), pew_arc[k][3], &x, &y);
            line(c, px, py, x, y, Q(1.0), C_RED);
            px = x;
            py = y;
        }
    }
}

void PewScreen::keys(Canvas& c, const PewView& v) {
    for (int i = 0; i < 6; i++)
        if (v.held[i]) line(c, pew_pulse[i][0], pew_pulse[i][1], pew_pulse[i][2], pew_pulse[i][3], PEW_KEY_W, C_GREEN);
}

void PewScreen::draw(Canvas& c, const int16_t k[4], const int16_t push[3], const PewView& view) {
    if (!cache_) buildCache();
    animate(k, push, view);
    c.copyRows(*cache_, 0, Canvas::H);
    notes(c);
    const int32_t white = k[2] < 0 ? 0 : k[2];
    bars(c);
    keys(c, view);
    lamp(c, white);
    smoke(c, white);
    rotor(c, view, k[3] < 0 ? 0 : k[3]);
}

void pewRegistration(int32_t blue, const int16_t* push, int32_t out[9]) { pew_registration(blue, push, out); }

}  // namespace dp
