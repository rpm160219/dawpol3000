#include "gfx/icons.h"
#include <math.h>

namespace dp {

static const float PI = 3.14159265f;

// Zasady: siatka 16x16, jedna grubosc kreski, zaokraglone konce, minimum detali
// (czytelne od 16 px). Wypelnienia tylko tam, gdzie kreska bylaby za gesta.
void drawIcon(Canvas& c, IconId id, float x, float y, float s, Color col, float lw) {
    float u = s / 16.f;
    auto X = [&](float v) { return x + v * u; };
    auto Y = [&](float v) { return y + v * u; };
    auto seg = [&](float x0, float y0, float x1, float y1) { c.strokeSeg(X(x0), Y(y0), X(x1), Y(y1), lw); };
    c.beginStroke();
    switch (id) {
    case ICON_KEYS: {  // 3 biale klawisze + 2 czarne
        c.endStroke(col);
        c.strokeRoundRect(X(1.5f), Y(3), 13 * u, 10 * u, 1.5f * u, lw, col);
        c.beginStroke();
        seg(6.17f, 9, 6.17f, 13);
        seg(10.83f, 9, 10.83f, 13);
        c.endStroke(col);
        c.fillRect(X(5.2f), Y(3), 1.9f * u, 6.2f * u, col);
        c.fillRect(X(9.9f), Y(3), 1.9f * u, 6.2f * u, col);
        return;
    }
    case ICON_WAVE: {  // jeden okres sinusa
        float px = X(1.5f), py = Y(8);
        for (int i = 1; i <= 24; i++) {
            float t = i / 24.f;
            float nx = X(1.5f + t * 13), ny = Y(8 - 4.5f * sinf(t * 2 * PI));
            c.strokeSeg(px, py, nx, ny, lw);
            px = nx; py = ny;
        }
        break;
    }
    case ICON_PATTERN: {  // ramka + 3 kreski nut
        c.endStroke(col);
        c.strokeRoundRect(X(1.5f), Y(1.5f), 13 * u, 13 * u, 2 * u, lw, col);
        c.beginStroke();
        seg(4.5f, 5, 7, 5);
        seg(7.5f, 8, 11.5f, 8);
        seg(5.5f, 11, 8, 11);
        break;
    }
    case ICON_ARRANGE: {  // trzy rzedy klipow
        seg(2, 3.5f, 7, 3.5f); seg(9.5f, 3.5f, 14, 3.5f);
        seg(5, 8, 11.5f, 8);
        seg(2, 12.5f, 5, 12.5f); seg(7.5f, 12.5f, 14, 12.5f);
        break;
    }
    case ICON_PLAY: seg(5, 3, 13, 8); seg(13, 8, 5, 13); seg(5, 13, 5, 3); break;
    case ICON_STOP:
        c.endStroke(col);
        c.strokeRoundRect(X(4), Y(4), 8 * u, 8 * u, 1.2f * u, lw, col);
        return;
    case ICON_REC: c.endStroke(col); c.fillCircle(X(8), Y(8), 4.5f * u, col); return;
    case ICON_PLUS:
        c.endStroke(col);
        c.circle(X(8), Y(8), 6.5f * u, lw, col);
        c.beginStroke();
        seg(8, 5, 8, 11); seg(5, 8, 11, 8);
        break;
    case ICON_TAPE: {  // dwie szpule + tasma
        c.endStroke(col);
        c.circle(X(4.5f), Y(7), 3 * u, lw, col);
        c.circle(X(11.5f), Y(7), 3 * u, lw, col);
        c.fillCircle(X(4.5f), Y(7), 0.9f * u, col);
        c.fillCircle(X(11.5f), Y(7), 0.9f * u, col);
        c.beginStroke();
        seg(4.5f, 10, 11.5f, 10);
        break;
    }
    case ICON_METRONOME:
        seg(4, 14, 6.5f, 2); seg(6.5f, 2, 9.5f, 2); seg(9.5f, 2, 12, 14); seg(12, 14, 4, 14);
        seg(8, 11, 10.3f, 5);
        break;
    case ICON_LOOP:  // zaokraglona petla ze strzalka
        c.endStroke(col);
        c.strokeRoundRect(X(2), Y(4.5f), 12 * u, 7.5f * u, 3.5f * u, lw, col);
        c.fillRect(X(6.5f), Y(4.5f) - lw, 3 * u, lw * 2, pal::bg);  // przerwa na grot
        c.beginStroke();
        seg(8, 2.5f, 10, 4.5f); seg(10, 4.5f, 8, 6.5f);
        break;
    case ICON_NOTE:
        c.endStroke(col);
        c.fillCircle(X(5.5f), Y(12), 2.3f * u, col);
        c.beginStroke();
        seg(7.6f, 12, 7.6f, 2.5f);
        seg(7.6f, 2.5f, 11.5f, 5.5f);
        break;
    default: break;
    }
    c.endStroke(col);
}

// Ilustracja: maly syntezator (obudowa, ekran z nutami, 4 kolorowe galki, klawiatura, kabel).
void drawSynthIllustration(Canvas& c, float x, float y, float w, float h) {
    Color body = rgb(150, 150, 150);
    float bh = h * 0.78f;
    c.strokeRoundRect(x, y, w, bh, 8, 1.6f, body);
    // ekran
    float sx = x + 12, sy = y + 12, sw = w * 0.42f, sh = bh * 0.36f;
    c.strokeRoundRect(sx, sy, sw, sh, 4, 1.2f, rgb(90, 90, 90));
    c.fillRoundRect(sx + 6, sy + sh * 0.22f, sw * 0.3f, 4, 2, pal::green);
    c.fillRoundRect(sx + sw * 0.4f, sy + sh * 0.5f, sw * 0.45f, 4, 2, rgb(150, 245, 190));
    c.fillRoundRect(sx + sw * 0.18f, sy + sh * 0.74f, sw * 0.22f, 4, 2, rgb(25, 110, 60));
    // 4 galki w kolorach enkoderow
    for (int i = 0; i < 4; i++) {
        float kx = sx + sw + 18 + i * ((w - sw - 36) / 3.f), ky = sy + sh * 0.5f;
        float r = 9;
        c.circle(kx, ky, r, 1.5f, pal::enc[i]);
        float a = -PI * 0.5f + (i - 1.5f) * 0.7f;
        c.line(kx, ky, kx + cosf(a) * (r - 2), ky + sinf(a) * (r - 2), 1.5f, pal::enc[i]);
    }
    // przyciski
    for (int i = 0; i < 8; i++) c.fillCircle(x + 18 + i * ((w - 36) / 7.f), y + bh * 0.58f, 2.2f, i == 0 ? pal::red : rgb(110, 110, 110));
    // klawiatura
    float ky = y + bh * 0.68f, kh = bh * 0.26f, kx0 = x + 10, kw = w - 20;
    int nw = 14;
    c.rect(kx0, ky, kw, kh, 1.2f, body);
    for (int i = 1; i < nw; i++) c.line(kx0 + i * kw / nw, ky, kx0 + i * kw / nw, ky + kh, 1.f, rgb(110, 110, 110));
    for (int i = 0; i < nw - 1; i++) {
        int k = i % 7;
        if (k == 2 || k == 6) continue;
        c.fillRect(kx0 + (i + 1) * kw / nw - 2.5f, ky, 5, kh * 0.58f, body);
    }
    // kabel
    float cab[2 * 24];
    for (int i = 0; i < 24; i++) {
        float t = i / 23.f;
        cab[i * 2] = x + w - 6 - t * w * 0.85f;
        cab[i * 2 + 1] = y + bh + sinf(t * PI) * h * 0.2f;
    }
    c.polyline(cab, 24, 1.3f, rgb(130, 95, 60));
}

// Ilustracja: magnetofon (szpule, wskazniki VU, suwaki).
void drawTapeIllustration(Canvas& c, float x, float y, float w, float h) {
    Color body = rgb(150, 150, 150);
    c.strokeRoundRect(x, y, w, h * 0.85f, 8, 1.6f, body);
    for (int i = 0; i < 4; i++) {  // VU
        float vx = x + 12 + i * (w - 24) / 4.f, vw = (w - 24) / 4.f - 6;
        c.rect(vx, y + 10, vw, 14, 1.f, rgb(200, 200, 200));
        c.line(vx + vw * 0.5f, y + 22, vx + vw * (0.3f + 0.12f * i), y + 13, 1.2f, pal::red);
    }
    float ry = y + h * 0.45f, r = h * 0.2f;
    for (int k = 0; k < 2; k++) {  // szpule
        float cx = x + w * (0.3f + 0.4f * k);
        c.circle(cx, ry, r, 1.6f, pal::tan);
        c.circle(cx, ry, r * 0.3f, 1.2f, pal::tan);
        for (int s = 0; s < 3; s++) {
            float a = s * 2.f * PI / 3.f + k;
            c.line(cx + cosf(a) * r * 0.3f, ry + sinf(a) * r * 0.3f, cx + cosf(a) * r * 0.85f, ry + sinf(a) * r * 0.85f, 1.f, pal::tan);
        }
    }
    c.line(x + w * 0.3f, ry + r, x + w * 0.7f, ry + r, 1.4f, pal::tan);
    for (int i = 0; i < 4; i++) {  // suwaki
        float fx = x + 20 + i * 18, fy = y + h * 0.68f;
        c.line(fx, fy, fx, fy + h * 0.12f, 1.f, rgb(110, 110, 110));
        c.fillRect(fx - 4, fy + 3 + i * 2, 8, 3, pal::red);
    }
}

}  // namespace dp
