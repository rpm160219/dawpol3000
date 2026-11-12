// Port sid_screen.c (op1r/sid, Hundekraft): rysunek z sid.svg + odczyty galek.
#include "ui/sid_screen.h"
#include <math.h>
#include <stdio.h>
#include "gfx/font.h"
#include "synth/sid_dsp_wrap.h"

#include "../../third_party/op1r_sid/sid_art.h"

namespace dp {

static float S = 2.f, OX = 0.f, OY = 44.f;
static inline Color hexc(unsigned v) { return rgb((v >> 16) & 255, (v >> 8) & 255, v & 255); }
static const Color COLORS[4] = {rgb(100, 151, 235), rgb(2, 209, 160), rgb(221, 221, 221), rgb(235, 69, 75)};

static void art(Canvas& c) {
    static float buf[2 * 1024];
    for (const SDItem& it : SD_ORDER) {
        if (it.kind == 0) {
            const SDFill& f = SD_FILLS[it.idx];
            if (f.group < -1 || f.count > 1024) continue;
            for (int i = 0; i < f.count; i++) { buf[2 * i] = OX + SD_PTS[(f.first + i) * 2] * S; buf[2 * i + 1] = OY + SD_PTS[(f.first + i) * 2 + 1] * S; }
            c.fillPolygon(buf, f.count, hexc(f.rgb));
        } else if (it.kind == 1) {
            const SDStroke& l = SD_STROKES[it.idx];
            if (l.group < -1) continue;
            c.beginStroke();
            const int segs = l.closed ? l.count : l.count - 1;
            for (int i = 0; i < segs; i++) {
                const int a = l.first + i, b = l.first + (i + 1 == l.count ? 0 : i + 1);
                c.strokeSeg(OX + SD_PTS[a * 2] * S, OY + SD_PTS[a * 2 + 1] * S, OX + SD_PTS[b * 2] * S, OY + SD_PTS[b * 2 + 1] * S,
                            l.width * S * 0.8f);
            }
            c.endStroke(hexc(l.rgb));
        }
    }
}

// odczyty: x = 238, wiersze co 14 px (oryginal: tekst przesuniety 4 px w gore)
static void readouts(Canvas& c, const int16_t q[4]) {
    static const char* names[6] = {"TRIANGLE", "SAW", "PULSE", "NOISE", "SYNC", "RING"};
    const unsigned mode = sidWaveMode(q[0]);
    char b[16];
    auto txt = [&](int y, const char* s, int col) { drawText(c, OX + 238 * S, OY + (y - 4 - 7) * S, s, 7 * S, S * 0.7f, COLORS[col]); };
    txt(18, "WAVE", 0);
    txt(32, names[mode], 0);
    txt(53, mode >= 4 ? "INTERVAL" : mode == 2 ? "WIDTH" : "VOICES", 1);
    if (mode >= 4) snprintf(b, sizeof b, "+%u ST", sidIntervalSemi(q[1]));
    else snprintf(b, sizeof b, "%u", mode == 2 ? 3 + (((unsigned)q[1] * 94 + 16383) >> 15) : (((unsigned)q[1] * 99 + 16383) >> 15));
    txt(67, b, 1);
    txt(88, "CUTOFF", 2);
    snprintf(b, sizeof b, "%u", ((unsigned)(q[2] < 0 ? 0 : q[2]) * 99 + 16383) >> 15);
    txt(102, b, 2);
    txt(123, "RES", 3);
    snprintf(b, sizeof b, "%u", ((unsigned)(q[3] < 0 ? 0 : q[3]) * 99 + 16383) >> 15);
    txt(137, b, 3);
}

void SidScreen::draw(Canvas& c, const int16_t k[4]) {
    if (!cache_) {
        cache_ = new Canvas();
        cache_->clear(pal::bg);
        art(*cache_);
    }
    c.copyRows(*cache_, (int)OY, (int)(OY + 160 * S));
    readouts(c, k);
}

void drawSidIllustration(Canvas& c, float x, float y, float w, float h) {
    const float s0 = S, x0 = OX, y0 = OY;
    S = fminf(w / 320.f, h / 160.f);
    OX = x + (w - 320 * S) * 0.5f;
    OY = y + (h - 160 * S) * 0.5f;
    art(c);
    static const int16_t def[4] = {6656, 16384, 32136, 8192};
    readouts(c, def);
    S = s0; OX = x0; OY = y0;
}

}  // namespace dp
