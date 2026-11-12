#include "ui/wavetable_screen.h"
#include "synth/wavetable_synth.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "../../third_party/op1r_wavetable/wavetable_geom.h"

namespace dp {

// Kolory oryginalu (gen_svg.py / wavetable_screen.c)
static const Color C_GREEN = rgb(0x00, 0xED, 0x95), C_GREEN_DIM = rgb(0x00, 0x8E, 0x59);
static const Color C_WHITE = rgb(0xFF, 0xFF, 0xFF), C_RED = rgb(0xFF, 0x3A, 0x5D);
static const Color C_BLUE = rgb(0x69, 0x8E, 0xFF), C_LAV = rgb(0xDE, 0xD7, 0xFF);
static const Color C_NAVY = rgb(0x38, 0x35, 0x72), C_PLUM = rgb(0x51, 0x49, 0x59), C_GREY = rgb(0x65, 0x65, 0x79);

// Jednostki ekranu OP-1 -> nasze piksele
static inline float PX(float u) { return WavetableScreen::OX + u * WavetableScreen::S; }
static inline float PY(float u) { return WavetableScreen::OY + u * WavetableScreen::S; }
static inline float LW(float units) { return units * WavetableScreen::S * 0.8f; }  // grubosc kreski
// Linia w Q10 (jak ln() w oryginale); szerokosc Q10 "display px" z RPX = 1.25
static void ln(Canvas& c, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, Color col) {
    c.line(PX(x0 / 1024.f), PY(y0 / 1024.f), PX(x1 / 1024.f), PY(y1 / 1024.f), LW(w / 1024.f / 1.25f), col);
}

static int16_t SIN_T[8192];
static void sinInit() {
    if (SIN_T[2048]) return;
    for (int i = 0; i < 8192; i++) SIN_T[i] = (int16_t)lrint(32767.0 * sin(2 * M_PI * i / 8192.0));
}
static inline int32_t sdeg(int32_t d10) { return SIN_T[((d10 * 2330) >> 10) & 8191]; }
static inline int32_t cdeg(int32_t d10) { return SIN_T[(((d10 * 2330) >> 10) + 2048) & 8191]; }

static int32_t lg8(int32_t v) {
    int n = 31 - __builtin_clz((uint32_t)v);
    const int32_t f = n >= 8 ? (v >> (n - 8)) & 255 : (v << (8 - n)) & 255;
    return n * 256 + f;
}
static int32_t log2_q16(uint32_t x) {
    const int n = 31 - __builtin_clz(x);
    const uint32_t m = n <= 30 ? x << (30 - n) : x >> (n - 30);
    const int64_t f = (m & 0x3FFFFFFFu) >> 14;
    int64_t acc = WT_LOG_C[0];
    for (int i = 1; i < 6; i++) acc = ((acc * f) >> 16) + WT_LOG_C[i];
    return (n - 16) * 65536 + (int32_t)acc;
}
static uint32_t exp2_q16(int32_t e) {
    const int n = e >> 16;
    const int64_t f = e & 0xFFFF;
    int64_t acc = WT_EXP_C[0];
    for (int i = 1; i < 5; i++) acc = ((acc * f) >> 16) + WT_EXP_C[i];
    return n >= 0 ? (uint32_t)acc << n : (uint32_t)acc >> -n;
}
static uint32_t isqrt32(uint32_t v) {
    uint32_t r = 0, b = 1u << 30;
    while (b > v) b >>= 2;
    while (b) {
        if (v >= r + b) { v -= r + b; r = (r >> 1) + b; } else r >>= 1;
        b >>= 2;
    }
    return r;
}

// Krzywa filtra na siatce 1 px (curve_shape z oryginalu). Zwraca indeks szczytu.
static int curveShape(int32_t k3, int32_t* cy) {
    const int32_t r15 = (int32_t)(((int64_t)k3 * isqrt32((uint32_t)k3 << 15)) >> 15);
    const uint32_t q16 = 45875u + ((uint32_t)(7 * r15) << 1);
    const int64_t invq = 0xFFFFFFFFu / q16, invq2 = (invq * invq) >> 16;
    int32_t best = 0x7FFFFFFF;
    int peak = WT_CI;
    for (int i = 0; i < WT_CN; i++) {
        const int64_t r2 = exp2_q16((i - WT_CI) * 3193), a = 65536 - r2;
        int64_t den = ((a * a) >> 16) + ((r2 * invq2) >> 16);
        if (den < 1) den = 1;
        if (den > 0x7FFFFFFF) den = 0x7FFFFFFF;
        int32_t y = WT_FLAT_Y * 1024 + (int32_t)(((int64_t)2774 * log2_q16((uint32_t)den)) >> 16);
        if (y < WT_TOP_Y * 1024) y = WT_TOP_Y * 1024;
        if (y > WT_BOT_Y * 1024) y = WT_BOT_Y * 1024;
        cy[i] = y;
        if (k3 > 655 && y < best) { best = y; peak = i; }
    }
    return peak;
}

// Krzywa (biala albo duch): kolano w kx (Q10), przycieta do panelu; plaska linia na lewo, podloga na prawo.
static void drawCurve(Canvas& c, const int32_t* cy, int32_t kx, int32_t w, Color col, bool ticks, int32_t endY = WT_BOT_Y * 1024) {
    int32_t px = kx - WT_CI * 1024, py = cy[0];
    if (px > WT_PX0) ln(c, WT_PX0, py, px, py, w, col);
    for (int i = 1; i <= WT_CN && px < WT_PX1; i++) {
        int32_t x, y;
        if (i < WT_CN) { x = kx + (i - WT_CI) * 1024; y = cy[i]; }
        else { x = WT_PX1; y = endY; }
        if (x <= px) continue;
        int32_t ax = px, ay = py, bx = x, by = y;
        px = x; py = y;
        if (bx <= WT_PX0) continue;
        if (bx > WT_PX1) { by = ay + (int32_t)((int64_t)(by - ay) * (WT_PX1 - ax) / (bx - ax)); bx = WT_PX1; }
        if (ax < WT_PX0) { ay += (int32_t)((int64_t)(by - ay) * (WT_PX0 - ax) / (bx - ax)); ax = WT_PX0; }
        if (bx > ax) {
            c.strokeSeg(PX(ax / 1024.f), PY(ay / 1024.f), PX(bx / 1024.f), PY(by / 1024.f), LW(w / 1024.f / 1.25f));
        }
    }
    c.endStroke(col);
    if (!ticks) return;
    for (int t = 0; t < 4; t++) {  // kreski pod krzywa wokol kolana
        const int i = WT_CI + WT_TICK_OFF[t];
        const int32_t x = kx + WT_TICK_OFF[t] * 1024, y = cy[i];
        int32_t len = WT_BOT_Y * 1024 - y;
        if (len > WT_TICK_LEN) len = WT_TICK_LEN;
        if (x < WT_PX0 || x > WT_PX1 || len < 512) continue;
        ln(c, x, y + 512, x, y + len, WT_TICK_W[t], C_WHITE);
    }
}

// Linia kropkowana jak w gen_svg.py: kreski 1.5 jednostki, przerwy 3.
static void dotted(Canvas& c, float x0, float y0, float x1, float y1, Color col, float w) {
    float L = hypotf(x1 - x0, y1 - y0), ux = (x1 - x0) / L, uy = (y1 - y0) / L;
    c.beginStroke();
    for (float s = 0; s < L - 0.3f; s += 4.5f) {
        float e = s + 1.5f < L ? s + 1.5f : L;
        c.strokeSeg(PX(x0 + ux * s), PY(y0 + uy * s), PX(x0 + ux * e), PY(y0 + uy * e), LW(w));
    }
    c.endStroke(col);
}

// Statyczna grafika (still() z gen_svg.py), raz do cache.
void WavetableScreen::buildCache() {
    sinInit();
    cache_ = new Canvas();
    Canvas& c = *cache_;
    c.clear(pal::bg);
    const float X0 = WT_X0 / 1024.f, Y0 = WT_Y0 / 1024.f, W = WT_W / 1024.f, H = WT_H / 1024.f;
    const float DX = WT_DX / 1024.f, DY = WT_DY / 1024.f, FL = WT_FLOOR / 1024.f;
    const int N = WT_NF;
    const float fy = Y0 + FL;
    for (int k = 0; k < N; k++) {  // podloga: linia poprzeczna pod kazda klatka
        float x = X0 + k * DX, y = fy + k * DY;
        dotted(c, x, y, x + W, y, C_NAVY, 0.8f);
    }
    for (int j = 1; j <= 3; j++) {  // i trzy wzdluzne
        float x = X0 + W * j / 4;
        dotted(c, x, fy, x + (N - 1) * DX, fy + (N - 1) * DY, C_NAVY, 0.8f);
    }
    for (int r = 0; r < 2; r++) {  // szyny
        float x = X0 + r * W;
        dotted(c, x, fy, x + (N - 1) * DX, fy + (N - 1) * DY, C_GREY, 0.8f);
    }
    for (int k = 0; k < N; k++) {  // znaczniki na przedniej szynie
        float x = X0 + k * DX, y = fy + k * DY;
        c.line(PX(x), PY(y + 1), PX(x), PY(y + 3), LW(0.8f), C_LAV);
    }
    for (int k = N - 1; k >= 0; k--) {  // klatki od tylu do przodu
        Color col = k > N / 2 ? C_NAVY : (k > 2 ? C_PLUM : C_GREY);
        int32_t k0 = (k * 32767 + 4) / 9;
        float ox = X0 + k * DX, oy = Y0 + k * DY;
        c.beginStroke();
        float pxp = 0, pyp = 0;
        for (int j = 0; j <= WT_NW; j++) {
            int32_t v = wavetableShape(k0, (uint32_t)((uint64_t)(j % WT_NW) * (0x100000000ull / WT_NW)));
            float x = PX(ox + W * j / WT_NW), y = PY(oy - H * v / 30000.f);
            if (j) c.strokeSeg(pxp, pyp, x, y, LW(1.0f));
            pxp = x; pyp = y;
        }
        c.endStroke(col);
    }
    const float AXIS_Y = WT_BOT_Y + 1, PX0 = WT_PX0 / 1024.f, PX1 = WT_PX1 / 1024.f;
    c.line(PX(PX0), PY(AXIS_Y), PX(PX1), PY(AXIS_Y), LW(0.8f), C_GREY);
    for (int i = 1; i < 10; i++) {
        float x = PX0 + 300.f * i / 10;
        c.line(PX(x), PY(AXIS_Y - 4), PX(x), PY(AXIS_Y), LW(0.8f), C_GREY);
    }
    int32_t gy[WT_CN];  // duch krzywej: CUTOFF w polowie, RESONANCE 0
    curveShape(0, gy);
    c.beginStroke();
    drawCurve(c, gy, WT_KNEE0 + (16384 * 15 >> 1), (int32_t)(1.0f * 1.25f * 1024), C_NAVY, false);
}

void WavetableScreen::draw(Canvas& c, const int16_t kin[4], float level, uint32_t drift, int voices, float dt) {
    if (!cache_) buildCache();
    c.copyRows(*cache_, (int)OY, (int)(OY + 160 * S));
    int32_t k[4];
    for (int i = 0; i < 4; i++) k[i] = kin[i] < 0 ? 0 : kin[i];

    // Aktywnosc i poziom (jak wavetable_screen_update; czas w klatkach ~60/s)
    const bool on = voices > 0;
    act_ += ((on ? 1024 : 0) - act_) >> 3;
    if (!on && act_ < 16) act_ = 0;
    int32_t sum = (int32_t)(level * 32767.f), t = 0;
    if (on && sum > 8) {
        t = (lg8(sum > 32767 ? 32767 : sum) - 3 * 256) * 32767 / 2944;
        if (t > 32767) t = 32767;
        if (t < 0) t = 0;
    }
    lvl_ += (t - lvl_) >> (t > lvl_ ? 1 : 3);
    if (!t && lvl_ < 16) lvl_ = 0;
    if (on) drift_ = drift - 0x55555555u;
    if (act_) ph_ += (uint32_t)((double)act_ / 1024.0 * dt * 4294967296.0);  // glowica: ok. jeden przebieg na sekunde
    else ph_ = 0;

    if (k[0] != k0_) {  // okres klatki przy WAVE
        k0_ = k[0];
        for (int j = 0; j <= WT_NW; j++) {
            wx_[j] = WT_W * j / WT_NW;
            wy_[j] = wavetableShape(k[0], (uint32_t)((uint64_t)(j % WT_NW) * (0x100000000ull / WT_NW)));
        }
        nwave_ = WT_NW;
    }
    if (k[3] != k3_) { k3_ = k[3]; peak_ = curveShape(k[3], cy_); }

    const int32_t f = k[0] * 9216 / 32767;  // glebokosc w klatkach, Q10
    const int32_t ox = WT_X0 + (f * WT_DX >> 10), oy = WT_Y0 + (f * WT_DY >> 10);
    const int32_t amp = WT_H * (1024 + (WT_SWELL * (lvl_ >> 5) >> 10)) >> 10;

    // Harmoniczne (prawy gorny rog)
    const int32_t hp = k[0] * (WT_NPOS - 1), hi = hp >> 15, hf = hp & 0x7FFF;
    const int32_t hs = 1024 + (WT_HB_SWELL * (lvl_ >> 5) >> 10);
    for (int i = 0; i < WT_NH; i++) {
        const int32_t a = WT_HARM[hi][i], b = WT_HARM[hi + (hi < WT_NPOS - 1)][i];
        const int32_t hq = (a * 32768 + (b - a) * hf) >> 15;
        const int32_t h = (hq * WT_HB_H / 255) * hs >> 10;
        if (h < 300) continue;
        const int32_t x = WT_HB_X0 + WT_HB_DX * i;
        ln(c, x, WT_HB_Y, x, WT_HB_Y - h, WT_W_BAR, i ? C_LAV : C_GREEN);
    }

    // Tryb rozszerzony: klatka OSC 2 na czerwono, na swojej glebokosci (rysowana za zielona)
    if (wave2_ >= 0) {
        static const Color C_OSC2 = rgb(0xFF, 0x3A, 0x5D);
        const int32_t f2 = wave2_ * 9216 / 32767;
        const int32_t ox2 = WT_X0 + (f2 * WT_DX >> 10), oy2 = WT_Y0 + (f2 * WT_DY >> 10);
        const int32_t sc = (int32_t)(((int64_t)amp << 15) / WT_PEAK);
        c.beginStroke();
        float px = 0, py = 0;
        for (int j = 0; j <= WT_NW; j++) {
            int32_t v = wavetableShape(wave2_, (uint32_t)((uint64_t)(j % WT_NW) * (0x100000000ull / WT_NW)));
            float x = PX((ox2 + WT_W * j / WT_NW) / 1024.f), y = PY((oy2 - (int32_t)(((int64_t)v * sc) >> 15)) / 1024.f);
            if (j) c.strokeSeg(px, py, x, y, LW(WT_W_GREEN / 1024.f / 1.25f));
            px = x; py = y;
        }
        c.endStroke(C_OSC2);
        const int32_t bx2 = ox2 + WT_W + 5 * 1024, bt2 = oy2 - WT_H - 4 * 1024, bb2 = oy2 + WT_H + 4 * 1024;  // nawias po prawej
        ln(c, bx2 - 3 * 1024, bt2, bx2, bt2, WT_W_BLUE, C_OSC2);
        ln(c, bx2, bt2 - 700, bx2, bb2 + 700, WT_W_BLUE, C_OSC2);
        ln(c, bx2, bb2, bx2 - 3 * 1024, bb2, WT_W_BLUE, C_OSC2);
    }

    // Klatka (zielona) i jej kopia przesunieta o DETUNE + dryf
    auto frame = [&](int32_t x0, int32_t y0, int32_t off, int32_t w, Color col) {
        const int32_t sc = (int32_t)(((int64_t)amp << 15) / WT_PEAK);
        const int32_t x1 = x0 + WT_W;
        c.beginStroke();
        for (int32_t base = x0 - off; base < x1; base += WT_W)
            for (int i = 0; i < nwave_; i++) {
                int32_t xa = base + wx_[i], xb = base + wx_[i + 1];
                if (xb <= x0 || xa >= x1) continue;
                int32_t ya = y0 - (int32_t)(((int64_t)wy_[i] * sc) >> 15), yb = y0 - (int32_t)(((int64_t)wy_[i + 1] * sc) >> 15);
                if (xa < x0 && xb > xa) { ya += (int32_t)((int64_t)(yb - ya) * (x0 - xa) / (xb - xa)); xa = x0; }
                if (xb > x1 && xb > xa) { yb -= (int32_t)((int64_t)(yb - ya) * (xb - x1) / (xb - xa)); xb = x1; }
                c.strokeSeg(PX(xa / 1024.f), PY(ya / 1024.f), PX(xb / 1024.f), PY(yb / 1024.f), LW(w / 1024.f / 1.25f));
            }
        c.endStroke(col);
    };
    const uint32_t cph = ((uint32_t)(WT_COPY_PH0 + (WT_COPY_PHD * k[1] >> 15)) << 16) + drift_;
    const int32_t coff = (int32_t)(((int64_t)(cph >> 16) * WT_W) >> 16);
    frame(ox + WT_COPY_DX, oy, coff, WT_W_COPY, C_GREEN_DIM);
    frame(ox, oy, 0, WT_W_GREEN, C_GREEN);

    // Linie od koncow klatki do podlogi
    ln(c, ox, oy + 512, ox, oy + WT_FLOOR, WT_W_LAV, C_LAV);
    ln(c, ox + WT_W, oy + 512, ox + WT_W, oy + WT_FLOOR, WT_W_LAV, C_LAV);
    // Niebieski nawias i znacznik na szynie
    const int32_t bx = ox - 5 * 1024, bt = oy - WT_H - 4 * 1024, bb = oy + WT_H + 4 * 1024;
    ln(c, bx + 3 * 1024, bt, bx, bt, WT_W_BLUE, C_BLUE);
    ln(c, bx, bt - 700, bx, bb + 700, WT_W_BLUE, C_BLUE);
    ln(c, bx, bb, bx + 3 * 1024, bb, WT_W_BLUE, C_BLUE);
    ln(c, ox, oy + WT_FLOOR, ox, oy + WT_FLOOR + 4 * 1024, WT_W_BLUE, C_BLUE);
    // Glowica
    const int32_t hx = ox + (int32_t)(((int64_t)(ph_ >> 16) * WT_W) >> 16);
    const int32_t hy = oy - (int32_t)(((int64_t)wavetableShape(k[0], ph_) * amp) / WT_PEAK);
    ln(c, hx - 1100, hy, hx + 1100, hy, WT_W_DOT, C_BLUE);

    // Panel filtra: biala krzywa (CUTOFF = kolano, RESONANCE = szczyt), czerwone luki
    const int32_t kx = WT_KNEE0 + (k[2] * 15 >> 1);
    // ksztalt krzywej wg trybu filtra: LP24 = oryginal, LP12 = polowa nachylenia, HP = odbicie wokol kolana, BP = oba
    const int32_t flat = WT_FLAT_Y * 1024, bot = WT_BOT_Y * 1024;
    for (int i = 0; i < WT_CN; i++) {
        int32_t lp = cy_[i], hp = cy_[(2 * WT_CI - i) < 0 ? 0 : ((2 * WT_CI - i) >= WT_CN ? WT_CN - 1 : 2 * WT_CI - i)];
        int32_t y = lp;
        if (fmode_ == 1) y = flat + (lp - flat) / 2;
        else if (fmode_ == 3) y = flat + (hp - flat) / 2;
        else if (fmode_ == 2) { int32_t a = flat + (lp - flat) / 2, b2 = flat + (hp - flat) / 2; y = a > b2 ? a : b2; }
        cyShown_[i] = y > bot ? bot : y;
    }
    const int32_t* cy = wave2_ >= 0 ? cyShown_ : cy_;
    c.beginStroke();
    drawCurve(c, cy, kx, WT_W_WHITE, C_WHITE, true, wave2_ >= 0 ? cy[WT_CN - 1] : WT_BOT_Y * 1024);
    int32_t px = kx + (peak_ - WT_CI) * 1024;
    if (px < WT_PX0) px = WT_PX0;
    if (px > WT_PX1) px = WT_PX1;
    const int32_t py = cy[peak_];
    const int narcs = 1 + k[3] * 3 / 32767;
    for (int a = 0; a < narcs; a++) {
        const int32_t rad = (5 + 4 * a) * 1024, ccy = py + rad * 3 / 10;
        int32_t qx = 0, qy = 0;
        for (int s = 0; s <= 6; s++) {
            const int32_t e = 1431 - 1063 * s / 6;
            const int32_t x = px + ((rad >> 5) * cdeg(e) >> 10), y = ccy - ((rad >> 5) * sdeg(e) >> 10);
            if (s) ln(c, qx, qy, x, y, WT_W_RED, C_RED);
            qx = x; qy = y;
        }
    }
}

}  // namespace dp
