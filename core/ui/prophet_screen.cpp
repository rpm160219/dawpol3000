#include "ui/prophet_screen.h"
#include "gfx/font.h"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace dp {
namespace {
#include "ui/prophet_art.inc"
struct Art {
    Canvas canvas;
    Art() {
        canvas.clear(pal::bg);
        float xy[2 * kProphetMaxPoints];
        for (const auto &a : kProphetContours) {
            for (unsigned i = 0; i < a.count; i++) {
                xy[2 * i] = 20 + .25f * kProphetPoints[a.start + i][0];
                xy[2 * i + 1] = 5 + .25f * kProphetPoints[a.start + i][1];
            }
            canvas.fillPolygon(xy, a.count, a.color);
        }
    }
};
const Canvas &reference() {
    static Art art;
    return art.canvas;
}
void wavePlot(Canvas &c, const int16_t *p, int osc, float x, float y) {
    int bits = p[osc ? P5_BWAVE : P5_AWAVE] * (osc ? 8 : 4) / 32768;
    float pw = .01f + .98f * p[osc ? P5_BPW : P5_APW] / 32767.f;
    float xy[260];
    for (int j = 0; j < 130; j++) {
        float t = std::fmod(2 * j / 129.f, 1.f), v = 0;
        if (bits & 1)
            v += 2 * t - 1;
        if (bits & (osc ? 4 : 2))
            v += t < pw ? 1 : -1;
        if (osc && (bits & 2))
            v += 1 - 4 * std::fabs(t - .5f);
        xy[j * 2] = x + j * 100 / 129.f;
        xy[j * 2 + 1] = y - v * 8;
    }
    c.line(x, y, x + 100, y, 1, pal::faint);
    c.polyline(xy, 130, 1.15f, osc ? pal::green : pal::blue);
}
} // namespace
void ProphetScreen::draw(Canvas &c, const int16_t *p, const ProphetView &v, float dt) {
    time_ += dt;
    const auto &art = reference();
    std::memcpy(c.pixels() + 44 * 640 * 4, art.pixels() + 44 * 640 * 4, 290 * 640 * 4);
    // Color changes operate on the source ink, preserving staff/hand geometry.
    for (int y = 129; y < 305; y++)
        for (int x = 178; x < 462; x++) {
            uint8_t *px = c.pixels() + 4 * (y * 640 + x);
            int osc = x < 220 ? 0 : x > 425 ? 1 : -1;
            if (osc < 0)
                continue;
            bool ink =
                osc ? px[1] > px[0] + 12 && px[1] > px[2] * 1.2f : px[2] > px[0] + 12 && px[2] > px[1] * 1.2f;
            if (!ink)
                continue;
            float level = p[osc ? P5_MIXB : P5_MIXA] / 32767.f;
            int high = std::max({px[0], px[1], px[2]});
            for (int k = 0; k < 3; k++)
                px[k] = (uint8_t)(high + level * (px[k] - high));
        }
    c.line(20, 305, 620, 305, 1.5f, rgb(85, 69, 115));
    for (int osc = 0; osc < 2; osc++) {
        float x = osc ? 500 : 40;
        Color col = osc ? pal::green : pal::blue;
        drawText(c, x + 50, 170, osc ? "OSC B" : "OSC A", 10, 1.2f, col, ALIGN_CENTER);
        wavePlot(c, p, osc, x, 201);
        char text[24];
        prophetValue(osc ? P5_BFREQ : P5_AFREQ, p[osc ? P5_BFREQ : P5_AFREQ], text, sizeof(text));
        drawText(c, x + 50, 222, osc && p[P5_BLOW] >= 16384 ? "LO FREQ" : text, 8, 1, pal::silver,
                 ALIGN_CENTER);
    }
    // Actual audio and five actual VCA levels, never synthetic six-voice meters.
    float xy[256];
    for (int j = 0; j < 128; j++) {
        xy[2 * j] = 40 + j * 100 / 127.f;
        xy[2 * j + 1] = 264 - std::clamp(v.scope[j] * 60, -17.f, 17.f);
    }
    c.polyline(xy, 128, 1, pal::tan);
    for (int i = 0; i < 5; i++)
        c.fillCircle(62 + i * 14, 288, 2.5f,
                     rgb((uint8_t)(55 + 180 * v.state.amp[i]), (uint8_t)(48 + 155 * v.state.amp[i]),
                         (uint8_t)(38 + 135 * v.state.amp[i])));
    float l = v.state.lfo;
    drawText(c, 550, 253, "WHEEL MOD", 7, 1, pal::dim, ALIGN_CENTER);
    c.line(505, 275, 595, 275, 1, pal::faint);
    c.fillCircle(550 + 35 * std::sin(v.state.lfoPhase * 6.283185f), 275 - 8 * l, 2.5f, pal::red);
    // Subtle light follows the common filter modulation without redrawing the eye.
    float mod = p[P5_WFILTER] >= 16384 ? p[P5_WHEEL] / 32767.f : 0;
    if (mod > 0) {
        float a = .15f + .10f * l;
        c.circle(320, 101, 8, 1, rgb(245, 220, 190, (uint8_t)(255 * a * mod)));
    }
}
void drawProphetIllustration(Canvas &c, float x, float y, float w, float h) {
    const auto &a = reference();
    float scale = std::min(w / 330, h / 250);
    int dw = (int)(330 * scale), dh = (int)(250 * scale);
    x += (w - dw) * .5f;
    for (int j = 0; j < dh; j++)
        for (int i = 0; i < dw; i++) {
            int sx = std::clamp((int)(155 + i / scale), 0, 639),
                sy = std::clamp((int)(57 + j / scale), 0, 479);
            const uint8_t *p = a.pixels() + 4 * (sy * 640 + sx);
            c.fillRect(x + i, y + j, 1, 1, rgb(p[0], p[1], p[2]));
        }
}
} // namespace dp
