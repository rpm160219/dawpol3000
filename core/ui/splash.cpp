#include "ui/splash.h"
#include "gfx/font.h"
#include <math.h>

namespace dp {

static float ramp(float t, float a, float b) { return t <= a ? 0.f : (t >= b ? 1.f : (t - a) / (b - a)); }
static Color fade(Color c, float a) { return rgb((uint8_t)(c.r * a), (uint8_t)(c.g * a), (uint8_t)(c.b * a)); }

void drawSplash(Canvas& c, float t) {
    c.clear(pal::bg);
    const float out = 1.f - ramp(t, SPLASH_LEN - 0.4f, SPLASH_LEN);  // wygaszenie calosci
    // (SPLASH_LEN w splash.h)
    // fala: 4 odcinki w kolorach enkoderow (niebieski, zielony, bialy, czerwony), rysowana od lewej
    const float draw = ramp(t, 0.05f, 0.85f), x0 = 80, x1 = 560, y0 = 300;
    const int N = 160;
    for (int s = 0; s < 4; s++) {
        c.beginStroke();
        for (int i = s * N / 4; i < (s + 1) * N / 4; i++) {
            const float u0 = (float)i / N, u1 = (float)(i + 1) / N;
            if (u1 > draw) break;
            auto y = [&](float u) {
                const float env = sinf(3.14159f * u);  // obwiednia: cicho na koncach
                return y0 - env * 26.f * sinf(u * 6.2831853f * 4.f + t * 3.f) * (0.6f + 0.4f * sinf(u * 9.f));
            };
            c.strokeSeg(x0 + u0 * (x1 - x0), y(u0), x0 + u1 * (x1 - x0), y(u1), 2.f);
        }
        c.endStroke(fade(pal::enc[s], out));
    }
    // kropka "glowicy" na koncu rysowanej fali
    if (draw > 0 && draw < 1) c.fillCircle(x0 + draw * (x1 - x0), y0, 3.f, fade(pal::white, out));
    // napis
    const float a1 = ramp(t, 0.5f, 1.1f) * out;
    drawText(c, 320, 150, "DAW-POL", 54, 2.2f, fade(pal::white, a1), ALIGN_CENTER);
    const float a2 = ramp(t, 0.8f, 1.4f) * out;
    drawText(c, 320, 222, "3000", 30, 1.8f, fade(pal::tan, a2), ALIGN_CENTER);
    const float a3 = ramp(t, 1.1f, 1.7f) * out;
    drawText(c, 320, 360, "POCKET SKETCHBOOK DAW", 10, 1.3f, fade(rgb(150, 150, 150), a3), ALIGN_CENTER);
    for (int i = 0; i < 4; i++) c.fillCircle(296 + i * 16, 396, 3.5f, fade(pal::enc[i], a3));
    drawText(c, 320, 448, "ENGINES BY HUNDEKRAFT (OP1R)  -  GPL-3.0", 7, 1.f, fade(rgb(80, 80, 80), a3), ALIGN_CENTER);
}

}  // namespace dp
