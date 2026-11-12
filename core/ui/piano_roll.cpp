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

bool PianoRoll::valid() {
    const Project& p = app_->project();
    return pat_ < p.nPatterns && track_ < p.nTracks;
}

void PianoRoll::onProjectReplaced() { clearSel(); bHeld_ = false; }

void PianoRoll::clearSel() { for (auto& s : sel_) s = false; }

int PianoRoll::selCount() {
    int n = 0;
    for (int i = 0; i < pat().count; i++) n += sel_[i];
    return n;
}

void PianoRoll::removeNote(int i) {
    Pattern& p = pat();
    int last = p.count - 1;
    p.remove(i);  // Pattern::remove przenosi ostatnia nute na miejsce i
    sel_[i] = sel_[last];
    sel_[last] = false;
}

void PianoRoll::selectRect(int s0, int p0, int s1, int p1) {
    Pattern& p = pat();
    int sa = s0 < s1 ? s0 : s1, sb = s0 < s1 ? s1 : s0, pa = p0 < p1 ? p0 : p1, pb = p0 < p1 ? p1 : p0;
    clearSel();
    for (int i = 0; i < p.count; i++) {
        const Note& n = p.notes[i];
        int st = n.start / TICKS_PER_STEP, en = (n.start + n.len - 1) / TICKS_PER_STEP;
        if (en >= sa && st <= sb && n.pitch >= pa && n.pitch <= pb) sel_[i] = true;
    }
}

// Przesuniecie zaznaczonych nut. Wszystko albo nic: przy kolizji z inna nuta albo wyjsciu
// poza pattern stan wraca do poprzedniego.
bool PianoRoll::shiftSelection(int ds, int dp) {
    Pattern& p = pat();
    static Pattern backup;
    static bool selBackup[Pattern::MAX_NOTES];
    static Note moved[Pattern::MAX_NOTES];
    int nm = 0;
    for (int i = 0; i < p.count; i++) {
        if (!sel_[i]) continue;
        const Note& n = p.notes[i];
        int st = n.start + ds * TICKS_PER_STEP, pt = n.pitch + dp;
        if (st < 0 || st + n.len > p.lengthTicks || pt < 0 || pt > 127) return false;
        moved[nm] = n;
        moved[nm].start = (uint16_t)st;
        moved[nm].pitch = (uint8_t)pt;
        nm++;
    }
    if (nm == 0) return false;
    backup = p;
    for (int i = 0; i < Pattern::MAX_NOTES; i++) selBackup[i] = sel_[i];
    for (int i = p.count - 1; i >= 0; i--) if (sel_[i]) removeNote(i);
    for (int k = 0; k < nm; k++) {
        if (p.find(moved[k].start, moved[k].pitch) >= 0 || p.maxLen(moved[k].start, moved[k].pitch) < moved[k].len) {
            p = backup;
            for (int i = 0; i < Pattern::MAX_NOTES; i++) sel_[i] = selBackup[i];
            return false;
        }
        int idx = p.add(moved[k]);
        if (idx < 0) {
            p = backup;
            for (int i = 0; i < Pattern::MAX_NOTES; i++) sel_[i] = selBackup[i];
            return false;
        }
        sel_[idx] = true;
    }
    app_->markDirty();
    return true;
}

void PianoRoll::duplicateSelection() {
    Pattern& p = pat();
    int a = 1 << 30, e = 0;
    for (int i = 0; i < p.count; i++)
        if (sel_[i]) {
            if (p.notes[i].start < a) a = p.notes[i].start;
            if (p.notes[i].start + p.notes[i].len > e) e = p.notes[i].start + p.notes[i].len;
        }
    if (a == (1 << 30)) { app_->toast("NOTHING SELECTED"); return; }
    int span = ((e - a + TICKS_PER_STEP - 1) / TICKS_PER_STEP) * TICKS_PER_STEP;
    static Note src[Pattern::MAX_NOTES];
    int n = 0;
    for (int i = 0; i < p.count; i++) if (sel_[i]) src[n++] = p.notes[i];
    clearSel();
    int added = 0;
    for (int k = 0; k < n; k++) {
        Note d = src[k];
        int st = d.start + span;
        if (st >= p.lengthTicks) continue;
        d.start = (uint16_t)st;
        int ex = p.find(d.start, d.pitch);
        if (ex >= 0) removeNote(ex);
        int idx = p.add(d);
        if (idx >= 0) { sel_[idx] = true; added++; }
    }
    if (added) {
        app_->markDirty();
        curStep_ += span / TICKS_PER_STEP;
        if (curStep_ >= steps()) curStep_ = steps() - 1;
    }
    char m[32];
    snprintf(m, sizeof m, added ? "DUPLICATED %d" : "NO ROOM IN PATTERN", added);
    app_->toast(m);
}

// Warstwa 2: przeksztalcenia zaznaczenia (trzymany przycisk + krzyzak).
void PianoRoll::updateTransform(const Buttons& b) {
    int dx = 0, dy = 0;
    if (b.repeat(B_RIGHT)) dx = 1;
    if (b.repeat(B_LEFT)) dx = -1;
    if (b.repeat(B_UP)) dy = 1;
    if (b.repeat(B_DOWN)) dy = -1;
    bool hb = b.down(B_B), ha = b.down(B_A), hy = b.down(B_Y);
    if ((hb || ha || hy) && selCount() == 0) {
        if (b.pressed(B_B) || b.pressed(B_A) || b.pressed(B_Y)) app_->toast("NOTHING SELECTED");
        return;
    }
    if (hb && dy) {  // TRANSPOSE o poltony
        if (shiftSelection(0, dy)) { curPitch_ += dy; previewTopSelected(); }
    } else if (hy && dy) {  // OCTAVE
        if (shiftSelection(0, dy * 12)) { curPitch_ += dy * 12; previewTopSelected(); }
    } else if (ha && (dx || dy)) {  // MOVE
        if (shiftSelection(dx, dy)) { curStep_ += dx; curPitch_ += dy; if (dy) previewTopSelected(); }
    } else if (!hb && !ha && !hy) {
        if (dx > 0 && curStep_ < steps() - 1) curStep_++;
        if (dx < 0 && curStep_ > 0) curStep_--;
        if (dy > 0 && curPitch_ < 127) curPitch_++;
        if (dy < 0 && curPitch_ > 0) curPitch_--;
    }
    if (curPitch_ < 0) curPitch_ = 0;
    if (curPitch_ > 127) curPitch_ = 127;
    if (b.pressed(B_X)) duplicateSelection();
}

void PianoRoll::previewTopSelected() {
    Pattern& p = pat();
    int best = -1;
    for (int i = 0; i < p.count; i++)
        if (sel_[i] && (best < 0 || p.notes[i].pitch > p.notes[best].pitch)) best = i;
    if (best >= 0) preview(p.notes[best].pitch, p.notes[best].vel);
}

void PianoRoll::updateSelect(const Buttons& b) {
    Pattern& p = pat();
    if (b.pressed(B_L2) && selLayer_ > 0) { selLayer_--; bHeld_ = false; }
    if (b.pressed(B_R2) && selLayer_ < 1) { selLayer_++; bHeld_ = false; }
    if (selLayer_ == 1) { updateTransform(b); return; }
    int nsteps = steps();
    int os = curStep_, op = curPitch_;
    if (b.repeat(B_RIGHT) && curStep_ < nsteps - 1) curStep_++;
    if (b.repeat(B_LEFT) && curStep_ > 0) curStep_--;
    if (b.repeat(B_UP) && curPitch_ < 127) curPitch_++;
    if (b.repeat(B_DOWN) && curPitch_ > 0) curPitch_--;
    bool moved = os != curStep_ || op != curPitch_;

    // B: klik = przelacz nute, trzymanie + ruch = prostokat
    if (b.pressed(B_B)) { bHeld_ = true; bMoved_ = false; anchorStep_ = curStep_; anchorPitch_ = curPitch_; }
    if (bHeld_ && moved) bMoved_ = true;
    if (bHeld_ && bMoved_) selectRect(anchorStep_, anchorPitch_, curStep_, curPitch_);
    if (b.released(B_B) && bHeld_) {
        bHeld_ = false;
        if (!bMoved_) {
            int n = noteAtCursor();
            if (n >= 0) sel_[n] = !sel_[n];
            else clearSel();
        }
    }
    // A: usun zaznaczone (albo nute pod kursorem)
    if (b.pressed(B_A)) {
        int removed = 0;
        for (int i = p.count - 1; i >= 0; i--)
            if (sel_[i]) { removeNote(i); removed++; }
        if (!removed) { int n = noteAtCursor(); if (n >= 0) { removeNote(n); removed = 1; } }
        if (removed) {
            app_->markDirty();
            char m[32];
            snprintf(m, sizeof m, "DELETED %d", removed);
            app_->toast(m);
        }
    }
    // Y: kopiuj (czas wzgledem pierwszej nuty, wysokosc wzgledem kursora)
    if (b.pressed(B_Y)) {
        int minStart = 1 << 30;
        for (int i = 0; i < p.count; i++) if (sel_[i] && p.notes[i].start < minStart) minStart = p.notes[i].start;
        if (minStart == (1 << 30)) { app_->toast("NOTHING SELECTED"); }
        else {
            clip_.n = 0;
            clip_.refPitch = curPitch_;
            for (int i = 0; i < p.count; i++)
                if (sel_[i]) {
                    Note n = p.notes[i];
                    n.start = (uint16_t)(n.start - minStart);
                    clip_.notes[clip_.n++] = n;
                }
            char m[32];
            snprintf(m, sizeof m, "COPIED %d", clip_.n);
            app_->toast(m);
        }
    }
    // X: wklej w miejscu kursora; wklejone zostaja zaznaczone
    if (b.pressed(B_X)) {
        if (clip_.n == 0) { app_->toast("CLIPBOARD EMPTY"); return; }
        clearSel();
        int base = curStep_ * TICKS_PER_STEP, dpitch = curPitch_ - clip_.refPitch, added = 0;
        for (int i = 0; i < clip_.n; i++) {
            Note n = clip_.notes[i];
            int st = base + n.start, pt = n.pitch + dpitch;
            if (st >= p.lengthTicks || pt < 0 || pt > 127) continue;
            int ex = p.find(st, pt);  // wklejana nuta zastepuje istniejaca w tym miejscu
            if (ex >= 0) removeNote(ex);
            n.start = (uint16_t)st;
            n.pitch = (uint8_t)pt;
            int idx = p.add(n);
            if (idx >= 0) { sel_[idx] = true; added++; }
        }
        if (added) app_->markDirty();
        char m[32];
        snprintf(m, sizeof m, "PASTED %d", added);
        app_->toast(m);
    }
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

    if (b.pressed(B_R3)) {
        selMode_ = !selMode_;
        bHeld_ = false;
        if (!selMode_) clearSel();
    }
    if (selMode_) {
        updateSelect(b);
        scrollToCursor();
        if (b.pressed(B_START) && eng) {
            if (eng->isPlaying()) eng->send(EC_STOP);
            else eng->send(EC_PLAY_PATTERN, track_, 0, pat_);
        }
        return;
    }

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
    if (b.pressed(B_B) && sel >= 0) { removeNote(sel); app_->markDirty(); sel = -1; }

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

    // Petla patternu: podswietlony zakres + flagi IN/OUT na gornej krawedzi siatki
    if (p.loopOut > p.loopIn) {
        int la = p.loopIn - viewStep0, le = p.loopOut - viewStep0;
        if (le > 0 && la < STEPS_PER_BAR) {
            float x0 = GX + (la < 0 ? 0 : la) * SW, x1 = GX + (le > STEPS_PER_BAR ? STEPS_PER_BAR : le) * SW;
            Color lc = p.loopOn ? pal::white : rgb(90, 90, 90);
            if (p.loopOn) c.fillRect(x0, GY, x1 - x0, GH, rgb(255, 255, 255, 14));
            c.line(x0, GY, x1, GY, 2.f, lc);
            if (la >= 0) { float f[6] = {x0, GY, x0 + 6, GY, x0, GY + 7}; c.polyline(f, 3, 1.f, lc, true); c.fillRect(x0, GY, 3, 4, lc); }
            if (le <= STEPS_PER_BAR) { float f[6] = {x1, GY, x1 - 6, GY, x1, GY + 7}; c.polyline(f, 3, 1.f, lc, true); c.fillRect(x1 - 3, GY, 3, 4, lc); }
        }
    }

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
        if (sel_[i]) {  // zaznaczona: gruba biala obwodka + jasny srodek
            c.strokeRoundRect(x - 2.5f, y - 2.5f, w + 5, h + 5, h * 0.5f + 2.5f, 2.f, pal::white);
            c.fillRoundRect(x + 3, y + h * 0.5f - 1, w - 6 > 0 ? w - 6 : 0, 2, 1, pal::white);
        } else if (i == sel && !selMode_) {
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

    // Prostokat zaznaczania (B + D-pad)
    if (selMode_ && bHeld_ && bMoved_) {
        int sa = anchorStep_ < curStep_ ? anchorStep_ : curStep_, sb = anchorStep_ < curStep_ ? curStep_ : anchorStep_;
        int pa = anchorPitch_ > curPitch_ ? anchorPitch_ : curPitch_, pb = anchorPitch_ > curPitch_ ? curPitch_ : anchorPitch_;
        float x0 = GX + (sa - viewStep0) * SW, x1 = GX + (sb + 1 - viewStep0) * SW;
        float y0 = GY + (topPitch_ - pa) * RH, y1 = GY + (topPitch_ - pb + 1) * RH;
        if (x0 < GX) x0 = GX;
        if (x1 > GX + GW) x1 = GX + GW;
        if (y0 < GY) y0 = GY;
        if (y1 > GY + GH) y1 = GY + GH;
        c.fillRect(x0, y0, x1 - x0, y1 - y0, rgb(64, 128, 255, 40));
        c.rect(x0, y0, x1 - x0, y1 - y0, 1.f, pal::steel);
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

    if (selMode_) { renderSelectBar(c); return; }

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

// Tryb zaznaczania: zamiast galek — kompaktowy pasek akcji (wspolny z paskiem narzedzi).
void PianoRoll::renderSelectBar(Canvas& c) {
    char st[128];
    const Color cols[4] = {pal::steel, pal::red, pal::tan, pal::pill};
    const char keys[4] = {'B', 'A', 'Y', 'X'};
    if (selLayer_ == 0) {
        snprintf(st, sizeof st, "EDIT 1/2 (L2/R2)  -  %d SELECTED  -  CLIPBOARD %d  -  HOLD B + D-PAD = AREA  -  R3 = EXIT",
                 selCount(), clip_.n);
        const char* labels[4] = {"SELECT", "DELETE", "COPY", "PASTE"};
        actionBar(c, labels, cols, keys, st);
    } else {
        snprintf(st, sizeof st, "TRANSFORM 2/2 (L2/R2)  -  %d SELECTED  -  HOLD BUTTON + D-PAD  -  R3 = EXIT", selCount());
        const char* labels[4] = {"TRANSPOSE", "MOVE", "OCTAVE", "DUPLICATE"};
        actionBar(c, labels, cols, keys, st);
    }
    // Kropki warstw paska (L2/R2) — na samym dole ekranu, pod linia statusu
    for (int i = 0; i < 2; i++) {
        float x = 314.5f + i * 12, y = 474.5f;
        if (i == selLayer_) c.fillCircle(x, y, 2.4f, pal::white);
        else c.circle(x, y, 2.f, 1.f, rgb(90, 90, 90));
    }
}

}  // namespace dp
