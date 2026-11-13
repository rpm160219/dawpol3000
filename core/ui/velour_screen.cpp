// Rysowanie ekranu VELOUR (zob. velour_screen.h). Scena 320x145 w skali ekranu, cienkie linie jak ekrany op1r.
#include "ui/velour_screen.h"
#include <math.h>
#include "gfx/font.h"
#include "synth/velour_dsp.h"

namespace dp {

namespace {
const float PI = 3.14159265f;
struct V {
    Canvas& c; float ox, oy, s;
    void ln(float x0, float y0, float x1, float y1, float w, Color k) const { c.line(ox + x0 * s, oy + y0 * s, ox + x1 * s, oy + y1 * s, w * s * .8f, k); }
    void poly(const float* p, int n, float w, Color k, bool closed = false) const {
        float q[2 * 80];
        n = n > 80 ? 80 : n;
        for (int i = 0; i < n; i++) { q[2 * i] = ox + p[2 * i] * s; q[2 * i + 1] = oy + p[2 * i + 1] * s; }
        c.polyline(q, n, w * s * .8f, k, closed);
    }
    void ci(float x, float y, float r, float w, Color k) const { c.circle(ox + x * s, oy + y * s, r * s, w * s * .8f, k); }
    void fci(float x, float y, float r, Color k) const { c.fillCircle(ox + x * s, oy + y * s, r * s, k); }
    void ar(float x, float y, float r, float a0, float a1, float w, Color k) const { c.arc(ox + x * s, oy + y * s, r * s, a0, a1, w * s * .8f, k); }
    void fpoly(const float* p, int n, Color k) const {
        float q[2 * 80];
        n = n > 80 ? 80 : n;
        for (int i = 0; i < n; i++) { q[2 * i] = ox + p[2 * i] * s; q[2 * i + 1] = oy + p[2 * i + 1] * s; }
        c.fillPolygon(q, n, k);
    }
};
inline Color mix(Color a, Color b, float t) {
    t = t < 0 ? 0 : (t > 1 ? 1 : t);
    return rgb((uint8_t)(a.r + (b.r - a.r) * t), (uint8_t)(a.g + (b.g - a.g) * t), (uint8_t)(a.b + (b.b - a.b) * t));
}
const Color OFF = rgb(52, 54, 62), FLOOR = rgb(60, 52, 70), CURT = rgb(150, 40, 60);

// instrument smyczkowy: korpus "osemka", szyjka, slimak, 4 struny drgajace (amp), otwory f; h = wysokosc
void bowed(const V& v, float cx, float base, float h, Color col, float amp, float ph, bool endpin) {
    const float y0 = base - .06f * h, y1 = base - .66f * h;
    float pts[2 * 42];
    int n = 0;
    for (int side = 0; side < 2; side++)
        for (int i = 0; i <= 20; i++) {
            const float u = side ? 1.f - i / 20.f : i / 20.f;
            const float w = h * (.035f + .16f * expf(-powf((u - .26f) / .2f, 2)) + .125f * expf(-powf((u - .8f) / .17f, 2)));
            pts[2 * n] = cx + (side ? w : -w);
            pts[2 * n + 1] = y0 + (y1 - y0) * u;
            n++;
        }
    v.fpoly(pts, n, rgb(10, 10, 12));
    v.poly(pts, n, 1.f, col, true);
    // szyjka, slimak, podstrunnica
    v.ln(cx - .02f * h, y1, cx - .02f * h, base - .97f * h, .8f, col);
    v.ln(cx + .02f * h, y1, cx + .02f * h, base - .97f * h, .8f, col);
    v.ci(cx, base - h, .03f * h, .8f, col);
    // otwory f
    v.ar(cx - .07f * h, base - .36f * h, .04f * h, -1.2f, 1.2f, .6f, mix(col, OFF, .4f));
    v.ar(cx + .07f * h, base - .36f * h, .04f * h, PI - 1.2f, PI + 1.2f, .6f, mix(col, OFF, .4f));
    // mostek i struny (drgaja w srodku dlugosci)
    v.ln(cx - .06f * h, base - .3f * h, cx + .06f * h, base - .3f * h, .8f, col);
    for (int k = 0; k < 4; k++) {
        const float xs = cx + (k - 1.5f) * .018f * h;
        float sp[2 * 11];
        for (int i = 0; i <= 10; i++) {
            const float u = i / 10.f, y = (base - .14f * h) + ((base - .95f * h) - (base - .14f * h)) * u;
            sp[2 * i] = xs + amp * sinf(PI * u) * sinf(ph * (1.f + .3f * k) + k);
            sp[2 * i + 1] = y;
        }
        v.poly(sp, 11, .5f, mix(col, rgb(255, 255, 255), .3f));
    }
    if (endpin) v.ln(cx, y0, cx, base + 2, .8f, col);
}

// fale dzwieku z czary (trabka, rog)
void waves(const V& v, float x, float y, float dir, float a, float ph, Color col) {
    if (a <= .02f) return;
    for (int k = 0; k < 3; k++) {
        const float t = fmodf(ph + k / 3.f, 1.f), r = 5.f + 16.f * t;
        v.ar(x, y, r, dir - .5f, dir + .5f, .7f, mix(rgb(10, 10, 12), col, a * (1.f - t)));
    }
}

void trumpet(const V& v, float cx, float cy, float L, Color col) {
    const float x0 = cx - L * .5f, x1 = cx + L * .28f;
    v.ln(x0, cy, x1, cy, 1.f, col);                    // rura
    v.ln(x0 + L * .12f, cy + L * .12f, x1, cy + L * .12f, 1.f, col);
    v.ar(x0 + L * .12f, cy + L * .06f, L * .06f, PI * .5f, PI * 1.5f, 1.f, col);
    v.ar(x1, cy + L * .06f, L * .06f, -PI * .5f, PI * .5f, 1.f, col);
    for (int k = 0; k < 3; k++) {                      // wentyle
        const float x = cx - L * .12f + k * L * .1f;
        v.ln(x, cy - L * .1f, x, cy + L * .14f, 1.f, col);
        v.ci(x, cy - L * .11f, L * .02f, .8f, col);
    }
    float b[2 * 9];                                    // czara
    for (int i = 0; i <= 8; i++) { const float u = i / 8.f; b[2 * i] = x1 + u * L * .22f; b[2 * i + 1] = cy - L * (.01f + .16f * u * u); }
    v.poly(b, 9, 1.f, col);
    for (int i = 0; i <= 8; i++) b[2 * i + 1] = cy + L * (.01f + .16f * (i / 8.f) * (i / 8.f));
    v.poly(b, 9, 1.f, col);
    v.ln(x1 + L * .22f, cy - L * .17f, x1 + L * .22f, cy + L * .17f, 1.f, col);
    v.ln(x0 - 2, cy - 1, x0 - 2, cy + 1, 1.f, col);    // ustnik
}

void horn(const V& v, float cx, float cy, float r, Color col) {
    v.ci(cx, cy, r, 1.f, col);
    v.ci(cx, cy, r * .62f, .8f, col);
    v.ci(cx, cy, r * .3f, .6f, mix(col, OFF, .5f));
    float b[2 * 10];                                   // czara w prawo i w gore
    for (int i = 0; i <= 9; i++) {
        const float u = i / 9.f, a = -.3f - .9f * u, rr = r * (1.f + .5f * u);
        b[2 * i] = cx + rr * cosf(a); b[2 * i + 1] = cy + rr * sinf(a) - r * .1f * u;
    }
    v.poly(b, 10, 1.f, col);
    const float bx = b[18], by = b[19];
    v.ci(bx, by, r * .38f, 1.f, col);
    v.ln(cx - r, cy, cx - r * 1.4f, cy + r * .5f, 1.f, col);  // rurka ustnika
    for (int k = 0; k < 3; k++) v.ci(cx - r * .2f + k * r * .2f, cy - r * .8f, r * .08f, .7f, col);  // wentyle
}

void scene(const V& v, const int16_t* p, float env, float bass, float trem, float chor, float phas, float vib, float wave, bool live) {
    auto lv = [&](int i) { return velourWordToFloat(p[i]); };
    const bool ens = lv(VL_ENSEMBLE) > 0.f, pha = lv(VL_PHASER) > 0.f;
    // podloga sceny i rampa
    v.ln(8, 128, 312, 128, 1.f, FLOOR);
    for (int k = 0; k < 12; k++) v.ln(20 + k * 25.f, 128, 8 + k * 27.f, 140, .5f, rgb(36, 32, 44));
    v.ln(0, 140, 320, 140, .8f, FLOOR);
    // reflektory = 3 linie ensemble (kolysanie: chorus, migotanie: tremolo)
    const float td = lv(VL_TREM_DEPTH), cd = lv(VL_CHORUS_DEPTH);
    for (int k = 0; k < 3; k++) {
        const float ph3 = k * 2.f * PI / 3.f;
        const float sway = ens ? .35f * cd * sinf(chor + ph3) : 0.f;
        const float fl = ens ? .5f + .5f * sinf(trem + ph3) : .5f;
        const float br = (live ? .18f + .5f * env : .12f) * (1.f - td * .6f + td * .6f * fl);
        const float lx = 70 + k * 90.f, ly = 14;
        const float a = PI * .5f + sway;
        const float tx = lx + 110 * cosf(a), ty = ly + 110 * sinf(a);
        const float nx = -sinf(a), ny = cosf(a);
        float beam[8] = {lx - 3 * nx, ly - 3 * ny, lx + 3 * nx, ly + 3 * ny, tx + 22 * nx, ty + 22 * ny, tx - 22 * nx, ty - 22 * ny};
        v.fpoly(beam, 4, mix(rgb(0, 0, 0), rgb(255, 236, 190), br * .35f));
        v.ln(lx - 3 * nx, ly - 3 * ny, tx - 22 * nx, ty - 22 * ny, .4f, mix(rgb(0, 0, 0), rgb(255, 236, 190), br));
        v.ln(lx + 3 * nx, ly + 3 * ny, tx + 22 * nx, ty + 22 * ny, .4f, mix(rgb(0, 0, 0), rgb(255, 236, 190), br));
        v.ci(lx, ly, 4, 1.f, rgb(200, 200, 210));
        v.fci(lx, ly, 2.5f, mix(rgb(40, 40, 40), rgb(255, 236, 190), br * 1.6f));
        v.ln(lx, ly - 4, lx, 4, .8f, rgb(120, 120, 130));
    }
    v.ln(30, 4, 290, 4, 1.f, rgb(120, 120, 130));  // rampa oswietleniowa
    // kula dyskotekowa = phaser
    if (pha) {
        const float bx = 160, by = 34, r = 9;
        v.ln(bx, 4, bx, by - r, .7f, rgb(120, 120, 130));
        v.ci(bx, by, r, 1.f, rgb(210, 210, 220));
        for (int k = 0; k < 6; k++) {
            const float a = phas + k * PI / 3.f, x = bx + r * .9f * cosf(a);
            if (cosf(a + PI * .5f) > 0) v.ln(x, by - r * .85f, x, by + r * .85f, .5f, rgb(150, 150, 160));
        }
        v.ln(bx - r, by, bx + r, by, .5f, rgb(150, 150, 160));
        const float colr = lv(VL_PHASER_COLOR);
        for (int k = 0; k < 8; k++) {  // plamki na scenie
            const float a = phas * 1.3f + k * .8f;
            const float sx = 160 + 130 * sinf(a), sy = 70 + 50 * fabsf(cosf(a * .7f));
            v.fci(sx, sy, .8f + 1.2f * colr, mix(rgb(0, 0, 0), pal::purple, .4f + .6f * colr));
        }
    }
    // instrumenty (statywy)
    struct In { int reg; float cx; Color col; };
    static const In ins[6] = {{VL_CONTRABASS, 44, pal::blue}, {VL_CELLO, 92, pal::green}, {VL_VIOLA, 134, pal::white},
                              {VL_VIOLIN, 170, pal::red}, {VL_TRUMPET, 222, pal::blue}, {VL_HORN, 278, pal::green}};
    for (const In& in : ins) {
        const float l = lv(in.reg);
        const Color col = l > 0.f ? mix(OFF, in.col, .35f + .65f * l) : OFF;
        const float e = in.reg <= VL_CELLO ? bass : env;
        const float amp = live && l > 0.f ? (in.reg <= VL_CELLO ? 2.2f : 1.4f) * e * (.4f + .6f * l) : 0.f;
        if (in.reg == VL_CONTRABASS) bowed(v, in.cx, 126, 92, col, amp, vib, true);
        else if (in.reg == VL_CELLO) bowed(v, in.cx, 126, 70, col, amp, vib, true);
        else if (in.reg == VL_VIOLA || in.reg == VL_VIOLIN) {
            const float h = in.reg == VL_VIOLA ? 46 : 40;
            v.ln(in.cx, 126, in.cx, 116, .8f, OFF);                 // statyw
            v.ln(in.cx - 6, 126, in.cx, 116, .6f, OFF); v.ln(in.cx + 6, 126, in.cx, 116, .6f, OFF);
            bowed(v, in.cx, 114, h, col, amp, vib, false);
        } else if (in.reg == VL_TRUMPET) {
            v.ln(in.cx, 126, in.cx, 96, .8f, OFF); v.ln(in.cx - 8, 126, in.cx, 112, .6f, OFF); v.ln(in.cx + 8, 126, in.cx, 112, .6f, OFF);
            trumpet(v, in.cx, 86, 46, col);
            waves(v, in.cx + 26, 86, 0.f, live && l > 0 ? env * l : 0.f, wave, in.col);
        } else {
            v.ln(in.cx, 126, in.cx, 100, .8f, OFF); v.ln(in.cx - 8, 126, in.cx, 112, .6f, OFF); v.ln(in.cx + 8, 126, in.cx, 112, .6f, OFF);
            horn(v, in.cx, 84, 15, col);
            waves(v, in.cx + 20, 64, -.6f, live && l > 0 ? env * l : 0.f, wave, in.col);
        }
    }
    // kurtyna: podniesiona z narastaniem (CRESCENDO), opada z wybrzmieniem (SUSTAIN)
    const float drop = 6.f + 20.f * (1.f - (live ? env : 1.f));
    for (int k = 0; k < 16; k++) {
        const float x = k * 20.f;
        v.ar(x + 10, 0, 10, PI * .1f, PI * .9f, 1.f, CURT);
        v.ln(x, 0, x, drop * (0.6f + .4f * ((k & 1) ? 1.f : .8f)), .8f, mix(rgb(0, 0, 0), CURT, .7f));
    }
    float val[2 * 33];
    for (int i = 0; i <= 32; i++) { val[2 * i] = i * 10.f; val[2 * i + 1] = drop + 2.5f * sinf(i * PI * .5f); }
    v.poly(val, 33, 1.2f, CURT);
}
}  // namespace

void VelourScreen::draw(Canvas& c, const int16_t* p, const ::VelourCore* core, float dt) {
    if (dt <= 0.f || dt > .1f) dt = .016f;
    const float e = velourEnv(core), b = velourBassEnv(core);
    env_ += (e - env_) * (1.f - expf(-dt / .03f));
    bass_ += (b - bass_) * (1.f - expf(-dt / .03f));
    auto lv = [&](int i) { return velourWordToFloat(p[i]); };
    tremPh_ += dt * 2.f * PI * (3.f + 6.f * lv(VL_TREM_RATE)) * .35f;  // widoczne wolniej niz slyszalne
    chorPh_ += dt * 2.f * PI * (.3f + .6f * lv(VL_CHORUS_RATE));
    phasPh_ += dt * 2.f * PI * (.05f * powf(160.f, lv(VL_PHASER_RATE))) * .5f;
    vib_ += dt * 2.f * PI * 7.f;
    wave_ += dt * 1.2f;
    wave_ -= floorf(wave_);
    const V v{c, 0.f, 50.f, 2.f};
    scene(v, p, env_, bass_, tremPh_, chorPh_, phasPh_, vib_, wave_, true);
}

void drawVelourIllustration(Canvas& c, float x, float y, float w, float h) {
    int16_t p[VELOUR_PARAMS];
    velourProgramWords(5, p);  // STRINGS+BRASS
    p[VL_CONTRABASS] = p[VL_CELLO] = p[VL_HORN] = 32767;
    p[VL_PHASER] = 32767;
    const float s = w / 320.f < h / 145.f ? w / 320.f : h / 145.f;
    const V v{c, x + (w - 320.f * s) * .5f, y + (h - 145.f * s) * .5f, s};
    scene(v, p, .7f, .7f, .8f, .5f, .3f, 0.f, .3f, false);
}

}  // namespace dp
