// Rysowanie ekranu HOLLOW SAINT: rysunek z hollow_saint.svg (hollow_saint_art.h) z przeksztalceniami i widocznoscia
// wezlow ustawianymi przez kod C oryginalu.
#include "ui/hollowsaint_screen.h"
#include <chrono>
#include "synth/hollowsaint_dsp.h"

#include "../../third_party/op1r_hollowsaint/hollow_saint_art.h"

namespace dp {

namespace {
struct Ctx { Canvas* c; float ox, oy, s; };
inline Color hexc(unsigned v) { return rgb((v >> 16) & 255, (v >> 8) & 255, v & 255); }
inline void xf(const int32_t* m, float px, float py, float& ox, float& oy) {
    if (!m) { ox = px; oy = py; return; }
    ox = (m[0] * px + m[3] * py) / 1024.f + m[6] / 1024.f;
    oy = (m[1] * px + m[4] * py) / 1024.f + m[7] / 1024.f;
}
// grupy wskaznikow galek z dolu oryginalu (m0..m3, kbd, kdot, rage0, rage, r0..r6): pominiete
inline bool hiddenGroup(int g) { return (g >= 24 && g <= 29) || g == 33 || g == 34 || g >= 36; }
void artCb(void* u, const int32_t* const* m, const uint8_t* vis) {
    const Ctx& x = *(const Ctx*)u;
    Canvas& c = *x.c;
    static float buf[2 * 512];
    for (const HSAItem& it : HSA_ORDER) {
        int g;
        if (it.kind == 0) g = HSA_FILLS[it.idx].group;
        else if (it.kind == 1) g = HSA_STROKES[it.idx].group;
        else g = HSA_CIRCLES[it.idx].group;
        if (g < -1 || (g >= 0 && (!vis[g] || hiddenGroup(g)))) continue;
        const int32_t* mm = g >= 0 ? m[g] : nullptr;
        if (it.kind == 0) {
            const HSAFill& f = HSA_FILLS[it.idx];
            if (f.count > 512) continue;
            for (int i = 0; i < f.count; i++) {
                float px, py;
                xf(mm, HSA_PTS[(f.first + i) * 2], HSA_PTS[(f.first + i) * 2 + 1], px, py);
                buf[2 * i] = x.ox + px * x.s; buf[2 * i + 1] = x.oy + py * x.s;
            }
            c.fillPolygon(buf, f.count, hexc(f.rgb));
        } else if (it.kind == 1) {
            const HSAStroke& l = HSA_STROKES[it.idx];
            if (g < 0 && HSA_PTS[l.first * 2 + 1] > 121.f) continue;  // statyczne ramki wskaznikow z dolu oryginalu
            c.beginStroke();
            const int segs = l.closed ? l.count : l.count - 1;
            for (int i = 0; i < segs; i++) {
                const int a = l.first + i, b = l.first + (i + 1 == l.count ? 0 : i + 1);
                float ax, ay, bx, by;
                xf(mm, HSA_PTS[a * 2], HSA_PTS[a * 2 + 1], ax, ay);
                xf(mm, HSA_PTS[b * 2], HSA_PTS[b * 2 + 1], bx, by);
                c.strokeSeg(x.ox + ax * x.s, x.oy + ay * x.s, x.ox + bx * x.s, x.oy + by * x.s, l.width * x.s * 0.8f);
            }
            c.endStroke(hexc(l.rgb));
        } else {
            const HSACircle& d = HSA_CIRCLES[it.idx];
            if (g < 0 && d.y > 121.f) continue;
            float cx, cy;
            xf(mm, d.x, d.y, cx, cy);
            const float r = mm ? d.r * m[g][0] / 1024.f : d.r;  // skala wezla (aureola)
            if (d.filled) c.fillCircle(x.ox + cx * x.s, x.oy + cy * x.s, (r < 0 ? -r : r) * x.s, hexc(d.rgb));
            else c.circle(x.ox + cx * x.s, x.oy + cy * x.s, (r < 0 ? -r : r) * x.s, d.width * x.s * 0.8f, hexc(d.rgb));
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

void HollowScreen::draw(Canvas& c, const int16_t words[8], const int16_t live[4], const ::HollowCore* core, bool relic) {
    Ctx x = {&c, 0.f, 52.f, 2.f};  // rysunek y 3..121 (bez wskaznikow z dolu)
    hollowScreenUpdate(&x, artCb, core, words, live, relic ? 1 : 0, frameMs());
}

void drawHollowIllustration(Canvas& c, float x, float y, float w, float h) {
    const float s = w / 320.f < h / 122.f ? w / 320.f : h / 122.f;
    Ctx k = {&c, x + (w - 320.f * s) * 0.5f, y + (h - 122.f * s) * 0.5f, s};
    static const int16_t kn[8] = {16384, 16384, 12000, 12000, 0, 0, 8192, 21064};
    hollowScreenUpdate(&k, artCb, nullptr, kn, kn, 0, frameMs());
}

}  // namespace dp
