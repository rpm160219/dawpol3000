// Rysowanie ekranu OUTATIME: nieruchomy rysunek z outatime.svg (outatime_art.h) i linie z kodu C oryginalu.
#include "ui/outatime_screen.h"
#include <chrono>
#include "synth/outatime_dsp.h"

#include "../../third_party/kennel_outatime/plugins/outatime/outatime_art.h"

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
    Canvas& c = *x.c;
    static float buf[2 * 512];
    for (const OTAItem& it : OTA_ORDER) {
        if (it.kind == 0) {
            const OTAFill& f = OTA_FILLS[it.idx];
            if (f.group < -1 || f.count > 512) continue;
            for (int i = 0; i < f.count; i++) {
                buf[2 * i] = x.ox + OTA_PTS[(f.first + i) * 2] * x.s;
                buf[2 * i + 1] = x.oy + OTA_PTS[(f.first + i) * 2 + 1] * x.s;
            }
            c.fillPolygon(buf, f.count, hexc(f.rgb));
        } else if (it.kind == 1) {
            const OTAStroke& l = OTA_STROKES[it.idx];
            if (l.group < -1) continue;
            c.beginStroke();
            const int segs = l.closed ? l.count : l.count - 1;
            for (int i = 0; i < segs; i++) {
                const int a = l.first + i, b = l.first + (i + 1 == l.count ? 0 : i + 1);
                c.strokeSeg(x.ox + OTA_PTS[a * 2] * x.s, x.oy + OTA_PTS[a * 2 + 1] * x.s, x.ox + OTA_PTS[b * 2] * x.s,
                            x.oy + OTA_PTS[b * 2 + 1] * x.s, l.width * x.s * 0.8f);
            }
            c.endStroke(hexc(l.rgb));
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

void OutatimeScreen::draw(Canvas& c, const int16_t words[8], const int16_t live[4], const ::OutatimeCore* core,
                          bool page2) {
    Ctx x = {&c, 0.f, 44.f, 2.f};
    outatimeScreenUpdate(&x, lineCb, artCb, core, words, live, page2 ? 1 : 0, frameMs());
}

void drawOutatimeIllustration(Canvas& c, float x, float y, float w, float h) {
    const float s = w / 320.f < h / 160.f ? w / 320.f : h / 160.f;
    Ctx k = {&c, x + (w - 320.f * s) * 0.5f, y + (h - 160.f * s) * 0.5f, s};
    static const int16_t kn[8] = {16549, 6620, 8274, 11584, 0, 13239, 16549, 9929};
    outatimeScreenUpdate(&k, lineCb, artCb, nullptr, kn, kn, 0, frameMs());
}

}  // namespace dp
