// Rysowanie ekranu BRASSOPHONE: rysunek z brass-handmade.svg (brass_art.h) z przeksztalceniami wezlow (tlumiki, suwak,
// balon, spinner) i linie z kodu C oryginalu (zielone linie suwaka).
#include "ui/brass_screen.h"
#include <chrono>
#include "synth/brass_dsp.h"

#include "../../third_party/op1r_brass/brass_art.h"

namespace dp {

namespace {
struct Ctx { Canvas* c; float ox, oy, s; };
inline Color hexc(unsigned v) { return rgb((v >> 16) & 255, (v >> 8) & 255, v & 255); }
void lineCb(void* u, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, const uint8_t* k) {
    const Ctx& x = *(const Ctx*)u;
    x.c->line(x.ox + x0 / 1024.f * x.s, x.oy + y0 / 1024.f * x.s, x.ox + x1 / 1024.f * x.s, x.oy + y1 / 1024.f * x.s,
              w / 1024.f * x.s * 0.8f, Color{k[0], k[1], k[2], 255});
}
inline void xf(const int32_t* m, float px, float py, float& ox, float& oy) {
    if (!m) { ox = px; oy = py; return; }
    ox = (m[0] * px + m[3] * py) / 1024.f + m[6] / 1024.f;
    oy = (m[1] * px + m[4] * py) / 1024.f + m[7] / 1024.f;
}
void artCb(void* u, const int32_t* const* m) {
    const Ctx& x = *(const Ctx*)u;
    Canvas& c = *x.c;
    static float buf[2 * 512];
    for (const BRItem& it : BR_ORDER) {
        if (it.kind == 0) {
            const BRFill& f = BR_FILLS[it.idx];
            if (f.group < -1 || f.count > 512) continue;
            const int32_t* mm = f.group >= 0 ? m[f.group] : nullptr;
            for (int i = 0; i < f.count; i++) {
                float px, py;
                xf(mm, BR_PTS[(f.first + i) * 2], BR_PTS[(f.first + i) * 2 + 1], px, py);
                buf[2 * i] = x.ox + px * x.s; buf[2 * i + 1] = x.oy + py * x.s;
            }
            c.fillPolygon(buf, f.count, hexc(f.rgb));
        } else if (it.kind == 1) {
            const BRStroke& l = BR_STROKES[it.idx];
            if (l.group < -1) continue;
            const int32_t* mm = l.group >= 0 ? m[l.group] : nullptr;
            c.beginStroke();
            const int segs = l.closed ? l.count : l.count - 1;
            for (int i = 0; i < segs; i++) {
                const int a = l.first + i, b = l.first + (i + 1 == l.count ? 0 : i + 1);
                float ax, ay, bx, by;
                xf(mm, BR_PTS[a * 2], BR_PTS[a * 2 + 1], ax, ay);
                xf(mm, BR_PTS[b * 2], BR_PTS[b * 2 + 1], bx, by);
                c.strokeSeg(x.ox + ax * x.s, x.oy + ay * x.s, x.ox + bx * x.s, x.oy + by * x.s, l.width * x.s * 0.8f);
            }
            c.endStroke(hexc(l.rgb));
        }
    }
}
int frameMs() {
    using clk = std::chrono::steady_clock;
    static clk::time_point last;
    static bool first = true;
    const clk::time_point now = clk::now();
    int ms = first ? 16 : (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count();
    first = false;
    last = now;
    return ms < 1 ? 1 : (ms > 100 ? 100 : ms);
}
}  // namespace

void BrassScreen::draw(Canvas& c, const int16_t live[4]) {
    Ctx x = {&c, 0.f, 44.f, 2.f};
    brassScreenUpdate(&x, lineCb, artCb, live, frameMs());
}

void drawBrassIllustration(Canvas& c, float x, float y, float w, float h) {
    const float s = w / 320.f < h / 145.f ? w / 320.f : h / 145.f;
    Ctx k = {&c, x + (w - 320.f * s) * 0.5f, y + (h - 145.f * s) * 0.5f, s};
    static const int16_t kn[4] = {19660, 16384, 3932, 7536};
    brassScreenUpdate(&k, lineCb, artCb, kn, frameMs());
}

}  // namespace dp
