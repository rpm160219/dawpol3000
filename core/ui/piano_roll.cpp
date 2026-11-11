#include "ui/piano_roll.h"
#include "gfx/font.h"
#include "gfx/icons.h"
#include "ui/app.h"
#include "ui/widgets.h"
#include <math.h>
#include <stdio.h>

namespace dp {

// Uklad ekranu
static const float GX = 66, GY = 52, SW = 35, RH = 22;  // siatka: x, y, szer. kroku, wys. wiersza
static const int ROWS = 16;
static const char* kNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
static bool isBlack(int p) { int k = p % 12; return k == 1 || k == 3 || k == 6 || k == 8 || k == 10; }
static void noteName(char* b, int n, int p) { snprintf(b, n, "%s%d", kNames[p % 12], p / 12 - 1); }

// Kolor nuty wg velocity: 1 = ciemna zielen, 64 = zielen enkodera VELOCITY, 127 = jasna mieta.
static Color velColor(int v) {
    float t = (v - 1) / 126.f;
    Color a = rgb(22, 70, 44), m = pal::green, b = rgb(205, 255, 222);
    Color p = t < 0.5f ? a : m, q = t < 0.5f ? m : b;
    float k = t < 0.5f ? t * 2 : (t - 0.5f) * 2;
    return rgb((uint8_t)(p.r + (q.r - p.r) * k), (uint8_t)(p.g + (q.g - p.g) * k), (uint8_t)(p.b + (q.b - p.b) * k));
}

static void lenName(char* b, int n, int ticks) {
    int s = ticks / TICKS_PER_STEP, d = 16;
    while (s % 2 == 0 && d > 1) { s /= 2; d /= 2; }
    snprintf(b, n, "%d/%d", s, d);
}

Pattern& PianoRoll::pat() { return app_->project().patterns[pat_]; }
int PianoRoll::steps() { return pat().lengthTicks / TICKS_PER_STEP; }
int PianoRoll::noteAtCursor() { return pat().find(curStep_ * TICKS_PER_STEP, curPitch_); }

void PianoRoll::open(int pattern, int track) {
    if (pattern != pat_) { curStep_ = 0; }
    pat_ = pattern;
    track_ = track;
    // ustaw kursor na pierwszej nucie, zeby od razu bylo cos widac
    Pattern& p = pat();
    if (p.count > 0 && noteAtCursor() < 0) { curPitch_ = p.notes[0].pitch; topPitch_ = curPitch_ + 7; }
    if (curStep_ >= steps()) curStep_ = 0;
    scrollToCursor();
}

void PianoRoll::scrollToCursor() {
    const int m = 2;  // margines wierszy przy krawedzi
    if (curPitch_ > topPitch_ - m) topPitch_ = curPitch_ + m;
    if (curPitch_ < topPitch_ - ROWS + 1 + m) topPitch_ = curPitch_ + ROWS - 1 - m;
    if (topPitch_ > 127) topPitch_ = 127;
    if (topPitch_ < ROWS - 1) topPitch_ = ROWS - 1;
}

void PianoRoll::preview(int pitch, int vel) {
    AudioEngine* e = app_->engine();
    if (!e || e->isPlaying()) return;
    stopPreview();
    e->send(EC_NOTE_ON, track_, pitch, 0, vel / 127.f);
    previewPitch_ = pitch;
    previewLeft_ = 0.25f;
}

void PianoRoll::stopPreview() {
    if (previewPitch_ >= 0 && app_->engine()) app_->engine()->send(EC_NOTE_OFF, track_, previewPitch_);
    previewPitch_ = -1;
}

void PianoRoll::setLen(int idx, int len) {
    Pattern& p = pat();
    Note& n = p.notes[idx];
    int mx = p.maxLen(n.start, n.pitch, idx);
    if (len < TICKS_PER_STEP) len = TICKS_PER_STEP;
    if (len > mx) len = mx;
    if (len != n.len) { n.len = (uint16_t)len; app_->markDirty(); }
    lastLen_ = len;
}

void PianoRoll::update(const Buttons& b, const InputState& in, float dt) {
    Project& prj = app_->project();
    Pattern& p = pat();
    AudioEngine* eng = app_->engine();
    if (previewPitch_ >= 0 && (previewLeft_ -= dt) <= 0.f) stopPreview();

    int sel = noteAtCursor();
    bool xHeld = b.down(B_X), yHeld = b.down(B_Y);
    int nsteps = steps();

    if (xHeld && sel >= 0) {
        if (b.repeat(B_RIGHT)) setLen(sel, p.notes[sel].len + TICKS_PER_STEP);
        if (b.repeat(B_LEFT)) setLen(sel, p.notes[sel].len - TICKS_PER_STEP);
    } else if (yHeld && sel >= 0) {
        Note n = p.notes[sel];
        int ds = 0, dpt = 0;
        if (b.repeat(B_RIGHT)) ds = 1;
        if (b.repeat(B_LEFT)) ds = -1;
        if (b.repeat(B_UP)) dpt = 1;
        if (b.repeat(B_DOWN)) dpt = -1;
        if (ds || dpt) {
            int ns = n.start / TICKS_PER_STEP + ds, np = n.pitch + dpt;
            if (ns >= 0 && ns < nsteps && np >= 0 && np <= 127) {
                p.remove(sel);
                Note m = n;
                m.start = (uint16_t)(ns * TICKS_PER_STEP);
                m.pitch = (uint8_t)np;
                if (p.add(m) >= 0) {
                    curStep_ = ns; curPitch_ = np;
                    if (dpt) preview(np, n.vel);
                } else p.add(n);  // kolizja — cofnij
                app_->markDirty();
            }
        }
    } else if (!xHeld && !yHeld) {
        if (b.repeat(B_RIGHT) && curStep_ < nsteps - 1) curStep_++;
        if (b.repeat(B_LEFT) && curStep_ > 0) curStep_--;
        if (b.repeat(B_UP) && curPitch_ < 127) curPitch_++;
        if (b.repeat(B_DOWN) && curPitch_ > 0) curPitch_--;
    }
    if (b.pressed(B_L1) && curPitch_ >= 12) curPitch_ -= 12;
    if (b.pressed(B_R1) && curPitch_ <= 115) curPitch_ += 12;
    scrollToCursor();
    sel = noteAtCursor();

    if (b.pressed(B_A)) {
        if (sel < 0) {
            int idx = p.add({(uint16_t)(curStep_ * TICKS_PER_STEP), (uint16_t)lastLen_, (uint8_t)curPitch_, (uint8_t)lastVel_});
            if (idx >= 0) { app_->markDirty(); preview(curPitch_, lastVel_); }
        } else preview(p.notes[sel].pitch, p.notes[sel].vel);
        sel = noteAtCursor();
    }
    if (b.pressed(B_B) && sel >= 0) { p.remove(sel); app_->markDirty(); sel = -1; }

    if (b.pressed(B_START) && eng) {
        stopPreview();
        if (eng->isPlaying()) eng->send(EC_STOP);
        else eng->send(EC_PLAY_PATTERN, track_, 0, pat_);
    }

    // Enkodery
    int dl = encLen_.steps(in.axis[AX_LX], dt, 8.f);
    if (dl) {
        if (sel >= 0) setLen(sel, p.notes[sel].len + dl * TICKS_PER_STEP);
        else {
            lastLen_ += dl * TICKS_PER_STEP;
            if (lastLen_ < TICKS_PER_STEP) lastLen_ = TICKS_PER_STEP;
            if (lastLen_ > p.lengthTicks) lastLen_ = p.lengthTicks;
        }
    }
    int dv = encVel_.steps(-in.axis[AX_LY], dt, 60.f);
    if (dv) {
        int v = (sel >= 0 ? p.notes[sel].vel : lastVel_) + dv;
        v = v < 1 ? 1 : (v > 127 ? 127 : v);
        if (sel >= 0) { p.notes[sel].vel = (uint8_t)v; app_->markDirty(); }
        lastVel_ = v;
    }
    Track& tr = prj.tracks[track_];
    float vol = tr.volume + RelEncoder::delta(in.axis[AX_RX], dt, 0.8f);
    vol = vol < 0 ? 0 : (vol > 1 ? 1 : vol);
    if (vol != tr.volume) { tr.volume = vol; app_->markDirty(); }
    int dtmp = encTempo_.steps(-in.axis[AX_RY], dt, 25.f);
    if (dtmp) {
        prj.bpm += dtmp;
        prj.bpm = prj.bpm < 40 ? 40 : (prj.bpm > 240 ? 240 : prj.bpm);
        app_->markDirty();
    }
}

void PianoRoll::render(Canvas& c) {
    char b[64];
    Project& prj = app_->project();
    Pattern& p = pat();
    AudioEngine* eng = app_->engine();
    const Track& tr = prj.tracks[track_];
    Color tc = pal::track[tr.color % 8];
    c.clear(pal::bg);
    bool playing = eng && eng->isPlaying();
    int sel = noteAtCursor();
    int viewBar = curStep_ / STEPS_PER_BAR, viewStep0 = viewBar * STEPS_PER_BAR;

    // Naglowek (wspolny z ARRANGE): pozycja w patternie = glowica przy odtwarzaniu, inaczej kursor
    char nn[16];
    int posTick = curStep_ * TICKS_PER_STEP;
    bool rollPlaying = playing && eng->playMode() == Sequencer::PATTERN;
    if (rollPlaying) posTick = eng->playTick() % p.lengthTicks;
    screenHeader(c, ICON_PATTERN, tc, p.name, tr.name, tc, posTick / TICKS_PER_BAR + 1,
                 (posTick % TICKS_PER_BAR) / PPQ + 1, playing, prj.bpm);

    // Numery miar (takt.miara)
    for (int beat = 0; beat < 4; beat++) {
        snprintf(b, sizeof b, "%d.%d", viewBar + 1, beat + 1);
        drawText(c, GX + beat * 4 * SW + 3, 45.5f, b, 5.5f, 1.f, pal::dim);
    }
    // Pasek pozycji w patternie (ktory takt jest widoczny)
    if (p.bars() > 1) {
        float w = SW * STEPS_PER_BAR, x0 = GX, seg = w / p.bars();
        for (int i = 0; i < p.bars(); i++)
            c.fillRoundRect(x0 + i * seg + 1, GY + RH * ROWS + 6, seg - 2, 4, 2, i == viewBar ? pal::white : rgb(55, 55, 55));
    }

    const float GW = SW * STEPS_PER_BAR, GH = RH * ROWS;
    // Tlo wierszy jak klawiatura: biale klawisze minimalnie jasniejsze, czarne = czern.
    const float KX = 40, KW = 18;
    for (int r = 0; r < ROWS; r++) {
        int pt = topPitch_ - r;
        float y = GY + r * RH;
        bool blk = isBlack(pt);
        if (!blk) c.fillRect(GX, y, GW, RH, rgb(13, 13, 13));
        if (!blk) c.fillRoundRect(KX, y + 1, KW, RH - 2, 2.5f, rgb(175, 175, 175));
        else c.strokeRoundRect(KX + 0.5f, y + 1.5f, KW - 1, RH - 3, 2.5f, 1.f, rgb(70, 70, 70));
        if (pt == curPitch_) c.fillCircle(KX + KW + 6, y + RH * 0.5f, 2.5f, pal::white);
        if (pt % 12 == 0 || pt == curPitch_) {
            noteName(nn, sizeof nn, pt);
            drawText(c, KX - 7, y + RH * 0.5f - 4.5f, nn, 9, 1.1f, pt == curPitch_ ? pal::white : pal::dim, ALIGN_RIGHT);
        }
        if (pt % 12 == 0) c.line(GX, y + RH, GX + GW, y + RH, 1.f, rgb(52, 52, 52));
    }
    for (int s2 = 0; s2 <= STEPS_PER_BAR; s2 += 4)
        c.line(GX + s2 * SW, GY, GX + s2 * SW, GY + GH, 1.f, (s2 % 16 == 0) ? rgb(70, 70, 70) : rgb(40, 40, 40));
    for (int r = 0; r <= ROWS; r++)
        for (int s2 = 1; s2 < STEPS_PER_BAR; s2++)
            if (s2 % 4) c.fillCircle(GX + s2 * SW, GY + r * RH, 0.9f, rgb(60, 60, 60));

    // Nuty (tylko widoczny takt; nuty wystajace z poprzedniego taktu tez)
    const int t0 = viewStep0 * TICKS_PER_STEP, t1 = t0 + TICKS_PER_BAR;
    for (int i = 0; i < p.count; i++) {
        const Note& n = p.notes[i];
        int r = topPitch_ - n.pitch;
        if (r < 0 || r >= ROWS) continue;
        if (n.start >= t1 || n.start + n.len <= t0) continue;
        float x = GX + (float)(n.start - t0) / TICKS_PER_STEP * SW + 2;
        float xe = GX + (float)(n.start + n.len - t0) / TICKS_PER_STEP * SW - 2;
        if (x < GX) x = GX + 1;            // ciagnie sie z poprzedniego taktu
        if (xe > GX + GW) xe = GX + GW - 1;  // ciagnie sie dalej
        float w = xe - x, y = GY + r * RH + 4, h = RH - 8;
        Color col = velColor(n.vel);
        c.fillRoundRect(x, y, w, h, h * 0.5f, col);
        if (i == sel) {
            c.strokeRoundRect(x - 2, y - 2, w + 4, h + 4, h * 0.5f + 2, 1.5f, pal::white);
            c.fillCircle(xe - h * 0.5f, y + h * 0.5f, 3.f, pal::blue);
        }
    }

    // Kursor (biale narozniki)
    {
        float x = GX + (curStep_ - viewStep0) * SW, y = GY + (topPitch_ - curPitch_) * RH, k = 5;
        float pts[][4] = {{x, y + k, x, y}, {x, y, x + k, y}, {x + SW - k, y, x + SW, y}, {x + SW, y, x + SW, y + k},
                          {x, y + RH - k, x, y + RH}, {x, y + RH, x + k, y + RH},
                          {x + SW - k, y + RH, x + SW, y + RH}, {x + SW, y + RH, x + SW, y + RH - k}};
        c.beginStroke();
        for (auto& q : pts) c.strokeSeg(q[0], q[1], q[2], q[3], 1.5f);
        c.endStroke(pal::white);
    }

    // Glowica: w trybie PATTERN pozycja w patternie; w SONG — jesli klip tego patternu gra na tej sciezce
    if (playing) {
        int pt = -1, tick = eng->playTick();
        if (eng->playMode() == Sequencer::PATTERN) pt = tick % p.lengthTicks;
        else
            for (int i = 0; i < prj.nClips; i++) {
                const Clip& cl = prj.clips[i];
                int st = cl.startBar * TICKS_PER_BAR;
                if (cl.track == track_ && cl.pattern == pat_ && tick >= st && tick < st + p.lengthTicks) pt = tick - st;
            }
        if (pt >= t0 && pt < t1) {
            float x = GX + (float)(pt - t0) / TICKS_PER_STEP * SW;
            c.line(x, GY, x, GY + GH, 1.5f, pal::red);
            float tri[6] = {x - 4, GY - 7, x + 4, GY - 7, x, GY - 1};
            c.polyline(tri, 3, 1.5f, pal::red, true);
        }
    }

    // Enkodery
    int len = sel >= 0 ? p.notes[sel].len : lastLen_;
    int vel = sel >= 0 ? p.notes[sel].vel : lastVel_;
    char v0[16], v1[16], v2[16], v3[16];
    lenName(v0, sizeof v0, len);
    snprintf(v1, sizeof v1, "%d", vel);
    snprintf(v2, sizeof v2, "%d", (int)lrintf(tr.volume * 100));
    snprintf(v3, sizeof v3, "%.0f", prj.bpm);
    const char* labels[4] = {"LENGTH", "VELOCITY", "VOLUME", "BPM"};
    const char* vals[4] = {v0, v1, v2, v3};
    float pos[4] = {(float)len / TICKS_PER_BAR, vel / 127.f, tr.volume, (prj.bpm - 40) / 200.f};
    encoderBar(c, labels, vals, pos);
}

}  // namespace dp
