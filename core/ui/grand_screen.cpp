// Ekran GRAND: wspolrzedne w ukladzie 600 x 300, skalowane do obszaru silnika (jak LOUNGE / FELT).
#include "ui/grand_screen.h"
#include <math.h>
#include "gfx/font.h"
#include "synth/grand_dsp.h"

namespace dp {

namespace {
const Color kLine = rgb(226, 224, 255), kBody = rgb(74, 71, 112), kLabel = rgb(110, 106, 156), kWood = rgb(150, 120, 90);
const Color kBlue = rgb(92, 124, 255), kGreen = rgb(19, 224, 143), kRed = rgb(255, 61, 94);

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

// wychylenie struny w punkcie x: dwa impulsy z miejsca uderzenia hx, odbijane od koncow x0 / x1 (odbicie z odwroceniem)
float pulseAt(float x, float hx, float x0, float x1, float t, float speed) {
    const float L = x1 - x0, period = 2.f * L;
    float sum = 0;
    for (int dir = -1; dir <= 1; dir += 2) {
        float p = (hx - x0) + dir * speed * t;  // polozenie na "rozwinietej" strunie
        p = fmodf(p, period);
        if (p < 0) p += period;
        float sign = 1.f, pos = p;
        if (pos > L) { pos = period - pos; sign = -1.f; }
        const float d = (x - x0 - pos) / 9.f;
        sum += sign * expf(-d * d);
    }
    return sum;
}

void drawAll(const Tr& t, const int16_t* kn, float motion, float swing, float damper, float squash, float since, int ns,
             float time, bool labels) {
    const float hard = kn[0] / 32767.f, dec = kn[2] / 32767.f, lid = kn[3] / 32767.f;
    const float uni = kn[5] / 32767.f, growl = kn[6] / 32767.f, board = kn[7] / 32767.f;
    const float ratio = grandStrikeRatio(kn[1]);
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
    // obudowa i pokrywa (LID: 2 .. 28 stopni)
    t.ln(30, 118, 570, 118, 2.f, kLine);
    {
        const float ang = (2.f + 19.f * lid) * 3.14159f / 180.f, L = 340.f;
        const float ex = 140 + L * cosf(ang), ey = 116 - L * sinf(ang);
        t.ln(140, 116, ex, ey, 2.2f, kLine);
        t.ln(140, 120, ex, ey + 4, .8f, kBody);
        const float sy = 116 - (400 - 140) * tanf(ang);
        if (118 - sy > 5) t.ln(400, 118, 400, sy + 3, 1.6f, kWood);
        if (labels) t.text(ex + 6, ey - 6, "LID", 12, dim(kRed, .8f));
    }
    // pulpit z nutami
    t.ln(66, 116, 84, 84, 1.6f, kWood);
    t.rect(70, 72, 118, 100, 1.2f, kLine);
    for (int i = 0; i < 3; i++) t.ln(74, 80 + i * 7, 114, 80 + i * 7, .6f, kLabel);
    t.c.fillCircle(t.X(84), t.Y(86), 1.6f * t.s, kLine);
    t.c.fillCircle(t.X(98), t.Y(79), 1.6f * t.s, kLine);
    t.ln(30, 118, 30, 266, 1.4f, kBody);
    t.ln(570, 118, 570, 266, 1.4f, kBody);
    t.c.arc(t.X(42), t.Y(266), 12 * t.s, 1.5708f, 3.14159f, 1.4f * t.s, kBody);
    t.c.arc(t.X(558), t.Y(266), 12 * t.s, 0.f, 1.5708f, 1.4f * t.s, kBody);
    t.ln(42, 278, 558, 278, 1.4f, kBody);
    // kolki i mostek
    const float x0 = 70, x1 = 520, sy0 = 160;
    t.rect(46, 148, 70, 174, 1.6f, kWood);
    for (int i = 0; i < 3; i++) t.c.circle(t.X(52 + i * 6), t.Y(154 + i * 6), 2.f * t.s, 1.1f * t.s, kWood);
    const float bp[] = {t.X(x1 - 6), t.Y(sy0 + 10), t.X(x1 + 6), t.Y(sy0 + 10), t.X(x1), t.Y(sy0 - 2)};
    t.c.polyline(bp, 3, 1.6f * t.s, kWood, true);
    if (labels) t.text(x1 + 8, sy0 + 12, "BRIDGE", 11, kLabel);
    // plyta rezonansowa i kregi BOARD
    {
        float pts[2 * 33];
        for (int i = 0; i <= 32; i++) {
            const float u = i / 32.f;
            pts[2 * i] = t.X(300 + 250 * u);
            pts[2 * i + 1] = t.Y(222 - 8 * sinf(3.14159f * u));
        }
        t.c.polyline(pts, 33, 1.4f * t.s, kWood);
        t.ln(x1, sy0 + 10, x1, 214, 1.2f, kWood);
        for (int i = 0; i < 4; i++) t.ln(330 + i * 56, 222, 330 + i * 56, 230, 1.f, dim(kWood, .7f));
        if (board > .03f && motion > .02f)
            for (int r = 0; r < 3; r++) {
                const float ph = fmodf(time * 1.6f + r / 3.f, 1.f), rad = 8 + 38 * ph;
                t.c.arc(t.X(x1 - 60), t.Y(222), rad * t.s, 3.14159f * .1f, 3.14159f * .9f, 1.f * t.s, dim(kLine, board * motion * (1.f - ph)));
            }
        if (labels) t.text(330, 238, "SOUNDBOARD", 11, kLabel);
    }
    // struny (1..3), wachlarz UNISON, fala z miejsca uderzenia, drgania podluzne GROWL
    const float hx = x0 + (x1 - x0) * ratio * 2.f;  // miejsce uderzenia (rozciagniete dla czytelnosci)
    const float amp = 9.f * motion * expf(-since * .9f) + 3.f * motion;
    for (int s = 0; s < 3; s++) {
        const bool on = s < ns;
        const float y = sy0 + (s - 1) * 3.5f;
        const float fan = (s - 1) * 6.f * uni;
        const Color col = on ? (s == 0 ? kLine : dim(kLine, s == 1 ? .75f : .6f)) : dim(kBody, .5f);
        float pts[2 * 61];
        for (int i = 0; i <= 60; i++) {
            const float u = i / 60.f, x = x0 + (x1 - x0) * u;
            const float w = on ? amp * pulseAt(x, hx, x0, x1, since + s * .002f, 330.f) * sinf(3.14159f * u) : 0.f;
            pts[2 * i] = t.X(x);
            pts[2 * i + 1] = t.Y(y + fan * u - w);
        }
        t.c.polyline(pts, 61, (on ? 1.6f : 1.f) * t.s, col);
    }
    if (growl > .05f && motion > .02f)
        for (int i = 0; i < 18; i++) {  // zageszczenia wzdluz struny (drgania podluzne)
            const float u = fmodf(i / 18.f + time * .7f, 1.f);
            const float x = x0 + (x1 - x0) * (u + .02f * sinf(u * 40.f + time * 30.f));
            t.ln(x, sy0 - 5, x, sy0 + 5, .9f, dim(kGreen, growl * motion));
        }
    // tlumik nad struna
    {
        const float dx = 380, dy = 138 - 12.f * damper;
        t.ln(dx, 120, dx, dy, 1.4f, kLine);
        t.rect(dx - 13, dy, dx + 13, dy + 12, 1.6f, kLine);
        if (labels) t.text(dx + 18, dy, "DAMPER", 11, kLabel);
    }
    // miejsce uderzenia i mlotek
    for (float yy = 172; yy < 250; yy += 8) t.ln(hx, yy, hx, yy + 4, 1.f, dim(kGreen, .6f));
    if (labels) t.text(hx, 128, "STRIKE", 11, dim(kGreen, .8f), ALIGN_CENTER);
    {
        const float rest = 214, hit = sy0 + 6 + 10.f;
        const float hy = hit + (rest - hit) * swing;
        const float rw = 16.f - 7.f * hard, rh = (10.f - 3.f * hard) * (1.f - .45f * squash);  // filc: miekki duzy, twardy maly
        const Color fc = rgb((uint8_t)(70 + 150 * hard), (uint8_t)(95 + 120 * hard), 255);
        float pts[2 * 25];
        for (int i = 0; i <= 24; i++) {
            const float a = 3.14159f + 3.14159f * i / 24.f;
            pts[2 * i] = t.X(hx + rw * cosf(a));
            pts[2 * i + 1] = t.Y(hy + rh * sinf(a));
        }
        t.c.polyline(pts, 25, 2.f * t.s, fc);
        t.rect(hx - 7, hy, hx + 7, hy + 12, 1.8f, kWood);
        t.ln(hx, hy + 12, hx - 70, 252, 2.f, kBlue);
        t.c.circle(t.X(hx - 70), t.Y(252), 4.f * t.s, 1.6f * t.s, kBlue);
        if (labels) t.text(hx + rw + 6, hy - 4, "HAMMER", 11, dim(kBlue, .8f));
    }
}
}  // namespace

void GrandScreen::draw(Canvas& c, const int16_t knobs[8], const ::GrandCore* core, float dt) {
    t_ += dt;
    since_ += dt;
    const uint32_t st = grandStrikes(core);
    if (st != strikes_) { strikes_ = st; swing_ = 0; since_ = 0; }
    swing_ += (1.f - swing_) * (dt * 7.f > 1.f ? 1.f : dt * 7.f);
    const float m = grandMotion(core);
    motion_ += (m - motion_) * (dt * 20.f > 1.f ? 1.f : dt * 20.f);
    const float dmp = grandHeld(core) > 0 ? 1.f : 0.f;
    damper_ += (dmp - damper_) * (dt * 14.f > 1.f ? 1.f : dt * 14.f);
    const float s = 290.f / 300.f;
    Tr t{c, (640.f - 600.f * s) * .5f, 44.f, s};
    drawAll(t, knobs, motion_, swing_, damper_, grandSquash(core), since_, grandLastStrings(core), t_, true);
}

void drawGrandIllustration(Canvas& c, float x, float y, float w, float h) {
    static const int16_t kn[8] = {16384, 14000, 16384, 26000, 0, 7000, 10000, 16384};
    const float s = w / 600.f < h / 300.f ? w / 600.f : h / 300.f;
    Tr t{c, x + (w - 600 * s) * .5f, y + (h - 300 * s) * .5f, s};
    drawAll(t, kn, .5f, .6f, 1.f, 0.f, .25f, 3, 0.f, false);
}

}  // namespace dp
