#include "ui/project_view.h"
#include "gfx/font.h"
#include "gfx/icons.h"
#include "ui/app.h"
#include <stdio.h>
#include <string.h>

namespace dp {

static const float LBX = 16, LBW = 372, RBX = 400, RBW = 224, BY = 12, BH = 42;
static const float PY = 430, PH = 34;

void ProjectView::onShow() {
    app_->store().refresh();
    confirm_ = 0;
    if (sel_ > app_->store().count()) sel_ = 0;
}

void ProjectView::message(const char* m, bool error) {
    snprintf(msg_, sizeof msg_, "%s", m);
    msgErr_ = error;
    msgT_ = 3.f;
}

void ProjectView::update(const Buttons& b, const InputState&, float dt) {
    ProjectStore& st = app_->store();
    Project& p = app_->project();
    if (msgT_ > 0) msgT_ -= dt;
    if (ed_.active()) {
        ed_.update(b);
        if (!ed_.active()) app_->markDirty();
        return;
    }
    int n = st.count() + 1;
    if (b.repeat(B_DOWN) && sel_ < n - 1) { sel_++; confirm_ = 0; }
    if (b.repeat(B_UP) && sel_ > 0) { sel_--; confirm_ = 0; }
    if (sel_ < first_) first_ = sel_;
    if (sel_ >= first_ + 11) first_ = sel_ - 10;

    if (b.pressed(B_X)) { ed_.begin(p.name); confirm_ = 0; }
    if (b.pressed(B_START)) app_->togglePlaySong();

    if (b.pressed(B_Y)) {  // zapis
        bool other = st.exists(p.name) && !app_->isOwnSave(p.name);
        if (other && confirm_ != 3) { confirm_ = 3; return; }
        confirm_ = 0;
        if (app_->saveProject()) message("SAVED");
        else message("SAVE FAILED", true);
        sel_ = 1;  // zapisany jest na gorze listy (najnowszy)
        first_ = 0;
    }
    if (b.pressed(B_A)) {  // wczytaj / nowy
        if (app_->isModified() && confirm_ != 1) { confirm_ = 1; return; }
        confirm_ = 0;
        if (sel_ == 0) { app_->newProject(); message("NEW PROJECT"); }
        else {
            char err[64];
            if (app_->loadProject(sel_ - 1, err, sizeof err)) message("LOADED");
            else message(err, true);
        }
    }
    if (b.pressed(B_B) && sel_ > 0) {  // usun
        if (confirm_ != 2) { confirm_ = 2; return; }
        confirm_ = 0;
        if (st.remove(sel_ - 1)) message("DELETED");
        else message("DELETE FAILED", true);
        if (sel_ > st.count()) sel_ = st.count();
    }
}

void ProjectView::render(Canvas& c) {
    ProjectStore& st = app_->store();
    Project& p = app_->project();
    char b[64];
    c.clear(pal::bg);
    headerBar(c, LBX, BY, LBW, BH, "PROJECT", pal::silver);
    headerBar(c, RBX, BY, RBW, BH, "PROJECTS", pal::tan);
    drawNotebookIllustration(c, 24, 84, 196, 150);
    nameBox(c, 236, 104, 152, 50, "NAME", p.name, &ed_, pal::steel);

    snprintf(b, sizeof b, "%d TRACKS", p.nTracks);
    drawText(c, 236, 172, b, 10, 1.3f, pal::dim);
    snprintf(b, sizeof b, "%d PATTERNS", p.nPatterns);
    drawText(c, 236, 188, b, 10, 1.3f, pal::dim);
    snprintf(b, sizeof b, "%d BARS", p.songBars());
    drawText(c, 236, 204, b, 10, 1.3f, pal::dim);
    snprintf(b, sizeof b, "%.0f BPM", p.bpm);
    drawText(c, 236, 220, b, 10, 1.3f, pal::red);
    bool mod = app_->isModified();
    c.fillCircle(240, 250, 4, mod ? pal::yellow : pal::green);
    drawText(c, 252, 245, mod ? "NOT SAVED" : "SAVED", 10, 1.3f, mod ? pal::yellow : pal::green);

    // potwierdzenia / komunikaty
    const char* info = nullptr;
    Color ic = pal::red;
    if (confirm_ == 1) info = "UNSAVED CHANGES - PRESS A AGAIN";
    else if (confirm_ == 2) { snprintf(b, sizeof b, "DELETE %s? PRESS B AGAIN", st.display(sel_ - 1)); info = b; }
    else if (confirm_ == 3) info = "NAME EXISTS - PRESS Y TO OVERWRITE";
    else if (msgT_ > 0) { info = msg_; ic = msgErr_ ? pal::red : pal::green; }
    if (info) drawText(c, 28, 300, info, 10, 1.4f, ic);
    if (ed_.active()) drawText(c, 28, 330, "UP/DOWN = LETTER  LEFT/RIGHT = MOVE  A = DONE", 9, 1.2f, pal::steel);
    else if (!st.ready()) drawText(c, 28, 330, "NO STORAGE (HEADLESS)", 9, 1.2f, pal::dim);
    else {
        snprintf(b, sizeof b, "%d PROJECTS ON CARD", st.count());
        drawText(c, 28, 330, b, 9, 1.2f, pal::dim);
    }

    static const char* items[ProjectStore::MAX + 1];
    items[0] = "+ NEW";
    for (int i = 0; i < st.count(); i++) items[i + 1] = st.display(i);
    listBox(c, RBX + 6, 72, RBW - 12, 340, items, st.count() + 1, sel_, first_, pal::tan);

    pillButton(c, 16, PY, 140, PH, "NAME", pal::steel, 'X');
    pillButton(c, 170, PY, 140, PH, confirm_ == 3 ? "SURE?" : "SAVE", pal::tan, 'Y');
    pillButton(c, 330, PY, 140, PH, confirm_ == 1 ? "SURE?" : (sel_ == 0 ? "NEW" : "LOAD"), pal::pill, 'A');
    pillButton(c, 484, PY, 140, PH, confirm_ == 2 ? "SURE?" : "DELETE", pal::red, 'B');
}

}  // namespace dp
