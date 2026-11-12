// Rysowanie ekranu DDDD: linie z kodu C oryginalu (dddd.svg jest pusty).
#include "ui/dddd_screen.h"
#include <chrono>
#include "synth/dddd_dsp.h"

namespace dp {

namespace {
struct Ctx { Canvas* c; float ox, oy, s; };
void lineCb(void* u, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, const uint8_t* k) {
    const Ctx& x = *(const Ctx*)u;
    x.c->line(x.ox + x0 / 1024.f * x.s, x.oy + y0 / 1024.f * x.s, x.ox + x1 / 1024.f * x.s, x.oy + y1 / 1024.f * x.s,
              w / 1024.f * x.s * 0.8f, Color{k[0], k[1], k[2], 255});
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

void DdddScreen::draw(Canvas& c, const int16_t words[8], const int16_t live[4], const ::DdddCore* core, bool page2) {
    Ctx x = {&c, 0.f, 80.f, 2.f};  // osa wokol y 54 (bez dolnego rzedu)
    ddddScreenUpdate(&x, lineCb, core, words, live, page2 ? 1 : 0, frameMs());
}

void drawDdddIllustration(Canvas& c, float x, float y, float w, float h) {
    const float s = w / 320.f < h / 105.f ? w / 320.f : h / 105.f;
    Ctx k = {&c, x + (w - 320.f * s) * 0.5f, y + (h - 105.f * s) * 0.5f, s};
    static const int16_t kn[8] = {0, 24000, 20000, 8000, 0, 0, 0, 0};
    ddddScreenUpdate(&k, lineCb, nullptr, kn, kn, 0, frameMs());
}

}  // namespace dp
