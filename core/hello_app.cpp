#include "hello_app.h"
#include "font.h"
#include <math.h>
#include <stdio.h>

namespace dp {

// Tryb KEYS: D-pad + ABXY = 8 stopni skali (kolejnosc: lewo, dol, prawo, gora, Y, B, A, X).
static const int kNoteBtn[8] = {B_LEFT, B_DOWN, B_RIGHT, B_UP, B_Y, B_B, B_A, B_X};
static const int kMajor[8] = {0, 2, 4, 5, 7, 9, 11, 12};
static const char* kNoteNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
static const char* kEncNames[4] = {"TUNE", "VOL", "PAN", "DRIVE"};

void HelloApp::init(ToneSynth* s) {
    syn_ = s;
    for (int i = 0; i < TP_COUNT; i++) sent_[i] = -12345.f;
}

void HelloApp::send(int p, float v) {
    if (sent_[p] == v) return;
    if (syn_ && syn_->send((uint8_t)p, v)) sent_[p] = v;
}

void HelloApp::update(const InputState& in, float dt) {
    auto pressed = [&](int b) { return in.btn[b] && !prev_[b]; };

    if (pressed(B_L1) && octave_ > 1) octave_--;
    if (pressed(B_R1) && octave_ < 7) octave_++;
    if (pressed(B_START)) drone_ = !drone_;
    if (in.btn[B_MENU] && in.btn[B_SELECT]) quit_ = true;

    for (int i = 0; i < 8; i++)
        if (pressed(kNoteBtn[i])) cur_ = i;
    if (cur_ >= 0 && !in.btn[kNoteBtn[cur_]]) {
        cur_ = -1;
        for (int i = 7; i >= 0; i--)
            if (in.btn[kNoteBtn[i]]) { cur_ = i; break; }
    }

    // Analogi jako enkodery wzgledne: wychylenie = predkosc zmiany.
    static const int axisOf[4] = {AX_LX, AX_LY, AX_RX, AX_RY};
    static const float sign[4] = {1.f, -1.f, 1.f, -1.f};
    for (int e = 0; e < 4; e++) {
        float a = in.axis[axisOf[e]] * sign[e];
        float m = fabsf(a);
        const float dz = 0.15f;
        if (m < dz) continue;
        float v = (m - dz) / (1.f - dz);
        v = v * v * (a < 0 ? -1.f : 1.f);
        enc_[e] += v * dt * 0.8f;
        if (enc_[e] < 0.f) enc_[e] = 0.f;
        if (enc_[e] > 1.f) enc_[e] = 1.f;
    }

    int tune = (int)lrintf((enc_[0] - 0.5f) * 24.f);
    bool gate = cur_ >= 0 || drone_;
    if (cur_ >= 0) midi_ = 12 * (octave_ + 1) + kMajor[cur_] + tune;
    else if (drone_) midi_ = 69 + tune;
    float freq = 440.f * powf(2.f, (midi_ - 69) / 12.f);

    send(TP_FREQ, freq);
    send(TP_GATE, gate ? 1.f : 0.f);
    send(TP_VOL, enc_[1]);
    send(TP_PAN, enc_[2] * 2.f - 1.f);
    send(TP_DRIVE, enc_[3]);

    for (int i = 0; i < BTN_COUNT; i++) prev_[i] = in.btn[i];
}

static void knob(Canvas& c, float cx, float cy, float r, float v, Color col, const char* val, const char* label) {
    const float a0 = 0.75f * 3.14159265f, a1 = 2.25f * 3.14159265f;
    c.arc(cx, cy, r, a0, a1, 1.5f, pal::faint);
    float av = a0 + (a1 - a0) * v;
    if (v > 0.002f) c.arc(cx, cy, r, a0, av, 2.5f, col);
    float px = cx + cosf(av) * r, py = cy + sinf(av) * r;
    c.fillCircle(px, py, 4.5f, col);
    c.line(cx + cosf(av) * (r - 8), cy + sinf(av) * (r - 8), cx + cosf(av) * (r - 14),
           cy + sinf(av) * (r - 14), 1.5f, col);
    drawText(c, cx, cy - 8, val, 16, 1.5f, pal::white, ALIGN_CENTER);
    drawText(c, cx, cy + r + 14, label, 11, 1.3f, col, ALIGN_CENTER);
}

void HelloApp::render(Canvas& c, const InputState& in, const AudioInfo& ai, float fps) {
    char buf[128];
    c.clear(pal::bg);

    // Naglowek
    drawText(c, 20, 14, "DAW-POL 3000", 13, 1.5f, pal::white);
    snprintf(buf, sizeof buf, "STAGE 0  %2.0f FPS", fps);
    drawText(c, 620, 14, buf, 11, 1.3f, pal::dim, ALIGN_RIGHT);
    c.line(20, 40, 620, 40, 1.f, pal::faint);

    // Duza czestotliwosc + nazwa nuty
    bool gate = sent_[TP_GATE] > 0.5f;
    float freq = 440.f * powf(2.f, (midi_ - 69) / 12.f);
    snprintf(buf, sizeof buf, "%.1f", freq);
    Color fc = gate ? pal::white : pal::dim;
    float fw = drawText(c, 20, 60, buf, 54, 1.8f, fc);
    drawText(c, 20 + fw + 12, 102, "HZ", 12, 1.3f, pal::dim);
    snprintf(buf, sizeof buf, "%s%d", kNoteNames[((midi_ % 12) + 12) % 12], midi_ / 12 - 1);
    drawText(c, 620, 60, buf, 30, 1.8f, gate ? pal::blue : pal::dim, ALIGN_RIGHT);
    snprintf(buf, sizeof buf, "OCTAVE %d%s", octave_, drone_ ? "  DRONE" : "");
    drawText(c, 620, 102, buf, 11, 1.3f, pal::dim, ALIGN_RIGHT);

    // 4 enkodery
    char v0[16], v1[16], v2[16], v3[16];
    int tune = (int)lrintf((enc_[0] - 0.5f) * 24.f);
    snprintf(v0, sizeof v0, "%+d", tune);
    snprintf(v1, sizeof v1, "%d", (int)lrintf(enc_[1] * 100));
    int pan = (int)lrintf((enc_[2] * 2 - 1) * 50);
    if (pan == 0) snprintf(v2, sizeof v2, "C");
    else snprintf(v2, sizeof v2, "%c%d", pan < 0 ? 'L' : 'R', pan < 0 ? -pan : pan);
    snprintf(v3, sizeof v3, "%d", (int)lrintf(enc_[3] * 100));
    const char* vals[4] = {v0, v1, v2, v3};
    for (int i = 0; i < 4; i++) knob(c, 92 + i * 152, 190, 40, enc_[i], pal::enc[i], vals[i], kEncNames[i]);

    // Oscyloskop (z wyzwalaniem na zboczu)
    const float sy = 300, sh = 34;
    c.line(20, sy, 620, sy, 1.f, pal::faint);
    float sc[1024];
    if (syn_) syn_->copyScope(sc, 1024);
    else for (int i = 0; i < 1024; i++) sc[i] = 0;
    int start = 0;
    for (int i = 1; i < 512; i++)
        if (sc[i - 1] <= 0.f && sc[i] > 0.f) { start = i; break; }
    float pts[2 * 300];
    for (int i = 0; i < 300; i++) {
        pts[i * 2] = 20 + i * 2.f;
        pts[i * 2 + 1] = sy - sc[start + i] * sh;
    }
    c.polyline(pts, 300, 1.5f, pal::green);

    // Przyciski
    static const char* lab[BTN_COUNT] = {"UP", "DN", "LT", "RT", "A", "B", "X", "Y",
                                         "L1", "R1", "L2", "R2", "SEL", "STA", "MEN"};
    for (int i = 0; i < BTN_COUNT; i++) {
        int row = i < 8 ? 0 : 1, col = i < 8 ? i : i - 8;
        float x = 40 + col * 46, y = 360 + row * 34;
        bool on = in.btn[i];
        Color cc = pal::enc[i % 4];
        drawText(c, x, y, lab[i], 11, 1.3f, on ? pal::white : pal::dim, ALIGN_CENTER);
        if (on) c.line(x - 10, y + 18, x + 10, y + 18, 2.f, cc);
    }

    // Galki analogowe
    for (int s = 0; s < 2; s++) {
        float cx = 450 + s * 120, cy = 385, r = 26;
        Color rc = s == 0 ? pal::blue : pal::red;
        c.circle(cx, cy, r, 1.2f, rc);
        c.line(cx - r, cy, cx + r, cy, 1.f, pal::faint);
        c.line(cx, cy - r, cx, cy + r, 1.f, pal::faint);
        float ax = in.axis[s * 2], ay = in.axis[s * 2 + 1];
        c.fillCircle(cx + ax * (r - 4), cy + ay * (r - 4), 4.f, pal::white);
        snprintf(buf, sizeof buf, "%+.2f %+.2f", ax, ay);
        drawText(c, cx, cy + r + 10, buf, 9, 1.1f, pal::dim, ALIGN_CENTER);
    }

    // Stopka: audio + surowe zdarzenia
    c.line(20, 438, 620, 438, 1.f, pal::faint);
    if (ai.ok) {
        float ms = ai.rate ? 1000.f * ai.frames / ai.rate : 0;
        snprintf(buf, sizeof buf, "AUDIO %s %d HZ %d FR %.1f MS", ai.driver, ai.rate, ai.frames, ms);
    } else {
        snprintf(buf, sizeof buf, "AUDIO: %s", ai.err[0] ? ai.err : "NONE");
    }
    drawText(c, 20, 448, buf, 9, 1.1f, ai.ok ? pal::dim : pal::red);
    drawText(c, 20, 464, in.pad, 9, 1.1f, pal::dim);
    drawText(c, 620, 448, in.raw[0][0] ? in.raw[0] : "RAW: -", 9, 1.1f, pal::green, ALIGN_RIGHT);
    drawText(c, 620, 464, in.raw[1], 9, 1.1f, pal::dim, ALIGN_RIGHT);
}

}  // namespace dp
