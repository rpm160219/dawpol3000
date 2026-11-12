#include "ui/app.h"
#include "gfx/font.h"
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
    synth.attach(this);
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

// ---- Undo ----
void App::resetHistory() {
    undo_.clear();
    redo_.clear();
    committed_ = prj_;
    pending_ = edited_ = false;
}

void App::commitStep() {
    undo_.push(committed_);  // stan sprzed gestu
    committed_ = prj_;
    pending_ = false;
    redo_.clear();           // nowa zmiana kasuje mozliwosc ponowienia
}

void App::undo() {
    if (pending_) commitStep();
    static Project tmp;
    if (!undo_.pop(tmp)) { toast("NOTHING TO UNDO"); return; }
    redo_.push(prj_);
    prj_ = tmp;
    committed_ = prj_;
    dirty_ = modified_ = true;
    edited_ = false;
    validateViews();
    roll.onProjectReplaced();
    char b[40];
    snprintf(b, sizeof b, "UNDO  (%d LEFT)", undo_.size());
    toast(b);
}

void App::redo() {
    if (pending_) commitStep();
    static Project tmp;
    if (!redo_.pop(tmp)) { toast("NOTHING TO REDO"); return; }
    undo_.push(prj_);
    prj_ = tmp;
    committed_ = prj_;
    dirty_ = modified_ = true;
    edited_ = false;
    validateViews();
    roll.onProjectReplaced();
    char b[40];
    snprintf(b, sizeof b, "REDO  (%d LEFT)", redo_.size());
    toast(b);
}

// ---- Petla i IN/OUT ----
void App::toggleLoop() {
    char b[48];
    if (cur_ == &roll) {
        Pattern& p = prj_.patterns[roll.patternIndex()];
        p.loopOn = !p.loopOn;
        if (p.loopOn && p.loopOut <= p.loopIn) snprintf(b, sizeof b, "LOOP ON - SET IN/OUT WITH Y");
        else snprintf(b, sizeof b, "LOOP %s", p.loopOn ? "ON" : "OFF");
    } else {
        prj_.loopOn = !prj_.loopOn;
        if (prj_.loopOn && prj_.loopOut <= prj_.loopIn) snprintf(b, sizeof b, "LOOP ON (WHOLE SONG)");
        else snprintf(b, sizeof b, prj_.loopOn ? "LOOP ON" : "LOOP OFF - SONG PLAYS ONCE");
    }
    markDirty();
    toast(b);
}

// Y raz = IN na kursorze, Y drugi raz = OUT na koncu komorki kursora (i wlacza petle).
void App::setInOut() {
    char b[48];
    bool rollView = cur_ == &roll;
    int pos = rollView ? roll.cursorStep() : arrange.cursorBar();
    int* in;
    int* out;
    bool* on;
    int pin, pout;  // dla patternu (uint16_t w strukturze)
    Pattern* p = rollView ? &prj_.patterns[roll.patternIndex()] : nullptr;
    if (p) { pin = p->loopIn; pout = p->loopOut; in = &pin; out = &pout; on = &p->loopOn; }
    else { in = &prj_.loopIn; out = &prj_.loopOut; on = &prj_.loopOn; }
    if (!inPending_) {
        *in = pos;
        *out = pos + 1;
        inPending_ = true;
        snprintf(b, sizeof b, "IN %d - NOW MOVE AND PRESS Y FOR OUT", pos + 1);
    } else {
        int a = *in, e = pos + 1;
        if (e <= a) { int t = a; a = pos; e = t + 1; }  // OUT przed IN — zamien
        *in = a;
        *out = e;
        *on = true;
        inPending_ = false;
        snprintf(b, sizeof b, "LOOP %d - %d ON", a + 1, e);
    }
    if (p) { p->loopIn = (uint16_t)pin; p->loopOut = (uint16_t)pout; }
    markDirty();
    toast(b);
}

void App::toolsUpdate(const InputState& in, float dt) {
    if (btn_.pressed(B_B)) undo();
    else if (btn_.pressed(B_A)) redo();
    else if (btn_.pressed(B_X)) toggleLoop();
    else if (btn_.pressed(B_Y)) setInOut();
    // Ekran dostaje tylko D-pad (kursor), Start i R3; galki wylaczone.
    static const int mask[] = {B_A, B_B, B_X, B_Y, B_L1, B_R1, B_SELECT, B_L3};
    Buttons mb = btn_.masked(mask, 8);
    InputState in2 = in;
    for (int a = 0; a < AXIS_COUNT; a++) in2.axis[a] = 0;
    cur_->update(mb, in2, dt);
}

void App::toolsRender(Canvas& c) {
    char status[96], loopLbl[16];
    bool rollView = cur_ == &roll;
    bool on;
    int a, e;
    if (rollView) {
        const Pattern& p = prj_.patterns[roll.patternIndex()];
        on = p.loopOn; a = p.loopIn; e = p.loopOut;
    } else { on = prj_.loopOn; a = prj_.loopIn; e = prj_.loopOut; }
    snprintf(loopLbl, sizeof loopLbl, "LOOP %s", on ? "ON" : "OFF");
    const char* unit = rollView ? "STEP" : "BAR";
    if (e > a) snprintf(status, sizeof status, "TOOLS  -  UNDO %d / REDO %d  -  LOOP %s %d-%d  -  L3 = EXIT", undo_.size(),
                        redo_.size(), unit, a + 1, e);
    else snprintf(status, sizeof status, "TOOLS  -  UNDO %d / REDO %d  -  LOOP: %s  -  L3 = EXIT", undo_.size(), redo_.size(),
                  rollView ? "WHOLE PATTERN" : "WHOLE SONG");
    const char* labels[4] = {"UNDO", "REDO", inPending_ ? "SET OUT" : "SET IN", loopLbl};
    const Color cols[4] = {pal::steel, pal::red, pal::tan, pal::pill};
    const char keys[4] = {'B', 'A', 'Y', 'X'};
    actionBar(c, labels, cols, keys, status);
}

// Po cofnieciu ekrany moga wskazywac na nieistniejace sciezki/patterny.
void App::validateViews() {
    if (lastPattern_ >= prj_.nPatterns || lastTrack_ >= prj_.nTracks) lastPattern_ = -1;
    if (cur_ == &roll && !roll.valid()) cur_ = &arrange;
    if (cur_ == &synth && synth.track() >= prj_.nTracks) cur_ = &arrange;
    arrange.clampCursor();
}

void App::toast(const char* msg) {
    snprintf(toast_, sizeof toast_, "%s", msg);
    toastT_ = 1.2f;
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
    resetHistory();
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

void App::openSynth(int track) {
    if (track < 0 || track >= prj_.nTracks || prj_.tracks[track].type != TRACK_MIDI) return;
    synth.open(track);
    next_ = &synth;
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
    if (toastT_ > 0) toastT_ -= dt;
    if (in.btn[B_MENU] && in.btn[B_SELECT]) quit_ = true;
    // Zakladki PROJECT | ARRANGE | PIANO ROLL (L2 = w lewo, R2 = w prawo); nie w oknach dialogowych
    bool dialog = cur_ == &trackDlg || cur_ == &patDlg;
    bool toolsView = cur_ == &arrange || cur_ == &roll;
    if (toolsView && btn_.pressed(B_L3)) {  // pasek narzedzi wl/wyl
        tools_ = !tools_;
        inPending_ = false;
        if (tools_) roll.exitSelect();
    }
    if (tools_ && (!toolsView || (cur_ == &roll && btn_.pressed(B_R3)))) tools_ = false;  // R3 -> zaznaczanie
    bool hasRoll = lastPattern_ >= 0 && lastPattern_ < prj_.nPatterns;
    // W menu zaznaczania (R3) L2/R2 przelaczaja warstwy paska, nie zakladki
    if (cur_ == &roll && roll.inSelectMode()) dialog = true;
    if (!dialog && btn_.pressed(B_L2)) {
        if (cur_ == &synth) {
            if (hasRoll && lastTrack_ == synth.track()) openPattern(lastPattern_, lastTrack_);
            else { int lp = prj_.tracks[synth.track()].lastPattern; if (lp >= 0) openPattern(lp, synth.track()); else showArrange(); }
        } else if (cur_ == &roll) showArrange();
        else if (cur_ == &arrange) showProject();
    } else if (!dialog && btn_.pressed(B_R2)) {
        if (cur_ == &projectView) showArrange();
        else if (cur_ == &roll) openSynth(roll.trackIndex());
        else if (cur_ == &arrange) {
            int t = 0, pt = arrange.highlightedPattern(&t);  // pattern pod kursorem
            if (pt >= 0) openPattern(pt, t);
            else if (hasRoll) openPattern(lastPattern_, lastTrack_);
        }
    } else if (tools_) toolsUpdate(in, dt);
    else if (!(toolsView && btn_.pressed(B_L3))) cur_->update(btn_, in, dt);  // L3 w SYNTH trafia do ekranu
    if (next_) { cur_ = next_; next_ = nullptr; }
    if (tools_ && cur_ != &arrange && cur_ != &roll) tools_ = false;
    if (dirty_) publish();
    // Krok historii konczy sie, gdy wszystkie przyciski sa puszczone i galki w spoczynku:
    // jedno wcisniecie = jeden krok, cale pokrecenie galka / przytrzymanie X+strzalka = jeden krok.
    // W oknach dialogowych czekamy do ich zamkniecia (cale okno = jeden krok).
    bool inDialog = cur_ == &trackDlg || cur_ == &patDlg;
    bool idle = true;
    for (int a = 0; a < AXIS_COUNT; a++) if (in.axis[a] > 0.2f || in.axis[a] < -0.2f) idle = false;
    for (int k = 0; k < BTN_COUNT; k++) if (in.btn[k]) idle = false;
    if (edited_) { pending_ = true; edited_ = false; }
    else if (pending_ && !inDialog && idle) commitStep();
}

void App::render(Canvas& c) {
    cur_->render(c);
    // Wskaznik zakladek (4 kropki pod naglowkiem): PROJECT, ARRANGE, PIANO ROLL, SYNTH
    int tab = cur_ == &projectView ? 0 : (cur_ == &arrange ? 1 : (cur_ == &roll ? 2 : (cur_ == &synth ? 3 : -1)));
    if (tab >= 0) {
        for (int i = 0; i < 4; i++) {
            float x = 302 + i * 12, y = tab == 0 ? 60 : 38;
            if (i == tab) c.fillCircle(x, y, 2.6f, pal::white);
            else c.circle(x, y, 2.2f, 1.f, rgb(90, 90, 90));
        }
    }
    if (tools_) toolsRender(c);
    if (toastT_ > 0) {  // komunikat (np. UNDO)
        float w = textWidth(toast_, 12) + 36, x = 320 - w * 0.5f, y = 368;
        c.fillRoundRect(x, y, w, 30, 15, rgb(30, 30, 30));
        c.strokeRoundRect(x, y, w, 30, 15, 1.2f, pal::dim);
        drawText(c, 320, y + 9, toast_, 12, 1.5f, pal::white, ALIGN_CENTER);
    }
}

void App::loadDemo() {
    prj_ = Project();
    int bass = prj_.addTrack(TRACK_MIDI, "BASS");
    int lead = prj_.addTrack(TRACK_MIDI, "LEAD");
    int keys = prj_.addTrack(TRACK_MIDI, "KEYS");
    prj_.addTrack(TRACK_AUDIO, "GUITAR");
    const int T = TICKS_PER_STEP;
    // brzmienia: BASS = pila->kwadrat, nisko, LEAD = domyslne, KEYS = cieple + rozstrojenie, miekki atak
    { Track& t = prj_.tracks[bass]; t.knob[0] = 12000; t.knob[1] = 3000; t.knob[2] = 14000; t.knob[3] = 14000;
      t.env[0] = 0.002f; t.env[1] = 0.25f; t.env[2] = 0.5f; t.env[3] = 0.12f; }
    { Track& t = prj_.tracks[keys]; t.knob[0] = 4700; t.knob[1] = 20000; t.knob[2] = 17000; t.knob[3] = 6000;
      t.env[0] = 0.35f; t.env[1] = 1.0f; t.env[2] = 0.8f; t.env[3] = 0.9f; t.volume = 0.6f; }
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
    resetHistory();
}

}  // namespace dp
