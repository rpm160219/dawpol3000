#include "ui/app.h"
#include <stdio.h>
#include <string.h>

namespace dp {

void App::init(AudioEngine* e) {
    eng_ = e;
    projectView.attach(this);
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
void App::showProject() { projectView.onShow(); next_ = &projectView; }

void App::openStore(const char* root) {
    store_.init(root);
    static Project tmp;
    if (store_.loadLast(tmp)) {
        prj_ = tmp;
        snprintf(savedAs_, sizeof savedAs_, "%s", prj_.name);
        afterLoad();
    } else {
        loadDemo();
    }
}

void App::afterLoad() {
    if (eng_) eng_->send(EC_STOP);
    lastPattern_ = -1;
    lastTrack_ = 0;
    for (int t = 0; t < prj_.nTracks; t++)
        if (prj_.tracks[t].lastPattern >= 0) { lastPattern_ = prj_.tracks[t].lastPattern; lastTrack_ = t; break; }
    arrange.setCursor(0, 0);
    dirty_ = true;
    publish();
    modified_ = false;
}

bool App::isOwnSave(const char* name) const {
    char a[64], b[64];
    ProjectStore::folderName(name, a, sizeof a);
    ProjectStore::folderName(savedAs_, b, sizeof b);
    return savedAs_[0] && !strcmp(a, b);
}

bool App::saveProject() {
    if (!store_.save(prj_)) return false;
    snprintf(savedAs_, sizeof savedAs_, "%s", prj_.name);
    modified_ = false;
    return true;
}

bool App::loadProject(int index, char* err, int errLen) {
    static Project tmp;
    if (!store_.load(tmp, index, err, errLen)) return false;
    prj_ = tmp;
    snprintf(savedAs_, sizeof savedAs_, "%s", prj_.name);
    afterLoad();
    return true;
}

void App::newProject() {
    prj_ = Project();
    for (int i = 1; i < 1000; i++) {  // pierwsza wolna nazwa PROJECT NN
        snprintf(prj_.name, sizeof prj_.name, "PROJECT %02d", i);
        if (!store_.exists(prj_.name)) break;
    }
    savedAs_[0] = 0;
    afterLoad();
}

void App::shutdown() {
    if (store_.ready() && modified_) saveProject();
    if (eng_) eng_->send(EC_STOP);
}

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
    // Zakladki PROJECT | ARRANGE | PIANO ROLL (L2 = w lewo, R2 = w prawo); nie w oknach dialogowych
    bool dialog = cur_ == &trackDlg || cur_ == &patDlg;
    bool hasRoll = lastPattern_ >= 0 && lastPattern_ < prj_.nPatterns;
    if (!dialog && btn_.pressed(B_L2)) {
        if (cur_ == &roll) showArrange();
        else if (cur_ == &arrange) showProject();
    } else if (!dialog && btn_.pressed(B_R2)) {
        if (cur_ == &projectView) showArrange();
        else if (cur_ == &arrange) {
            int t = 0, pt = arrange.highlightedPattern(&t);  // pattern pod kursorem
            if (pt >= 0) openPattern(pt, t);
            else if (hasRoll) openPattern(lastPattern_, lastTrack_);
        }
    } else cur_->update(btn_, in, dt);
    if (next_) { cur_ = next_; next_ = nullptr; }
    if (dirty_) publish();
}

void App::render(Canvas& c) {
    cur_->render(c);
    // Wskaznik zakladek (3 kropki pod naglowkiem): PROJECT, ARRANGE, PIANO ROLL
    int tab = cur_ == &projectView ? 0 : (cur_ == &arrange ? 1 : (cur_ == &roll ? 2 : -1));
    if (tab >= 0) {
        for (int i = 0; i < 3; i++) {
            float x = 308 + i * 12, y = tab == 0 ? 60 : 38;
            if (i == tab) c.fillCircle(x, y, 2.6f, pal::white);
            else c.circle(x, y, 2.2f, 1.f, rgb(90, 90, 90));
        }
    }
}

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
    savedAs_[0] = 0;
    dirty_ = true;
    publish();
    modified_ = false;
}

}  // namespace dp
