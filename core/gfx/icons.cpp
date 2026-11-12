#include "gfx/icons.h"
#include "../../third_party/op1r_taffy/taffy_art.h"
#include "../../third_party/op1r_taffy/taffy_geom.h"
#include "../../third_party/op1r_pew/pew_art.h"
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
    case ICON_PROJECT:  // dyskietka
        seg(2, 2, 12, 2); seg(12, 2, 14, 4); seg(14, 4, 14, 14); seg(14, 14, 2, 14); seg(2, 14, 2, 2);
        seg(5, 2, 5, 6); seg(5, 6, 11, 6); seg(11, 6, 11, 2);
        seg(4.5f, 9.5f, 11.5f, 9.5f); seg(11.5f, 9.5f, 11.5f, 14); seg(4.5f, 9.5f, 4.5f, 14);
        break;
    default: break;
    }
    c.endStroke(col);
}

// Fala do ilustracji (czyste ksztalty, bez falbanek): m 0..1 przechodzi sinus -> trojkat -> pila -> prostokat
static float illusWave(float m, float p) {
    float sh[4] = {sinf(p * 6.2831853f), p < 0.25f ? 4 * p : (p < 0.75f ? 2 - 4 * p : 4 * p - 4),
                   p < 0.5f ? 2 * p : 2 * p - 2, p < 0.5f ? 0.85f : -0.85f};
    float f = m * 3.f;
    int i = (int)f;
    if (i >= 3) return sh[3];
    float t = f - i;
    return sh[i] + (sh[i + 1] - sh[i]) * t;
}

void drawEngineIllustration(Canvas& c, int engine, float x, float y, float w, float h) {
    const Color NAVY = rgb(0x38, 0x35, 0x72), PLUM = rgb(0x51, 0x49, 0x59), GREY = rgb(0x65, 0x65, 0x79);
    const Color GREEN = rgb(0x00, 0xED, 0x95), BLUE = rgb(0x69, 0x8E, 0xFF), RED = rgb(0xFF, 0x3A, 0x5D);
    if (engine == 2) {  // TAFFY: dziewczyna z grafiki ekranu (taffy.svg) — twarz, usta, nici gumy (slina), gwiazdki
        // bez balonu i chmur; nici i iskry przy ustach liczone jak w screen.c dla balonu w spoczynku (makieta)
        enum { FACE, MOUTH, STAR };
        auto kind = [](const TFLine& l) -> int {
            float mx = 0;
            for (int i = 0; i < l.count; i++) mx = fmaxf(mx, TF_PTS[(l.first + i) * 2]);
            if (l.rgb == 0x9A98E1 && mx <= 136.f) return FACE;
            if (l.rgb == 0xFFFFFF && mx <= 140.f) return MOUTH;
            if (l.rgb == 0xA899FC && mx <= 170.f) return STAR;
            return -1;
        };
        // nici: krzywe kwadratowe od ust do obrysu balonu (jednostki ekranu OP-1)
        float sp[5][9][2];
        {
            float sx[3] = {}, sy[3] = {};
            const float bx = TF_BX / 1024.f, by = TF_BY / 1024.f, br = TF_BR / 1024.f;
            const float mx = TF_MX / 1024.f, my = TF_MY / 1024.f;
            for (int s = 0; s < 5; s++) {
                const int32_t* d = tf_strand[s];
                const float an = d[2] * 6.2831853f / 8192.f;
                const float ex = bx + br * cosf(an), ey = by + br * sinf(an);
                float x0, y0;
                if (d[0] < 0) { x0 = mx + (ex - mx) * d[3] / 1024.f; y0 = my + (ey - my) * d[3] / 1024.f; }
                else { x0 = sx[d[0]]; y0 = sy[d[0]]; }
                const float x1 = (x0 + ex) * 0.5f, y1 = (y0 + ey) * 0.5f - (ex - x0) * d[4] / 1024.f;
                auto q = [&](float u, float* ox, float* oy) {
                    const float v = 1 - u;
                    *ox = v * v * x0 + 2 * u * v * x1 + u * u * ex;
                    *oy = v * v * y0 + 2 * u * v * y1 + u * u * ey;
                };
                for (int i = 0; i <= 8; i++) q(i / 8.f, &sp[s][i][0], &sp[s][i][1]);
                const int32_t tq = s == 0 ? tf_strand[3][1] : s == 2 ? tf_strand[4][1] : 0;
                if (s < 3) q(tq / 1024.f, &sx[s], &sy[s]);
            }
        }
        float minx = 1e9f, miny = 1e9f, maxx = -1e9f, maxy = -1e9f;
        auto grow = [&](float px, float py) { minx = fminf(minx, px); maxx = fmaxf(maxx, px); miny = fminf(miny, py); maxy = fmaxf(maxy, py); };
        for (const TFLine& l : TF_LINES)
            if (kind(l) >= 0) for (int i = 0; i < l.count; i++) grow(TF_PTS[(l.first + i) * 2], TF_PTS[(l.first + i) * 2 + 1]);
        for (int s = 0; s < 5; s++) for (int i = 0; i <= 8; i++) grow(sp[s][i][0], sp[s][i][1]);
        const float sc = fminf((w - 16) / (maxx - minx), (h - 10) / (maxy - miny));
        const float ox = x + (w - (maxx - minx) * sc) * 0.5f, oy = y + (h - (maxy - miny) * sc) * 0.5f;
        auto X = [&](float u) { return ox + (u - minx) * sc; };
        auto Y = [&](float u) { return oy + (u - miny) * sc; };
        const Color VIO = rgb(0x9A, 0x98, 0xE1), STARC = rgb(0xA8, 0x99, 0xFC), WHITE = rgb(255, 255, 255);
        for (const TFLine& l : TF_LINES) {
            const int k = kind(l);
            if (k < 0) continue;
            c.beginStroke();
            for (int i = 0; i + 1 < l.count; i++) {
                const float* q = TF_PTS + (l.first + i) * 2;
                c.strokeSeg(X(q[0]), Y(q[1]), X(q[2]), Y(q[3]), 1.6f);
            }
            c.endStroke(k == FACE ? VIO : k == MOUTH ? WHITE : STARC);
        }
        for (const TFCircle& d : TF_CIRCLES)
            if (d.x <= 170.f) c.fillCircle(X(d.x), Y(d.y), 1.3f, WHITE);
        for (int s = 0; s < 5; s++) {
            c.beginStroke();
            for (int i = 0; i < 8; i++) c.strokeSeg(X(sp[s][i][0]), Y(sp[s][i][1]), X(sp[s][i + 1][0]), Y(sp[s][i + 1][1]), 1.6f);
            c.endStroke(GREEN);
        }
        for (const auto& e : tf_lipspark)  // kreski przy ustach
            c.line(X(e[0] / 1024.f), Y(e[1] / 1024.f), X(e[2] / 1024.f), Y(e[3] / 1024.f), 1.6f, WHITE);
        (void)NAVY; (void)PLUM; (void)GREY; (void)BLUE; (void)RED;
        return;
    }
    if (engine == 3) {  // PEW: cala scena "jazz club" z pew.svg (nuty na swoich miejscach)
        const float sc = fminf(w / 320.f, h / 160.f);
        const float ox = x + (w - 320 * sc) * 0.5f, oy = y + (h - 160 * sc) * 0.5f;
        auto hc = [](unsigned v) { return rgb((v >> 16) & 255, (v >> 8) & 255, v & 255); };
        static float buf[2 * 512];
        for (const PWFill& f : PW_FILLS) {
            if (f.count > 512) continue;
            for (int i = 0; i < f.count; i++) {
                buf[2 * i] = ox + PW_PTS[(f.first + i) * 2] * sc;
                buf[2 * i + 1] = oy + PW_PTS[(f.first + i) * 2 + 1] * sc;
            }
            c.fillPolygon(buf, f.count, hc(f.rgb));
        }
        for (const PWStroke& l : PW_STROKES) {
            c.beginStroke();
            const int segs = l.closed ? l.count : l.count - 1;
            for (int i = 0; i < segs; i++) {
                const int a = l.first + i, b = l.first + (i + 1 == l.count ? 0 : i + 1);
                c.strokeSeg(ox + PW_PTS[a * 2] * sc, oy + PW_PTS[a * 2 + 1] * sc, ox + PW_PTS[b * 2] * sc, oy + PW_PTS[b * 2 + 1] * sc,
                            fmaxf(1.1f, l.width * sc * 0.9f));
            }
            c.endStroke(hc(l.rgb));
        }
        for (const PWCircle& d : PW_CIRCLES) c.circle(ox + d.x * sc, oy + d.y * sc, d.r * sc, 1.f, hc(d.rgb));
        (void)NAVY; (void)PLUM; (void)GREY; (void)GREEN; (void)BLUE; (void)RED;
        return;
    }
    if (engine == 22) {  // TEST SAW: prosta pila w ramce
        c.strokeRoundRect(x, y, w, h, 8, 1.4f, rgb(80, 80, 80));
        float px = x + 16, py = y + h * 0.5f;
        for (int k = 0; k < 4; k++) {
            float x0 = x + 16 + k * (w - 32) / 4.f, x1 = x0 + (w - 32) / 4.f;
            c.line(x0, py + h * 0.25f, x1, py - h * 0.25f, 2.f, rgb(150, 150, 150));
            c.line(x1, py - h * 0.25f, x1, py + h * 0.25f, 2.f, rgb(150, 150, 150));
            px = x1;
        }
        (void)px;
        return;
    }
    const int N = 6;
    const float fw = w * 0.62f, fh = h * 0.16f;               // klatka: szerokosc, pol-wysokosc
    const float dx = (w - fw - 14) / (N - 1), dy = -(h * 0.42f) / (N - 1);
    const float x0 = x + 10, y0 = y + h * 0.72f;              // przednia klatka
    // podloga z kropek: poprzeczne linie pod klatkami i dwie szyny
    for (int k = 0; k < N; k++) {
        float fx = x0 + k * dx, fy = y0 + fh * 0.35f + k * dy;
        for (float t = 0; t < fw; t += 7) c.fillCircle(fx + t, fy, 0.8f, NAVY);
    }
    for (int r = 0; r < 2; r++)
        for (int k = 0; k < (N - 1) * 6; k++) {
            float t = k / (float)((N - 1) * 6);
            c.fillCircle(x0 + r * fw + t * (N - 1) * dx, y0 + fh * 0.35f + t * (N - 1) * dy, 0.8f, GREY);
        }
    auto frame = [&](int k, float m, Color col, float lw) {
        float fx = x0 + k * dx, fy = y0 + k * dy;
        float pts[2 * 97];
        for (int i = 0; i <= 96; i++) {
            float p = i / 96.f;
            pts[i * 2] = fx + fw * p;
            pts[i * 2 + 1] = fy - fh * illusWave(m, p);
        }
        c.polyline(pts, 97, lw, col);
    };
    for (int k = N - 1; k >= 0; k--) frame(k, k / (float)(N - 1), k > 3 ? NAVY : (k > 1 ? PLUM : GREY), 1.2f);
    if (engine == 1) frame(4, 1.f, RED, 2.f);                 // WAVETABLE+: druga fala (OSC 2) na czerwono
    frame(1, 0.4f, GREEN, 2.2f);                               // biezaca fala (OSC 1) na zielono
    // niebieski nawias przy zielonej klatce
    float bx = x0 + dx - 6, by = y0 + dy;
    c.line(bx + 4, by - fh - 6, bx, by - fh - 6, 1.6f, BLUE);
    c.line(bx, by - fh - 7, bx, by + fh + 7, 1.6f, BLUE);
    c.line(bx, by + fh + 6, bx + 4, by + fh + 6, 1.6f, BLUE);
    if (engine == 1) {  // czerwony nawias przy klatce OSC 2
        float rx = x0 + 4 * dx + fw + 6, ry = y0 + 4 * dy;
        c.line(rx - 4, ry - fh - 6, rx, ry - fh - 6, 1.6f, RED);
        c.line(rx, ry - fh - 7, rx, ry + fh + 7, 1.6f, RED);
        c.line(rx, ry + fh + 6, rx - 4, ry + fh + 6, 1.6f, RED);
    }
}

void drawCategoryIcon(Canvas& c, int cat, float x, float y, float s, Color col, float lw) {
    float u = s / 16.f;
    auto X = [&](float v) { return x + v * u; };
    auto Y = [&](float v) { return y + v * u; };
    auto seg = [&](float x0, float y0, float x1, float y1) { c.strokeSeg(X(x0), Y(y0), X(x1), Y(y1), lw); };
    c.beginStroke();
    switch (cat) {
    case -1:  // ALL: siatka 4 kropek
        c.endStroke(col);
        for (int i = 0; i < 4; i++) c.fillCircle(X(4.5f + (i % 2) * 7), Y(4.5f + (i / 2) * 7), 2.f * u, col);
        return;
    case 0: {  // BASS: niski, szeroki sinus + podloga
        float px = X(1.5f), py = Y(8);
        for (int i = 1; i <= 20; i++) {
            float t = i / 20.f, nx = X(1.5f + 13 * t), ny = Y(8 + 4 * sinf(t * 6.2831853f));
            c.strokeSeg(px, py, nx, ny, lw * 1.3f);
            px = nx; py = ny;
        }
        seg(2, 14.5f, 14, 14.5f);
        break;
    }
    case 1:  // LEAD: pila
        seg(1.5f, 12, 5.5f, 4); seg(5.5f, 4, 5.5f, 12); seg(5.5f, 12, 9.5f, 4); seg(9.5f, 4, 9.5f, 12); seg(9.5f, 12, 14, 4);
        break;
    case 2:  // PAD: chmura (trzy luki)
        c.endStroke(col);
        c.arc(X(5), Y(9), 3.2f * u, 3.14159f * 0.9f, 3.14159f * 1.9f, lw, col);
        c.arc(X(8.5f), Y(7.5f), 3.8f * u, 3.14159f * 1.05f, 3.14159f * 1.95f, lw, col);
        c.arc(X(11.5f), Y(9.5f), 2.8f * u, 3.14159f * 1.2f, 3.14159f * 2.1f, lw, col);
        c.beginStroke();
        seg(2, 12, 14.3f, 12);
        break;
    case 3:  // KEYS
        c.endStroke(col);
        drawIcon(c, ICON_KEYS, x, y, s, col, lw);
        return;
    case 4: {  // PLUCK: szpic i wygasanie
        seg(2, 13, 4, 2.5f);
        float px = X(4), py = Y(2.5f);
        for (int i = 1; i <= 12; i++) {
            float t = i / 12.f, nx = X(4 + 10.5f * t), ny = Y(13 - 10.5f * expf(-4.f * t));
            c.strokeSeg(px, py, nx, ny, lw);
            px = nx; py = ny;
        }
        break;
    }
    case 5:  // BELL: dzwonek
        seg(8, 1.5f, 8, 3); seg(5, 4, 11, 4); seg(5, 4, 4.5f, 10); seg(11, 4, 11.5f, 10); seg(2.5f, 12, 13.5f, 12);
        seg(4.5f, 10, 2.5f, 12); seg(11.5f, 10, 13.5f, 12);
        c.endStroke(col);
        c.fillCircle(X(8), Y(14), 1.4f * u, col);
        return;
    case 6:  // FX: iskra
        seg(8, 1.5f, 8, 14.5f); seg(1.5f, 8, 14.5f, 8); seg(3.5f, 3.5f, 12.5f, 12.5f); seg(12.5f, 3.5f, 3.5f, 12.5f);
        break;
    default:  // OTHER: kolo z kropka
        c.endStroke(col);
        c.circle(X(8), Y(8), 6 * u, lw, col);
        c.fillCircle(X(8), Y(8), 1.8f * u, col);
        return;
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

// Ilustracja: dyskietka 3.5". Kwadrat wycentrowany w polu w x h (to samo pole co inne ilustracje).
void drawFloppyIllustration(Canvas& c, float x, float y, float w, float h) {
    const Color body = rgb(150, 150, 150), mid = rgb(80, 80, 80);
    float s = h < w ? h : w;
    float fx = x + (w - s) * 0.5f, fy = y + (h - s) * 0.5f, ch = s * 0.09f;
    // obudowa ze scietym prawym gornym rogiem
    float o[10] = {fx, fy, fx + s - ch, fy, fx + s, fy + ch, fx + s, fy + s, fx, fy + s};
    c.polyline(o, 5, 1.6f, body, true);
    // metalowa zasuwka z okienkiem
    float zx = fx + s * 0.24f, zw = s * 0.50f, zh = s * 0.34f;
    c.strokeRoundRect(zx, fy, zw, zh, 3, 1.4f, body);
    c.fillRoundRect(zx + zw * 0.62f, fy + zh * 0.16f, zw * 0.2f, zh * 0.66f, 2, mid);
    // etykieta: pasek w 4 kolorach enkoderow + linie do pisania
    float lx = fx + s * 0.12f, ly = fy + s * 0.46f, lw = s * 0.76f, lh = s * 0.54f - 6;
    c.strokeRoundRect(lx, ly, lw, lh, 4, 1.4f, body);
    for (int i = 0; i < 4; i++) c.fillRect(lx + 6 + i * (lw - 12) / 4.f, ly + 7, (lw - 12) / 4.f, 4, pal::enc[i]);
    for (int i = 0; i < 3; i++) c.line(lx + 8, ly + 24 + i * 12, lx + lw - 8, ly + 24 + i * 12, 1.f, mid);
    // otwory zabezpieczenia
    c.strokeRoundRect(fx + 5, fy + s - 15, 8, 8, 1.5f, 1.1f, body);
    c.fillRoundRect(fx + s - 13, fy + s - 15, 8, 8, 1.5f, body);
}

}  // namespace dp
