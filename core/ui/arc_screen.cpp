// Rysowanie ekranu ARC: uchwyty elektrod z duddell.svg (duddell_art.h; tarcze z dolu pominiete) i linie z kodu C.
#include "ui/arc_screen.h"
#include <chrono>
#include "synth/arc_dsp.h"

#include "../../third_party/op1r_arc/duddell_art.h"

namespace dp {

namespace {
struct Ctx { Canvas* c; float ox, oy, s; };
inline Color hexc(unsigned v) { return rgb((v >> 16) & 255, (v >> 8) & 255, v & 255); }
void lineCb(void* u, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, const uint8_t* k) {
    const Ctx& x = *(const Ctx*)u;
    x.c->line(x.ox + x0 / 1024.f * x.s, x.oy + y0 / 1024.f * x.s, x.ox + x1 / 1024.f * x.s, x.oy + y1 / 1024.f * x.s,
              w / 1024.f * x.s * 0.8f, Color{k[0], k[1], k[2], 255});
}
void artCb(void* u) {
    const Ctx& x = *(const Ctx*)u;
    for (const ARAStroke& l : ARA_STROKES) {
        if (l.group < -1 || ARA_PTS[l.first * 2 + 1] > 120.f) continue;  // tarcze z dolu oryginalu
        for (int i = 0; i + 1 < l.count; i++) {
            const int a = l.first + i, b = a + 1;
            x.c->line(x.ox + ARA_PTS[a * 2] * x.s, x.oy + ARA_PTS[a * 2 + 1] * x.s, x.ox + ARA_PTS[b * 2] * x.s,
                      x.oy + ARA_PTS[b * 2 + 1] * x.s, l.width * x.s * 0.8f, hexc(l.rgb));
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

void ArcScreen::draw(Canvas& c, const int16_t words[8], const int16_t live[4], const ::ArcCore* core, bool page2) {
    Ctx x = {&c, 0.f, 64.f, 2.f};  // rysunek y 0..~120 (bez tarcz)
    arcScreenUpdate(&x, lineCb, artCb, core, words, live, page2 ? 1 : 0, frameMs());
}

void drawArcIllustration(Canvas& c, float x, float y, float w, float h) {
    const float s = w / 320.f < h / 124.f ? w / 320.f : h / 124.f;
    Ctx k = {&c, x + (w - 320.f * s) * 0.5f, y + (h - 124.f * s) * 0.5f, s};
    static const int16_t kn[8] = {12000, 6000, 5000, 9000, 0, 0, 0, 0};
    arcScreenUpdate(&k, lineCb, artCb, nullptr, kn, kn, 0, frameMs());
}

}  // namespace dp
