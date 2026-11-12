// Port screen.c z op1r/elg (Hundekraft). Arytmetyka i liczby jak w oryginale; rysowanie przez nasz Canvas.
#include "ui/elg_screen.h"
#include <math.h>

#include "../../third_party/op1r_elg/elg_art.h"
#include "../../third_party/op1r_elg/elg_geom.h"

namespace dp {

static float S = 2.f, OX = 0.f, OY = 44.f;  // ekran OP-1 -> nasz (ilustracja ustawia inne)
static inline float PX(float u) { return OX + u * S; }
static inline float PY(float u) { return OY + u * S; }
static inline float LW(float units) { return units * S * 0.8f; }

#define ONE 1024
#define Q2Q10(v) ((int32_t)(v) << 8)  // cwierc piksela -> Q10

static const Color C_RED = rgb(0xFF, 0x3A, 0x5D), C_BLUE = rgb(0x69, 0x8E, 0xFF), C_BROWN = rgb(0xB0, 0x7A, 0x50),
                   C_LAV = rgb(0xDE, 0xD7, 0xFF), C_WHITE = rgb(0xFF, 0xFF, 0xFF);
static inline Color hexc(unsigned v) { return rgb((v >> 16) & 255, (v >> 8) & 255, v & 255); }

static int16_t SIN_T[8192];
static void sinInit() {
    if (SIN_T[2048]) return;
    for (int i = 0; i < 8192; i++) SIN_T[i] = (int16_t)lrint(32767.0 * sin(2 * M_PI * i / 8192.0));
}
static inline int32_t SVG_SIN(uint32_t a) { return SIN_T[a & 8191]; }

static void seg(Canvas& c, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, Color col) {
    c.line(PX(x0 / 1024.f), PY(y0 / 1024.f), PX(x1 / 1024.f), PY(y1 / 1024.f), LW(w / 1024.f), col);
}

// Elementy elg.svg danej grupy (-1 = statyczne); sc/px/py = skala wokol punktu (svg_scale), w pikselach
static void drawArt(Canvas& c, int group, float sc, float px, float py) {
    auto X = [&](float x) { return PX(px + (x - px) * sc); };
    auto Y = [&](float y) { return PY(py + (y - py) * sc); };
    static float buf[2 * 512];
    for (const ELFill& f : EL_FILLS) {
        if (f.group != group || f.count > 512 || f.rgb == 0x010101) continue;
        for (int i = 0; i < f.count; i++) {
            buf[2 * i] = X(EL_PTS[(f.first + i) * 2]);
            buf[2 * i + 1] = Y(EL_PTS[(f.first + i) * 2 + 1]);
        }
        c.fillPolygon(buf, f.count, hexc(f.rgb));
    }
    for (const ELStroke& l : EL_STROKES) {
        if (l.group != group || l.rgb == 0x010101) continue;
        c.beginStroke();
        const int segs = l.closed ? l.count : l.count - 1;
        for (int i = 0; i < segs; i++) {
            const int a = l.first + i, b = l.first + (i + 1 == l.count ? 0 : i + 1);
            c.strokeSeg(X(EL_PTS[a * 2]), Y(EL_PTS[a * 2 + 1]), X(EL_PTS[b * 2]), Y(EL_PTS[b * 2 + 1]), LW(l.width * sc));
        }
        c.endStroke(hexc(l.rgb));
    }
    for (const ELCircle& d : EL_CIRCLES) {
        if (d.group != group || d.rgb == 0x010101) continue;  // ukryte paski iter (base0..3)
        if (d.filled) c.fillCircle(X(d.x), Y(d.y), (d.r + d.width * 0.5f) * S * sc, hexc(d.rgb));
        else c.circle(X(d.x), Y(d.y), d.r * S * sc, LW(d.width * sc), hexc(d.rgb));
    }
}

void ElgScreen::animate(const int16_t set[4], const ElgView& v) {
    const uint32_t blocks = v.blocks;
    const int on = blocks != lastBlocks_;
    lastBlocks_ = blocks;
    act_ += ((on ? ONE : 0) - act_) >> 4;
    playing_ = act_ > 64;
    const int32_t level = on ? v.level : 0, lfo = on ? v.lfo : 0;
    auto K = [&](int i) -> int32_t { const int32_t x = playing_ ? v.knobs[i] : set[i]; return x < 0 ? 0 : x; };

    // poroze: rozchylenie ze stopniem interwalu (galka jak ustawiona, bez LFO), dochodzi jednobiegunowo
    const int32_t step = elgInterval(set[0] < 0 ? 0 : set[0]);
    const int32_t tx = ONE + (step - 2) * 36, ty = ONE + (step - 2) * 18;
    asx_ = asx_ ? asx_ + ((tx - asx_) >> 4) : tx;
    asy_ = asy_ ? asy_ + ((ty - asy_) >> 4) : ty;
    // oddech: 87,5 .. 137,5 % z rozstrojeniem, plus do 6 % z poziomem
    breath_ = 896 + ((K(1) * 512) >> 15) + ((level * act_) >> 19);
    // poswiata ksiezyca
    const int32_t t = K(2) - 20000;
    glow_ = (t > 0 ? (t * 246) >> 10 : 0) + ((level * act_) >> 16);
    if (glow_ > 3072) glow_ = 3072;
    // zorza
    const int32_t m = K(3);
    aph_ += 24 + ((m * 40) >> 15);
    const int32_t swing = 24576 + (lfo >> 2);
    aurAmp_ = (((m * 3072) >> 15) * swing) >> 15;
    aurStr_ = (((m * 461) >> 15) * swing) >> 15;
    tph_ += playing_ ? 96 : 0;
}

static void strokes(Canvas& c, int first, int last, Color col) {
    for (int s = first; s < last; s++) {
        const int16_t* p = EG_PTS + 2 * EG_STROKE[s][0];
        for (int i = 1; i < EG_STROKE[s][1]; i++, p += 2)
            seg(c, Q2Q10(p[0]), Q2Q10(p[1]), Q2Q10(p[2]), Q2Q10(p[3]), EG_STROKE[s][2], col);
    }
}

void ElgScreen::draw(Canvas& c, const int16_t set[4], const ElgView& v) {
    if (!cache_) {
        sinInit();
        cache_ = new Canvas();
        cache_->clear(pal::bg);
        drawArt(*cache_, -1, 1.f, 0.f, 0.f);
    }
    animate(set, v);
    c.copyRows(*cache_, (int)OY, (int)(OY + 160 * S));
    drawLive(c);
}

void ElgScreen::drawLive(Canvas& c) {
    // oblaczki oddechu: skala wokol nozdrzy (svg_scale)
    drawArt(c, 0, breath_ / 1024.f, EG_NOSE_LX / 4.f, EG_NOSE_Y / 4.f);
    drawArt(c, 1, breath_ / 1024.f, EG_NOSE_RX / 4.f, EG_NOSE_Y / 4.f);

    // poswiata ksiezyca: lukami 2,5 / 5 / 7,5 px na zewnatrz pierscienia
    for (int k = 0; k < 3; k++) {
        int32_t f = glow_ - k * 1024;
        if (f <= 0) break;
        if (f > ONE) f = ONE;
        const int32_t lvl = (f * (120 - 32 * k)) >> 10;
        const Color col = rgb((uint8_t)lvl, (uint8_t)lvl, (uint8_t)lvl);
        const int32_t r = Q2Q10(EG_MOON_R) + (5 * 512) * (k + 1);
        for (int a = 0; a < EG_NARCS; a++) {
            const int32_t a0 = EG_ARC[a][0] + 4 * (k + 1), a1 = EG_ARC[a][1] - 4 * (k + 1);
            if (a1 - a0 < 4) continue;
            int32_t px = 0, py = 0;
            for (int d = a0;; d += 6) {
                if (d > a1) d = a1;
                const uint32_t ang = (uint32_t)((d * 23302) >> 10) & 8191;
                const int32_t x = Q2Q10(EG_MOON_X) + ((r >> 5) * SVG_SIN(ang + 2048) >> 10);
                const int32_t y = Q2Q10(EG_MOON_Y) + ((r >> 5) * SVG_SIN(ang) >> 10);
                if (d != a0) seg(c, px, py, x, y, EG_W_WHITE, col);
                px = x, py = y;
                if (d == a1) break;
            }
        }
    }
    // zorza: wstegi opadaja falujac, fredzle sie rozciagaja
    auto drop = [&](int32_t xq2) -> int32_t {
        const int32_t cc = SVG_SIN(aph_ * 8 - (uint32_t)xq2 * 27 + 2048);
        return (aurAmp_ * (32767 - cc)) >> 16;
    };
    for (int s = EG_RIBBON_0; s < EG_RIBBON_1; s++) {
        const int16_t* p = EG_PTS + 2 * EG_STROKE[s][0];
        int32_t x0 = Q2Q10(p[0]), y0 = Q2Q10(p[1]) + drop(p[0]);
        for (int i = 1; i < EG_STROKE[s][1]; i++) {
            p += 2;
            const int32_t x1 = Q2Q10(p[0]), y1 = Q2Q10(p[1]) + drop(p[0]);
            seg(c, x0, y0, x1, y1, EG_W_RED, C_RED);
            x0 = x1, y0 = y1;
        }
    }
    for (int s = EG_FRINGE_0; s < EG_FRINGE_1; s++) {
        const int16_t* p = EG_PTS + 2 * EG_STROKE[s][0];
        const int32_t d = drop(p[0]);
        const int32_t st = ONE + ((aurStr_ * (32767 + SVG_SIN(aph_ * 5 + (uint32_t)p[0] * 19))) >> 16);
        const int32_t top = Q2Q10(p[1]);
        int32_t x0 = Q2Q10(p[0]), y0 = top + d;
        for (int i = 1; i < EG_STROKE[s][1]; i++) {
            const int16_t* q = p + 2 * i;
            const int32_t x1 = Q2Q10(q[0]), y1 = top + d + (((Q2Q10(q[1]) - top) * st) >> 10);
            seg(c, x0, y0, x1, y1, EG_W_RED, C_RED);
            x0 = x1, y0 = y1;
        }
    }
    // poroze
    const int32_t bx = Q2Q10(EG_ANT_X), by = Q2Q10(EG_ANT_Y);
    for (int s = EG_ANTLERS_0; s < EG_ANTLERS_1; s++) {
        const int16_t* p = EG_PTS + 2 * EG_STROKE[s][0];
        int32_t x0 = bx + ((Q2Q10(p[0]) - bx) * asx_ >> 10), y0 = by + ((Q2Q10(p[1]) - by) * asy_ >> 10);
        for (int i = 1; i < EG_STROKE[s][1]; i++) {
            p += 2;
            const int32_t x1 = bx + ((Q2Q10(p[0]) - bx) * asx_ >> 10), y1 = by + ((Q2Q10(p[1]) - by) * asy_ >> 10);
            seg(c, x0, y0, x1, y1, EG_STROKE[s][2], C_BLUE);
            x0 = x1, y0 = y1;
        }
    }
    strokes(c, EG_FUR_0, EG_FUR_1, C_BROWN);
    strokes(c, EG_FACE_LAV_0, EG_FACE_LAV_1, C_LAV);
    strokes(c, EG_FACE_WHITE_0, EG_FACE_WHITE_1, C_WHITE);
    // gwiazdy
    for (int i = 0; i < EG_NSTARS; i++) {
        const int32_t x = Q2Q10(EG_STAR[i][0]), y = Q2Q10(EG_STAR[i][1]);
        int32_t r = Q2Q10(EG_STAR[i][2]);
        if (playing_) r = (r * (26 + (SVG_SIN(tph_ * 3 + (uint32_t)i * 1500) >> 12))) >> 5;
        seg(c, x - r, y, x + r, y, EG_W_LAV, C_LAV);
        seg(c, x, y - r, x, y + r, EG_W_LAV, C_LAV);
        const int32_t h = r >> 1, g = r >> 3;
        seg(c, x - h, y, x + h, y, EG_W_LAV * 2, C_LAV);
        seg(c, x, y - h, x, y + h, EG_W_LAV * 2, C_LAV);
        seg(c, x - g, y, x + g, y, EG_W_LAV * 4, C_LAV);
    }
}

// Ilustracja na karte ENGINE: cala scena przy domyslnych galkach, wpisana w prostokat
void drawElgIllustration(Canvas& c, float x, float y, float w, float h) {
    sinInit();
    const float s0 = S, x0 = OX, y0 = OY;
    S = fminf(w / 320.f, h / 160.f);
    OX = x + (w - 320 * S) * 0.5f;
    OY = y + (h - 160 * S) * 0.5f;
    ElgScreen e;
    static const int16_t def[4] = {16384, 8192, 20000, 8192};
    ElgView v = {};
    e.animate(def, v);
    drawArt(c, -1, 1.f, 0.f, 0.f);
    e.drawLive(c);
    S = s0; OX = x0; OY = y0;
}

}  // namespace dp
