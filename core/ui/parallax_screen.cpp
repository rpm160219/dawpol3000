// Ekran PARALLAX. Wspolrzedne w ukladzie 752 x 344 (proporcje ekranu wzorca), skalowane do obszaru silnika.
#include "ui/parallax_screen.h"
#include <math.h>
#include <stdio.h>
#include "gfx/font.h"
#include "synth/parallax_dsp.h"

namespace dp {

namespace {
const Color kBlue = rgb(108, 140, 255), kGreen = rgb(0, 214, 140), kGrey = rgb(150, 150, 162), kRed = rgb(255, 58, 93);
const Color kTrace = rgb(178, 196, 255), kGlow = rgb(52, 66, 120), kGhost = rgb(120, 136, 196);
const Color kAxis = rgb(92, 92, 104), kIcon = rgb(74, 88, 136), kIconHi = rgb(150, 170, 235), kSel = rgb(26, 32, 58);

struct Tr {
    Canvas& c; float ox, oy, s;
    float X(float x) const { return ox + x * s; }
    float Y(float y) const { return oy + y * s; }
    void ln(float x0, float y0, float x1, float y1, float w, Color col) const { c.line(X(x0), Y(y0), X(x1), Y(y1), w * s, col); }
};

// ikona fali w wierszu kola (srodek wiersza cy, szerokosc 12..128)
void waveIcon(const Tr& t, int w, float cy, Color col, float alpha) {
    Color k = rgb((uint8_t)(col.r * alpha), (uint8_t)(col.g * alpha), (uint8_t)(col.b * alpha));
    const float x0 = 14, x1 = 126, top = cy - 11, bot = cy + 11;
    if (w == 4) {  // NOISE: pigulka z napisem
        t.c.strokeRoundRect(t.X(12), t.Y(cy - 12), 102 * t.s, 24 * t.s, 7 * t.s, 1.6f * t.s, k);
        drawText(t.c, t.X(63), t.Y(cy - 8), "NOISE", 16 * t.s, 1.5f * t.s, k, ALIGN_CENTER);
        return;
    }
    if (w == 0) {  // waskie impulsy
        for (int i = 0; i < 2; i++) {
            const float a = 28 + i * 52;
            t.ln(a - 14, bot, a, bot, 1.6f, k); t.ln(a, bot, a, top, 1.6f, k); t.ln(a, top, a + 12, top, 1.6f, k); t.ln(a + 12, top, a + 12, bot, 1.6f, k);
        }
        t.ln(104, bot, x1, bot, 1.6f, k);
        return;
    }
    if (w == 1) {  // kwadrat
        for (int i = 0; i < 4; i++) {
            const float a = x0 + i * 28, m = a + 14, b = a + 28;
            t.ln(a, i ? top : bot, a, top, 1.6f, k); t.ln(a, top, m, top, 1.6f, k); t.ln(m, top, m, bot, 1.6f, k); t.ln(m, bot, b, bot, 1.6f, k);
        }
        return;
    }
    // pila (2) i pila + SUB (3)
    const int teeth = w == 2 ? 4 : 2;
    const float span = (w == 2 ? x1 - x0 : 64.f) / teeth;
    for (int i = 0; i < teeth; i++) {
        const float a = x0 + i * span;
        t.ln(a, bot, a + span, top, 1.6f, k);
        t.ln(a + span, top, a + span, bot, 1.6f, k);
    }
    if (w == 3) {
        t.c.strokeRoundRect(t.X(82), t.Y(cy - 11), 46 * t.s, 22 * t.s, 7 * t.s, 1.6f * t.s, k);
        drawText(t.c, t.X(105), t.Y(cy - 7), "SUB", 14 * t.s, 1.4f * t.s, k, ALIGN_CENTER);
    }
}

void drawAll(Canvas& c, float ox, float oy, float s, const int16_t* kn, const float* main, const float* ghost, bool labels) {
    Tr t{c, ox, oy, s};
    char b[24];
    // nazwy galek
    if (labels) {
        const float ty = 10, ts = 18;
        drawText(c, t.X(4), t.Y(ty), "WAVEFORM", ts * s, 1.6f * s, kBlue);
        drawText(c, t.X(168), t.Y(ty), "UNISON", ts * s, 1.6f * s, kGreen);
        const float u = kn[1] / 32767.f;
        t.ln(300, 19, 362, 19, 1.3f, kGreen);
        t.ln(300 + 62 * u, 12, 300 + 62 * u, 26, 1.6f, kGreen);
        drawText(c, t.X(380), t.Y(ty), "FILTER FREQ", ts * s, 1.6f * s, kGrey);
        snprintf(b, sizeof b, "%d", (int)lrintf(parallaxCutoffHz(kn[2])));
        drawText(c, t.X(668), t.Y(ty), b, ts * s, 1.6f * s, rgb(235, 235, 245), ALIGN_RIGHT);
        drawText(c, t.X(746), t.Y(ty), "RES", ts * s, 1.6f * s, kRed, ALIGN_RIGHT);
    }
    // pole wykresu: osie, kreski podzialki, przerywane -1 i 1
    t.ln(150, 42, 150, 307, 1.2f, kAxis);
    t.ln(150, 307, 697, 307, 1.2f, kAxis);
    t.ln(150, 42, 697, 42, .8f, rgb(48, 48, 56));
    for (int i = 0; i < 5; i++) {
        const float y = 86 + i * 44;
        t.ln(140, y, 150, y, 1.f, kAxis);
        t.ln(687, y, 697, y, 1.f, kAxis);
    }
    for (int k = 0; k < 2; k++) {
        const float x = k ? 527 : 262;
        for (float y = 46; y < 300; y += 12) t.ln(x, y, x, y + 6, 1.f, kAxis);
    }
    drawText(c, t.X(262), t.Y(316), "-1", 18 * s, 1.4f * s, kGrey, ALIGN_CENTER);
    drawText(c, t.X(395), t.Y(316), "0", 18 * s, 1.4f * s, kGrey, ALIGN_CENTER);
    drawText(c, t.X(527), t.Y(316), "1", 18 * s, 1.4f * s, kGrey, ALIGN_CENTER);
    // kolo fal: biezaca pozycja na srodku (y 175), wiersze co 45, zawijane
    const float wp = kn[0] * 4.f / 32767.f;
    t.c.fillRoundRect(t.X(0), t.Y(153), 148 * s, 44 * s, 3 * s, kSel);
    t.c.strokeRoundRect(t.X(0), t.Y(153), 148 * s, 44 * s, 3 * s, 1.4f * s, kIcon);
    for (int r = -3; r <= 8; r++) {
        const float cy = 175 + (r - wp) * 45;
        if (cy < 52 || cy > 296) continue;
        const int w = ((r % 5) + 5) % 5;
        const float d = fabsf(cy - 175);
        const float a = d < 22 ? 1.f : (d > 120 ? .55f : 1.f - (d - 22) / 220.f);
        waveIcon(t, w, cy, d < 22 ? kIconHi : kIcon, a);
    }
    // oscyloskop XY: srodek (395, 175), +-1 = +-132
    const int N = 128, lag = N / 4;
    float pm[2 * 129], pg[2 * 129];
    for (int i = 0; i <= N; i++) {
        const int a = i % N, q = (i + lag) % N;
        pm[2 * i] = t.X(395 + main[a] * 132); pm[2 * i + 1] = t.Y(175 - main[q] * 120);
        const int qg = (i + lag + 3) % N;
        pg[2 * i] = t.X(395 + 14 + ghost[a] * 128); pg[2 * i + 1] = t.Y(175 - 6 - ghost[qg] * 118);
    }
    c.polyline(pg, N + 1, 1.1f * s, kGhost);
    c.polyline(pm, N + 1, 6.f * s, kGlow);
    c.polyline(pm, N + 1, 2.6f * s, kTrace);
    // szczyt rezonansu
    const float res = kn[3] / 32767.f, h = 6 + 70 * res, hw = 22 - 12 * res;
    float pk[2 * 26];
    for (int i = 0; i <= 24; i++) {
        const float u = (i - 12) / 12.f, e = expf(-u * u * (2.f + 8.f * res));
        pk[2 * i] = t.X(722 + u * hw); pk[2 * i + 1] = t.Y(305 - h * e);
    }
    pk[50] = t.X(722 + hw); pk[51] = t.Y(305);
    c.fillPolygon(pk, 26, kRed);
}
}  // namespace

void ParallaxScreen::draw(Canvas& c, const int16_t knobs[4], float dt) {
    anim_ += dt;
    bool ch = false;
    for (int i = 0; i < 4; i++) if (knobs[i] != last_[i]) { last_[i] = knobs[i]; ch = true; }
    const bool noisy = knobs[0] > 3 * 32767 / 4 + 1000;
    if (ch || noisy) parallaxScope(knobs, main_, 128, 0.f, 12345u + (uint32_t)(anim_ * 30));
    // cien: rozstrojony glos unison obraca sie powoli (szybciej przy wiekszym UNISON)
    parallaxScope(knobs, ghost_, 128, fmodf(anim_ * (0.05f + 0.6f * knobs[1] / 32767.f), 1.f), 777u + (uint32_t)(anim_ * 30));
    drawAll(c, 0.f, 44.f, 640.f / 752.f, knobs, main_, ghost_, true);
}

void drawParallaxIllustration(Canvas& c, float x, float y, float w, float h) {
    static const int16_t kn[4] = {12000, 12000, 22000, 14000};
    static float m[128], g[128];
    static bool ready = false;
    if (!ready) { parallaxScope(kn, m, 128, 0.f, 1u); parallaxScope(kn, g, 128, .03f, 2u); ready = true; }
    const float s = w / 752.f < h / 344.f ? w / 752.f : h / 344.f;
    drawAll(c, x + (w - 752 * s) * .5f, y + (h - 344 * s) * .5f, s, kn, m, g, false);
}

}  // namespace dp
