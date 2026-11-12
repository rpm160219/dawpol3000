#include "ui/arrange.h"
#include "gfx/font.h"
#include "gfx/icons.h"
#include "ui/app.h"
#include "ui/widgets.h"
#include <math.h>
#include <stdio.h>

namespace dp {

static const float LX = 14, TX = 160, TY = 62, TRACK_H = 28, TEND = 620;
static const int ROWS = 12;
static const float kZoom[] = {12, 18, 28, 42, 60};
static const int NZOOM = 5;

float ArrangeView::barW() const { return kZoom[zoom_]; }
int ArrangeView::visibleBars() const { return (int)((TEND - TX) / barW()); }

int ArrangeView::highlightedPattern(int* track) const {
    const Project& p = app_->project();
    if (curTrack_ >= p.nTracks || p.tracks[curTrack_].type != TRACK_MIDI) return -1;
    *track = curTrack_;
    int clip = p.clipAt(curTrack_, curBar_);
    if (clip >= 0) return p.clips[clip].pattern;
    return p.tracks[curTrack_].lastPattern;
}

void ArrangeView::clampCursor() {
    int n = app_->project().nTracks;
    if (curTrack_ > n) curTrack_ = n;
    if (topTrack_ > curTrack_) topTrack_ = curTrack_;
}

void ArrangeView::scroll() {
    int vb = visibleBars();
    if (curBar_ < firstBar_) firstBar_ = curBar_;
    if (curBar_ >= firstBar_ + vb) firstBar_ = curBar_ - vb + 1;
    if (curTrack_ < topTrack_) topTrack_ = curTrack_;
    if (curTrack_ >= topTrack_ + ROWS) topTrack_ = curTrack_ - ROWS + 1;
}

void ArrangeView::update(const Buttons& b, const InputState& in, float dt) {
    Project& p = app_->project();
    int rowsTotal = p.nTracks + 1;  // + wiersz ADD TRACK
    bool onTrack = curTrack_ < p.nTracks;
    int clip = onTrack ? p.clipAt(curTrack_, curBar_) : -1;

    if (b.down(B_X) && clip >= 0) {
        // X + lewo/prawo: przesun klip (kwantyzacja do taktu)
        int d = b.repeat(B_RIGHT) ? 1 : (b.repeat(B_LEFT) ? -1 : 0);
        if (d) {
            Clip& c = p.clips[clip];
            int ns = c.startBar + d;
            if (p.fits(c.track, ns, p.clipBars(c), clip)) {
                c.startBar = (uint16_t)ns;
                curBar_ += d;
                app_->markDirty();
            }
            xMoved_ = true;
        }
    } else {
        if (b.repeat(B_UP) && curTrack_ > 0) curTrack_--;
        if (b.repeat(B_DOWN) && curTrack_ < rowsTotal - 1) curTrack_++;
        if (b.repeat(B_LEFT) && curBar_ > 0) curBar_--;
        if (b.repeat(B_RIGHT) && curBar_ < MAX_SONG_BARS - 1) curBar_++;
    }
    if (b.pressed(B_X)) xMoved_ = false;
    onTrack = curTrack_ < p.nTracks;
    clip = onTrack ? p.clipAt(curTrack_, curBar_) : -1;

    if (b.released(B_X) && !xMoved_ && clip >= 0) {  // klon za klipem
        Clip c = p.clips[clip];
        int ns = c.startBar + p.clipBars(c);
        if (p.addClip(c.track, c.pattern, ns) >= 0) { curBar_ = ns; app_->markDirty(); }
    }
    if (b.pressed(B_A)) {
        if (!onTrack) app_->openNewTrack();
        else if (clip >= 0) app_->openPattern(p.clips[clip].pattern, curTrack_);
        else if (p.tracks[curTrack_].type == TRACK_MIDI) {
            int lp = p.tracks[curTrack_].lastPattern;
            if (lp >= 0 && p.addClip(curTrack_, lp, curBar_) >= 0) app_->markDirty();
            else if (lp < 0) app_->openNewPattern(curTrack_, curBar_);
        }
    }
    if (b.pressed(B_Y) && onTrack && clip < 0 && p.tracks[curTrack_].type == TRACK_MIDI)
        app_->openNewPattern(curTrack_, curBar_);
    if (b.pressed(B_B) && clip >= 0) { p.removeClip(clip); app_->markDirty(); }
    if (b.pressed(B_START)) app_->togglePlaySong();
    if (onTrack && b.pressed(B_SELECT) && !b.down(B_MENU)) app_->openTrackEdit(curTrack_);
    if (onTrack && b.pressed(B_L1)) { p.tracks[curTrack_].mute = !p.tracks[curTrack_].mute; app_->markDirty(); }
    if (onTrack && b.pressed(B_R1)) { p.tracks[curTrack_].solo = !p.tracks[curTrack_].solo; app_->markDirty(); }

    // Enkodery
    if (onTrack) {
        float v = p.tracks[curTrack_].volume + RelEncoder::delta(in.axis[AX_LX], dt, 0.8f);
        v = v < 0 ? 0 : (v > 1 ? 1 : v);
        if (v != p.tracks[curTrack_].volume) { p.tracks[curTrack_].volume = v; app_->markDirty(); }
    }
    int dz = encZoom_.steps(-in.axis[AX_LY], dt, 5.f);
    if (dz) { zoom_ += dz; zoom_ = zoom_ < 0 ? 0 : (zoom_ >= NZOOM ? NZOOM - 1 : zoom_); }
    float mv = p.masterVol + RelEncoder::delta(in.axis[AX_RX], dt, 0.8f);
    mv = mv < 0 ? 0 : (mv > 1 ? 1 : mv);
    if (mv != p.masterVol) { p.masterVol = mv; app_->markDirty(); }
    int db = encBpm_.steps(-in.axis[AX_RY], dt, 25.f);
    if (db) {
        p.bpm += db;
        p.bpm = p.bpm < 40 ? 40 : (p.bpm > 240 ? 240 : p.bpm);
        app_->markDirty();
    }
    scroll();
}

// Miniatura nut patternu w klipie. x0..x1 = pelny klip na osi czasu (bez marginesow), wiec nuty
// leza dokladnie na liniach taktow; [clipL, clipR] = widoczna czesc (nic nie wychodzi poza klip).
static void clipNotes(Canvas& c, const Pattern& pt, float x0, float x1, float clipL, float clipR, float y, float h,
                      Color col) {
    if (pt.count == 0) return;
    int lo = 127, hi = 0;
    for (int i = 0; i < pt.count; i++) {
        if (pt.notes[i].pitch < lo) lo = pt.notes[i].pitch;
        if (pt.notes[i].pitch > hi) hi = pt.notes[i].pitch;
    }
    int span = hi - lo < 6 ? 6 : hi - lo;
    float base = lo - (span - (hi - lo)) * 0.5f;
    float w = x1 - x0;
    c.beginStroke();
    for (int i = 0; i < pt.count; i++) {
        const Note& n = pt.notes[i];
        float a = x0 + w * n.start / pt.lengthTicks + 1.5f;
        float e = x0 + w * (n.start + n.len) / pt.lengthTicks - 1.5f;
        if (a < clipL + 2) a = clipL + 2;
        if (e > clipR - 2) e = clipR - 2;
        if (e < a) continue;
        float ny = y + h - h * (n.pitch - base) / span;
        c.strokeSeg(a, ny, e, ny, 1.3f);
    }
    c.endStroke(col);
}

void ArrangeView::render(Canvas& c) {
    Project& p = app_->project();
    AudioEngine* eng = app_->engine();
    char b[64];
    c.clear(pal::bg);
    bool playing = eng && eng->isPlaying() && eng->playMode() == Sequencer::SONG;

    // Naglowek (wspolny z PIANO ROLL): pozycja = glowica przy odtwarzaniu, inaczej kursor
    int songTick = playing ? eng->playTick() : curBar_ * TICKS_PER_BAR;
    screenHeader(c, ICON_ARRANGE, pal::white, p.name, "", pal::white, songTick / TICKS_PER_BAR + 1,
                 (songTick % TICKS_PER_BAR) / PPQ + 1, playing, p.bpm);

    // Numery taktow
    int vb = visibleBars();
    int every = barW() >= 26 ? 1 : (barW() >= 18 ? 2 : 4);
    for (int i = 0; i <= vb; i++) {
        int bar = firstBar_ + i;
        float x = TX + i * barW();
        if (bar % every == 0) {
            snprintf(b, sizeof b, "%d", bar + 1);
            drawText(c, x + 3, 49, b, 8, 1.1f, bar == curBar_ ? pal::white : pal::dim);
        }
    }

    // Petla utworu: linia pod numerami taktow + flagi IN/OUT (biala = wlaczona)
    {
        int la = p.loopIn, le = p.loopOut;
        bool whole = le <= la;
        if (whole) { la = 0; le = p.songBars(); }
        Color lc = p.loopOn ? (whole ? rgb(110, 110, 110) : pal::white) : rgb(60, 60, 60);
        float x0 = TX + (la - firstBar_) * barW(), x1 = TX + (le - firstBar_) * barW();
        float cx0 = x0 < TX ? TX : x0, cx1 = x1 > TEND ? TEND : x1;
        if (cx1 > cx0) c.line(cx0, 59, cx1, 59, 2.f, lc);
        if (!whole) {
            if (x0 >= TX && x0 <= TEND) { float f[6] = {x0, 55.5f, x0 + 5, 59, x0, 62.5f}; c.polyline(f, 3, 1.2f, lc, true); }
            if (x1 >= TX && x1 <= TEND) { float f[6] = {x1, 55.5f, x1 - 5, 59, x1, 62.5f}; c.polyline(f, 3, 1.2f, lc, true); }
        }
    }

    // Sciezki
    for (int r = 0; r < ROWS; r++) {
        int t = topTrack_ + r;
        float y = TY + r * TRACK_H;
        if (t > p.nTracks) break;
        bool cur = t == curTrack_;
        if (t == p.nTracks) {  // wiersz ADD TRACK
            drawIcon(c, ICON_PLUS, LX, y + 5, 18, cur ? pal::white : pal::dim);
            drawText(c, LX + 26, y + 9, "ADD TRACK", 10, 1.3f, cur ? pal::white : pal::dim);
            snprintf(b, sizeof b, "MIDI %d/%d   AUDIO %d/%d", p.countTracks(TRACK_MIDI), MAX_MIDI_TRACKS,
                     p.countTracks(TRACK_AUDIO), MAX_AUDIO_TRACKS);
            drawText(c, TX + 8, y + 10, b, 8, 1.1f, pal::dim);
            if (cur) c.strokeRoundRect(LX - 6, y + 2, TX - LX - 2, TRACK_H - 4, 6, 1.2f, pal::white);
            break;
        }
        const Track& tr = p.tracks[t];
        bool aud = p.audible(t);
        Color tc0 = pal::track[tr.color % 8];
        Color tc = aud ? tc0 : rgb(tc0.r / 3, tc0.g / 3, tc0.b / 3);
        bool midi = tr.type == TRACK_MIDI;
        drawIcon(c, midi ? ICON_KEYS : ICON_WAVE, LX, y + 5, 18, tc);
        drawText(c, LX + 26, y + 5, tr.name, 9, 1.2f, cur ? pal::white : (aud ? rgb(170, 170, 170) : pal::dim));
        // pasek glosnosci + wskazniki M (mute) i S (solo)
        c.line(LX + 26, y + 21, LX + 26 + 72, y + 21, 1.f, pal::faint);
        c.line(LX + 26, y + 21, LX + 26 + 72 * tr.volume, y + 21, 1.6f, cur ? pal::blue : rgb(70, 70, 70));
        const char* ms[2] = {"M", "S"};
        bool on[2] = {tr.mute, tr.solo};
        Color onc[2] = {pal::yellow, pal::cyan};
        for (int k = 0; k < 2; k++) {
            float bx = LX + 104 + k * 18, by = y + 15, bw = 14, bh = 11;
            if (on[k]) {
                c.fillRoundRect(bx, by, bw, bh, 3, onc[k]);
                drawText(c, bx + bw * 0.5f, by + 2.5f, ms[k], 6, 1.2f, rgb(0, 0, 0), ALIGN_CENTER);
            } else {
                c.strokeRoundRect(bx, by, bw, bh, 3, 1.f, rgb(60, 60, 60));
                drawText(c, bx + bw * 0.5f, by + 2.5f, ms[k], 6, 1.f, rgb(80, 80, 80), ALIGN_CENTER);
            }
        }
        // tlo rzedu
        c.fillRect(TX, y + 1, TEND - TX, TRACK_H - 2, cur ? rgb(16, 16, 16) : rgb(8, 8, 8));
    }
    // Podswietlenie zakresu petli na sciezkach (tylko gdy ustawione IN/OUT i petla wlaczona)
    if (p.loopOn && p.loopOut > p.loopIn) {
        float x0 = TX + (p.loopIn - firstBar_) * barW(), x1 = TX + (p.loopOut - firstBar_) * barW();
        if (x0 < TX) x0 = TX;
        if (x1 > TEND) x1 = TEND;
        float yb = TY + (p.nTracks - topTrack_ < ROWS ? p.nTracks - topTrack_ : ROWS) * TRACK_H;
        if (x1 > x0 && yb > TY) c.fillRect(x0, TY, x1 - x0, yb - TY, rgb(255, 255, 255, 12));
    }
    // Linie taktow
    for (int i = 0; i <= vb; i++) {
        int bar = firstBar_ + i;
        float x = TX + i * barW();
        float yb = TY + (p.nTracks - topTrack_ < ROWS ? p.nTracks - topTrack_ : ROWS) * TRACK_H;
        if (bar % 4 == 0) c.line(x, TY, x, yb, 1.f, rgb(48, 48, 48));
        else for (float yy = TY + TRACK_H * 0.5f; yy < yb; yy += TRACK_H) c.fillCircle(x, yy, 0.9f, rgb(60, 60, 60));
    }
    // Klipy (na wierzchu siatki)
    for (int r = 0; r < ROWS; r++) {
        int t = topTrack_ + r;
        if (t >= p.nTracks) break;
        float y = TY + r * TRACK_H;
        const Track& tr = p.tracks[t];
        Color tc0 = pal::track[tr.color % 8];
        Color tc = p.audible(t) ? tc0 : rgb(tc0.r / 3, tc0.g / 3, tc0.b / 3);
        for (int i = 0; i < p.nClips; i++) {
            const Clip& cl = p.clips[i];
            if (cl.track != t) continue;
            int bars = p.clipBars(cl);
            float x0 = TX + (cl.startBar - firstBar_) * barW(), x1 = x0 + bars * barW();
            if (x1 < TX || x0 > TEND) continue;
            float cx0 = x0 < TX ? TX : x0, cx1 = x1 > TEND ? TEND : x1;
            float cy = y + 3, ch = TRACK_H - 6;
            c.fillRoundRect(cx0 + 1, cy, cx1 - cx0 - 2, ch, 5, rgb(tc.r / 5, tc.g / 5, tc.b / 5));
            c.strokeRoundRect(cx0 + 1, cy, cx1 - cx0 - 2, ch, 5, 1.2f, tc);
            const Pattern& pt = p.patterns[cl.pattern];
            // nazwa przycieta do widocznej czesci klipu
            bool label = (cx1 - cx0) > 16 && drawTextFit(c, cx0 + 6, cy + 3, pt.name, 7, 1.f, pal::white, cx1 - cx0 - 11);
            clipNotes(c, pt, x0, x1, cx0, cx1, cy + (label ? 12 : 4), ch - (label ? 15 : 8), tc);
        }
    }
    // Kursor
    if (curTrack_ < p.nTracks) {
        float x = TX + (curBar_ - firstBar_) * barW(), y = TY + (curTrack_ - topTrack_) * TRACK_H, w = barW(), h = TRACK_H, k = 5;
        float pts[][4] = {{x, y + k, x, y}, {x, y, x + k, y}, {x + w - k, y, x + w, y}, {x + w, y, x + w, y + k},
                          {x, y + h - k, x, y + h}, {x, y + h, x + k, y + h}, {x + w - k, y + h, x + w, y + h}, {x + w, y + h, x + w, y + h - k}};
        c.beginStroke();
        for (auto& q : pts) c.strokeSeg(q[0], q[1], q[2], q[3], 1.5f);
        c.endStroke(pal::white);
    }
    // Glowica
    if (playing) {
        float x = TX + ((float)eng->playTick() / TICKS_PER_BAR - firstBar_) * barW();
        if (x >= TX && x <= TEND) {
            float yb = TY + (p.nTracks - topTrack_ + 1 < ROWS ? p.nTracks - topTrack_ : ROWS) * TRACK_H;
            c.line(x, TY - 2, x, yb, 1.5f, pal::red);
            float tri[6] = {x - 4, TY - 8, x + 4, TY - 8, x, TY - 2};
            c.polyline(tri, 3, 1.5f, pal::red, true);
        }
    }

    // Enkodery
    bool onTrack = curTrack_ < p.nTracks;
    char v0[16], v1[16], v2[16], v3[16];
    if (onTrack) snprintf(v0, sizeof v0, "%d", (int)lrintf(p.tracks[curTrack_].volume * 100));
    else snprintf(v0, sizeof v0, "-");
    snprintf(v1, sizeof v1, "%d", zoom_ + 1);
    snprintf(v2, sizeof v2, "%d", (int)lrintf(p.masterVol * 100));
    snprintf(v3, sizeof v3, "%.0f", p.bpm);
    const char* labels[4] = {"VOLUME", "ZOOM", "MASTER", "BPM"};
    const char* vals[4] = {v0, v1, v2, v3};
    float pos[4] = {onTrack ? p.tracks[curTrack_].volume : 0.f, zoom_ / (float)(NZOOM - 1), p.masterVol, (p.bpm - 40) / 200.f};
    encoderBar(c, labels, vals, pos);
}

}  // namespace dp
