#include "ui/app.h"
#include <string.h>

namespace dp {

void App::init(AudioEngine* e) {
    eng_ = e;
    arrange.attach(this);
    roll.attach(this);
    trackDlg.attach(this);
    patDlg.attach(this);
    cur_ = &arrange;
    publish();
}

void App::publish() {
    if (eng_) {
        eng_->projectBack() = prj_;
        eng_->publishProject();
    }
    dirty_ = false;
}

void App::showArrange() { next_ = &arrange; }

void App::openPattern(int pattern, int track) {
    if (pattern < 0 || pattern >= prj_.nPatterns) return;
    lastPattern_ = pattern;
    lastTrack_ = track;
    roll.open(pattern, track);
    next_ = &roll;
}

void App::openNewTrack() {
    trackDlg.open();
    next_ = &trackDlg;
}

void App::openTrackEdit(int track) {
    if (track < 0 || track >= prj_.nTracks) return;
    trackDlg.openEdit(track);
    next_ = &trackDlg;
}

void App::openNewPattern(int track, int bar) {
    patDlg.open(track, bar);
    next_ = &patDlg;
}

void App::togglePlaySong() {
    if (!eng_) return;
    if (eng_->isPlaying()) eng_->send(EC_STOP);
    else eng_->send(EC_PLAY_SONG);
}

void App::update(const InputState& in, float dt) {
    btn_.update(in, dt);
    if (in.btn[B_MENU] && in.btn[B_SELECT]) quit_ = true;
    // Globalne przelaczanie ekranow (tylko poza oknami dialogowymi)
    bool dialog = cur_ == &trackDlg || cur_ == &patDlg;
    if (!dialog && btn_.pressed(B_L2)) showArrange();
    else if (!dialog && btn_.pressed(B_R2) && lastPattern_ >= 0) openPattern(lastPattern_, lastTrack_);
    else cur_->update(btn_, in, dt);
    if (next_) { cur_ = next_; next_ = nullptr; }
    if (dirty_) publish();
}

void App::render(Canvas& c) { cur_->render(c); }

void App::loadDemo() {
    prj_ = Project();
    int bass = prj_.addTrack(TRACK_MIDI, "BASS");
    int lead = prj_.addTrack(TRACK_MIDI, "LEAD");
    int keys = prj_.addTrack(TRACK_MIDI, "KEYS");
    prj_.addTrack(TRACK_AUDIO, "GUITAR");
    const int T = TICKS_PER_STEP;
    int pb = prj_.addPattern("BASS A", 1);
    struct N { int s, l, p, v; };
    N bn[] = {{0, 3, 36, 120}, {4, 2, 36, 90}, {6, 2, 43, 100}, {8, 3, 39, 115}, {12, 2, 41, 95}, {14, 2, 43, 100}};
    for (auto& n : bn) prj_.patterns[pb].add({(uint16_t)(n.s * T), (uint16_t)(n.l * T), (uint8_t)n.p, (uint8_t)n.v});
    int pl = prj_.addPattern("HOOK", 2);
    N ln[] = {{0, 4, 60, 110}, {0, 2, 67, 45}, {4, 2, 63, 75}, {6, 2, 67, 95}, {8, 6, 72, 127}, {12, 2, 74, 25},
              {14, 2, 70, 100}, {16, 4, 72, 100}, {20, 2, 70, 80}, {22, 2, 67, 90}, {24, 8, 63, 110}};
    for (auto& n : ln) prj_.patterns[pl].add({(uint16_t)(n.s * T), (uint16_t)(n.l * T), (uint8_t)n.p, (uint8_t)n.v});
    int pk = prj_.addPattern("PADS", 4);
    int ch[4][3] = {{60, 63, 67}, {58, 63, 67}, {56, 60, 63}, {55, 58, 63}};
    for (int b = 0; b < 4; b++)
        for (int k = 0; k < 3; k++)
            prj_.patterns[pk].add({(uint16_t)(b * TICKS_PER_BAR), (uint16_t)(TICKS_PER_BAR - T), (uint8_t)ch[b][k], 70});
    for (int b = 0; b < 8; b++) prj_.addClip(bass, pb, b);
    prj_.addClip(lead, pl, 2);
    prj_.addClip(lead, pl, 6);
    prj_.addClip(keys, pk, 0);
    prj_.addClip(keys, pk, 4);
    lastPattern_ = pl;
    lastTrack_ = lead;
    dirty_ = true;
    publish();
}

}  // namespace dp
