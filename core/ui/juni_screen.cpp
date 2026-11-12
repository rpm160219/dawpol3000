// Rysowanie ekranu JUNI: nieruchomy rysunek z juni.svg (juni_art.h) i linie z kodu C oryginalu.
#include "ui/juni_screen.h"
#include <chrono>
#include "synth/juni_dsp.h"

#include "../../third_party/op1r_juni/juni_art.h"

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
    for (const JNAStroke& l : JNA_STROKES) {
        if (l.group < -1) continue;
        x.c->beginStroke();
        const int segs = l.closed ? l.count : l.count - 1;
        for (int i = 0; i < segs; i++) {
            const int a = l.first + i, b = l.first + (i + 1 == l.count ? 0 : i + 1);
            x.c->strokeSeg(x.ox + JNA_PTS[a * 2] * x.s, x.oy + JNA_PTS[a * 2 + 1] * x.s, x.ox + JNA_PTS[b * 2] * x.s,
                           x.oy + JNA_PTS[b * 2 + 1] * x.s, l.width * x.s * 0.8f);
        }
        x.c->endStroke(hexc(l.rgb));
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

void JuniScreen::draw(Canvas& c, const int16_t words[8], const int16_t live[4], const ::JuniCore* core) {
    Ctx x = {&c, 0.f, 44.f, 2.f};
    juniScreenUpdate(&x, lineCb, artCb, core, words, live, frameMs());
}

void drawJuniIllustration(Canvas& c, float x, float y, float w, float h) {
    const float s = w / 320.f < h / 160.f ? w / 320.f : h / 160.f;
    Ctx k = {&c, x + (w - 320.f * s) * 0.5f, y + (h - 160.f * s) * 0.5f, s};
    static const int16_t kn[8] = {18000, 16384, 11000, 9800, 0, 0, 0, 0};
    juniScreenUpdate(&k, lineCb, artCb, nullptr, kn, kn, frameMs());
}

}  // namespace dp
