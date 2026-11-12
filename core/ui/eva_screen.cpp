// Port czesci ekranowej eva.c (op1r/eva, Hundekraft). Arytmetyka i liczby jak w oryginale; rysowanie przez Canvas.
#include "ui/eva_screen.h"
#include <math.h>

#include "../../third_party/op1r_eva/eva_art.h"
#include "../../third_party/op1r_eva/eva_geom.h"

namespace dp {

static float S = 2.f, OX = 0.f, OY = 44.f;  // ekran OP-1 -> nasz (ilustracja ustawia inne)
#define ONE 1024
enum { G_STRIP, G_FOG, G_CROSS, G_M0, G_M1, G_M2, G_COMLABEL, G_OCTLABEL, G_M3, NG };

static int16_t SIN_T[8192];
static void sinInit() {
    if (SIN_T[2048]) return;
    for (int i = 0; i < 8192; i++) SIN_T[i] = (int16_t)lrint(32767.0 * sin(2 * M_PI * i / 8192.0));
}
static inline Color hexc(unsigned v) { return rgb((v >> 16) & 255, (v >> 8) & 255, v & 255); }

// przeksztalcenie grupy (x' = a x + tx, y' = d y + ty; tx/ty w Q10 px jak SVG_MATRIX), widocznosc
struct GroupXf { int32_t a = ONE, d = ONE, tx = 0, ty = 0; bool visible = true; };

static void drawItem(Canvas& c, const EVItem& it, const GroupXf* xf) {
    auto X = [](float x, int g, const GroupXf* f) { return OX + (g < 0 ? x : (x * f[g].a + f[g].tx) / 1024.f) * S; };
    auto Y = [](float y, int g, const GroupXf* f) { return OY + (g < 0 ? y : (y * f[g].d + f[g].ty) / 1024.f) * S; };
    static float buf[2 * 1024];
    if (it.kind == 0) {
        const EVFill& f = EV_FILLS[it.idx];
        if ((f.group >= 0 && !xf[f.group].visible) || f.count > 1024) return;
        for (int i = 0; i < f.count; i++) {
            buf[2 * i] = X(EV_PTS[(f.first + i) * 2], f.group, xf);
            buf[2 * i + 1] = Y(EV_PTS[(f.first + i) * 2 + 1], f.group, xf);
        }
        c.fillPolygon(buf, f.count, hexc(f.rgb));
    } else if (it.kind == 1) {
        const EVStroke& l = EV_STROKES[it.idx];
        if (l.group >= 0 && !xf[l.group].visible) return;
        if (l.rgb == 0x010101 && l.count == 2 && EV_PTS[l.first * 2 + 1] <= 2.f) return;  // ukryte paski iter (bar0..3)
        const float sc = l.group >= 0 ? xf[l.group].a / 1024.f : 1.f;
        c.beginStroke();
        const int segs = l.closed ? l.count : l.count - 1;
        for (int i = 0; i < segs; i++) {
            const int a = l.first + i, b = l.first + (i + 1 == l.count ? 0 : i + 1);
            c.strokeSeg(X(EV_PTS[a * 2], l.group, xf), Y(EV_PTS[a * 2 + 1], l.group, xf), X(EV_PTS[b * 2], l.group, xf),
                        Y(EV_PTS[b * 2 + 1], l.group, xf), l.width * S * 0.8f * (sc > 0.3f ? sc : 0.3f));
        }
        c.endStroke(hexc(l.rgb));
    } else {
        const EVCircle& d = EV_CIRCLES[it.idx];
        if (d.group >= 0 && !xf[d.group].visible) return;
        if (d.filled) c.fillCircle(X(d.x, d.group, xf), Y(d.y, d.group, xf), (d.r + d.width * 0.5f) * S, hexc(d.rgb));
        else c.circle(X(d.x, d.group, xf), Y(d.y, d.group, xf), d.r * S, d.width * S * 0.8f, hexc(d.rgb));
    }
}

static int itemGroup(const EVItem& it) {
    return it.kind == 0 ? EV_FILLS[it.idx].group : it.kind == 1 ? EV_STROKES[it.idx].group : EV_CIRCLES[it.idx].group;
}

void EvaScreen::draw(Canvas& c, const int16_t kn[4], int16_t k7, bool page2, const EvaView& v) {
    const int NI = (int)(sizeof EV_ORDER / sizeof EV_ORDER[0]);
    if (!cache_) {  // statyczny poczatek dokumentu (do pierwszego elementu grupy) raz do cache
        sinInit();
        cache_ = new Canvas();
        cache_->clear(pal::bg);
        GroupXf none[NG];
        while (firstLive_ < NI && itemGroup(EV_ORDER[firstLive_]) < 0) drawItem(*cache_, EV_ORDER[firstLive_++], none);
    }
    int32_t k[4];
    for (int i = 0; i < 4; i++) k[i] = kn[i] < 0 ? 0 : kn[i];
    const uint32_t blocks = v.blocks;
    const int on = blocks != lastBlocks_;
    lastBlocks_ = blocks;
    act_ += ((on ? ONE : 0) - act_) >> 3;
    frame_++;
    const int dropout = on && v.gate < 24000;
    const int flick = dropout || (on && k[3] > 4096 && (int32_t)(rnd() & 0x7FFF) < ((k[3] - 4096) >> 3));
    GroupXf xf[NG];
    static const int maskG[4] = {G_M0, G_M1, G_M2, G_M3};
    for (int i = 0; i < 4; i++) {
        int32_t n = (k[i] * 16 + 16383) >> 15;
        if (i == 3 && flick) n = dropout ? (int32_t)(rnd() % 3) : n - 1 - (int32_t)(rnd() % 3);
        if (n < 0) n = 0;
        xf[maskG[i]].tx = n >= 16 ? 200 * ONE : n * EVA_TICK_DX;
    }
    xf[G_CROSS].tx = k[1] * EVA_CROSS_SPAN / 32767;
    int32_t target;
    if (act_ > ONE / 2) target = v.breath >> 5;
    else {
        const int32_t s = SIN_T[(frame_ * 34) & 8191], inh = s > 0 ? s : 0;
        target = ((k[2] >> 5) * (128 + ((inh * 896) >> 15))) >> 10;
    }
    fog_ += (target - fog_) >> 2;
    const int32_t f = fog_ < 20 ? 20 : fog_ > ONE ? ONE : fog_, sxf = 154 + ((f * 870) >> 10);
    xf[G_FOG].a = sxf;
    xf[G_FOG].d = f;
    xf[G_FOG].tx = EVA_FOG_X - ((sxf * EVA_FOG_X) >> 10);
    xf[G_FOG].ty = EVA_FOG_Y - ((f * EVA_FOG_Y) >> 10);
    const int show = k[3] > 0 && !dropout && !(on && v.beep && (frame_ & 4)) && !(flick && (frame_ & 1));
    xf[G_STRIP].visible = show;
    // strona 2 COM: napis OCT, kreski = oktawa sygnalu (1 nizej, 8 oryginal, 15 wyzej)
    const unsigned bo = k7 <= 0 ? 1u : ((unsigned)k7 * 3u) >> 15;
    xf[G_COMLABEL].visible = !page2;
    xf[G_OCTLABEL].visible = page2;
    if (page2) xf[G_M3].tx = ((bo == 0 ? 1 : bo == 1 ? 8 : 15) * EVA_TICK_DX);

    c.copyRows(*cache_, (int)OY, (int)(OY + 160 * S));
    for (int i = firstLive_; i < NI; i++) drawItem(c, EV_ORDER[i], xf);
}

void drawEvaIllustration(Canvas& c, float x, float y, float w, float h) {
    sinInit();
    const float s0 = S, x0 = OX, y0 = OY;
    S = fminf(w / 320.f, h / 160.f);
    OX = x + (w - 320 * S) * 0.5f;
    OY = y + (h - 160 * S) * 0.5f;
    // galki domyslne (plugin.json): wskazniki, celownik, mgla w spoczynku, pasek radia widoczny (COM > 0)
    static const int32_t k[4] = {20000, 16384, 12000, 3000};
    GroupXf xf[NG];
    static const int maskG[4] = {G_M0, G_M1, G_M2, G_M3};
    for (int i = 0; i < 4; i++) { const int32_t n = (k[i] * 16 + 16383) >> 15; xf[maskG[i]].tx = n >= 16 ? 200 * ONE : n * EVA_TICK_DX; }
    xf[G_CROSS].tx = k[1] * EVA_CROSS_SPAN / 32767;
    const int32_t f = 200, sxf = 154 + ((f * 870) >> 10);
    xf[G_FOG].a = sxf; xf[G_FOG].d = f;
    xf[G_FOG].tx = EVA_FOG_X - ((sxf * EVA_FOG_X) >> 10);
    xf[G_FOG].ty = EVA_FOG_Y - ((f * EVA_FOG_Y) >> 10);
    xf[G_OCTLABEL].visible = false;
    for (const EVItem& it : EV_ORDER) drawItem(c, it, xf);
    // maski wskaznikow wysuwaja sie za scene (na ekranie poza krawedz): przyciecie do prostokata ilustracji
    const float right = OX + 320 * S;
    if (right < Canvas::W) c.fillRect(right, y, Canvas::W - right, h, pal::bg);
    S = s0; OX = x0; OY = y0;
}

}  // namespace dp
