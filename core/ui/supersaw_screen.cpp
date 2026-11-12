// Rysowanie ekranu SUPERSAW: rysunek z supersaw.svg (supersaw_art.h) z przeksztalceniami wezlow (drganie "saw",
// obrot zebatki "sprk"; tarcze "bar0..3" pominiete — zdublowane z galkami DAW-POL) i linie z kodu C oryginalu (lancuch, uchwyt, korek, spust, dym).
#include "ui/supersaw_screen.h"
#include <chrono>
#include "synth/supersaw_dsp.h"

#include "../../third_party/op1r_supersaw/supersaw_art.h"

namespace dp {

namespace {
struct Ctx { Canvas* c; float ox, oy, s; };
inline Color hexc(unsigned v) { return rgb((v >> 16) & 255, (v >> 8) & 255, v & 255); }

void lineCb(void* u, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, const uint8_t* k) {
    const Ctx& x = *(const Ctx*)u;
    x.c->line(x.ox + x0 / 1024.f * x.s, x.oy + y0 / 1024.f * x.s, x.ox + x1 / 1024.f * x.s, x.oy + y1 / 1024.f * x.s,
              w / 1024.f * x.s * 0.8f, Color{k[0], k[1], k[2], 255});
}
// punkt rysunku przez macierz wezla (Q10, uklad jak op1_node_set_transform: x' = m0 x + m3 y + m6)
inline void xf(const int32_t* m, float px, float py, float& ox, float& oy) {
    if (!m) { ox = px; oy = py; return; }
    ox = (m[0] * px + m[3] * py) / 1024.f + m[6] / 1024.f;
    oy = (m[1] * px + m[4] * py) / 1024.f + m[7] / 1024.f;
}
void artCb(void* u, const int32_t* const* m) {
    const Ctx& x = *(const Ctx*)u;
    Canvas& c = *x.c;
    for (const SSItem& it : SS_ORDER) {
        if (it.kind == 1) {
            const SSStroke& l = SS_STROKES[it.idx];
            if (l.group < -1 || l.group >= 2 || SS_PTS[l.first * 2 + 1] > 110.f) continue;  // bez tarcz OP-1 (mamy swoje galki)
            const int32_t* mm = l.group >= 0 ? m[l.group] : nullptr;
            c.beginStroke();
            const int segs = l.closed ? l.count : l.count - 1;
            for (int i = 0; i < segs; i++) {
                const int a = l.first + i, b = l.first + (i + 1 == l.count ? 0 : i + 1);
                float ax, ay, bx, by;
                xf(mm, SS_PTS[a * 2], SS_PTS[a * 2 + 1], ax, ay);
                xf(mm, SS_PTS[b * 2], SS_PTS[b * 2 + 1], bx, by);
                c.strokeSeg(x.ox + ax * x.s, x.oy + ay * x.s, x.ox + bx * x.s, x.oy + by * x.s, l.width * x.s * 0.8f);
            }
            c.endStroke(hexc(l.rgb));
        } else if (it.kind == 2) {
            const SSCircle& d = SS_CIRCLES[it.idx];
            if (d.group < -1 || d.group >= 2 || d.y > 110.f) continue;  // bez tarcz OP-1
            float cx, cy;
            xf(d.group >= 0 ? m[d.group] : nullptr, d.x, d.y, cx, cy);
            if (d.filled) c.fillCircle(x.ox + cx * x.s, x.oy + cy * x.s, d.r * x.s, hexc(d.rgb));
            else c.circle(x.ox + cx * x.s, x.oy + cy * x.s, d.r * x.s, d.width * x.s * 0.8f, hexc(d.rgb));
        }
    }
}
// czas klatki: oryginal liczy ruch na klatke (~60 Hz), dt tylko przekazywany dalej
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

void SupersawScreen::draw(Canvas& c, const int16_t live[4], const ::SupersawCore* core) {
    Ctx x = {&c, 0.f, 80.f, 2.f};  // nizej niz w oryginale: bez rzedu tarcz pila wypelnia ekran (rysunek y 10..92)
    supersawScreenUpdate(&x, lineCb, artCb, core, live, frameMs());
}

void drawSupersawIllustration(Canvas& c, float x, float y, float w, float h) {
    const float s = w / 320.f < h / 110.f ? w / 320.f : h / 110.f;  // rysunek bez tarcz: y 0..110
    Ctx k = {&c, x + (w - 320.f * s) * 0.5f, y + (h - 110.f * s) * 0.5f, s};
    static const int16_t kn[4] = {8000, 0, 12000, 6000};
    supersawScreenUpdate(&k, lineCb, artCb, nullptr, kn, frameMs());
}

}  // namespace dp
