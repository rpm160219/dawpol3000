// Rysowanie ekranow efektow op1r: rysunek z SVG (*_art.h, z grupami = elementy z id) + linie i napisy kodu C oryginalu.
// Grupy rysunku biora transformacje, widocznosc i kolor z wezlow, ktore ustawia kod ekranu (fx_screen_host.c).
#include "ui/fx_screen.h"
#include <chrono>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include "gfx/font.h"
#include "seq/project.h"
#include "ui/fx_screen_c.h"

#include "../../third_party/op1r_fx/juni_chorus/juni_chorus_art.h"
#include "../../third_party/op1r_fx/sister/sister_art.h"
#include "../../third_party/op1r_fx/snowglobe/snowglobe_art.h"
#include "../../third_party/op1r_fx/rackverb/rackverb_art.h"
#include "../../third_party/op1r_fx/tape/tape_art.h"
#include "../../third_party/op1r_fx/father/father_art.h"
#include "../../third_party/op1r_fx/stutter/stutter_art.h"
#include "../../third_party/op1r_fx/crush/crush_art.h"
#include "../../third_party/op1r_fx/ente/ente_art.h"

namespace dp {

namespace {
struct Ctx { Canvas* c; float ox, oy, s; int fx; };
inline Color hexc(unsigned v) { return rgb((v >> 16) & 255, (v >> 8) & 255, v & 255); }

void lineCb(void* u, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, const uint8_t* k) {
    const Ctx& x = *(const Ctx*)u;
    x.c->line(x.ox + x0 / 1024.f * x.s, x.oy + y0 / 1024.f * x.s, x.ox + x1 / 1024.f * x.s, x.oy + y1 / 1024.f * x.s,
              w / 1024.f * x.s * 0.8f, Color{k[0], k[1], k[2], 255});
}

// identyfikatory grup rysunkow (kolejnosc jak w poleceniu svg_to_art2.py)
const char* const G_SG[] = {"halo", "snow0", "snow1", "snow2", "shimmer", "smoke", "key", "base0", "bar0", "base1", "bar1", "base2", "bar2", "base3", "bar3"};
const char* const G_RV[] = {"logo0", "logo1", "logo2", "logo3"};
const char* const G_TP[] = {"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "pinch", "base0", "base1", "base2", "base3", "bar0", "bar1", "bar2", "bar3"};
const char* const G_FA[] = {"gate_person", "face"};
const char* const G_ST[] = {"reel"};

inline void xf(const int32_t* m, float px, float py, float& ox, float& oy) {
    if (!m) { ox = px; oy = py; return; }
    ox = (m[0] * px + m[3] * py) / 1024.f + m[6] / 1024.f;
    oy = (m[1] * px + m[4] * py) / 1024.f + m[7] / 1024.f;
}

template <class S, class F, class C, class I, size_t NS, size_t NF, size_t NC, size_t NI>
void drawArt(const Ctx& x, const float* P, const S (&st)[NS], const F (&fl)[NF], const C (&ci)[NC], const I (&ord)[NI],
             const char* const* groups) {
    static float buf[2 * 512];
    for (const I& it : ord) {
        int g;
        unsigned col;
        if (it.kind == 0) { g = fl[it.idx].group; col = fl[it.idx].rgb; }
        else if (it.kind == 1) { g = st[it.idx].group; col = st[it.idx].rgb; }
        else { g = ci[it.idx].group; col = ci[it.idx].rgb; }
        if (g < -1) continue;
        const FxNode* nd = g >= 0 && groups ? fxNodeFind(groups[g]) : nullptr;
        if (nd && !nd->visible) continue;
        const int32_t* m = nd && nd->hasM ? nd->m : nullptr;
        Color cc = hexc(col);
        if (nd && nd->colorSet) cc = Color{nd->rgba[0], nd->rgba[1], nd->rgba[2], 255};
        if (it.kind == 0) {
            const F& f = fl[it.idx];
            if (f.count > 512) continue;
            for (int i = 0; i < f.count; i++) {
                float px, py;
                xf(m, P[(f.first + i) * 2], P[(f.first + i) * 2 + 1], px, py);
                buf[2 * i] = x.ox + px * x.s; buf[2 * i + 1] = x.oy + py * x.s;
            }
            x.c->fillPolygon(buf, f.count, cc);
        } else if (it.kind == 1) {
            const S& l = st[it.idx];
            x.c->beginStroke();
            const int segs = l.closed ? l.count : l.count - 1;
            for (int i = 0; i < segs; i++) {
                const int a = l.first + i, b = l.first + (i + 1 == l.count ? 0 : i + 1);
                float ax, ay, bx, by;
                xf(m, P[a * 2], P[a * 2 + 1], ax, ay);
                xf(m, P[b * 2], P[b * 2 + 1], bx, by);
                x.c->strokeSeg(x.ox + ax * x.s, x.oy + ay * x.s, x.ox + bx * x.s, x.oy + by * x.s, l.width * x.s * 0.8f);
            }
            x.c->endStroke(cc);
        } else {
            const C& k = ci[it.idx];
            float cx, cy;
            xf(m, k.x, k.y, cx, cy);
            const float sc = m ? sqrtf((m[0] * m[0] + m[1] * m[1]) / (1024.f * 1024.f)) : 1.f;
            if (k.filled) x.c->fillCircle(x.ox + cx * x.s, x.oy + cy * x.s, k.r * sc * x.s, cc);
            if (!k.filled || k.width > 0.f) x.c->circle(x.ox + cx * x.s, x.oy + cy * x.s, k.r * sc * x.s, (k.width > 0.f ? k.width : 1.f) * x.s * 0.8f, cc);
        }
    }
}

// kolor poczatkowy wezla = kolor pierwszego elementu grupy w rysunku
template <class S, class F, class C, class I, size_t NS, size_t NF, size_t NC, size_t NI>
bool firstColor(const S (&st)[NS], const F (&fl)[NF], const C (&ci)[NC], const I (&ord)[NI], const char* const* groups, int ng,
                const char* id, uint8_t* rgba) {
    int gi = -1;
    for (int i = 0; i < ng; i++) if (!strcmp(groups[i], id)) gi = i;
    if (gi < 0) return false;
    for (const I& it : ord) {
        unsigned col; int g;
        if (it.kind == 0) { g = fl[it.idx].group; col = fl[it.idx].rgb; }
        else if (it.kind == 1) { g = st[it.idx].group; col = st[it.idx].rgb; }
        else { g = ci[it.idx].group; col = ci[it.idx].rgb; }
        if (g == gi) { rgba[0] = (col >> 16) & 255; rgba[1] = (col >> 8) & 255; rgba[2] = col & 255; rgba[3] = 255; return true; }
    }
    return false;
}
#define NG(a) (int)(sizeof(a) / sizeof(a[0]))

void colorCb(void* u, const char* id, uint8_t* rgba) {
    const Ctx& x = *(const Ctx*)u;
    switch (x.fx) {
    case FX_SNOWGLOBE: firstColor(SGA_STROKES, SGA_FILLS, SGA_CIRCLES, SGA_ORDER, G_SG, NG(G_SG), id, rgba); break;
    case FX_RACKVERB: firstColor(RVA_STROKES, RVA_FILLS, RVA_CIRCLES, RVA_ORDER, G_RV, NG(G_RV), id, rgba); break;
    case FX_TAPE: firstColor(TPA_STROKES, TPA_FILLS, TPA_CIRCLES, TPA_ORDER, G_TP, NG(G_TP), id, rgba); break;
    case FX_FATHER: firstColor(FAA_STROKES, FAA_FILLS, FAA_CIRCLES, FAA_ORDER, G_FA, NG(G_FA), id, rgba); break;
    case FX_STUTTER: firstColor(STA_STROKES, STA_FILLS, STA_CIRCLES, STA_ORDER, G_ST, NG(G_ST), id, rgba); break;
    default: break;
    }
}

void artCb(void* u) {
    const Ctx& x = *(const Ctx*)u;
    switch (x.fx) {
    case FX_JUNI_CHORUS: drawArt(x, JCA_PTS, JCA_STROKES, JCA_FILLS, JCA_CIRCLES, JCA_ORDER, nullptr); break;
    case FX_SISTER: drawArt(x, SIA_PTS, SIA_STROKES, SIA_FILLS, SIA_CIRCLES, SIA_ORDER, nullptr); break;
    case FX_SNOWGLOBE: drawArt(x, SGA_PTS, SGA_STROKES, SGA_FILLS, SGA_CIRCLES, SGA_ORDER, G_SG); break;
    case FX_RACKVERB: drawArt(x, RVA_PTS, RVA_STROKES, RVA_FILLS, RVA_CIRCLES, RVA_ORDER, G_RV); break;
    case FX_TAPE: drawArt(x, TPA_PTS, TPA_STROKES, TPA_FILLS, TPA_CIRCLES, TPA_ORDER, G_TP); break;
    case FX_FATHER: drawArt(x, FAA_PTS, FAA_STROKES, FAA_FILLS, FAA_CIRCLES, FAA_ORDER, G_FA); break;
    case FX_STUTTER: drawArt(x, STA_PTS, STA_STROKES, STA_FILLS, STA_CIRCLES, STA_ORDER, G_ST); break;
    case FX_CRUSH: drawArt(x, CRA_PTS, CRA_STROKES, CRA_FILLS, CRA_CIRCLES, CRA_ORDER, nullptr); break;
    case FX_ENTE: drawArt(x, ENA_PTS, ENA_STROKES, ENA_FILLS, ENA_CIRCLES, ENA_ORDER, nullptr); break;
    default: break;
    }
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
    if (fx <= FX_NONE || fx >= FX_COUNT) return;
    static int lastFx = -1;
    if (fx != lastFx) { reset = true; lastFx = fx; }  // inny efekt: ekran od nowa (wezly, stan animacji)
    if (reset) fxNodesReset();
    Ctx k = {&c, x, y, s, fx};
    const uint8_t page[4] = {(uint8_t)(page2 ? 1 : 0), (uint8_t)(page2 ? 1 : 0), (uint8_t)(page2 ? 1 : 0), (uint8_t)(w[4] != 0)};
    fxScreenSetColorFn(colorCb);
    fxScreenSetup(&k, lineCb, artCb, textCb, w, page);
    static const uint8_t zero[256] = {};
    const uint8_t* t = tel ? tel : zero;
    const int dt = frameMs();
    switch (fx) {
    case FX_JUNI_CHORUS: fxScreenJuniChorus(t, reset, dt); break;
    case FX_SISTER: fxScreenSister(t, reset, dt); break;
    case FX_SNOWGLOBE: fxScreenSnowglobe(t, reset, dt); break;
    case FX_RACKVERB: fxScreenRackverb(t, reset, dt); break;
    case FX_TAPE: fxScreenTape(t, reset, dt); break;
    case FX_FATHER: fxScreenFather(t, reset, dt); break;
    case FX_STUTTER: fxScreenStutter(t, reset, dt); break;
    case FX_CRUSH: fxScreenCrush(t, reset, dt); break;
    case FX_ENTE: fxScreenEnte(t, reset, dt); break;
    default: break;
    }
}

}  // namespace dp
