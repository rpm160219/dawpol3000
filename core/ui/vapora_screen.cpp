// Rysowanie ekranu VAPORA: funkcje zwrotne dla oryginalnego kodu C (linie Q10, tarcza slonca z SVG).
#include "ui/vapora_screen.h"
#include <chrono>
#include "ui/vapora_screen_c.h"

namespace dp {

namespace {
struct Ctx { Canvas* c; float ox, oy, s; };
void lineCb(void* u, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, const uint8_t* k) {
    const Ctx& x = *(const Ctx*)u;
    x.c->line(x.ox + x0 / 1024.f * x.s, x.oy + y0 / 1024.f * x.s, x.ox + x1 / 1024.f * x.s, x.oy + y1 / 1024.f * x.s,
              w / 1024.f * x.s * 0.8f, Color{k[0], k[1], k[2], 255});
}
void diskCb(void* u, int32_t cx, int32_t cy, int32_t r, const uint8_t* k) {
    const Ctx& x = *(const Ctx*)u;
    x.c->fillCircle(x.ox + cx / 1024.f * x.s, x.oy + cy / 1024.f * x.s, r / 1024.f * x.s, Color{k[0], k[1], k[2], 255});
}
// czas miedzy klatkami (oryginal: licznik cykli procesora); w trybie headless stale 16 ms
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

void VaporaScreen::draw(Canvas& c, const int16_t knobs[8], const int16_t live[4], const ::VaporaCore* core, int sun, int track) {
    Ctx x = {&c, 0.f, 44.f, 2.f};
    vaporaScreenUpdate(&x, lineCb, diskCb, core, knobs, live, sun, track, frameMs());
}

void drawVaporaIllustration(Canvas& c, float x, float y, float w, float h) {
    const float s = w / 320.f < h / 160.f ? w / 320.f : h / 160.f;
    Ctx k = {&c, x + (w - 320.f * s) * 0.5f, y + (h - 160.f * s) * 0.5f, s};
    static const int16_t kn[8] = {9000, 26000, 18500, 12500, 0, 8192, 8192, 8192};
    vaporaScreenUpdate(&k, lineCb, diskCb, nullptr, kn, kn, 0, -2, frameMs());
}

}  // namespace dp
