#include "gfx/icons.h"
#include <math.h>

namespace dp {

static const float PI = 3.14159265f;

void drawIcon(Canvas& c, IconId id, float x, float y, float s, Color col, float lw) {
    float u = s / 16.f;  // ikona projektowana na siatce 16x16
    auto X = [&](float v) { return x + v * u; };
    auto Y = [&](float v) { return y + v * u; };
    switch (id) {
    case ICON_KEYS: {
        c.strokeRoundRect(X(1), Y(3), 14 * u, 10 * u, 1.5f * u, lw, col);
        for (int i = 1; i < 4; i++) c.line(X(1 + i * 3.5f), Y(8.5f), X(1 + i * 3.5f), Y(13), lw * 0.8f, col);
        for (int i = 0; i < 3; i++) c.fillRoundRect(X(3.6f + i * 3.5f), Y(3), 1.8f * u, 5.5f * u, 0.6f * u, col);
        break;
    }
    case ICON_WAVE: {
        float p[2 * 33];
        for (int i = 0; i <= 32; i++) {
            float t = i / 32.f;
            float env = sinf(t * PI);
            p[i * 2] = X(1 + t * 14);
            p[i * 2 + 1] = Y(8 - sinf(t * PI * 6) * 5.5f * env);
        }
        c.polyline(p, 33, lw, col);
        break;
    }
    case ICON_PATTERN: {
        c.strokeRoundRect(X(1), Y(1.5f), 14 * u, 13 * u, 1.5f * u, lw, col);
        c.fillRoundRect(X(3), Y(4), 4 * u, 2 * u, u, col);
        c.fillRoundRect(X(7.5f), Y(7), 5.5f * u, 2 * u, u, col);
        c.fillRoundRect(X(4), Y(10.5f), 3 * u, 2 * u, u, col);
        break;
    }
    case ICON_ARRANGE: {
        c.fillRoundRect(X(1), Y(2), 7 * u, 3 * u, 1.5f * u, col);
        c.fillRoundRect(X(9), Y(2), 6 * u, 3 * u, 1.5f * u, col);
        c.strokeRoundRect(X(4), Y(6.5f), 9 * u, 3 * u, 1.5f * u, lw, col);
        c.fillRoundRect(X(1), Y(11), 5 * u, 3 * u, 1.5f * u, col);
        c.strokeRoundRect(X(7), Y(11), 8 * u, 3 * u, 1.5f * u, lw, col);
        break;
    }
    case ICON_PLAY: {
        float t[6] = {X(4), Y(2.5f), X(13.5f), Y(8), X(4), Y(13.5f)};
        c.polyline(t, 3, lw, col, true);
        break;
    }
    case ICON_STOP: c.strokeRoundRect(X(3), Y(3), 10 * u, 10 * u, 1.5f * u, lw, col); break;
    case ICON_REC: c.fillCircle(X(8), Y(8), 5 * u, col); break;
    case ICON_PLUS: {
        c.circle(X(8), Y(8), 6.5f * u, lw, col);
        c.line(X(8), Y(4.5f), X(8), Y(11.5f), lw, col);
        c.line(X(4.5f), Y(8), X(11.5f), Y(8), lw, col);
        break;
    }
    case ICON_TAPE: {
        c.circle(X(4.5f), Y(7), 3.5f * u, lw, col);
        c.circle(X(11.5f), Y(7), 3.5f * u, lw, col);
        c.fillCircle(X(4.5f), Y(7), 1.f * u, col);
        c.fillCircle(X(11.5f), Y(7), 1.f * u, col);
        c.line(X(4.5f), Y(10.5f), X(11.5f), Y(10.5f), lw, col);
        c.line(X(2), Y(14), X(14), Y(14), lw, col);
        break;
    }
    case ICON_METRONOME: {
        float t[8] = {X(5), Y(14), X(6.5f), Y(2), X(9.5f), Y(2), X(11), Y(14)};
        c.polyline(t, 4, lw, col, true);
        c.line(X(8), Y(11), X(12.5f), Y(4), lw, col);
        break;
    }
    case ICON_LOOP: {
        c.arc(X(8), Y(8), 5.5f * u, PI * 0.15f, PI * 0.95f, lw, col);
        c.arc(X(8), Y(8), 5.5f * u, PI * 1.15f, PI * 1.95f, lw, col);
        float a1[6] = {X(1.5f), Y(6), X(2.6f), Y(9), X(5), Y(7.5f)};
        c.polyline(a1, 3, lw, col);
        float a2[6] = {X(14.5f), Y(10), X(13.4f), Y(7), X(11), Y(8.5f)};
        c.polyline(a2, 3, lw, col);
        break;
    }
    case ICON_NOTE: {
        c.fillCircle(X(5.5f), Y(12), 2.5f * u, col);
        c.line(X(7.8f), Y(12), X(7.8f), Y(2), lw, col);
        float f[6] = {X(7.8f), Y(2), X(12.5f), Y(5), X(12.5f), Y(8.5f)};
        c.polyline(f, 3, lw, col);
        break;
    }
    default: break;
    }
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
