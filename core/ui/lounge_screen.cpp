// Ekran LOUNGE: wspolrzedne w ukladzie 600 x 300, skalowane do obszaru silnika.
#include "ui/lounge_screen.h"
#include <math.h>
#include "gfx/font.h"
#include "synth/lounge_dsp.h"

namespace dp {

namespace {
const Color kLine = rgb(226, 224, 255), kBody = rgb(74, 71, 112), kLabel = rgb(110, 106, 156), kPurple = rgb(155, 106, 214);
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

// drgajaca linia (zab / jezyczek / struna): ksztalt pierwszego modu, wychylenie amp
void vibLine(const Tr& t, float x0, float x1, float y, float amp, float ph, bool fixedBoth, Color col, float w) {
    float pts[2 * 41];
    for (int i = 0; i <= 40; i++) {
        const float u = i / 40.f;
        const float shape = fixedBoth ? sinf(3.14159f * u) : u * u * (1.5f - .5f * u);  // struna / wspornik
        pts[2 * i] = t.X(x0 + (x1 - x0) * u);
        pts[2 * i + 1] = t.Y(y + amp * shape * sinf(ph));
    }
    t.c.polyline(pts, 41, w * t.s, col);
}

void drawAll(const Tr& t, const int16_t* kn, float motion, float swing, int voices, float time, bool labels) {
    const int type = loungeType(kn[3]);
    const float bark = kn[0] / 32767.f, tone = kn[1] / 32767.f, dec = kn[2] / 32767.f;
    // DECAY: krzywa w lewym gornym rogu
    if (labels) t.text(56, 6, "DECAY", 13, kLabel);
    t.ln(56, 28, 56, 66, 1.4f, kLine);
    {
        float pts[2 * 25];
        const float tau = .08f + .9f * dec;
        for (int i = 0; i <= 24; i++) {
            const float u = i / 24.f;
            pts[2 * i] = t.X(58 + 70 * u);
            pts[2 * i + 1] = t.Y(28 + 36 * (1.f - expf(-u / tau)));
        }
        t.c.polyline(pts, 25, 2.f * t.s, kLine);
    }
    // pokrywa i koktajl
    t.ln(30, 87, 570, 87, 2.f, kLine);
    if (labels) t.text(242, 66, "LID", 13, kLabel);
    {
        const float cx = 472, top = 22, rim = 70;
        const float cup[] = {t.X(cx - 30), t.Y(top), t.X(cx + 30), t.Y(top), t.X(cx), t.Y(top + 38)};
        t.c.polyline(cup, 3, 2.f * t.s, kLine, true);
        const float lv = sinf(time * 1.7f) * 1.5f;  // napoj kolysze sie
        t.ln(cx - 24, top + 7 + lv, cx + 24, top + 7 - lv, 2.f, kGreen);
        t.ln(cx, top + 38, cx, 82, 2.f, kLine);
        t.ln(cx - 14, 84, cx + 14, 84, 2.4f, kLine);
        t.ln(cx + 4, top + 22, cx + 22, top - 14, 1.6f, kLine);  // wykalaczka
        t.c.circle(t.X(cx + 2), t.Y(top + 17), 5.f * t.s, 1.8f * t.s, kRed);  // oliwka
        for (int i = 0; i < 4; i++) {  // odbicie w lakierze pokrywy
            const float w = 70 - i * 12;
            t.ln(cx - w, 90 + i * 2.2f, cx + w, 90 + i * 2.2f, .9f, kPurple);
        }
        (void)rim;
    }
    // obudowa
    t.ln(44, 87, 44, 282, 1.4f, kBody);
    t.ln(556, 87, 556, 282, 1.4f, kBody);
    t.ln(44, 262, 556, 262, 1.4f, kBody);
    t.ln(44, 282, 556, 282, 1.4f, kBody);
    // nazwa typu
    static const char* N[3] = {"TINE", "REED", "CLAV"};
    t.text(172, 110, N[type], 24, kRed);
    const float ph = time * 38.f, amp = 10.f * motion;
    const float lift = 26.f * (.3f + .7f * bark) * swing;  // zamach mlotka (uderzenie = swing -> 0)
    const float py = 180 + (tone - .5f) * 30.f;           // przetwornik: TONE przesuwa go wzgledem zeba
    if (type == 0) {
        t.rect(74, 134, 106, 248, 2.f, kLine);             // blok
        t.rect(106, 137, 388, 153, 2.f, kLine);            // listwa tonowa
        vibLine(t, 106, 435, 180, amp, ph, false, kLine, 2.f);
        t.ln(106, 178, 435, 168 - amp * .4f, .9f, kPurple);
        t.ln(106, 182, 435, 192 + amp * .4f, .9f, kPurple);
        for (int i = 0; i < 4; i++) t.ln(350 + i * 6, 176 + (i & 1) * 8, 356 + i * 6, 176 + ((i + 1) & 1) * 8, 1.4f, kLine);  // sprezynka
        if (motion > .05f) for (int k = 0; k < 2; k++) {   // iskry przy uderzeniu
            const float x = 185 + k * 70;
            t.ln(x, 158, x + 6, 164, 1.4f, kBlue); t.ln(x + 6, 164, x + 2, 166, 1.4f, kBlue); t.ln(x + 2, 166, x + 10, 172, 1.4f, kBlue);
        }
        const float hy = 187 + lift;
        t.rect(209, hy, 234, hy + 16, 2.f, kBlue);         // mlotek
        t.ln(147, 252, 214, hy + 16, 2.f, kBlue);
        t.c.circle(t.X(147), t.Y(252), 4.f * t.s, 1.6f * t.s, kBlue);
        t.rect(452, py - 15, 474, py + 15, 2.f, kGreen);   // przetwornik (cewka)
        for (int i = 0; i < 2; i++) t.ln(459 + i * 7, py - 15, 459 + i * 7, py + 15, 1.6f, kGreen);
        t.ln(437, 180, 452, py, 1.2f, kGreen);
        t.ln(474, py, 537, py, 2.f, kGreen);
        t.ln(537, py, 537, 249, 2.f, kGreen);
    } else if (type == 1) {
        t.rect(74, 150, 120, 230, 2.f, kLine);             // blok jezyczkow
        vibLine(t, 120, 380, 182, amp * .8f, ph * 1.3f, false, kLine, 3.f);
        t.rect(370, 176 + amp * .8f * sinf(ph * 1.3f) - 4, 392, 188 + amp * .8f * sinf(ph * 1.3f) + 4, 1.8f, kLine);  // ciezarek
        const float gap = 14 + 20 * tone;                  // przetwornik plytkowy: szczelina = TONE
        t.ln(400, 182 - gap, 460, 182 - gap, 2.4f, kGreen);
        t.ln(400, 182 + gap, 460, 182 + gap, 2.4f, kGreen);
        t.ln(460, 182 - gap, 460, 182 + gap, 1.4f, kGreen);
        t.ln(460, 182, 537, 182, 2.f, kGreen);
        t.ln(537, 182, 537, 249, 2.f, kGreen);
        const float hy = 196 + lift;
        t.rect(236, hy, 262, hy + 14, 2.f, kBlue);
        t.ln(160, 252, 241, hy + 14, 2.f, kBlue);
        t.c.circle(t.X(160), t.Y(252), 4.f * t.s, 1.6f * t.s, kBlue);
    } else {
        t.ln(80, 172, 80, 190, 2.4f, kLine);               // mostki
        t.ln(520, 172, 520, 190, 2.4f, kLine);
        vibLine(t, 80, 520, 180, amp * .7f, ph * .8f, true, kLine, 1.8f);
        for (int i = 0; i < 5; i++) t.ln(92 + i * 6, 170 + (i & 1) * 20, 98 + i * 6, 170 + ((i + 1) & 1) * 20, 1.4f, kPurple);  // przedza
        const float tx = 440, hy = 186 + lift * .6f;       // stycznik
        t.rect(tx - 8, hy, tx + 8, hy + 10, 2.f, kBlue);
        t.ln(tx, hy + 10, tx - 50, 250, 2.f, kBlue);
        t.c.circle(t.X(tx - 50), t.Y(250), 4.f * t.s, 1.6f * t.s, kBlue);
        const float px = 120 + 180 * tone;                 // przetworniki: TONE przesuwa wzdluz struny
        for (int k = 0; k < 2; k++) t.rect(px + k * 40, 196, px + k * 40 + 24, 210, 2.f, kGreen);
        t.ln(px + 64, 203, px + 74, 203, 2.f, kGreen);
        t.ln(px + 74, 203, px + 74, 218, 2.f, kGreen);
        t.ln(px + 74, 218, 537, 218, 2.f, kGreen);
        t.ln(537, 218, 537, 249, 2.f, kGreen);
    }
    t.c.circle(t.X(537), t.Y(255), 6.f * t.s, 2.f * t.s, kGreen);  // gniazdo
    // glosy
    for (int i = 0; i < 6; i++) {
        const float x = 286 + i * 17;
        if (i < voices) t.ln(x, 229, x, 255, 6.f, rgb(200, 198, 236));
        else t.rect(x - 4, 255, x + 4, 261, 1.2f, kBody);
    }
    if (labels) {
        t.text(221, 270, "HAMMER", 13, rgb(88, 98, 170), ALIGN_CENTER);
        t.text(326, 270, "VOICES", 13, kLabel, ALIGN_CENTER);
        t.text(468, 270, "PICKUP", 13, rgb(60, 150, 120), ALIGN_CENTER);
    }
}
}  // namespace

void LoungeScreen::draw(Canvas& c, const int16_t knobs[4], const ::LoungeCore* core, int voices, float dt) {
    t_ += dt;
    const uint32_t st = loungeStrikes(core);
    if (st != strikes_) { strikes_ = st; swing_ = 0; }
    swing_ += (1.f - swing_) * (dt * 6.f > 1.f ? 1.f : dt * 6.f);  // mlotek wraca do zamachu
    const float m = loungeMotion(core);
    motion_ += (m - motion_) * (dt * 20.f > 1.f ? 1.f : dt * 20.f);
    Tr t{c, 20.f * 640.f / 600.f * 0.f, 44.f, 290.f / 300.f};
    t.ox = (640.f - 600.f * t.s) * .5f;
    drawAll(t, knobs, motion_, swing_, voices, t_, true);
}

void drawLoungeIllustration(Canvas& c, float x, float y, float w, float h) {
    static const int16_t kn[4] = {16000, 16384, 18000, 0};
    const float s = w / 600.f < h / 300.f ? w / 600.f : h / 300.f;
    Tr t{c, x + (w - 600 * s) * .5f, y + (h - 300 * s) * .5f, s};
    drawAll(t, kn, .4f, 1.f, 3, 0.f, false);
}

}  // namespace dp
