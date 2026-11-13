// Rysowanie ekranow LINEA (zob. linea_screen.h).
#include "ui/linea_screen.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "gfx/font.h"
#include "io/linea_banks.h"
#include "synth/linea_dsp.h"

namespace dp {

namespace {
const float PI = 3.14159265f;
const Color AMBER = rgb(255, 170, 40), AMBER_D = rgb(70, 40, 6), LCD_BG = rgb(24, 14, 2);
inline Color mixc(Color a, Color b, float t) {
    t = t < 0 ? 0 : (t > 1 ? 1 : t);
    return rgb((uint8_t)(a.r + (b.r - a.r) * t), (uint8_t)(a.g + (b.g - a.g) * t), (uint8_t)(a.b + (b.b - a.b) * t));
}
uint32_t hashName(const uint8_t* p) {
    uint32_t h = 2166136261u;
    for (int k = 0; k < 18; k++) { h ^= p[6 * 64 + k]; h *= 16777619u; }
    return h;
}
// czy partial (0..3: U1 U2 L1 L2) jest PCM wg struktury tonu (1 S+S, 2 S*S, 3 P+S, 4 P*S, 5 S*P, 6 P+P, 7 P*P)
bool isPcm(const uint8_t* p, int part) {
    const int s = lineaStructure(p, part >= 2);
    const int k = part & 1;
    if (s == 6 || s == 7) return true;
    if (s == 3 || s == 4) return k == 0;
    if (s == 5) return k == 1;
    return false;
}
bool isRing(const uint8_t* p, int tone) { const int s = lineaStructure(p, tone); return s == 2 || s == 4 || s == 5 || s == 7; }
}  // namespace

void lineaLcd(Canvas& c, float x, float y, float w, float h, const char* l1, const char* l2) {
    c.fillRoundRect(x - 4, y - 4, w + 8, h + 8, 6, rgb(40, 40, 44));
    c.fillRoundRect(x, y, w, h, 3, LCD_BG);
    for (float gx = x + 4; gx < x + w - 2; gx += 7.5f) c.line(gx, y + 2, gx, y + h - 2, 1.f, rgb(30, 18, 4));  // siatka matrycy
    const float sz = h * .26f;
    drawText(c, x + 10, y + h * .17f, l1, sz, 1.3f, AMBER);
    drawText(c, x + 10, y + h * .58f, l2, sz, 1.3f, AMBER);
}

void LineaScreen::drawPlay(Canvas& c, const uint8_t* p, const ::LineaCore* core, int cut, int res, const char* bankLabel, float dt) {
    if (dt <= 0 || dt > .1f) dt = .016f;
    t_ += dt;
    const uint32_t notes = lineaNoteCount(core);
    if (notes != seenNotes_) {  // uderzenie: iskry z partiali PCM, fala z kazdej formy
        seenNotes_ = notes;
        for (int k = 0; k < 4; k++) burst_[k] = 1.f;
    }
    for (int k = 0; k < 4; k++) burst_[k] *= expf(-dt / .45f);
    const float lv = lineaOutLevel(core);
    glow_ += ((lv * 3.f > 1.f ? 1.f : lv * 3.f) - glow_) * (1.f - expf(-dt / .08f));
    const uint32_t h = hashName(p);
    const float cx = 320, cy = 166, hw = 150, hh = 104;
    const uint8_t* pb = p + 6 * 64;
    const float tb = (pb[33] > 100 ? 100 : pb[33]) / 100.f;
    const float bu = (p[2 * 64 + 47] > 100 ? 100 : p[2 * 64 + 47]) / 100.f;
    const float bl = (p[5 * 64 + 47] > 100 ? 100 : p[5 * 64 + 47]) / 100.f;
    const float w[4] = {(1 - bu) * (1 - tb), bu * (1 - tb), (1 - bl) * tb, bl * tb};
    const float cutN = (cut + 100) / 200.f, resN = (res + 30) / 60.f;
    // tlo: gwiazdozbior z nazwy patcha (pozycje i polaczenia z hasha), migocze przy uderzeniu
    float sx[36], sy[36];
    for (int i = 0; i < 36; i++) {
        const uint32_t r = (h ^ (i * 0x9E3779B9u)) * 2246822519u;
        sx[i] = 24 + (r % 592);
        sy[i] = 52 + ((r >> 12) % 232);
    }
    for (int i = 0; i < 36; i++) {
        int best = -1; float bd = 1e9f;
        for (int k = 0; k < 36; k++) {
            if (k == i) continue;
            const float d = (sx[k] - sx[i]) * (sx[k] - sx[i]) + (sy[k] - sy[i]) * (sy[k] - sy[i]);
            if (d < bd) { bd = d; best = k; }
        }
        if (best > i || (h >> (i % 16) & 1)) c.line(sx[i], sy[i], sx[best], sy[best], .8f, rgb(26, 26, 34));
        const float tw = .5f + .5f * sinf(t_ * (1.f + (i % 5) * .4f) + i);
        c.fillCircle(sx[i], sy[i], .9f + .8f * tw * (.3f + burst_[i & 3]), mixc(rgb(40, 40, 50), rgb(160, 160, 190), tw * .5f + .5f * burst_[i & 3]));
    }
    // pole wektora
    c.strokeRoundRect(cx - hw, cy - hh, 2 * hw, 2 * hh, 14, 1.f, rgb(48, 48, 58));
    for (int k = 1; k < 4; k++) {
        c.line(cx - hw + k * hw * .5f, cy - hh + 4, cx - hw + k * hw * .5f, cy + hh - 4, .6f, rgb(30, 30, 38));
        c.line(cx - hw + 4, cy - hh + k * hh * .5f, cx + hw - 4, cy - hh + k * hh * .5f, .6f, rgb(30, 30, 38));
    }
    const float vx = (bu + bl) * .5f, vy = tb;
    const float px = cx - hw + 36 + (2 * hw - 72) * vx, py = cy - hh + 30 + (2 * hh - 60) * vy;
    static const float PX[4] = {-1, 1, -1, 1}, PY[4] = {-1, -1, 1, 1};
    const Color base[4] = {pal::blue, pal::green, pal::white, pal::red};
    static const char* tag[4] = {"U1", "U2", "L1", "L2"};
    float fx[4], fy[4];
    for (int k = 0; k < 4; k++) { fx[k] = cx + PX[k] * (hw - 46); fy[k] = cy + PY[k] * (hh - 38); }
    // linie energii: punkt wektora -> formy (grubosc i jasnosc = waga partiala)
    for (int k = 0; k < 4; k++) {
        const float wk = w[k];
        float pts[2 * 17];
        for (int i = 0; i <= 16; i++) {
            const float u = i / 16.f, wob = sinf(u * PI) * 4.f * glow_ * sinf(t_ * 9.f + i + k);
            pts[2 * i] = px + (fx[k] - px) * u - (fy[k] - py) * .02f * wob;
            pts[2 * i + 1] = py + (fy[k] - py) * u + (fx[k] - px) * .02f * wob;
        }
        c.polyline(pts, 17, .6f + 2.2f * wk, mixc(rgb(20, 20, 26), base[k], .15f + .6f * wk));
    }
    for (int k = 0; k < 4; k++) {
        const uint8_t* q = p + (k < 2 ? k : k + 1) * 64;
        const float wk = .25f + .75f * w[k];
        const float rr = 18.f + 20.f * w[k] + 5.f * burst_[k];
        const Color col = mixc(rgb(30, 30, 36), mixc(base[k], rgb(255, 255, 255), .2f * glow_), .35f + .65f * wk);
        const uint32_t hk = h ^ (0x9E3779B9u * (k + 1));
        // orbita i fala uderzenia
        c.arc(fx[k], fy[k], rr + 10, t_ * (k & 1 ? -.6f : .6f), t_ * (k & 1 ? -.6f : .6f) + 4.5f, .6f, mixc(rgb(20, 20, 26), col, .5f));
        if (burst_[k] > .02f) c.circle(fx[k], fy[k], rr + 10 + 40.f * (1.f - burst_[k]), 1.f, mixc(rgb(0, 0, 0), col, burst_[k]));
        char lab[16];
        if (isPcm(p, k)) {  // PCM: chmura iskier
            const int n = 30 + (int)(hk % 14);
            for (int i = 0; i < n; i++) {
                const uint32_t r1 = (hk + i * 7919u) * 2654435761u;
                const float a = (r1 % 6283) / 1000.f + t_ * (.2f + ((r1 >> 20) % 10) * .03f);
                const float d = rr * (.15f + ((r1 >> 8) % 100) / 115.f) * (1.f + 1.6f * burst_[k] * (((r1 >> 16) % 3) + 1) / 3.f);
                c.fillCircle(fx[k] + d * cosf(a), fy[k] + d * sinf(a) * .85f, .8f + ((r1 >> 4) % 3) * .45f, col);
            }
            snprintf(lab, sizeof lab, "%s %s", tag[k], lineaPcmName(q[7] % 100));
        } else {    // SYNTH: rozeta (SQU platki, SAW zeby), rezonans = ostrosc, cutoff = grubosc i poswiata
            const int petals = 3 + (int)(hk % 5) + (q[6] ? 1 : 0);
            const float sharp = .15f + .6f * (q[14] / 30.f) * (.5f + resN);
            float pts[2 * 97];
            for (int i = 0; i <= 96; i++) {
                const float a = i * 2 * PI / 96 + t_ * (.15f + (hk % 7) * .03f) * (k & 1 ? -1 : 1);
                float m;
                if (q[6]) m = 2.f * (fmodf(petals * a / (2 * PI) + 10.f, 1.f) - .5f);
                else { m = cosf(petals * a + (hk % 100) * .1f); m = m > 0 ? powf(m, 1.f - sharp * .8f) : -powf(-m, 1.f - sharp * .8f); }
                const float r = rr * (.62f + .38f * m * (.4f + sharp));
                pts[2 * i] = fx[k] + r * cosf(a);
                pts[2 * i + 1] = fy[k] + r * sinf(a) * .85f;
            }
            const float lw = 1.f + 1.6f * cutN * wk;
            c.polyline(pts, 97, lw + 5.f, mixc(rgb(0, 0, 0), col, (.08f + .25f * glow_) * cutN), true);  // poswiata
            c.polyline(pts, 97, lw, col, true);
            c.fillCircle(fx[k], fy[k], 2.f, col);
            snprintf(lab, sizeof lab, "%s %s", tag[k], q[6] ? "SAW" : "SQU");
        }
        drawText(c, fx[k], fy[k] + (PY[k] < 0 ? -rr - 22 : rr + 14), lab, 6, 1.f, mixc(rgb(40, 40, 40), col, .85f), ALIGN_CENTER);
    }
    // modulator pierscieniowy: iskrzaca linia miedzy partialami tonu
    for (int tone = 0; tone < 2; tone++) {
        if (!isRing(p, tone)) continue;
        const int a = tone * 2, b = a + 1;
        float pts[2 * 33];
        for (int i = 0; i <= 32; i++) {
            const float u = i / 32.f;
            const float j = (i % 2 ? 1.f : -1.f) * (2.5f + 4.f * glow_) * sinf(PI * u);
            pts[2 * i] = fx[a] + 30 + (fx[b] - fx[a] - 60) * u;
            pts[2 * i + 1] = fy[a] + j;
        }
        c.polyline(pts, 33, 1.f, mixc(rgb(40, 30, 10), AMBER, .5f + .5f * glow_));
        drawText(c, cx, fy[a] - 12, "RING", 6, 1.f, rgb(150, 100, 30), ALIGN_CENTER);
    }
    // punkt wektora
    c.circle(px, py, 9 + 3 * glow_, 1.4f, pal::blue);
    c.circle(px, py, 15 + 8 * glow_, .7f, mixc(rgb(0, 0, 0), pal::blue, .4f));
    c.fillCircle(px, py, 3, pal::white);
    drawText(c, cx - hw + 10, cy - hh + 8, "UPPER", 6, 1.f, rgb(80, 80, 92));
    drawText(c, cx - hw + 10, cy + hh - 14, "LOWER", 6, 1.f, rgb(80, 80, 92));
    // nazwa patcha w LCD
    char nm[19], l2[48];
    lineaPatchName(p, nm);
    snprintf(l2, sizeof l2, "%s  S%d/S%d", bankLabel, lineaStructure(p, 0), lineaStructure(p, 1));
    lineaLcd(c, 140, 286, 360, 44, nm, l2);
}

void drawLineaIllustration(Canvas& c, float x, float y, float w, float h) {
    // statyczna konstelacja: 4 formy w rogach i punkt wektora
    const float cx = x + w * .5f, cy = y + h * .45f, hw = w * .3f, hh = h * .3f;
    const Color base[4] = {pal::blue, pal::green, pal::white, pal::red};
    static const float PX[4] = {-1, 1, -1, 1}, PY[4] = {-1, -1, 1, 1};
    for (int k = 0; k < 4; k++) {
        const float fx = cx + PX[k] * hw, fy = cy + PY[k] * hh, r = h * .13f;
        if (k == 1 || k == 2) {
            for (int i = 0; i < 24; i++) { const float a = i * .9f, d = r * (.3f + (i % 5) * .15f); c.fillCircle(fx + d * cosf(a), fy + d * sinf(a), 1.f, base[k]); }
        } else {
            float pts[2 * 61];
            for (int i = 0; i <= 60; i++) { const float a = i * 2 * PI / 60, m = cosf((4 + k) * a); pts[2 * i] = fx + r * (.65f + .35f * m) * cosf(a); pts[2 * i + 1] = fy + r * (.65f + .35f * m) * sinf(a); }
            c.polyline(pts, 61, 1.2f, base[k], true);
        }
    }
    c.circle(cx, cy, 5, 1.2f, pal::blue);
    c.fillCircle(cx, cy, 2, pal::white);
    lineaLcd(c, x + w * .2f, y + h * .82f, w * .6f, h * .16f, lineaRomReady() ? "LINEA" : "NO ROM", lineaRomReady() ? "LA SYNTHESIS" : "ROMS/LINEA");
}

bool lineaLoadPatch(Track& t, int bank, int slot) {
    const uint8_t* p = lineaBanks().patch(bank, slot);
    if (!p) return false;
    memcpy(t.linea, p, LINEA_PATCH_BYTES);
    snprintf(t.lineaBank, sizeof t.lineaBank, "%s", lineaBanks().name(bank));
    t.lineaSlot = (int16_t)slot;
    t.lineaSet = true;
    static uint32_t idc = 1;
    t.lineaId = ++idc;
    char nm[19];
    lineaPatchName(p, nm);
    snprintf(t.preset, sizeof t.preset, "%s", nm);
    return true;
}

void lineaEnsure(Track& t) {
    if (t.lineaSet) return;
    LineaBanks& b = lineaBanks();
    for (int k = 0; k < b.count(); k++) if (b.patches(k) > 0 && lineaLoadPatch(t, k, 0)) return;
}

}  // namespace dp
