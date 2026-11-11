#include "ui/piano_roll.h"
#include "gfx/font.h"
#include <math.h>
#include <stdio.h>

namespace dp {

// Uklad ekranu
static const float GX = 64, GY = 52, SW = 35, RH = 22;  // siatka: x, y, szer. kroku, wys. wiersza
static const int ROWS = 16;
static const char* kNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
static bool isBlack(int p) { int k = p % 12; return k == 1 || k == 3 || k == 6 || k == 8 || k == 10; }
static void noteName(char* b, int n, int p) { snprintf(b, n, "%s%d", kNames[p % 12], p / 12 - 1); }

static void lenName(char* b, int n, int ticks) {
    int s = ticks / TICKS_PER_STEP, d = 16;
    while (s % 2 == 0 && d > 1) { s /= 2; d /= 2; }
    snprintf(b, n, "%d/%d", s, d);
}

void PianoRoll::init(AudioEngine* e) {
    eng_ = e;
    scrollToCursor();
    if (eng_) {
        eng_->send(EC_TEMPO, 0, tempo_);
        eng_->send(EC_VOLUME, 0, volume_);
    }
    publish();
}

void PianoRoll::loadDemo() {
    pat_.clear();
    const int T = TICKS_PER_STEP;
    struct { int s, l, p, v; } d[] = {
        {0, 4, 60, 110}, {0, 2, 67, 90}, {4, 2, 63, 95}, {6, 2, 67, 90},
        {8, 6, 72, 120}, {12, 2, 74, 80}, {14, 2, 70, 100},
    };
    for (auto& n : d) pat_.add({(uint16_t)(n.s * T), (uint16_t)(n.l * T), (uint8_t)n.p, (uint8_t)n.v});
    publish();
}

void PianoRoll::publish() {
    if (!eng_) return;
    eng_->patternBack() = pat_;
    eng_->publishPattern();
    dirty_ = false;
}

void PianoRoll::scrollToCursor() {
    const int m = 2;  // margines wierszy przy krawedzi
    if (curPitch_ > topPitch_ - m) topPitch_ = curPitch_ + m;
    if (curPitch_ < topPitch_ - ROWS + 1 + m) topPitch_ = curPitch_ + ROWS - 1 - m;
    if (topPitch_ > 127) topPitch_ = 127;
    if (topPitch_ < ROWS - 1) topPitch_ = ROWS - 1;
}

void PianoRoll::preview(int pitch, int vel) {
    if (!eng_ || eng_->isPlaying()) return;
    if (previewPitch_ >= 0) eng_->send(EC_NOTE_OFF, previewPitch_);
    eng_->send(EC_NOTE_ON, pitch, vel / 127.f);
    previewPitch_ = pitch;
    previewLeft_ = 0.25f;
}

void PianoRoll::setLen(int idx, int len) {
    Note& n = pat_.notes[idx];
    int mx = pat_.maxLen(n.start, n.pitch, idx);
    if (len < TICKS_PER_STEP) len = TICKS_PER_STEP;
    if (len > mx) len = mx;
    if (len != n.len) { n.len = (uint16_t)len; dirty_ = true; }
    lastLen_ = len;
}

void PianoRoll::update(const InputState& in, float dt) {
    btn_.update(in, dt);
    if (in.btn[B_MENU] && in.btn[B_SELECT]) quit_ = true;

    if (previewPitch_ >= 0 && (previewLeft_ -= dt) <= 0.f) {
        if (eng_) eng_->send(EC_NOTE_OFF, previewPitch_);
        previewPitch_ = -1;
    }

    int sel = noteAtCursor();
    bool xHeld = btn_.down(B_X), yHeld = btn_.down(B_Y);

    if (xHeld && sel >= 0) {
        // X + lewo/prawo = dlugosc
        if (btn_.repeat(B_RIGHT)) setLen(sel, pat_.notes[sel].len + TICKS_PER_STEP);
        if (btn_.repeat(B_LEFT)) setLen(sel, pat_.notes[sel].len - TICKS_PER_STEP);
    } else if (yHeld && sel >= 0) {
        // Y + D-pad = przesun nute
        Note n = pat_.notes[sel];
        int ds = 0, dp = 0;
        if (btn_.repeat(B_RIGHT)) ds = 1;
        if (btn_.repeat(B_LEFT)) ds = -1;
        if (btn_.repeat(B_UP)) dp = 1;
        if (btn_.repeat(B_DOWN)) dp = -1;
        if (ds || dp) {
            int ns = n.start / TICKS_PER_STEP + ds, np = n.pitch + dp;
            if (ns >= 0 && ns < STEPS_PER_BAR && np >= 0 && np <= 127) {
                pat_.remove(sel);
                Note m = n;
                m.start = (uint16_t)(ns * TICKS_PER_STEP);
                m.pitch = (uint8_t)np;
                if (pat_.add(m) >= 0) {
                    curStep_ = ns; curPitch_ = np;
                    if (dp) preview(np, n.vel);
                } else pat_.add(n);  // kolizja — cofnij
                dirty_ = true;
                scrollToCursor();
            }
        }
    } else if (!xHeld && !yHeld) {
        // Ruch kursora
        if (btn_.repeat(B_RIGHT) && curStep_ < STEPS_PER_BAR - 1) curStep_++;
        if (btn_.repeat(B_LEFT) && curStep_ > 0) curStep_--;
        if (btn_.repeat(B_UP) && curPitch_ < 127) curPitch_++;
        if (btn_.repeat(B_DOWN) && curPitch_ > 0) curPitch_--;
    }
    if (btn_.pressed(B_L1)) curPitch_ = curPitch_ >= 12 ? curPitch_ - 12 : curPitch_;
    if (btn_.pressed(B_R1)) curPitch_ = curPitch_ <= 115 ? curPitch_ + 12 : curPitch_;
    scrollToCursor();
    sel = noteAtCursor();

    if (btn_.pressed(B_A)) {
        if (sel < 0) {
            int idx = pat_.add({(uint16_t)(curStep_ * TICKS_PER_STEP), (uint16_t)lastLen_, (uint8_t)curPitch_, (uint8_t)lastVel_});
            if (idx >= 0) { dirty_ = true; preview(curPitch_, lastVel_); }
        } else preview(pat_.notes[sel].pitch, pat_.notes[sel].vel);
        sel = noteAtCursor();
    }
    if (btn_.pressed(B_B) && sel >= 0) { pat_.remove(sel); dirty_ = true; sel = -1; }

    if (btn_.pressed(B_START) && eng_) {
        if (previewPitch_ >= 0) { eng_->send(EC_NOTE_OFF, previewPitch_); previewPitch_ = -1; }
        eng_->send(eng_->isPlaying() ? EC_STOP : EC_PLAY);
    }

    // Enkodery
    int dl = encLen_.steps(in.axis[AX_LX], dt, 8.f);
    if (dl) {
        if (sel >= 0) setLen(sel, pat_.notes[sel].len + dl * TICKS_PER_STEP);
        else {
            lastLen_ += dl * TICKS_PER_STEP;
            if (lastLen_ < TICKS_PER_STEP) lastLen_ = TICKS_PER_STEP;
            if (lastLen_ > pat_.lengthTicks) lastLen_ = pat_.lengthTicks;
        }
    }
    int dv = encVel_.steps(-in.axis[AX_LY], dt, 60.f);
    if (dv) {
        int v = (sel >= 0 ? pat_.notes[sel].vel : lastVel_) + dv;
        v = v < 1 ? 1 : (v > 127 ? 127 : v);
        if (sel >= 0) { pat_.notes[sel].vel = (uint8_t)v; dirty_ = true; }
        lastVel_ = v;
    }
    float vol = volume_ + RelEncoder::delta(in.axis[AX_RX], dt, 0.8f);
    vol = vol < 0 ? 0 : (vol > 1 ? 1 : vol);
    if (vol != volume_) { volume_ = vol; if (eng_) eng_->send(EC_VOLUME, 0, volume_); }
    int dtmp = encTempo_.steps(-in.axis[AX_RY], dt, 25.f);
    if (dtmp) {
        tempo_ += dtmp;
        tempo_ = tempo_ < 40 ? 40 : (tempo_ > 240 ? 240 : tempo_);
        if (eng_) eng_->send(EC_TEMPO, 0, tempo_);
    }

    if (dirty_) publish();
}

static void miniKnob(Canvas& c, float x, float y, float v, Color col, const char* label, const char* val) {
    const float a0 = 0.75f * 3.14159265f, a1 = 2.25f * 3.14159265f, r = 14;
    float cx = x + 16, cy = y + 18;
    c.arc(cx, cy, r, a0, a1, 1.2f, pal::faint);
    float av = a0 + (a1 - a0) * (v < 0 ? 0 : (v > 1 ? 1 : v));
    if (v > 0.002f) c.arc(cx, cy, r, a0, av, 2.f, col);
    c.fillCircle(cx + cosf(av) * r, cy + sinf(av) * r, 3.f, col);
    drawText(c, x + 42, y + 2, label, 9, 1.1f, col);
    drawText(c, x + 42, y + 18, val, 16, 1.5f, pal::white);
}

void PianoRoll::render(Canvas& c) {
    char b[64];
    c.clear(pal::bg);
    bool playing = eng_ && eng_->isPlaying();
    int sel = noteAtCursor();

    // Naglowek
    drawText(c, 20, 13, "PATTERN 01", 13, 1.5f, pal::white);
    char nn[16];
    noteName(nn, sizeof nn, curPitch_);
    snprintf(b, sizeof b, "%s  %d.%d", nn, curStep_ / 4 + 1, curStep_ % 4 + 1);
    drawText(c, 320, 15, b, 11, 1.3f, pal::dim, ALIGN_CENTER);
    snprintf(b, sizeof b, "%.0f BPM", tempo_);
    float tw = drawText(c, 620, 15, b, 11, 1.3f, pal::dim, ALIGN_RIGHT);
    float ix = 620 - tw - 22;
    if (playing) {
        float tri[6] = {ix, 13.f, ix + 11, 19.5f, ix, 26.f};
        c.polyline(tri, 3, 1.5f, pal::green, true);
    } else c.rect(ix, 14, 11, 11, 1.5f, pal::dim);
    c.line(20, 36, 620, 36, 1.f, pal::faint);

    // Numery miarowe
    for (int beat = 0; beat < 4; beat++) {
        snprintf(b, sizeof b, "%d", beat + 1);
        drawText(c, GX + beat * 4 * SW + 3, 40, b, 8, 1.1f, pal::dim);
    }

    const float GW = SW * STEPS_PER_BAR, GH = RH * ROWS;
    // Wiersze (czarne klawisze ciemniejsze tlo) + nazwy C
    for (int r = 0; r < ROWS; r++) {
        int p = topPitch_ - r;
        float y = GY + r * RH;
        if (isBlack(p)) c.fillRect(GX, y, GW, RH, rgb(14, 14, 16));
        if (p == curPitch_) c.fillRect(GX, y, GW, RH, rgb(24, 24, 24));
        if (p % 12 == 0 || p == curPitch_) {
            noteName(nn, sizeof nn, p);
            drawText(c, GX - 8, y + RH * 0.5f - 4.5f, nn, 9, 1.1f, p == curPitch_ ? pal::white : pal::dim, ALIGN_RIGHT);
        }
        if (p % 12 == 0) c.line(GX, y + RH, GX + GW, y + RH, 1.f, rgb(48, 48, 48));
    }
    // Pionowe linie: kroki, miary, takt
    for (int s = 0; s <= STEPS_PER_BAR; s++) {
        float x = GX + s * SW;
        Color col = (s % 4 == 0) ? rgb(64, 64, 64) : rgb(26, 26, 26);
        c.line(x, GY, x, GY + GH, 1.f, col);
    }
    c.line(GX, GY, GX + GW, GY, 1.f, rgb(64, 64, 64));
    c.line(GX, GY + GH, GX + GW, GY + GH, 1.f, rgb(64, 64, 64));

    // Nuty
    for (int i = 0; i < pat_.count; i++) {
        const Note& n = pat_.notes[i];
        int r = topPitch_ - n.pitch;
        if (r < 0 || r >= ROWS) continue;
        float x = GX + (float)n.start / TICKS_PER_STEP * SW + 2;
        float w = (float)n.len / TICKS_PER_STEP * SW - 4;
        float y = GY + r * RH + 3, h = RH - 6;
        bool isSel = i == sel;
        Color base = isSel ? pal::blue : pal::white;
        Color fill = base;
        fill.a = (uint8_t)(24 + n.vel * 0.6f);  // jasnosc = velocity
        c.fillRect(x, y, w, h, fill);
        c.rect(x, y, w, h, isSel ? 1.6f : 1.1f, base);
    }

    // Kursor (narozniki)
    {
        float x = GX + curStep_ * SW, y = GY + (topPitch_ - curPitch_) * RH, k = 6;
        Color cc = pal::green;
        float pts[][4] = {{x, y + k, x, y}, {x, y, x + k, y}, {x + SW - k, y, x + SW, y}, {x + SW, y, x + SW, y + k},
                          {x, y + RH - k, x, y + RH}, {x, y + RH, x + k, y + RH},
                          {x + SW - k, y + RH, x + SW, y + RH}, {x + SW, y + RH, x + SW, y + RH - k}};
        c.beginStroke();
        for (auto& q : pts) c.strokeSeg(q[0], q[1], q[2], q[3], 1.6f);
        c.endStroke(cc);
    }

    // Glowica odtwarzania
    if (playing) {
        float x = GX + (float)eng_->playTick() / TICKS_PER_STEP * SW;
        c.line(x, GY - 4, x, GY + GH + 4, 1.5f, pal::red);
        c.fillCircle(x, GY - 6, 3.f, pal::red);
    }

    // Enkodery
    c.line(20, 418, 620, 418, 1.f, pal::faint);
    int len = sel >= 0 ? pat_.notes[sel].len : lastLen_;
    int vel = sel >= 0 ? pat_.notes[sel].vel : lastVel_;
    char v0[16], v1[16], v2[16], v3[16];
    lenName(v0, sizeof v0, len);
    snprintf(v1, sizeof v1, "%d", vel);
    snprintf(v2, sizeof v2, "%d", (int)lrintf(volume_ * 100));
    snprintf(v3, sizeof v3, "%.0f", tempo_);
    miniKnob(c, 20, 428, (float)len / pat_.lengthTicks, pal::blue, "LENGTH", v0);
    miniKnob(c, 172, 428, vel / 127.f, pal::green, "VELOCITY", v1);
    miniKnob(c, 324, 428, volume_, pal::white, "VOLUME", v2);
    miniKnob(c, 476, 428, (tempo_ - 40) / 200.f, pal::red, "TEMPO", v3);
}

}  // namespace dp
