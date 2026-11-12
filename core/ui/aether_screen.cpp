// Rysowanie ekranu AETHER: rysunek z aether.svg (aether_art.h) z przeksztalceniami wezlow (czasza, fale) i linie z kodu
// C oryginalu (sygnaly, gwiazdy). Grupy: 0 waves, 1..4 wave0..3 (przeksztalcenie "waves", wlasna widocznosc), 5 dish,
// dalej tarcze i suwaki (pominiete); statyczne prowadnice suwakow (0x333333) tez pominiete.
#include "ui/aether_screen.h"
#include <chrono>
#include "synth/aether_dsp.h"

#include "../../third_party/op1r_aether/aether_art.h"

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
void artCb(void* u, const int32_t* const* m, const uint8_t* vis) {
    const Ctx& x = *(const Ctx*)u;
    Canvas& c = *x.c;
    for (const AEAItem& it : AEA_ORDER) {
        if (it.kind != 1) continue;  // (same kreski)
        const AEAStroke& l = AEA_STROKES[it.idx];
        const int g = l.group;
        if (g < -1 || g >= 6 || (g < 0 && l.rgb == 0x333333)) continue;
        if (g >= 0 && !vis[g]) continue;
        const int32_t* mm = g < 0 ? nullptr : (g >= 1 && g <= 4 ? m[0] : m[g]);  // fale: przeksztalcenie grupy "waves"
        c.beginStroke();
        const int segs = l.closed ? l.count : l.count - 1;
        for (int i = 0; i < segs; i++) {
            const int a = l.first + i, b = l.first + (i + 1 == l.count ? 0 : i + 1);
            float ax, ay, bx, by;
            xf(mm, AEA_PTS[a * 2], AEA_PTS[a * 2 + 1], ax, ay);
            xf(mm, AEA_PTS[b * 2], AEA_PTS[b * 2 + 1], bx, by);
            c.strokeSeg(x.ox + ax * x.s, x.oy + ay * x.s, x.ox + bx * x.s, x.oy + by * x.s, l.width * x.s * 0.8f);
        }
        c.endStroke(hexc(l.rgb));
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

void AetherScreen::draw(Canvas& c, const int16_t words[8], const int16_t live[4], const ::AetherCore* core, bool page2) {
    Ctx x = {&c, 0.f, 40.f, 2.f};
    aetherScreenUpdate(&x, lineCb, artCb, core, words, live, page2 ? 1 : 0, frameMs());
}

void drawAetherIllustration(Canvas& c, float x, float y, float w, float h) {
    const float s = w / 320.f < h / 150.f ? w / 320.f : h / 150.f;
    Ctx k = {&c, x + (w - 320.f * s) * 0.5f, y + (h - 150.f * s) * 0.5f, s};
    static const int16_t kn[8] = {15671, 11469, 16384, 32767, 0, 0, 0, 0};
    aetherScreenUpdate(&k, lineCb, artCb, nullptr, kn, kn, 0, frameMs());
}

}  // namespace dp
