#include "ui/dialogs.h"
#include "gfx/font.h"
#include "gfx/icons.h"
#include "ui/app.h"
#include <stdio.h>
#include <string.h>

namespace dp {

// Wspolny uklad okna: lewa belka (szara), prawa belka (tan), pigulki na dole.
static const float LBX = 16, LBW = 372, RBX = 400, RBW = 224, BY = 12, BH = 42;
static const float PY = 430, PH = 34;

static void pills(Canvas& c, const char* l0, const char* l1, const char* l2, const char* l3) {
    pillButton(c, 16, PY, 140, PH, l0, pal::steel, 'X');
    pillButton(c, 170, PY, 140, PH, l1, pal::tan, 0);
    pillButton(c, 330, PY, 140, PH, l2, pal::pill, 'A');
    pillButton(c, 484, PY, 140, PH, l3, pal::red, 'B');
}

// ---------------- NEW TRACK ----------------

void TrackDialog::open() {
    edit_ = -1;
    Project& p = app_->project();
    type_ = p.canAddTrack(TRACK_MIDI) ? TRACK_MIDI : TRACK_AUDIO;
    p.nextTrackName(type_, name_, sizeof name_);
    nameTouched_ = false;
}

void TrackDialog::openEdit(int track) {
    Project& p = app_->project();
    edit_ = track;
    color_ = p.tracks[track].color % 8;
    type_ = p.tracks[track].type;
    memcpy(name_, p.tracks[track].name, sizeof name_);
    confirmDelete_ = false;
}

static const char* kColorNames[8] = {"BLUE", "GREEN", "RED", "TAN", "PURPLE", "CYAN", "YELLOW", "PINK"};

void TrackDialog::updateEdit(const Buttons& b) {
    Project& p = app_->project();
    if (b.repeat(B_DOWN)) { color_ = (color_ + 1) % 8; confirmDelete_ = false; }
    if (b.repeat(B_UP)) { color_ = (color_ + 7) % 8; confirmDelete_ = false; }
    if (b.pressed(B_X)) ed_.begin(name_);
    if (b.pressed(B_B)) app_->showArrange();
    if (b.pressed(B_Y)) {
        if (!confirmDelete_) confirmDelete_ = true;
        else {
            p.removeTrack(edit_);
            app_->markDirty();
            app_->arrange.setCursor(edit_ < p.nTracks ? edit_ : p.nTracks, 0);
            app_->showArrange();
        }
    }
    if (b.pressed(B_A)) {
        Track& t = p.tracks[edit_];
        memcpy(t.name, name_, sizeof t.name);
        t.color = (uint8_t)color_;
        app_->markDirty();
        app_->showArrange();
    }
}

void TrackDialog::renderEdit(Canvas& c) {
    Project& p = app_->project();
    const Track& t = p.tracks[edit_];
    c.clear(pal::bg);
    headerBar(c, LBX, BY, LBW, BH, "TRACK", pal::silver);
    headerBar(c, RBX, BY, RBW, BH, "COLOR", pal::tan);
    if (type_ == TRACK_MIDI) drawSynthIllustration(c, 24, 84, 196, 150);
    else drawTapeIllustration(c, 24, 84, 196, 150);
    nameBox(c, 236, 104, 152, 50, "NAME", name_, &ed_, pal::steel);
    Color tc = pal::track[color_];
    drawIcon(c, type_ == TRACK_MIDI ? ICON_KEYS : ICON_WAVE, 236, 186, 22, tc);
    drawText(c, 266, 191, type_ == TRACK_MIDI ? "MIDI TRACK" : "AUDIO TRACK", 11, 1.4f, tc);
    int clips = 0;
    for (int i = 0; i < p.nClips; i++) if (p.clips[i].track == edit_) clips++;
    char b[48];
    snprintf(b, sizeof b, "%d CLIPS   VOL %d", clips, (int)(t.volume * 100 + 0.5f));
    drawText(c, 236, 218, b, 10, 1.3f, pal::dim);
    if (confirmDelete_) drawText(c, 28, 300, "PRESS Y AGAIN TO DELETE TRACK + CLIPS", 10, 1.4f, pal::red);
    else drawText(c, 28, 300, "Y = DELETE TRACK", 10, 1.3f, pal::dim);
    if (ed_.active()) drawText(c, 28, 330, "UP/DOWN = LETTER  LEFT/RIGHT = MOVE  A = DONE", 9, 1.2f, pal::steel);
    // lista kolorow z probkami
    const float lx = RBX + 6, ly = 72, lw = RBW - 12;
    c.line(lx, ly, lx, ly + 330, 2.f, pal::steel);
    c.line(lx + lw, ly, lx + lw, ly + 330, 2.f, rgb(70, 70, 70));
    for (int i = 0; i < 8; i++) {
        float ry = ly + 8 + i * 38;
        if (i == color_) c.fillRoundRect(lx + 8, ry - 5, lw - 16, 30, 6, rgb(pal::tan.r, pal::tan.g, pal::tan.b, 40));
        c.fillRoundRect(lx + 18, ry, 20, 20, 5, pal::track[i]);
        drawText(c, lx + 50, ry + 3, kColorNames[i], 14, 1.6f, i == color_ ? rgb(245, 210, 160) : pal::tan);
    }
    pillButton(c, 16, PY, 140, PH, "NAME", pal::steel, 'X');
    pillButton(c, 170, PY, 140, PH, confirmDelete_ ? "SURE?" : "DELETE", pal::tan, 'Y');
    pillButton(c, 330, PY, 140, PH, "OK", pal::pill, 'A');
    pillButton(c, 484, PY, 140, PH, "CANCEL", pal::red, 'B');
}

void TrackDialog::update(const Buttons& b, const InputState&, float) {
    Project& p = app_->project();
    if (ed_.active()) { ed_.update(b); return; }
    if (edit_ >= 0) { updateEdit(b); return; }
    int nt = type_;
    if (b.repeat(B_UP) || b.repeat(B_DOWN) || b.repeat(B_LEFT) || b.repeat(B_RIGHT)) nt = 1 - type_;
    if (nt != type_ && p.canAddTrack(nt)) {
        type_ = nt;
        if (!nameTouched_) p.nextTrackName(type_, name_, sizeof name_);
    }
    if (b.pressed(B_X)) { ed_.begin(name_); nameTouched_ = true; }
    if (b.pressed(B_B)) app_->showArrange();
    if (b.pressed(B_A) && p.canAddTrack(type_)) {
        int t = p.addTrack(type_, name_);
        app_->markDirty();
        app_->arrange.setCursor(t, 0);
        if (type_ == TRACK_MIDI) app_->openNewPattern(t, 0);
        else app_->showArrange();
    }
}

void TrackDialog::render(Canvas& c) {
    if (edit_ >= 0) { renderEdit(c); return; }
    Project& p = app_->project();
    c.clear(pal::bg);
    headerBar(c, LBX, BY, LBW, BH, "NEW TRACK", pal::silver);
    headerBar(c, RBX, BY, RBW, BH, "TYPE", pal::tan);
    if (type_ == TRACK_MIDI) drawSynthIllustration(c, 24, 84, 196, 150);
    else drawTapeIllustration(c, 24, 84, 196, 150);
    nameBox(c, 236, 104, 152, 50, "NAME", name_, &ed_, pal::steel);
    char b[48];
    snprintf(b, sizeof b, "MIDI  %d/%d", p.countTracks(TRACK_MIDI), MAX_MIDI_TRACKS);
    drawText(c, 236, 190, b, 11, 1.4f, pal::dim);
    snprintf(b, sizeof b, "AUDIO %d/%d", p.countTracks(TRACK_AUDIO), MAX_AUDIO_TRACKS);
    drawText(c, 236, 212, b, 11, 1.4f, pal::dim);
    drawText(c, 28, 300, type_ == TRACK_MIDI ? "NOTES FROM PATTERNS -> SYNTH" : "AUDIO CLIPS FROM TAPE RECORDING", 10, 1.3f,
             pal::dim);
    if (ed_.active()) drawText(c, 28, 330, "UP/DOWN = LETTER  LEFT/RIGHT = MOVE  A = DONE", 9, 1.2f, pal::steel);
    const char* items[2] = {"MIDI", "AUDIO"};
    bool dis[2] = {!p.canAddTrack(TRACK_MIDI), !p.canAddTrack(TRACK_AUDIO)};
    listBox(c, RBX + 6, 72, RBW - 12, 330, items, 2, type_, 0, pal::tan, dis);
    for (int r = 0; r < 2; r++) {
        float rowMid = 72 + 6 + r * 30 + 9;  // srodek wiersza listy (listBox: rowH 30, tekst 18)
        drawIcon(c, r == 0 ? ICON_KEYS : ICON_WAVE, RBX + RBW - 48, rowMid - 10, 20,
                 type_ == r ? pal::tan : rgb(90, 70, 50));
    }
    pills(c, "NAME", "TYPE", "CREATE", "CANCEL");
}

// ---------------- NEW PATTERN ----------------

void PatternDialog::open(int track, int bar) {
    Project& p = app_->project();
    track_ = track;
    bar_ = bar;
    sel_ = 0;
    first_ = 0;
    bars_ = 1;
    p.nextPatternName(name_, sizeof name_);
}

void PatternDialog::update(const Buttons& b, const InputState& in, float dt) {
    Project& p = app_->project();
    if (ed_.active()) { ed_.update(b); return; }
    int n = p.nPatterns + 1;
    if (b.repeat(B_DOWN) && sel_ < n - 1) sel_++;
    if (b.repeat(B_UP) && sel_ > 0) sel_--;
    if (sel_ < first_) first_ = sel_;
    if (sel_ >= first_ + 9) first_ = sel_ - 8;
    if (sel_ == 0) {
        int d = encLen_.steps(in.axis[AX_LX], dt, 6.f);
        if (b.repeat(B_RIGHT)) d++;
        if (b.repeat(B_LEFT)) d--;
        bars_ += d;
        bars_ = bars_ < 1 ? 1 : (bars_ > MAX_PATTERN_BARS ? MAX_PATTERN_BARS : bars_);
        if (b.pressed(B_X)) ed_.begin(name_);
    }
    if (b.pressed(B_B)) app_->showArrange();
    if (b.pressed(B_A)) {
        int pat = sel_ == 0 ? p.addPattern(name_, bars_) : sel_ - 1;
        if (pat < 0) return;
        int bar = bar_;
        if (!p.fits(track_, bar, p.patterns[pat].bars())) {  // szukaj miejsca dalej na sciezce
            while (bar < MAX_SONG_BARS && !p.fits(track_, bar, p.patterns[pat].bars())) bar++;
        }
        if (p.addClip(track_, pat, bar) >= 0) app_->arrange.setCursor(track_, bar);
        p.tracks[track_].lastPattern = pat;
        app_->markDirty();
        app_->openPattern(pat, track_);
    }
}

void PatternDialog::render(Canvas& c) {
    Project& p = app_->project();
    c.clear(pal::bg);
    headerBar(c, LBX, BY, LBW, BH, "NEW PATTERN", pal::silver);
    headerBar(c, RBX, BY, RBW, BH, "PATTERNS", pal::tan);
    drawSynthIllustration(c, 24, 84, 196, 150);

    bool isNew = sel_ == 0;
    const Pattern* ex = isNew ? nullptr : &p.patterns[sel_ - 1];
    nameBox(c, 236, 104, 152, 50, "NAME", isNew ? name_ : ex->name, isNew ? &ed_ : nullptr, pal::steel);
    char b[32];
    snprintf(b, sizeof b, "%d", isNew ? bars_ : ex->bars());
    valueBox(c, 236, 196, 84, 56, "LENGTH", b, (isNew ? bars_ : ex->bars()) == 1 ? "BAR" : "BARS", pal::tan, isNew);
    const Track& tr = p.tracks[track_];
    drawIcon(c, ICON_KEYS, 28, 290, 18, pal::track[tr.color % 8]);
    snprintf(b, sizeof b, "%s  -  BAR %d", tr.name, bar_ + 1);
    drawText(c, 54, 294, b, 11, 1.4f, pal::white);
    drawText(c, 28, 322, "4/4  -  16 STEPS PER BAR", 10, 1.3f, pal::dim);
    if (ed_.active()) drawText(c, 28, 350, "UP/DOWN = LETTER  LEFT/RIGHT = MOVE  A = DONE", 9, 1.2f, pal::steel);

    static const char* items[MAX_PATTERNS + 1];
    items[0] = "+ NEW";
    for (int i = 0; i < p.nPatterns; i++) items[i + 1] = p.patterns[i].name;
    listBox(c, RBX + 6, 72, RBW - 12, 330, items, p.nPatterns + 1, sel_, first_, pal::tan);
    pills(c, "NAME", "LENGTH", isNew ? "CREATE" : "USE", "CANCEL");
}

}  // namespace dp
