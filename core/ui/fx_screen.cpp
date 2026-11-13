// Rysowanie ekranow efektow op1r: rysunek z SVG (juni_chorus_art.h, sister_art.h) + linie kodu C oryginalu.
#include "ui/fx_screen.h"
#include <chrono>
#include <ctype.h>
#include "gfx/font.h"
#include "seq/project.h"
#include "ui/fx_screen_c.h"

#include "../../third_party/op1r_fx/juni_chorus/juni_chorus_art.h"
#include "../../third_party/op1r_fx/sister/sister_art.h"

namespace dp {

namespace {
struct Ctx { Canvas* c; float ox, oy, s; int fx; };
inline Color hexc(unsigned v) { return rgb((v >> 16) & 255, (v >> 8) & 255, v & 255); }

void lineCb(void* u, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, const uint8_t* k) {
    const Ctx& x = *(const Ctx*)u;
    x.c->line(x.ox + x0 / 1024.f * x.s, x.oy + y0 / 1024.f * x.s, x.ox + x1 / 1024.f * x.s, x.oy + y1 / 1024.f * x.s,
              w / 1024.f * x.s * 0.8f, Color{k[0], k[1], k[2], 255});
}

template <class S, class C, class I, size_t NS, size_t NC, size_t NI>
void drawArt(const Ctx& x, const float* P, const S (&st)[NS], const C (&ci)[NC], const I (&ord)[NI]) {
    for (const I& it : ord) {
        if (it.kind == 1) {
            const S& l = st[it.idx];
            if (l.group < -1) continue;
            x.c->beginStroke();
            const int segs = l.closed ? l.count : l.count - 1;
            for (int i = 0; i < segs; i++) {
                const int a = l.first + i, b = l.first + (i + 1 == l.count ? 0 : i + 1);
                x.c->strokeSeg(x.ox + P[a * 2] * x.s, x.oy + P[a * 2 + 1] * x.s, x.ox + P[b * 2] * x.s, x.oy + P[b * 2 + 1] * x.s,
                               l.width * x.s * 0.8f);
            }
            x.c->endStroke(hexc(l.rgb));
        } else if (it.kind == 2) {
            const C& k = ci[it.idx];
            if (k.group < -1) continue;
            if (k.filled) x.c->fillCircle(x.ox + k.x * x.s, x.oy + k.y * x.s, k.r * x.s, hexc(k.rgb));
            else x.c->circle(x.ox + k.x * x.s, x.oy + k.y * x.s, k.r * x.s, k.width * x.s * 0.8f, hexc(k.rgb));
        }
    }
}

void artCb(void* u) {
    const Ctx& x = *(const Ctx*)u;
    if (x.fx == FX_JUNI_CHORUS) drawArt(x, JCA_PTS, JCA_STROKES, JCA_CIRCLES, JCA_ORDER);
    else drawArt(x, SIA_PTS, SIA_STROKES, SIA_CIRCLES, SIA_ORDER);
}

// napisy OP-1 (y = gora, size = wysokosc czcionki) czcionka DAW-POL (tylko wielkie litery)
void textCb(void* u, int32_t xq, int32_t y, int32_t size, const char* s, const uint8_t* k) {
    const Ctx& x = *(const Ctx*)u;
    char b[32];
    int i = 0;
    for (; s[i] && i < 31; i++) b[i] = (char)toupper((unsigned char)s[i]);
    b[i] = 0;
    const float h = size * .55f;
    drawText(*x.c, x.ox + xq / 1024.f * x.s, x.oy + (y + size * .2f) * x.s, b, h * x.s, (size > 16 ? 1.f : .7f) * x.s,
             Color{k[0], k[1], k[2], 255});
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

void drawFxScreen(Canvas& c, int fx, const int16_t w[8], bool page2, const uint8_t* tel, bool reset, float x, float y, float s) {
    if (fx != FX_JUNI_CHORUS && fx != FX_SISTER) return;
    Ctx k = {&c, x, y, s, fx};
    const uint8_t page[4] = {0, (uint8_t)(page2 ? 1 : 0), 0, 0};
    fxScreenSetup(&k, lineCb, artCb, textCb, w, page);
    static const uint8_t zero[64] = {};
    const int dt = frameMs();
    if (fx == FX_JUNI_CHORUS) fxScreenJuniChorus(tel ? tel : zero, reset, dt);
    else fxScreenSister(tel ? tel : zero, reset, dt);
}

}  // namespace dp
