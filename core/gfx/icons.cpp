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
    case ICON_PROJECT:  // notatnik ze spirala i liniami
        c.endStroke(col);
        c.strokeRoundRect(X(2.5f), Y(2.5f), 11 * u, 12 * u, 1.5f * u, lw, col);
        c.beginStroke();
        for (int i = 0; i < 3; i++) seg(5 + i * 3, 1, 5 + i * 3, 4);
        seg(5, 8, 11, 8);
        seg(5, 11, 9, 11);
        break;
    default: break;
    }
    c.endStroke(col);
}

// Ilustracja: maly syntezator. Wszystko liczone od obudowy (x, y, w, h) z wewnetrznym
// marginesem, wiec zaden element nie wychodzi poza obrys.
void drawSynthIllustration(Canvas& c, float x, float y, float w, float h) {
    const Color body = rgb(150, 150, 150), mid = rgb(105, 105, 105);
    const float pad = 10;
    c.strokeRoundRect(x, y, w, h, 8, 1.6f, body);
    float ix = x + pad, iw = w - 2 * pad;  // obszar wewnetrzny
    // gorny rzad: ekran (lewo) + 4 galki (prawo)
    float topY = y + pad, topH = h * 0.32f;
    float sw = iw * 0.42f;
    c.strokeRoundRect(ix, topY, sw, topH, 4, 1.2f, mid);
    float nh = 4;
    c.fillRoundRect(ix + sw * 0.12f, topY + topH * 0.25f - nh * 0.5f, sw * 0.30f, nh, 2, pal::green);
    c.fillRoundRect(ix + sw * 0.45f, topY + topH * 0.50f - nh * 0.5f, sw * 0.42f, nh, 2, rgb(150, 245, 190));
    c.fillRoundRect(ix + sw * 0.22f, topY + topH * 0.75f - nh * 0.5f, sw * 0.22f, nh, 2, rgb(25, 110, 60));
    float kx0 = ix + sw + pad, kw = (ix + iw) - kx0;  // obszar galek
    float cell = kw / 4.f;
    float r = cell * 0.5f - 3;
    if (r > topH * 0.5f - 2) r = topH * 0.5f - 2;
    for (int i = 0; i < 4; i++) {
        float cx = kx0 + cell * (i + 0.5f), cy = topY + topH * 0.5f;
        c.circle(cx, cy, r, 1.5f, pal::enc[i]);
        float ang = -PI * 0.5f + (i - 1.5f) * 0.6f;
        c.line(cx, cy, cx + cosf(ang) * (r - 2.5f), cy + sinf(ang) * (r - 2.5f), 1.5f, pal::enc[i]);
    }
    // rzad przyciskow
    float by = topY + topH + h * 0.10f;
    for (int i = 0; i < 8; i++) c.fillCircle(ix + 3 + i * (iw - 6) / 7.f, by, 2.2f, i == 0 ? pal::red : mid);
    // klawiatura (2 oktawy = 14 bialych)
    float ky = by + h * 0.09f, kh = (y + h - pad) - ky;
    const int nw = 14;
    float wk = iw / nw;
    c.strokeRoundRect(ix, ky, iw, kh, 2, 1.2f, body);
    for (int i = 1; i < nw; i++) c.line(ix + i * wk, ky + kh * 0.58f, ix + i * wk, ky + kh, 1.f, mid);
    for (int i = 0; i < nw - 1; i++) {
        int k = i % 7;
        if (k == 2 || k == 6) continue;  // brak czarnego miedzy E-F i H-C
        c.fillRect(ix + (i + 1) * wk - wk * 0.3f, ky, wk * 0.6f, kh * 0.58f, body);
    }
}

// Ilustracja: magnetofon — 4 wskazniki VU w jednej osi, dwie szpule z tasma, przyciski transportu.
void drawTapeIllustration(Canvas& c, float x, float y, float w, float h) {
    const Color body = rgb(150, 150, 150), mid = rgb(105, 105, 105);
    const float pad = 10;
    c.strokeRoundRect(x, y, w, h, 8, 1.6f, body);
    float ix = x + pad, iw = w - 2 * pad;
    // VU: 4 rowne pola z rowna przerwa, wskazowka z osia na srodku dolnej krawedzi
    const int nv = 4;
    float gap = 6, vw = (iw - gap * (nv - 1)) / nv, vh = h * 0.16f, vy = y + pad;
    for (int i = 0; i < nv; i++) {
        float vx = ix + i * (vw + gap);
        c.strokeRoundRect(vx, vy, vw, vh, 2, 1.f, rgb(200, 200, 200));
        float px = vx + vw * 0.5f, py = vy + vh - 2;
        float ang = -PI * 0.5f + (i - 1.5f) * 0.25f;
        c.line(px, py, px + cosf(ang) * (vh - 5), py + sinf(ang) * (vh - 5), 1.2f, pal::red);
    }
    // szpule
    float ry = vy + vh + h * 0.30f, r = h * 0.21f;
    float cx0 = x + w * 0.29f, cx1 = x + w * 0.71f;
    for (int k = 0; k < 2; k++) {
        float cx = k ? cx1 : cx0;
        c.circle(cx, ry, r, 1.6f, pal::tan);
        c.circle(cx, ry, r * 0.28f, 1.2f, pal::tan);
        for (int sp = 0; sp < 3; sp++) {
            float a = sp * 2.f * PI / 3.f - PI * 0.5f;
            c.line(cx + cosf(a) * r * 0.28f, ry + sinf(a) * r * 0.28f, cx + cosf(a) * r * 0.8f, ry + sinf(a) * r * 0.8f, 1.f, pal::tan);
        }
    }
    c.line(cx0, ry + r, cx1, ry + r, 1.4f, pal::tan);
    // przyciski transportu (rowne, wycentrowane)
    const int nb = 4;
    float bw = iw * 0.16f, bgap = (iw - nb * bw) / (nb + 1), bh = h * 0.09f, byy = y + h - pad - bh;
    for (int i = 0; i < nb; i++)
        c.strokeRoundRect(ix + bgap + i * (bw + bgap), byy, bw, bh, 3, 1.1f, i == 3 ? pal::red : mid);
}

// Ilustracja: notatnik szkicow — spirala, pieciolinie z nutami w kolorach enkoderow, zakladka.
void drawNotebookIllustration(Canvas& c, float x, float y, float w, float h) {
    const Color body = rgb(150, 150, 150), mid = rgb(70, 70, 70);
    const float pad = 10;
    float top = y + 8;  // miejsce na spirale
    c.strokeRoundRect(x, top, w, h - 8, 8, 1.6f, body);
    // spirala: rowno rozlozone kolka przechodzace przez gorna krawedz
    const int nr = 9;
    float step = (w - 2 * pad) / (nr - 1);
    for (int i = 0; i < nr; i++) {
        float cx = x + pad + i * step;
        c.strokeRoundRect(cx - 3, y, 6, 16, 3, 1.3f, body);
    }
    // zakladka
    float bx = x + w - pad - 22;
    float rib[10] = {bx, top, bx, top + h * 0.42f, bx + 6, top + h * 0.36f, bx + 12, top + h * 0.42f, bx + 12, top};
    c.polyline(rib, 5, 1.4f, pal::red);
    // dwie pieciolinie
    float ix = x + pad + 4, iw = w - 2 * pad - 8 - 30;
    for (int st = 0; st < 2; st++) {
        float sy = top + 22 + st * (h - 8 - 32) * 0.5f, gap = 5;
        for (int l = 0; l < 5; l++) c.line(ix, sy + l * gap, ix + iw, sy + l * gap, 1.f, mid);
        // nuty: glowki (kolka) + laski, kolory 4 enkoderow
        static const int pos[2][5] = {{6, 4, 5, 2, 3}, {3, 5, 7, 4, 6}};
        for (int k = 0; k < 5; k++) {
            float nx = ix + 14 + k * (iw - 24) / 4.f, ny = sy + pos[st][k] * gap * 0.5f;
            Color nc = pal::enc[(k + st) % 4];
            c.fillCircle(nx, ny, 3.2f, nc);
            c.line(nx + 3, ny, nx + 3, ny - 15, 1.2f, nc);
        }
    }
    // dolna linia "podpisu"
    c.line(ix, y + h - pad - 4, ix + iw * 0.5f, y + h - pad - 4, 1.2f, mid);
}

}  // namespace dp
