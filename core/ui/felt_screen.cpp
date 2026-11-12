// Ekran FELT: wspolrzedne w ukladzie 600 x 300, skalowane do obszaru silnika (jak LOUNGE).
#include "ui/felt_screen.h"
#include <math.h>
#include "gfx/font.h"
#include "synth/felt_dsp.h"

namespace dp {

namespace {
const Color kLine = rgb(226, 224, 255), kBody = rgb(74, 71, 112), kLabel = rgb(110, 106, 156), kWood = rgb(150, 120, 90);
const Color kBlue = rgb(92, 124, 255), kGreen = rgb(19, 224, 143), kRed = rgb(255, 61, 94), kFlame = rgb(255, 196, 92);

struct Tr {
    Canvas& c; float ox, oy, s;
    float X(float x) const { return ox + x * s; }
    float Y(float y) const { return oy + y * s; }
    void ln(float x0, float y0, float x1, float y1, float w, Color col) const { c.line(X(x0), Y(y0), X(x1), Y(y1), w * s, col); }
    void rect(float x0, float y0, float x1, float y1, float w, Color col) const {
        ln(x0, y0, x1, y0, w, col); ln(x1, y0, x1, y1, w, col); ln(x1, y1, x0, y1, w, col); ln(x0, y1, x0, y0, w, col);
    }
    void text(float x, float y, const char* s0, float size, Color col, int al = ALIGN_LEFT) const {
        drawText(c, X(x), Y(y), s0, size * s, 1.4f * s, col, al);
    }
};
inline Color dim(Color c, float a) { return rgb((uint8_t)(c.r * a), (uint8_t)(c.g * a), (uint8_t)(c.b * a)); }

void drawAll(const Tr& t, const int16_t* kn, float motion, float swing, float damper, float dust, float time, bool labels) {
    const float felt = kn[0] / 32767.f, corda = kn[1] / 32767.f, dec = kn[2] / 32767.f, noise = kn[3] / 32767.f;
    const int ns = feltStrings(kn[1]);
    // DECAY
    if (labels) t.text(30, 6, "DECAY", 13, kLabel);
    t.ln(30, 28, 30, 66, 1.4f, kLine);
    {
        float pts[2 * 25];
        const float tau = .1f + .9f * dec;
        for (int i = 0; i <= 24; i++) {
            const float u = i / 24.f;
            pts[2 * i] = t.X(32 + 70 * u);
            pts[2 * i + 1] = t.Y(28 + 36 * (1.f - expf(-u / tau)));
        }
        t.c.polyline(pts, 25, 2.f * t.s, kLine);
    }
    // obudowa z podniesiona pokrywa i podporka
    t.ln(30, 110, 570, 110, 2.f, kLine);
    t.ln(150, 108, 470, 34, 2.f, kLine);       // pokrywa
    t.ln(150, 112, 470, 38, .8f, kBody);
    t.ln(390, 110, 390, 53, 1.6f, kWood);      // podporka
    // swieca na obudowie
    {
        const float cx = 530;
        t.rect(cx - 7, 76, cx + 7, 108, 1.6f, kLine);
        t.ln(cx, 76, cx, 70, 1.2f, kLine);
        const float fl = sinf(time * 9.f) * 1.2f + sinf(time * 23.f) * .6f;
        const float fp[] = {t.X(cx), t.Y(52 + fl), t.X(cx + 5), t.Y(64), t.X(cx), t.Y(70), t.X(cx - 5), t.Y(64)};
        t.c.fillPolygon(fp, 4, kFlame);
        t.c.circle(t.X(cx), t.Y(62), 13.f * t.s, 1.f * t.s, dim(kFlame, .25f));
    }
    t.ln(30, 110, 30, 264, 1.4f, kBody);
    t.ln(570, 110, 570, 264, 1.4f, kBody);
    t.c.arc(t.X(42), t.Y(264), 12 * t.s, 1.5708f, 3.14159f, 1.4f * t.s, kBody);
    t.c.arc(t.X(558), t.Y(264), 12 * t.s, 0.f, 1.5708f, 1.4f * t.s, kBody);
    t.ln(42, 276, 558, 276, 1.4f, kBody);
    // napis CORDA (zielony)
    static const char* C[3] = {"UNA CORDA", "DUE CORDE", "TRE CORDE"};
    t.text(300, 120, C[ns - 1], 20, kGreen, ALIGN_CENTER);
    // kolki i mostek
    t.rect(50, 146, 76, 174, 1.6f, kWood);
    for (int i = 0; i < 3; i++) t.c.circle(t.X(56 + i * 7), t.Y(152 + i * 7), 2.2f * t.s, 1.2f * t.s, kWood);
    const float bp[] = {t.X(500), t.Y(166), t.X(512), t.Y(166), t.X(506), t.Y(152)};
    t.c.polyline(bp, 3, 1.6f * t.s, kWood, true);
    // plyta rezonansowa z zebrami
    {
        float pts[2 * 33];
        for (int i = 0; i <= 32; i++) {
            const float u = i / 32.f;
            pts[2 * i] = t.X(330 + 210 * u);
            pts[2 * i + 1] = t.Y(214 - 8 * sinf(3.14159f * u) - motion * 2.f * sinf(3.14159f * u) * sinf(time * 40.f));
        }
        t.c.polyline(pts, 33, 1.4f * t.s, kWood);
        t.ln(506, 166, 506, 206, 1.2f, kWood);
        for (int i = 0; i < 4; i++) t.ln(362 + i * 48, 216, 362 + i * 48, 224, 1.f, dim(kWood, .7f));
    }
    // struny
    const float ph = time * 46.f;
    for (int s = 0; s < 3; s++) {
        const float y = 158 + (s - 1) * 4.f;
        const bool on = s < ns;
        const Color col = on ? (s == 0 ? kLine : dim(kLine, s == 1 ? .75f : .6f)) : dim(kBody, .5f);
        float pts[2 * 41];
        for (int i = 0; i <= 40; i++) {
            const float u = i / 40.f;
            pts[2 * i] = t.X(76 + (500 - 76) * u);
            pts[2 * i + 1] = t.Y(y + (on ? 7.f * motion * sinf(3.14159f * u) * sinf(ph + s * 1.7f) : 0.f));
        }
        t.c.polyline(pts, 41, (on ? 1.6f : 1.f) * t.s, col);
    }
    // pasek filcu (moderator) miedzy mlotkiem a strunami
    if (felt > .08f) {
        const Color fc = dim(kBlue, .25f + .5f * felt);
        t.ln(200, 166, 340, 166, 3.5f * (.4f + felt), fc);
        for (int i = 0; i < 14; i++) t.ln(203 + i * 10, 163, 199 + i * 10, 169, .8f, dim(kBlue, .5f));
    }
    // mlotek: obrot wokol osi (px, py); swing 0 = filc przy strunach, 1 = spoczynek
    {
        const float px = 140, py = 238, R = 122;
        const int layers = 1 + (int)(felt * 4.f);
        const float top = 11.f + (layers - 1) * 3.f;             // wysokosc filcu nad srodkiem glowki
        const float yHit = 170 + top, yRest = 206 + top * .5f;    // srodek glowki: przy strunach / w spoczynku
        const float yc = yHit + (yRest - yHit) * swing;
        const float sa = (yc - py) / R;
        const float hx = px + R * sqrtf(1.f - sa * sa), hy = yc;
        t.c.circle(t.X(px), t.Y(py), 4.f * t.s, 1.6f * t.s, kBlue);
        t.ln(px, py, hx, hy + 12, 2.f, kBlue);                     // trzonek
        t.rect(hx - 9, hy, hx + 9, hy + 14, 2.f, kWood);           // drewniany rdzen
        for (int l = 0; l < layers; l++) {                          // warstwy filcu
            const float r = 11.f + l * 3.f;
            t.c.arc(t.X(hx), t.Y(hy + 2), r * t.s, 3.14159f * 1.08f, 3.14159f * 1.92f, 1.6f * t.s, l == 0 ? kBlue : dim(kBlue, .8f - l * .12f));
        }
        if (labels) t.text(hx + 26, hy + 8, "HAMMER", 12, dim(kBlue, .75f));
        if (labels && felt > .08f) t.text(344, 172, "FELT", 12, dim(kBlue, .75f));
    }
    // tlumik: unosi sie przy trzymanym klawiszu
    {
        const float dx = 420, dy = 138 - 14.f * damper;
        t.ln(dx, 112, dx, dy - 2, 1.4f, kLine);
        t.rect(dx - 14, dy - 2, dx + 14, dy + 12, 1.6f, kLine);
        for (int i = 0; i < 4; i++) t.ln(dx - 10 + i * 7, dy + 12, dx - 14 + i * 7, dy + 2, .8f, dim(kLine, .5f));
        if (labels) t.text(dx + 20, dy - 2, "DAMPER", 12, kLabel);
    }
    // klawisz na dzwigni
    {
        const float tilt = 6.f * damper;
        t.ln(40, 254 + tilt, 250, 254 - tilt * .6f, 2.4f, kLine);
        const float tp[] = {t.X(160), t.Y(255), t.X(166), t.Y(264), t.X(154), t.Y(264)};
        t.c.polyline(tp, 3, 1.4f * t.s, kLine, true);
        if (labels) t.text(48, 236, "KEY", 12, kLabel);
        if (labels) t.text(82, 126, "STRINGS", 12, dim(kGreen, .7f));
    }
    // pyl mechaniki
    if (noise > .02f && dust > .02f) {
        uint32_t r = 1234567u;
        const int nd = 4 + (int)(noise * 18.f);
        for (int i = 0; i < nd; i++) {
            r = r * 1664525u + 1013904223u;
            const float ax = 180 + (r >> 8) % 260, ay = 180 + ((r >> 4) % 50) - (1.f - dust) * 30.f;
            t.c.fillCircle(t.X(ax), t.Y(ay), 1.4f * t.s, dim(kRed, dust));
        }
    }
    (void)corda;
}
}  // namespace

void FeltScreen::draw(Canvas& c, const int16_t knobs[4], const ::FeltCore* core, float dt) {
    t_ += dt;
    const uint32_t st = feltStrikes(core);
    if (st != strikes_) { strikes_ = st; swing_ = 0; dust_ = 1; }
    swing_ += (1.f - swing_) * (dt * 7.f > 1.f ? 1.f : dt * 7.f);
    dust_ *= expf(-dt * 2.5f);
    const float m = feltMotion(core);
    motion_ += (m - motion_) * (dt * 20.f > 1.f ? 1.f : dt * 20.f);
    const float dmp = feltHeld(core) > 0 ? 1.f : 0.f;
    damper_ += (dmp - damper_) * (dt * 14.f > 1.f ? 1.f : dt * 14.f);
    const float s = 290.f / 300.f;
    Tr t{c, (640.f - 600.f * s) * .5f, 44.f, s};
    drawAll(t, knobs, motion_, swing_, damper_, dust_, t_, true);
}

void drawFeltIllustration(Canvas& c, float x, float y, float w, float h) {
    static const int16_t kn[4] = {20000, 20000, 20000, 12000};
    const float s = w / 600.f < h / 300.f ? w / 600.f : h / 300.f;
    Tr t{c, x + (w - 600 * s) * .5f, y + (h - 300 * s) * .5f, s};
    drawAll(t, kn, .3f, .5f, 1.f, 0.f, 0.f, false);
}

}  // namespace dp
