// Rysowanie ekranu BLOODMOON: rysunek z bloodmoon.svg (bloodmoon_art.h) z przeksztalceniami i widocznoscia wezlow
// ustawianymi przez kod C oryginalu.
#include "ui/bloodmoon_screen.h"
#include <chrono>
#include "synth/bloodmoon_dsp.h"

#include "../../third_party/op1r_bloodmoon/bloodmoon_art.h"

namespace dp {

namespace {
struct Ctx { Canvas* c; float ox, oy, s; };
inline Color hexc(unsigned v) { return rgb((v >> 16) & 255, (v >> 8) & 255, v & 255); }
inline void xf(const int32_t* m, float px, float py, float& ox, float& oy) {
    if (!m) { ox = px; oy = py; return; }
    ox = (m[0] * px + m[3] * py) / 1024.f + m[6] / 1024.f;
    oy = (m[1] * px + m[4] * py) / 1024.f + m[7] / 1024.f;
}
void artCb(void* u, const int32_t* const* m, const uint8_t* vis) {
    const Ctx& x = *(const Ctx*)u;
    Canvas& c = *x.c;
    static float buf[2 * 512];
    for (const BMAItem& it : BMA_ORDER) {
        int g;
        if (it.kind == 0) g = BMA_FILLS[it.idx].group;
        else if (it.kind == 1) g = BMA_STROKES[it.idx].group;
        else g = BMA_CIRCLES[it.idx].group;
        if (g < -1 || (g >= 0 && !vis[g])) continue;
        const int32_t* mm = g >= 0 ? m[g] : nullptr;
        if (it.kind == 0) {
            const BMAFill& f = BMA_FILLS[it.idx];
            if (f.count > 512) continue;
            for (int i = 0; i < f.count; i++) {
                float px, py;
                xf(mm, BMA_PTS[(f.first + i) * 2], BMA_PTS[(f.first + i) * 2 + 1], px, py);
                buf[2 * i] = x.ox + px * x.s; buf[2 * i + 1] = x.oy + py * x.s;
            }
            c.fillPolygon(buf, f.count, hexc(f.rgb));
        } else if (it.kind == 1) {
            const BMAStroke& l = BMA_STROKES[it.idx];
            c.beginStroke();
            const int segs = l.closed ? l.count : l.count - 1;
            for (int i = 0; i < segs; i++) {
                const int a = l.first + i, b = l.first + (i + 1 == l.count ? 0 : i + 1);
                float ax, ay, bx, by;
                xf(mm, BMA_PTS[a * 2], BMA_PTS[a * 2 + 1], ax, ay);
                xf(mm, BMA_PTS[b * 2], BMA_PTS[b * 2 + 1], bx, by);
                c.strokeSeg(x.ox + ax * x.s, x.oy + ay * x.s, x.ox + bx * x.s, x.oy + by * x.s, l.width * x.s * 0.8f);
            }
            c.endStroke(hexc(l.rgb));
        } else {
            const BMACircle& d = BMA_CIRCLES[it.idx];
            float cx, cy;
            xf(mm, d.x, d.y, cx, cy);
            const float r = d.r;
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

void BloodScreen::draw(Canvas& c, const int16_t live[4]) {
    Ctx x = {&c, 0.f, 44.f, 2.f};
    bloodScreenUpdate(&x, artCb, live, frameMs());
}

void drawBloodIllustration(Canvas& c, float x, float y, float w, float h) {
    const float s = w / 320.f < h / 160.f ? w / 320.f : h / 160.f;
    Ctx k = {&c, x + (w - 320.f * s) * 0.5f, y + (h - 160.f * s) * 0.5f, s};
    static const int16_t kn[4] = {14000, 20000, 7000, 9000};
    bloodScreenUpdate(&k, artCb, kn, frameMs());
}

}  // namespace dp
