// Ekran GIN: linie z gin_screen.c (oryginal) i napisy wlasna czcionka (gfx/font.cpp, zwezona do proporcji oryginalu).
#include "ui/ginscreen.h"
#include <chrono>
#include <string.h>
#include "gfx/font.h"
#include "synth/gin_dsp.h"

extern "C" {
typedef void (*GinLineFn)(void* user, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, const uint8_t* rgba);
struct GinUi* ginUiGet(void);
void ginUiLoad(struct GinUi* g, const GinSound* s, int kit, int selected, int mode, int page, int shift);
void ginScreenDraw(void* user, GinLineFn line_fn, const struct GinUi* g, uint32_t now, uint32_t hits, int dt);
extern int gin_dp_mode;
typedef struct { int32_t x, y; } op1_pt;
void gns_raster_line(void* r, const op1_pt* a, const op1_pt* b, int32_t w, const uint8_t* rgba);
}

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

// Czcionka DAW-POL w miejsce czcionki OP-1: wysokosc wielkiej litery = h, szerokosc zwezona (XS), zeby napisy
// miescily sie w polach ukladu oryginalu.
const float XS = 0.72f;
struct TextCtx { void* r; int32_t x, y; const int32_t* m; int32_t w; const uint8_t* c; float sx; };
void textSeg(void* u, float x0, float y0, float x1, float y1) {
    const TextCtx& t = *(const TextCtx*)u;
    op1_pt a = {t.x + (int32_t)(x0 * t.sx * 1024.f), t.y + (int32_t)(y0 * 1024.f)};
    op1_pt b = {t.x + (int32_t)(x1 * t.sx * 1024.f), t.y + (int32_t)(y1 * 1024.f)};
    if (t.m) {
        op1_pt p = {(int32_t)(((int64_t)a.x * t.m[0] + (int64_t)a.y * t.m[3]) / 1024) + t.m[6],
                    (int32_t)(((int64_t)a.x * t.m[1] + (int64_t)a.y * t.m[4]) / 1024) + t.m[7]};
        op1_pt q = {(int32_t)(((int64_t)b.x * t.m[0] + (int64_t)b.y * t.m[3]) / 1024) + t.m[6],
                    (int32_t)(((int64_t)b.x * t.m[1] + (int64_t)b.y * t.m[4]) / 1024) + t.m[7]};
        a = p; b = q;
    }
    gns_raster_line(t.r, &a, &b, t.w, t.c);
}
}  // namespace

uint32_t ginNowMs() {
    using clk = std::chrono::steady_clock;
    static const clk::time_point t0 = clk::now();
    return (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(clk::now() - t0).count();
}

void GinScreen::draw(Canvas& c, const struct ::GinUi* ui, uint32_t hits) {
    Ctx x = {&c, 0.f, 44.f, 2.f};
    ginScreenDraw(&x, lineCb, ui, ginNowMs(), hits, frameMs());
}

void drawGinIllustration(Canvas& c, float x, float y, float w, float h) {
    static GinSound s[24];
    static bool ready = false;
    if (!ready) { for (int i = 0; i < 24; i++) ginFactorySound(0, i, &s[i]); s[0].morph = 380; ready = true; }
    struct GinUi* g = ginUiGet();
    ginUiLoad(g, s, 0, 0, 0, 0, 0);
    const float sc = w / 320.f < h / 160.f ? w / 320.f : h / 160.f;
    Ctx k = {&c, x + (w - 320.f * sc) * 0.5f, y + (h - 160.f * sc) * 0.5f, sc};
    ginScreenDraw(&k, lineCb, g, ginNowMs(), 0, frameMs());
}

}  // namespace dp

// Napisy ekranu GIN (zamiast gin_text.c, ktory wymaga czcionki z firmware OP-1). Pomijane: odczyty galek trybu gry
// i stopka przyciskow OP-1 (wartosci sa na pasku galek DAW-POL, a przyciski inne).
extern "C" void gin_native_draw_text(void* r, const char* s, int32_t x, int32_t y, int32_t h, const uint8_t rgba[4],
                                     int align, int32_t weight, int32_t max, const int32_t m[9]) {
    if (!r || !s || !rgba || h <= 0 || weight <= 0) return;
    if (!m && (gin_dp_mode == 0 || y >= 130 * 1024)) return;
    float hp = h / 1024.f;
    float w = dp::textWidth(s, hp) * dp::XS * 1024.f;
    if (max > 0 && w > max) { hp *= max / w; w = (float)max; }
    if (align == 0) x -= (int32_t)(w / 2);
    else if (align > 0) x -= (int32_t)w;
    dp::TextCtx t = {r, x, y, m, weight, rgba, dp::XS};
    dp::textStrokes(s, hp, dp::textSeg, &t);
}

extern "C" int32_t gin_native_text_cursor(const char* s, unsigned at, int32_t h, int32_t max) {
    if (!s || h <= 0) return 0;
    char buf[48];
    unsigned n = 0;
    while (n < at && n < 47 && s[n]) { buf[n] = s[n]; n++; }
    buf[n] = 0;
    float hp = h / 1024.f;
    if (max > 0) {
        const float full = dp::textWidth(s, hp) * dp::XS * 1024.f;
        if (full > max) hp *= max / full;
    }
    float w = dp::textWidth(buf, hp) * dp::XS * 1024.f;
    if (n) w += 4.f * hp / 12.f * dp::XS * 1024.f;  // kursor za odstepem miedzy znakami
    return (int32_t)w;
}
