// Ekran MIXER i jego okna (zob. mixer_view.h).
#include "ui/mixer_view.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "audio/engine.h"
#include "audio/mixer.h"
#include "fx/fx_registry.h"
#include "gfx/font.h"
#include "gfx/icons.h"
#include "ui/app.h"
#include "ui/fx_screen.h"
#include "ui/widgets.h"

namespace dp {

namespace {
const int PER_PAGE = 5;
const float VU_REF = 12.f;  // 0 VU = -12 dBFS (czerwona strefa = ostatnie 3 dB przed przesterowaniem)

const float SW = 100.f, SX0 = 8.f, MX = 516.f, MW = 116.f;
const Color PLATE_CH = rgb(0, 0, 0), PLATE_AUX = rgb(140, 140, 146), PLATE_MASTER = rgb(228, 228, 222);
const float INS_Y = 246.f, INS_H = 12.f, INS_STEP = 15.f;
inline Color dimc(Color c, float a) { return rgb((uint8_t)(c.r * a), (uint8_t)(c.g * a), (uint8_t)(c.b * a)); }
Color auxColor(int a) { static const Color C[MIX_AUX] = {pal::cyan, pal::purple, pal::yellow, pal::pink, pal::tan, pal::silver}; return C[a % MIX_AUX]; }
const int axisOf[4] = {AX_LX, AX_LY, AX_RX, AX_RY};
const float sgn[4] = {1.f, -1.f, 1.f, -1.f};
float stick(const InputState& in, int i, float dt) { return RelEncoder::delta(in.axis[axisOf[i]] * sgn[i], dt, .7f); }
float clampf(float x, float a, float b) { return x < a ? a : (x > b ? b : x); }
const char* compName(int t) { static const char* N[COMP_TYPES] = {"OPTO", "VCA", "FET"}; return N[t % COMP_TYPES]; }

// luk VU na plytce: dB -20..+3, czerwona strefa powyzej 0. plate = kolor tla (ciemny / szary / bialy) — na jasnym
// tle podzialka i wskazowka ciemne.
void drawVu(Canvas& c, float x, float y, float w, float h, float db, Color plate) {
    c.fillRoundRect(x, y, w, h, 6, plate);
    const bool light = plate.r > 110;
    if (plate.r < 20) c.strokeRoundRect(x, y, w, h, 6, 1.f, rgb(54, 54, 60));
    const Color pivot = light ? rgb(20, 20, 20) : rgb(200, 200, 200);
    const Color sc = light ? rgb(40, 40, 44) : rgb(170, 170, 178), lab = light ? rgb(70, 70, 74) : rgb(130, 130, 136);
    const Color rz = light ? rgb(215, 40, 30) : pal::red, needle = light ? rgb(20, 20, 20) : rgb(255, 120, 110);
    const float cx = x + w * .5f, cy = y + h - 7, r = h - 21;
    auto ang = [](float d) { const float u = clampf((d + 20.f) / 23.f, 0.f, 1.f); return -1.5707963f + (u - .5f) * 1.7f; };
    float pts[2 * 25];
    for (int i = 0; i <= 24; i++) { const float a = ang(-20.f + 20.f * i / 24.f); pts[2 * i] = cx + r * cosf(a); pts[2 * i + 1] = cy + r * sinf(a); }
    c.polyline(pts, 25, 1.2f, sc);
    for (int i = 0; i <= 8; i++) { const float a = ang(3.f * i / 8.f); pts[2 * i] = cx + r * cosf(a); pts[2 * i + 1] = cy + r * sinf(a); }
    c.polyline(pts, 9, 3.f, rz);
    static const float T[5] = {-20.f, -10.f, -5.f, 0.f, 3.f};
    static const char* TL[5] = {"20", "10", "5", "0", "3"};
    for (int k = 0; k < 5; k++) {
        const float a = ang(T[k]);
        c.line(cx + (r - 5) * cosf(a), cy + (r - 5) * sinf(a), cx + r * cosf(a), cy + r * sinf(a), 1.f, k >= 3 ? rz : sc);
        drawText(c, cx + (r + 7) * cosf(a), cy + (r + 7) * sinf(a) - 3, TL[k], 6, 1.f, k >= 3 ? rz : lab, ALIGN_CENTER);
    }
    const float a = ang(db);
    c.line(cx, cy, cx + (r + 2) * cosf(a), cy + (r + 2) * sinf(a), 1.6f, needle);
    c.fillCircle(cx, cy, 3.2f, pivot);
}

// segmentowy miernik jak w ARRANGE: -48..0 dB, zielony -> zolty (-12) -> czerwony (-3)
void segMeter(Canvas& c, float x, float y, float w, float level) {
    const int segs = 12;
    const float db = level > 1e-5f ? 20.f * log10f(level) : -100.f;
    const int lit = (int)((db + 48.f) / 48.f * segs + .5f);
    const float sw = w / segs;
    for (int i = 0; i < segs; i++) {
        const float sdb = -48.f + 48.f * (i + 1) / segs;
        const Color on = sdb > -3.f ? pal::red : (sdb > -12.f ? pal::yellow : pal::green);
        c.fillRect(x + i * sw, y, sw - 1.f, 3.f, i < lit ? on : rgb(34, 34, 38));
    }
}

// mala galeczka (wartosc 0..1 lukiem w kolorze)
void miniDial(Canvas& c, float cx, float cy, float r, float v, Color col) {
    const float a0 = .75f * 3.14159265f, a1 = 2.25f * 3.14159265f, a = a0 + (a1 - a0) * clampf(v, 0, 1);
    c.arc(cx, cy, r, a0, a1, 1.f, rgb(60, 60, 66));
    if (v > 0.f) c.arc(cx, cy, r, a0, a, 1.8f, col);
    c.line(cx, cy, cx + (r - .5f) * cosf(a), cy + (r - .5f) * sinf(a), 1.2f, v > 0.f ? pal::white : pal::dim);
}

// przelacznik / rzad przyciskow (np. RATIO): sel podswietlony
void segButtons(Canvas& c, float x, float y, float w, float h, const char* const* labels, int n, int sel, bool on) {
    const float gap = 6.f, bw = (w - gap * (n - 1)) / n;
    for (int k = 0; k < n; k++) {
        const float bx = x + k * (bw + gap);
        if (k == sel) c.fillRoundRect(bx, y, bw, h, 5, on ? pal::white : pal::dim);
        else c.strokeRoundRect(bx, y, bw, h, 5, 1.2f, pal::dim);
        drawText(c, bx + bw * .5f, y + h * .5f - 5, labels[k], 10, 1.3f, k == sel ? rgb(12, 12, 12) : pal::dim, ALIGN_CENTER);
    }
}

// duza galka okna (kolo, luk wartosci w kolorze enkodera, wskaznik, podpis i wartosc)
void bigKnob(Canvas& c, float cx, float cy, float r, float v, Color col, const char* label, const char* val, bool on = true) {
    const float a0 = .75f * 3.14159265f, a1 = 2.25f * 3.14159265f;
    const Color base = on ? rgb(110, 110, 120) : rgb(60, 60, 64);
    c.circle(cx, cy, r, 1.6f, base);
    c.arc(cx, cy, r + 5, a0, a1, 1.f, pal::faint);
    if (on) c.arc(cx, cy, r + 5, a0, a0 + (a1 - a0) * clampf(v, 0, 1), 2.4f, col);
    const float a = a0 + (a1 - a0) * clampf(v, 0, 1);
    c.line(cx + r * .25f * cosf(a), cy + r * .25f * sinf(a), cx + r * .9f * cosf(a), cy + r * .9f * sinf(a), 2.f, on ? pal::white : pal::dim);
    drawText(c, cx, cy + r + 12, label, 8, 1.2f, on ? col : pal::dim, ALIGN_CENTER);
    if (val && val[0]) drawText(c, cx, cy + r + 26, val, 9, 1.3f, on ? pal::white : pal::dim, ALIGN_CENTER);
}

// miernik sciszania (luk 0..-20 dB, wskazowka w lewo)
void grMeter(Canvas& c, float cx, float cy, float r, float gr, const char* title, bool on) {
    auto ang = [](float d) { const float u = 1.f - clampf(d / 20.f, 0.f, 1.f); return -1.5707963f + (u - .5f) * 1.9f; };
    float pts[2 * 33];
    for (int i = 0; i <= 32; i++) { const float a = ang(20.f * i / 32.f); pts[2 * i] = cx + r * cosf(a); pts[2 * i + 1] = cy + r * sinf(a); }
    c.polyline(pts, 33, 1.4f, rgb(150, 150, 160));
    static const int T[6] = {0, 2, 5, 10, 15, 20};
    char b[8];
    for (int k = 0; k < 6; k++) {
        const float a = ang((float)T[k]);
        c.line(cx + (r - 7) * cosf(a), cy + (r - 7) * sinf(a), cx + r * cosf(a), cy + r * sinf(a), 1.2f, rgb(150, 150, 160));
        snprintf(b, sizeof b, k ? "-%d" : "%d", T[k]);
        drawText(c, cx + (r + 12) * cosf(a), cy + (r + 12) * sinf(a) - 4, b, 7, 1.1f, pal::dim, ALIGN_CENTER);
    }
    const float a = ang(on ? gr : 0.f);
    c.line(cx, cy, cx + (r - 2) * cosf(a), cy + (r - 2) * sinf(a), 2.f, pal::red);
    c.fillCircle(cx, cy, 4.f, pal::white);
    drawText(c, cx, cy + 14, title, 8, 1.2f, pal::dim, ALIGN_CENTER);
}

void eqCurve(Canvas& c, const MixEq& e, float x, float y, float w, float h, float rangeDb, Color col, float lw) {
    float pts[2 * 49];
    for (int i = 0; i <= 48; i++) {
        const float u = i / 48.f, hz = 20.f * powf(1000.f, u);
        const float db = eqResponseDb(e, hz, 48000.f);
        pts[2 * i] = x + w * u;
        pts[2 * i + 1] = y + h * .5f - clampf(db / rangeDb, -1.f, 1.f) * h * .5f;
    }
    c.polyline(pts, 49, lw, col);
}
}  // namespace

// ---------------- dane kanalu ----------------
static int nStrips(const Project& p) { return p.nTracks + MIX_AUX; }

bool MixerView::rowValid(int strip, int row) const {
    const Project& p = app_->project();
    if (strip < 0) return row == R_COMP || row == R_VOL;  // master: BUS COMP, glosnosc
    if (strip >= p.nTracks) return row == R_EQ || row == R_INS0 || row == R_VOL;  // AUX: EQ, efekt, poziom
    return true;
}

void MixerView::moveRow(int d) {
    for (int k = 0; k < R_COUNT; k++) {
        row_ = (row_ + d + R_COUNT) % R_COUNT;
        if (rowValid(strip_, row_)) return;
    }
}

void MixerView::openWindow(int w, int slot) {
    win_ = w;
    slot_ = slot;
    eqLayer_ = 0;
    fxPage_ = 0;
    fxReset_ = true;
    if (w == W_FX) {
        const Project& p = app_->project();
        const FxSlot& s = strip_ >= p.nTracks ? p.aux[strip_ - p.nTracks].fx : p.tracks[strip_].ins[slot];
        fxSel_ = s.fx ? s.fx : 1;
        fxList_ = s.fx == FX_NONE;  // pusty slot: od razu lista wyboru
    }
}

// ---------------- sterowanie ----------------
void MixerView::update(const Buttons& b, const InputState& in, float dt) {
    dt_ = dt;
    if (b.pressed(B_START)) app_->togglePlaySong();
    const Project& p = app_->project();
    if (strip_ >= nStrips(p)) strip_ = nStrips(p) - 1;
    switch (win_) {
    case W_EQ: updateEq(b, in, dt); return;
    case W_COMP: updateComp(b, in, dt); return;
    case W_BUS: updateBus(b, in, dt); return;
    case W_FX: updateFx(b, in, dt); return;
    default: updateMain(b, in, dt);
    }
}

void MixerView::updateMain(const Buttons& b, const InputState& in, float dt) {
    Project& p = app_->project();
    const int N = nStrips(p);
    const int page = strip_ >= 0 ? strip_ / PER_PAGE : 0;
    if (b.repeat(B_RIGHT)) {
        if (strip_ < 0) strip_ = ((page + 1) * PER_PAGE < N) ? (page + 1) * PER_PAGE : 0;
        else if (strip_ % PER_PAGE == PER_PAGE - 1 || strip_ == N - 1) strip_ = -1;
        else strip_++;
    }
    if (b.repeat(B_LEFT)) {
        if (strip_ < 0) { const int last = page * PER_PAGE + PER_PAGE - 1; strip_ = last < N ? last : N - 1; }
        else if (strip_ % PER_PAGE == 0) strip_ = -1;
        else strip_--;
    }
    if (b.pressed(B_R1) && strip_ >= 0 && (page + 1) * PER_PAGE < N) strip_ = (page + 1) * PER_PAGE;
    if (b.pressed(B_L1) && strip_ >= 0 && page > 0) strip_ = (page - 1) * PER_PAGE;
    if (!rowValid(strip_, row_)) { row_ = R_VOL; }
    if (b.repeat(B_UP)) moveRow(-1);
    if (b.repeat(B_DOWN)) moveRow(1);
    if (row_ >= R_SEND0 && row_ < R_SEND0 + MIX_AUX) sendSel_ = row_ - R_SEND0;
    const bool isTrack = strip_ >= 0 && strip_ < p.nTracks;
    if (b.pressed(B_A)) {
        if (row_ == R_EQ) openWindow(W_EQ);
        else if (row_ == R_COMP) openWindow(strip_ < 0 ? W_BUS : W_COMP);
        else if (row_ >= R_INS0 && row_ < R_INS0 + MIX_INSERTS) openWindow(W_FX, row_ - R_INS0);
        else if (row_ == R_PAN && isTrack) { p.tracks[strip_].pan = 0.f; app_->markDirty(); }
    }
    if (b.pressed(B_B) && strip_ >= 0 && row_ >= R_INS0 && row_ < R_INS0 + MIX_INSERTS) {  // B = BYPASS insertu / efektu AUX
        FxSlot* fs = isTrack ? &p.tracks[strip_].ins[row_ - R_INS0] : (row_ == R_INS0 ? &p.aux[strip_ - p.nTracks].fx : nullptr);
        if (fs && fs->fx) { fs->bypass = !fs->bypass; app_->markDirty(); }
    }
    if (isTrack && b.pressed(B_Y)) { p.tracks[strip_].mute = !p.tracks[strip_].mute; app_->markDirty(); }
    if (isTrack && b.pressed(B_X)) { p.tracks[strip_].solo = !p.tracks[strip_].solo; app_->markDirty(); }
    // galki: niebieska = glosnosc kanalu, zielona = panorama, biala = wysylka, czerwona = master
    float d[4];
    for (int i = 0; i < 4; i++) d[i] = stick(in, i, dt);
    bool ch = false;
    if (d[0] != 0.f) {
        float* v = strip_ < 0 ? &p.masterVol : (isTrack ? &p.tracks[strip_].volume : &p.aux[strip_ - p.nTracks].level);
        *v = clampf(*v + d[0], 0.f, 1.f); ch = true;
    }
    if (d[1] != 0.f && isTrack) { p.tracks[strip_].pan = clampf(p.tracks[strip_].pan + d[1] * 2.f, -1.f, 1.f); ch = true; }
    if (d[2] != 0.f && isTrack) { float& s = p.tracks[strip_].send[sendSel_]; s = clampf(s + d[2], 0.f, 1.f); ch = true; }
    if (d[3] != 0.f) { p.masterVol = clampf(p.masterVol + d[3], 0.f, 1.f); ch = true; }
    if (ch) app_->markDirty();
}

static MixEq* eqTarget(Project& p, int strip) {
    if (strip >= 0 && strip < p.nTracks) return &p.tracks[strip].eq;
    if (strip >= p.nTracks && strip < p.nTracks + MIX_AUX) return &p.aux[strip - p.nTracks].eq;
    return nullptr;
}

void MixerView::updateEq(const Buttons& b, const InputState& in, float dt) {
    Project& p = app_->project();
    MixEq* e = eqTarget(p, strip_);
    if (!e || b.pressed(B_B)) { win_ = W_NONE; return; }
    if (b.pressed(B_L3)) eqLayer_ = (eqLayer_ + 1) % 3;
    bool ch = false;
    if (b.pressed(B_Y)) { e->max = !e->max; ch = true; }
    if (b.pressed(B_X)) { *e = MixEq(); ch = true; }  // CLEAN: plasko
    if (b.pressed(B_A)) { e->on = !e->on; ch = true; }
    const float range = e->max ? 18.f : 12.f;
    static const float FMIN[3] = {20.f, 100.f, 1000.f}, FMAX[3] = {800.f, 10000.f, 18000.f};
    for (int i = 0; i < 4; i++) {
        const float d = stick(in, i, dt);
        if (d == 0.f) continue;
        ch = true;
        if (i == 3) { e->out = clampf(e->out + d * 24.f, -12.f, 12.f); continue; }
        if (eqLayer_ == 0) e->gain[i] = clampf(e->gain[i] + d * range * 2.f, -range, range);
        else if (eqLayer_ == 1) e->freq[i] = clampf(e->freq[i] * powf(2.f, d * 6.f), FMIN[i], FMAX[i]);
        else e->q[i] = clampf(e->q[i] * powf(2.f, d * 3.f), .3f, i == 1 ? 8.f : 2.f);
    }
    if (!e->max) for (int i = 0; i < 3; i++) e->gain[i] = clampf(e->gain[i], -12.f, 12.f);
    if (ch) app_->markDirty();
}

void MixerView::updateComp(const Buttons& b, const InputState& in, float dt) {
    Project& p = app_->project();
    if (strip_ < 0 || strip_ >= p.nTracks || b.pressed(B_B)) { win_ = W_NONE; return; }
    MixComp& c = p.tracks[strip_].comp;
    bool ch = false;
    if (b.pressed(B_L1)) { c.type = (uint8_t)((c.type + COMP_TYPES - 1) % COMP_TYPES); ch = true; }
    if (b.pressed(B_R1)) { c.type = (uint8_t)((c.type + 1) % COMP_TYPES); ch = true; }
    if (b.pressed(B_A)) { c.on = !c.on; ch = true; }
    if (b.pressed(B_L3)) compAdv_ = !compAdv_;
    float* q = c.p[c.type];
    if (b.pressed(B_Y)) {
        if (c.type == COMP_OPTO) q[2] = q[2] >= .5f ? 0.f : 1.f;
        else if (c.type == COMP_VCA) q[3] = q[3] >= .5f ? 0.f : 1.f;
        else { int r = (int)(q[4] * 4.999f); r = (r + 1) % 5; q[4] = r / 4.f; }
        ch = true;
    }
    static const int MAP[COMP_TYPES][4] = {{0, 1, 3, -1}, {0, 1, 2, -1}, {0, 1, 2, 3}};
    for (int i = 0; i < 4; i++) {
        const float d = stick(in, i, dt);
        const int k = MAP[c.type][i];
        if (d == 0.f || k < 0) continue;
        q[k] = clampf(q[k] + d, 0.f, 1.f);
        ch = true;
    }
    if (ch) app_->markDirty();
}

void MixerView::updateBus(const Buttons& b, const InputState& in, float dt) {
    Project& p = app_->project();
    if (b.pressed(B_B)) { win_ = W_NONE; return; }
    BusComp& s = p.bus;
    bool ch = false;
    if (b.pressed(B_A)) { s.on = !s.on; ch = true; }
    if (b.pressed(B_Y)) { s.ratio = (uint8_t)((s.ratio + 1) % 3); ch = true; }
    float d = stick(in, 0, dt);
    if (d != 0.f) { s.thresh = clampf(s.thresh + d, 0.f, 1.f); ch = true; }
    d = stick(in, 1, dt);
    if (d != 0.f) { s.makeup = clampf(s.makeup + d * 15.f, 0.f, 15.f); ch = true; }
    int st = stepEnc_[2].steps(in.axis[AX_RX], dt, 4.f);
    if (st) { int a = s.attack + st; s.attack = (uint8_t)(a < 0 ? 0 : (a > 5 ? 5 : a)); ch = true; }
    st = stepEnc_[3].steps(-in.axis[AX_RY], dt, 4.f);
    if (st) { int r = s.release + st; s.release = (uint8_t)(r < 0 ? 0 : (r > 4 ? 4 : r)); ch = true; }
    if (ch) app_->markDirty();
}

static FxSlot* fxTarget(Project& p, int strip, int slot) {
    if (strip >= 0 && strip < p.nTracks) return &p.tracks[strip].ins[slot];
    if (strip >= p.nTracks && strip < p.nTracks + MIX_AUX) return &p.aux[strip - p.nTracks].fx;
    return nullptr;
}

void MixerView::updateFx(const Buttons& b, const InputState& in, float dt) {
    Project& p = app_->project();
    FxSlot* s = fxTarget(p, strip_, slot_);
    if (!s) { win_ = W_NONE; return; }
    bool ch = false;
    if (fxList_) {  // lista wyboru efektu
        if (b.pressed(B_B)) { if (s->fx) fxList_ = false; else win_ = W_NONE; return; }
        if (b.repeat(B_UP) && fxSel_ > 0) fxSel_--;
        if (b.repeat(B_DOWN) && fxSel_ < FX_COUNT - 1) fxSel_++;
        if (b.pressed(B_A)) {
            if (fxSel_ != s->fx) { fxDefaults(*s, fxSel_); ch = true; }
            fxPage_ = 0;
            fxReset_ = true;
            if (s->fx) fxList_ = false; else win_ = W_NONE;
        }
        if (ch) app_->markDirty();
        return;
    }
    // edycja efektu: ekran efektu + galki
    if (b.pressed(B_B)) { win_ = W_NONE; return; }
    if (b.pressed(B_X)) { fxList_ = true; fxSel_ = s->fx; return; }
    if (b.pressed(B_Y)) { fxDefaults(*s, FX_NONE); app_->markDirty(); win_ = W_NONE; return; }
    if (b.pressed(B_A)) { s->bypass = !s->bypass; app_->markDirty(); }
    const FxInfo& fi = fxInfo(s->fx);
    const bool hasP2 = fi.page2[0][0] || fi.page2[1][0] || fi.page2[2][0];
    if (b.pressed(B_L3) && hasP2) fxPage_ ^= 1;
    for (int i = 0; i < 4; i++) {
        const float d = stick(in, i, dt);
        if (d == 0.f) continue;
        if (fxPage_ && (i > 2 || !fi.page2[i][0])) continue;
        const int w = fxPage_ ? 5 + i : i;
        int x = s->w[w] + (int)lrintf(d * 32767.f);
        s->w[w] = (int16_t)(x < 0 ? 0 : (x > 32767 ? 32767 : x));
        ch = true;
    }
    if (ch) app_->markDirty();
}

// ---------------- rysowanie ----------------
void MixerView::renderStrip(Canvas& c, int strip, float x, float w, float dt) {
    const Project& p = app_->project();
    AudioEngine* e = app_->engine();
    const bool isTrack = strip < p.nTracks;
    const int a = strip - p.nTracks;
    const Track* t = isTrack ? &p.tracks[strip] : nullptr;
    const Color col = isTrack ? pal::track[t->color % pal::TRACK_COLORS] : auxColor(a);
    const bool sel = strip == strip_;
    const float cx = x + w * .5f;
    if (sel) c.fillRoundRect(x + 2, 48, w - 4, 332, 8, rgb(18, 18, 22));
    // VU na plytce: kanal ciemnoszary, powrot AUX szary
    const float pk = e ? (isTrack ? e->chPeak(strip) : e->auxPeak(a)) : 0.f;
    const float db = 20.f * log10f(pk > 1e-5f ? pk : 1e-5f) + VU_REF;
    float& v = vu_[strip & 63];
    v = db > v ? db : v - 24.f * dt;
    if (v < -40.f) v = -40.f;
    drawVu(c, x + 6, 52, w - 12, 60, v, isTrack ? PLATE_CH : PLATE_AUX);
    auto hl = [&](int row, float y, float h) {
        if (sel && row_ == row && win_ == W_NONE) c.strokeRoundRect(x + 3, y - 2, w - 6, h + 4, 5, 1.4f, pal::white);
    };
    // podglad EQ
    const MixEq& eq = isTrack ? t->eq : p.aux[a].eq;
    c.strokeRoundRect(x + 7, 118, w - 14, 20, 4, 1.f, pal::faint);
    eqCurve(c, eq, x + 9, 120, w - 18, 16, 12.f, eq.on && !eq.flat() ? col : rgb(80, 80, 80), 1.3f);
    drawText(c, x + 11, 121, "EQ", 5, .9f, pal::dim);
    hl(R_EQ, 118, 20);
    // inserty (kanal: 4, powrot AUX: 1 efekt)
    const int nIns = isTrack ? MIX_INSERTS : 1;
    for (int k = 0; k < nIns; k++) {
        const float y = INS_Y + k * INS_STEP;
        const FxSlot& s = isTrack ? t->ins[k] : p.aux[a].fx;
        const bool act = s.fx && !s.bypass;
        if (act) c.fillRoundRect(x + 7, y, w - 14, INS_H, 6, rgb(40, 34, 26));
        c.strokeRoundRect(x + 7, y, w - 14, INS_H, 6, 1.f, act ? pal::tan : (s.fx ? rgb(90, 70, 50) : pal::faint));
        drawText(c, cx, y + 3, s.fx ? fxInfo(s.fx).shortName : (isTrack ? "INSERT" : "EFFECT"), 6, 1.f, act ? pal::tan : pal::dim, ALIGN_CENTER);
        if (s.fx && s.bypass) c.fillCircle(x + 14, y + INS_H * .5f, 2.2f, pal::red);  // BYPASS
        hl(R_INS0 + k, y, INS_H);
    }
    if (isTrack) {
        // podglad kompresora
        const MixComp& cp = t->comp;
        c.strokeRoundRect(x + 7, 142, w - 14, 16, 4, 1.f, pal::faint);
        drawText(c, x + 12, 146, cp.on ? compName(cp.type) : "COMP", 7, 1.1f, cp.on ? pal::white : pal::dim);
        if (cp.on) {
            const float gr = e ? e->chGr(strip) : 0.f, bw = (w - 60) * clampf(gr / 12.f, 0.f, 1.f);
            c.line(x + 48, 150, x + w - 12, 150, 3.f, pal::faint);
            if (bw > .5f) c.line(x + w - 12 - bw, 150, x + w - 12, 150, 3.f, pal::red);
        }
        hl(R_COMP, 142, 16);
        // wysylki A..F: litera, galeczka (poziom wysylki), miernik sygnalu wysylki (jak w ARRANGE)
        for (int k = 0; k < MIX_AUX; k++) {
            const float y = 170 + k * 12.5f;
            const float sv = t->send[k];
            char L[2] = {(char)('A' + k), 0};
            drawText(c, x + 10, y - 3, L, 6, 1.f, sv > 0 ? auxColor(k) : pal::dim);
            miniDial(c, x + 26, y, 4.5f, sv, auxColor(k));
            segMeter(c, x + 36, y - 1.5f, w - 46, sv > 0 && e ? e->chPeak(strip) * sv : 0.f);
            hl(R_SEND0 + k, y - 5, 10);
        }
        // panorama
        const float py = 318, ang = -1.5707963f + t->pan * 2.3f;
        c.fillCircle(cx, py, 9.f, dimc(col, .55f));
        c.line(cx, py, cx + 8 * cosf(ang), py + 8 * sinf(ang), 2.f, pal::white);
        drawText(c, cx - 24, py - 3, "L", 6, 1.f, pal::dim);
        drawText(c, cx + 19, py - 3, "R", 6, 1.f, pal::dim);
        hl(R_PAN, 308, 20);
    } else {
        drawText(c, cx, 186, "RETURN", 7, 1.f, pal::dim, ALIGN_CENTER);
        char lb[8];
        snprintf(lb, sizeof lb, "AUX %c", 'A' + a);
        drawText(c, cx, 202, lb, 10, 1.3f, col, ALIGN_CENTER);
    }
    // glosnosc i nazwa
    char b[16];
    const float vol = isTrack ? t->volume : p.aux[a].level;
    snprintf(b, sizeof b, "%02d", (int)lrintf(vol * 99.f));
    const bool muted = isTrack && !p.audible(strip);
    drawText(c, cx, 336, b, 22, 1.5f, muted ? dimc(col, .4f) : col, ALIGN_CENTER);
    hl(R_VOL, 334, 26);
    char nm[16];
    if (isTrack) snprintf(nm, sizeof nm, "%s", t->name);
    else snprintf(nm, sizeof nm, "AUX %c", 'A' + a);
    float tw = textWidth(nm, 8);
    if (tw > w - 24) tw = w - 24;
    drawTextFit(c, cx - tw * .5f, 366, nm, 8, 1.1f, pal::white, w - 24);
    if (isTrack && t->mute) drawText(c, x + 7, 366, "M", 7, 1.f, pal::red);
    if (isTrack && t->solo) drawText(c, x + w - 13, 366, "S", 7, 1.f, pal::yellow);
}

void MixerView::renderMaster(Canvas& c, float x, float w) {
    const Project& p = app_->project();
    AudioEngine* e = app_->engine();
    const bool sel = strip_ < 0;
    const float cx = x + w * .5f;
    if (sel) c.fillRoundRect(x + 2, 48, w - 4, 332, 8, rgb(18, 18, 22));
    const float pk = e ? e->masterPeak() : 0.f;
    const float db = 20.f * log10f(pk > 1e-5f ? pk : 1e-5f) + VU_REF;
    vuM_ = db > vuM_ ? db : vuM_ - 24.f * dt_;
    if (vuM_ < -40.f) vuM_ = -40.f;
    drawVu(c, x + 6, 52, w - 12, 60, vuM_, PLATE_MASTER);
    // BUS COMP
    c.strokeRoundRect(x + 8, 120, w - 16, 40, 5, 1.f, pal::faint);
    drawText(c, cx, 128, "BUS COMP", 7, 1.1f, p.bus.on ? pal::white : pal::dim, ALIGN_CENTER);
    if (p.bus.on) {
        static const char* R[3] = {"2:1", "4:1", "10:1"};
        drawText(c, x + 14, 142, R[p.bus.ratio % 3], 7, 1.f, pal::dim);
        const float gr = e ? e->masterGr() : 0.f, bw = (w - 64) * clampf(gr / 12.f, 0.f, 1.f);
        c.line(x + 48, 145.5f, x + w - 14, 145.5f, 3.f, pal::faint);
        if (bw > .5f) c.line(x + w - 14 - bw, 145.5f, x + w - 14, 145.5f, 3.f, pal::red);
    } else drawText(c, cx, 142, "OFF", 7, 1.f, pal::dim, ALIGN_CENTER);
    if (sel && row_ == R_COMP) c.strokeRoundRect(x + 4, 117, w - 8, 46, 6, 1.4f, pal::white);
    char b[8];
    snprintf(b, sizeof b, "%02d", (int)lrintf(p.masterVol * 99.f));
    drawText(c, cx, 336, b, 22, 1.5f, pal::white, ALIGN_CENTER);
    if (sel && row_ == R_VOL) c.strokeRoundRect(x + 4, 332, w - 8, 30, 6, 1.4f, pal::white);
    drawText(c, cx, 366, "MASTER", 8, 1.1f, pal::white, ALIGN_CENTER);
}

void MixerView::render(Canvas& c) {
    c.clear(pal::bg);
    if (!vuInit_) { for (float& x : vu_) x = -40.f; vuInit_ = true; }
    if (win_ == W_EQ) { renderEq(c); return; }
    if (win_ == W_COMP) { renderComp(c); return; }
    if (win_ == W_BUS) { renderBus(c); return; }
    if (win_ == W_FX) { renderFx(c); return; }
    const Project& p = app_->project();
    AudioEngine* e = app_->engine();
    const bool playing = e && e->isPlaying() && e->playMode() == Sequencer::SONG;
    const int tick = playing ? e->playTick() : 0;
    const int N = nStrips(p), pages = (N + PER_PAGE - 1) / PER_PAGE;
    const int page = strip_ >= 0 ? strip_ / PER_PAGE : lastPage_;
    lastPage_ = page;
    char pg[16];
    snprintf(pg, sizeof pg, "PAGE %d/%d", page + 1, pages);
    screenHeader(c, ICON_ARRANGE, pal::white, "MIXER", pg, pal::dim, tick / TICKS_PER_BAR + 1, (tick % TICKS_PER_BAR) / PPQ + 1, playing, p.bpm);
    for (int i = 0; i < PER_PAGE; i++) {
        const int s = page * PER_PAGE + i;
        if (s >= N) break;
        renderStrip(c, s, SX0 + i * SW, SW, dt_);
        if (i) c.line(SX0 + i * SW, 60, SX0 + i * SW, 372, 1.f, rgb(28, 28, 32));
    }
    c.line(MX - 4, 52, MX - 4, 376, 1.f, rgb(60, 60, 66));
    renderMaster(c, MX, MW);
    // podpowiedz i pasek galek
    const bool onIns = row_ >= R_INS0 && row_ < R_INS0 + MIX_INSERTS;
    drawText(c, 320, 392, onIns ? "A = EDIT   B = BYPASS   Y = MUTE   X = SOLO   L1/R1 = PAGE" : "A = OPEN   Y = MUTE   X = SOLO   L1/R1 = PAGE   D-PAD = MOVE", 7, 1.f, rgb(80, 80, 80), ALIGN_CENTER);
    const bool isTrack = strip_ >= 0 && strip_ < p.nTracks;
    char v[4][16];
    const float vol = strip_ < 0 ? p.masterVol : (isTrack ? p.tracks[strip_].volume : p.aux[strip_ - p.nTracks].level);
    snprintf(v[0], 16, "%02d", (int)lrintf(vol * 99.f));
    if (isTrack) {
        const float pn = p.tracks[strip_].pan;
        if (fabsf(pn) < .02f) snprintf(v[1], 16, "C"); else snprintf(v[1], 16, "%c%d", pn < 0 ? 'L' : 'R', (int)lrintf(fabsf(pn) * 50.f));
        snprintf(v[2], 16, "%02d", (int)lrintf(p.tracks[strip_].send[sendSel_] * 99.f));
    } else { v[1][0] = 0; v[2][0] = 0; }
    snprintf(v[3], 16, "%02d", (int)lrintf(p.masterVol * 99.f));
    char sl[12];
    snprintf(sl, sizeof sl, "SEND %c", 'A' + sendSel_);
    const char* labels[4] = {strip_ < 0 ? "MASTER" : (isTrack ? "VOLUME" : "RETURN"), isTrack ? "PAN" : "", isTrack ? sl : "", "MASTER"};
    const char* vals[4] = {v[0], v[1], v[2], v[3]};
    const float pos[4] = {vol, isTrack ? (p.tracks[strip_].pan + 1.f) * .5f : 0.f, isTrack ? p.tracks[strip_].send[sendSel_] : 0.f, p.masterVol};
    encoderBar(c, labels, vals, pos);
}

static void windowHeader(Canvas& c, const char* title, const char* sub, Color subCol) {
    drawText(c, 20, 14, title, 13, 1.5f, pal::white);
    if (sub && sub[0]) drawText(c, 36 + textWidth(title, 13), 17, sub, 10, 1.3f, subCol);
    c.line(20, 44, 620, 44, 1.f, pal::faint);
}

static void stripTitle(const Project& p, int strip, char* out, int n, Color* col) {
    if (strip >= 0 && strip < p.nTracks) { snprintf(out, n, "%s", p.tracks[strip].name); *col = pal::track[p.tracks[strip].color % pal::TRACK_COLORS]; }
    else { snprintf(out, n, "AUX %c", 'A' + (strip - p.nTracks)); *col = auxColor(strip - p.nTracks); }
}

void MixerView::renderEq(Canvas& c) {
    Project& p = app_->project();
    MixEq* e = eqTarget(p, strip_);
    if (!e) return;
    char nm[16];
    Color col;
    stripTitle(p, strip_, nm, sizeof nm, &col);
    windowHeader(c, "EQ", nm, col);
    const float px = 20, py = 56, pw = 600, ph = 330;
    c.fillRoundRect(px, py, pw, ph, 12, rgb(8, 8, 10));
    c.strokeRoundRect(px, py, pw, ph, 12, 1.f, rgb(50, 50, 56));
    const float range = e->max ? 18.f : 12.f;
    const float fy0 = 112, fy1 = 320, fmid = (fy0 + fy1) * .5f;
    static const float CXS[3] = {130, 260, 390};
    static const Color FC[3] = {rgb(90, 125, 200), rgb(176, 130, 80), rgb(150, 155, 165)};
    static const char* BN[3] = {"LOW", "MID", "HIGH"};
    c.line(470, py + 10, 470, py + ph - 10, 1.f, rgb(50, 50, 56));
    for (int k = 0; k < 4; k++) {
        const float cx = k < 3 ? CXS[k] : 545;
        for (int t = 0; t <= 14; t++) {  // podzialka
            const float y = fy0 + (fy1 - fy0) * t / 14.f;
            c.line(cx - (t == 7 ? 10 : 6), y, cx + (t == 7 ? 10 : 6), y, 1.f, t == 7 ? rgb(90, 90, 96) : rgb(48, 48, 54));
        }
    }
    // krzywa odpowiedzi na calej szerokosci pasm
    if (e->on) {
        MixEq tmp = *e;
        tmp.out = 0;
        float pts[2 * 81];
        for (int i = 0; i <= 80; i++) {
            const float u = i / 80.f, hz = 20.f * powf(1000.f, u), db = eqResponseDb(tmp, hz, 48000.f);
            pts[2 * i] = 50 + 410 * u;
            pts[2 * i + 1] = fmid - clampf(db / range, -1.f, 1.f) * (fmid - fy0);
        }
        c.polyline(pts, 81, 1.6f, rgb(150, 70, 120));
    }
    char b[24];
    for (int k = 0; k < 3; k++) {
        const float cx = CXS[k];
        const float f = e->freq[k];
        if (f >= 1000.f) snprintf(b, sizeof b, f >= 10000.f ? "%.0f KHZ" : "%.1f KHZ", f / 1000.f); else snprintf(b, sizeof b, "%.0f HZ", f);
        drawText(c, cx, 78, b, 10, 1.3f, eqLayer_ == 1 ? pal::enc[k] : rgb(110, 110, 116), ALIGN_CENTER);
        const float y = fmid - clampf(e->gain[k] / range, -1.f, 1.f) * (fmid - fy0);
        c.fillRoundRect(cx - 20, y - 7, 40, 14, 3, e->on ? FC[k] : rgb(60, 60, 64));
        c.line(cx - 18, y, cx + 18, y, 1.5f, pal::white);
        drawText(c, cx, 334, BN[k], 12, 1.5f, pal::white, ALIGN_CENTER);
        snprintf(b, sizeof b, "Q %.2f", e->q[k]);
        drawText(c, cx, 356, b, 8, 1.1f, eqLayer_ == 2 ? pal::enc[k] : pal::dim, ALIGN_CENTER);
        snprintf(b, sizeof b, "%+.1f DB", e->gain[k]);
        drawText(c, cx, 96, b, 8, 1.1f, eqLayer_ == 0 ? pal::enc[k] : pal::dim, ALIGN_CENTER);
    }
    // wyjscie, MAX EQ, CLEAN
    const float oy = fmid - clampf(e->out / 12.f, -1.f, 1.f) * (fmid - fy0);
    c.fillRoundRect(525, oy - 7, 40, 14, 3, pal::red);
    c.line(527, oy, 563, oy, 1.5f, pal::white);
    if (e->max) c.fillRoundRect(495, 66, 100, 26, 5, pal::red);
    else c.strokeRoundRect(495, 66, 100, 26, 5, 1.2f, pal::red);
    drawText(c, 545, 73, "MAX EQ", 11, 1.4f, e->max ? pal::white : pal::red, ALIGN_CENTER);
    c.fillRoundRect(495, 330, 100, 26, 5, rgb(205, 205, 195));
    drawText(c, 545, 337, "CLEAN", 11, 1.4f, rgb(20, 20, 20), ALIGN_CENTER);
    if (!e->on) drawText(c, 260, 196, "EQ OFF", 22, 2.f, pal::dim, ALIGN_CENTER);
    static const char* LY[3] = {"GAIN", "FREQ", "Q"};
    snprintf(b, sizeof b, "L3 = LAYER: %s", LY[eqLayer_]);
    drawText(c, 20, 394, b, 8, 1.1f, pal::white);
    drawText(c, 620, 394, "A = ON/OFF  Y = MAX EQ  X = CLEAN  B = BACK", 7, 1.f, rgb(90, 90, 90), ALIGN_RIGHT);
    char v[4][16];
    const char* L[4];
    float pos[4];
    static const char* GN[3] = {"LOW", "MID", "HIGH"};
    static const char* FN[3] = {"LOW FREQ", "MID FREQ", "HIGH FREQ"};
    static const char* QN[3] = {"LOW Q", "MID Q", "HIGH Q"};
    for (int k = 0; k < 3; k++) {
        if (eqLayer_ == 0) { L[k] = GN[k]; snprintf(v[k], 16, "%+.1f", e->gain[k]); pos[k] = (e->gain[k] / range + 1.f) * .5f; }
        else if (eqLayer_ == 1) {
            L[k] = FN[k];
            const float f = e->freq[k];
            if (f >= 1000.f) snprintf(v[k], 16, "%.1fK", f / 1000.f); else snprintf(v[k], 16, "%.0f", f);
            pos[k] = log10f(f / 20.f) / 3.f;
        } else { L[k] = QN[k]; snprintf(v[k], 16, "%.2f", e->q[k]); pos[k] = log10f(e->q[k] / .3f) / log10f(k == 1 ? 8.f / .3f : 2.f / .3f); }
    }
    L[3] = "OUTPUT"; snprintf(v[3], 16, "%+.1f", e->out); pos[3] = (e->out / 12.f + 1.f) * .5f;
    const char* vals[4] = {v[0], v[1], v[2], v[3]};
    encoderBar(c, L, vals, pos);
}

// Wspolny uklad paneli kompresorow (symetria): panel 20..620 x 56..386, nazwa w lewym gornym rogu, BYPASS w prawym;
// po lewej okno miernika (44..296 x 112..362) z przyciskami pod miernikiem, po prawej siatka galek 2 x 2
// (srodki x 400 / 540, y 168 / 292) — pojedyncza galka w dolnym rzedzie na srodku (x 470).
namespace {
const float PX = 20, PY = 56, PW = 600, PH = 330, MBX = 44, MBY = 112, MBW = 252, MBH = 250;
const float KX[2] = {400, 540}, KY[2] = {168, 292}, KR = 30, KR2 = 26;
void compPanel(Canvas& c, const char* name, const char* style, Color edge, bool on) {
    c.fillRoundRect(PX, PY, PW, PH, 12, pal::bg);
    c.strokeRoundRect(PX, PY, PW, PH, 12, 1.4f, edge);
    for (int k = 0; k < 2; k++)  // sruby w rogach
        for (int j = 0; j < 2; j++) c.circle(PX + 12 + j * (PW - 24), PY + 12 + k * (PH - 24), 3.f, 1.f, pal::dim);
    drawText(c, PX + 28, PY + 12, name, 14, 1.8f, pal::white);
    drawText(c, PX + 28, PY + 34, style, 7, 1.f, pal::dim);
    if (!on) { c.fillRoundRect(PX + PW - 148, PY + 14, 120, 24, 12, rgb(60, 20, 20)); drawText(c, PX + PW - 88, PY + 20, "BYPASS", 11, 1.4f, pal::red, ALIGN_CENTER); }
    else { c.fillCircle(PX + PW - 40, PY + 26, 5.f, pal::red); drawText(c, PX + PW - 52, PY + 22, "ON", 8, 1.1f, pal::white, ALIGN_RIGHT); }
    c.strokeRoundRect(MBX, MBY, MBW, MBH, 8, 1.2f, rgb(90, 90, 96));
    c.strokeRoundRect(MBX + 10, MBY + 10, MBW - 20, 160, 6, 1.f, rgb(60, 60, 66));  // okienko miernika (z podpisem)
}
void compMeter(Canvas& c, float gr, const char* title, bool on) { grMeter(c, MBX + MBW * .5f, MBY + 132, 84, gr, title, on); }
}  // namespace

void MixerView::renderComp(Canvas& c) {
    Project& p = app_->project();
    if (strip_ < 0 || strip_ >= p.nTracks) return;
    MixComp& cp = p.tracks[strip_].comp;
    AudioEngine* e = app_->engine();
    char nm[16];
    Color col;
    stripTitle(p, strip_, nm, sizeof nm, &col);
    windowHeader(c, "COMPRESSOR", nm, col);
    static const char* MN[COMP_TYPES] = {"OPTO", "VCA", "FET"};
    segButtons(c, 330, 12, 290, 24, MN, COMP_TYPES, cp.type, true);
    const bool on = cp.on;
    const float gr = e ? e->chGr(strip_) : 0.f;
    const float* q = cp.p[cp.type];
    const float mcx = MBX + MBW * .5f, by = MBY + 186;  // srodek okna miernika, rzad pod miernikiem
    char b[4][24];
    if (cp.type == COMP_OPTO && !compAdv_) {
        renderOptoHand(c, q, gr, e ? e->chPeak(strip_) : 0.f, on, col);
    } else if (cp.type == COMP_VCA && !compAdv_) {
        renderVcaDam(c, q, gr, e ? e->chPeak(strip_) : 0.f, on, col);
    } else if (cp.type == COMP_OPTO) {
        compPanel(c, "OPTO-2", "LA-2A STYLE", rgb(150, 150, 160), on);
        compMeter(c, gr, "GAIN REDUCTION", on);
        static const char* ML[2] = {"COMPRESS", "LIMIT"};
        segButtons(c, MBX + 26, by, MBW - 52, 28, ML, 2, q[2] >= .5f ? 1 : 0, on);
        drawText(c, mcx, by + 40, "Y = MODE", 7, 1.f, pal::dim, ALIGN_CENTER);
        snprintf(b[0], 24, "%d", (int)lrintf(q[0] * 100));
        bigKnob(c, KX[0], KY[0], KR, q[0], pal::blue, "PEAK REDUCTION", b[0], on);
        snprintf(b[1], 24, "%+.0f DB", -10.f + 35.f * q[1]);
        bigKnob(c, KX[1], KY[0], KR, q[1], pal::green, "GAIN", b[1], on);
        snprintf(b[2], 24, "%d%%", (int)lrintf(q[3] * 100));
        bigKnob(c, (KX[0] + KX[1]) * .5f, KY[1], KR2, q[3], pal::white, "MIX", b[2], on);
    } else if (cp.type == COMP_VCA) {
        compPanel(c, "VCA-160", "DBX 160 STYLE", rgb(110, 110, 118), on);
        compMeter(c, gr, "GAIN REDUCTION", on);
        const bool above = gr > .5f, oe = q[3] >= .5f;
        c.fillCircle(MBX + 26, by + 14, 6, on && !above ? pal::green : rgb(30, 50, 30));
        c.fillCircle(MBX + MBW - 26, by + 14, 6, on && above ? pal::red : rgb(60, 26, 26));
        drawText(c, MBX + 26, by + 26, "BELOW", 6, 1.f, pal::dim, ALIGN_CENTER);
        drawText(c, MBX + MBW - 26, by + 26, "ABOVE", 6, 1.f, pal::dim, ALIGN_CENTER);
        static const char* OE[1] = {"OVEREASY"};
        if (oe) segButtons(c, MBX + 56, by, MBW - 112, 28, OE, 1, 0, on);
        else segButtons(c, MBX + 56, by, MBW - 112, 28, OE, 1, -1, on);
        drawText(c, mcx, by + 40, "Y = OVEREASY", 7, 1.f, pal::dim, ALIGN_CENTER);
        snprintf(b[0], 24, "%.0f DB", -50.f + 50.f * q[0]);
        bigKnob(c, KX[0], KY[0], KR, q[0], pal::blue, "THRESHOLD", b[0], on);
        if (q[1] > .97f) snprintf(b[1], 24, "INF:1"); else snprintf(b[1], 24, "%.1f:1", 1.f + 40.f * q[1] * q[1]);
        bigKnob(c, KX[1], KY[0], KR, q[1], pal::green, "COMPRESSION", b[1], on);
        snprintf(b[2], 24, "%+.0f DB", -20.f + 40.f * q[2]);
        bigKnob(c, (KX[0] + KX[1]) * .5f, KY[1], KR2, q[2], pal::white, "OUTPUT GAIN", b[2], on);
    } else if (!compAdv_) {
        renderFetPress(c, q, gr, e ? e->chPeak(strip_) : 0.f, on, col);
    } else {
        compPanel(c, "FET-76", "1176 STYLE", rgb(90, 90, 96), on);
        compMeter(c, gr, "GAIN REDUCTION", on);
        static const char* RN[5] = {"4", "8", "12", "20", "ALL"};
        segButtons(c, MBX + 14, by, MBW - 28, 28, RN, 5, (int)(q[4] * 4.999f), on);
        drawText(c, mcx, by + 40, "Y = RATIO", 7, 1.f, pal::dim, ALIGN_CENTER);
        snprintf(b[0], 24, "%+.0f DB", -10.f + 40.f * q[0]);
        bigKnob(c, KX[0], KY[0], KR, q[0], pal::blue, "INPUT", b[0], on);
        snprintf(b[1], 24, "%+.0f DB", -30.f + 40.f * q[1]);
        bigKnob(c, KX[1], KY[0], KR, q[1], pal::green, "OUTPUT", b[1], on);
        snprintf(b[2], 24, "%.0f US", (.0008f * (1.f - q[2]) + .00002f * q[2]) * 1e6f);
        bigKnob(c, KX[0], KY[1], KR2, q[2], pal::white, "ATTACK", b[2], on);
        snprintf(b[3], 24, "%.0f MS", (1.1f * (1.f - q[3]) + .05f * q[3]) * 1000.f);
        bigKnob(c, KX[1], KY[1], KR2, q[3], pal::red, "RELEASE", b[3], on);
    }
    drawText(c, 20, 394, "L1/R1 = MODEL", 8, 1.1f, pal::white);
    drawText(c, 150, 396, compAdv_ ? "L3 = SCENE" : "L3 = ADVANCED", 7, 1.f, pal::dim);
    static const char* YH[COMP_TYPES] = {"Y = MODE   A = ON/OFF   B = BACK", "Y = OVEREASY   A = ON/OFF   B = BACK", "Y = RATIO   A = ON/OFF   B = BACK"};
    drawText(c, 620, 394, YH[cp.type], 7, 1.f, rgb(90, 90, 90), ALIGN_RIGHT);
    char v[4][16];
    const char* L[4] = {"", "", "", ""};
    float pos[4] = {0, 0, 0, 0};
    if (cp.type == COMP_OPTO) {
        L[0] = "PEAK RED"; L[1] = "GAIN"; L[2] = "MIX";
        snprintf(v[0], 16, "%d", (int)lrintf(q[0] * 100)); snprintf(v[1], 16, "%+.0f", -10.f + 35.f * q[1]); snprintf(v[2], 16, "%d%%", (int)lrintf(q[3] * 100));
        pos[0] = q[0]; pos[1] = q[1]; pos[2] = q[3];
    } else if (cp.type == COMP_VCA) {
        L[0] = "THRESHOLD"; L[1] = "RATIO"; L[2] = "OUTPUT";
        snprintf(v[0], 16, "%.0f", -50.f + 50.f * q[0]);
        if (q[1] > .97f) snprintf(v[1], 16, "INF"); else snprintf(v[1], 16, "%.1f", 1.f + 40.f * q[1] * q[1]);
        snprintf(v[2], 16, "%+.0f", -20.f + 40.f * q[2]);
        pos[0] = q[0]; pos[1] = q[1]; pos[2] = q[2];
    } else {
        L[0] = "INPUT"; L[1] = "OUTPUT"; L[2] = "ATTACK"; L[3] = "RELEASE";
        snprintf(v[0], 16, "%+.0f", -10.f + 40.f * q[0]); snprintf(v[1], 16, "%+.0f", -30.f + 40.f * q[1]);
        snprintf(v[2], 16, "%d", (int)lrintf(q[2] * 100)); snprintf(v[3], 16, "%d", (int)lrintf(q[3] * 100));
        for (int k = 0; k < 4; k++) pos[k] = q[k];
    }
    for (int k = 0; k < 4; k++) if (!L[k][0]) v[k][0] = 0;
    const char* vals[4] = {v[0], v[1], v[2], v[3]};
    encoderBar(c, L, vals, pos);
}

// FET — widok prasy (scena 320x160 jak ekrany op1r, skala 2). Tor sygnalu od lewej: interfejs audio (galka = INPUT,
// kreci sie w strone oporu, diody = poziom) -> kabel do prasy (grubosc = RATIO 4 / 8 / 12 / 20; ALL = gruby i czerwony)
// -> prasa hydrauliczna na cala wysokosc: tlok schodzi o tyle, ile kompresor sciszna, i zgniata poduszke z fala w
// kolorze sciezki (szczyty fali scinaja sie o plyte = kompresja), manometr na koronie = GR -> kabel -> glosnik na
// wyjsciu (wielkosc = OUTPUT, membrana drga z poziomem). Narozniki: INPUT, OUTPUT, ATTACK, RELEASE.
namespace {
struct Scn {
    Canvas& c; float ox, oy, s;
    void ln(float x0, float y0, float x1, float y1, float w, Color col) const { c.line(ox + x0 * s, oy + y0 * s, ox + x1 * s, oy + y1 * s, w * s * .8f, col); }
    void rr(float x, float y, float w, float h, float r, float lw, Color col) const { c.strokeRoundRect(ox + x * s, oy + y * s, w * s, h * s, r * s, lw * s * .8f, col); }
    void frr(float x, float y, float w, float h, float r, Color col) const { c.fillRoundRect(ox + x * s, oy + y * s, w * s, h * s, r * s, col); }
    void ci(float x, float y, float r, float lw, Color col) const { c.circle(ox + x * s, oy + y * s, r * s, lw * s * .8f, col); }
    void fci(float x, float y, float r, Color col) const { c.fillCircle(ox + x * s, oy + y * s, r * s, col); }
    void ar(float x, float y, float r, float a0, float a1, float lw, Color col) const { c.arc(ox + x * s, oy + y * s, r * s, a0, a1, lw * s * .8f, col); }
    void poly(const float* p, int n, float lw, Color col, bool closed = false) const {
        float q[2 * 96];
        for (int i = 0; i < n && i < 96; i++) { q[2 * i] = ox + p[2 * i] * s; q[2 * i + 1] = oy + p[2 * i + 1] * s; }
        c.polyline(q, n < 96 ? n : 96, lw * s * .8f, col, closed);
    }
    void tx(float x, float y, const char* t, float size, float w, Color col, int al = ALIGN_LEFT) const { drawText(c, ox + x * s, oy + y * s, t, size, w, col, al); }
};
inline float bez(float a, float b, float c2, float d, float t) { const float u = 1.f - t; return u * u * u * a + 3 * u * u * t * b + 3 * u * t * t * c2 + t * t * t * d; }
}  // namespace

void MixerView::renderFetPress(Canvas& c, const float* q, float gr, float level, bool on, Color col) {
    const float dt = dt_ > 0.f && dt_ < .1f ? dt_ : .016f;
    const Scn S{c, 0.f, 52.f, 2.f};
    const int ri = (int)(q[4] * 4.999f);
    const Color frame = rgb(196, 200, 212), dimf = rgb(70, 74, 86), steelc = rgb(110, 140, 196);
    // ---- ruch ----
    const float BASE = 134.f, REST = 62.f, LOW = 118.f;  // dol glowicy tloka: spoczynek / maks. zgniecenie
    const float g = on ? clampf(gr / 20.f, 0.f, 1.f) : 0.f;
    const float target = REST + (LOW - REST) * powf(g, .7f);
    const float prev = press_ < REST ? REST : press_;
    press_ = prev + (target - prev) * (1.f - expf(-dt / .03f));
    pressV_ = (press_ - prev) / dt;
    const float ldb = 20.f * log10f(level > 1e-5f ? level : 1e-5f);
    const float ln = clampf((ldb + 40.f) / 40.f, 0.f, 1.f);
    lvl_ += (ln - lvl_) * (1.f - expf(-dt / .06f));
    wob_ += dt * (1.2f + 2.5f * lvl_);
    static float gaugeA = 0.f;
    gaugeA += (g - gaugeA) * (1.f - expf(-dt / .08f));
    char b[24];

    // ---- kable (pod spodem): interfejs -> wtyk w podstawie prasy (grubosc = RATIO), wtyk -> glosnik ----
    static float flowS = 0.f;
    flowS += dt * (.25f + .9f * lvl_);
    auto cable = [&](const float P[4][2], float w, Color outer, Color sig) {
        float pts[2 * 41];
        for (int i = 0; i <= 40; i++) { const float t = i / 40.f; pts[2 * i] = bez(P[0][0], P[1][0], P[2][0], P[3][0], t); pts[2 * i + 1] = bez(P[0][1], P[1][1], P[2][1], P[3][1], t); }
        S.poly(pts, 41, w + 1.4f, outer);
        S.poly(pts, 41, w * .55f, rgb(12, 12, 14));
        for (int k = 0; k < 8; k++) {  // sygnal plynie od wejscia do wyjscia
            float t = fmodf(k / 8.f + flowS, 1.f);
            const float t2 = fminf(t + .035f, 1.f);
            S.ln(bez(P[0][0], P[1][0], P[2][0], P[3][0], t), bez(P[0][1], P[1][1], P[2][1], P[3][1], t), bez(P[0][0], P[1][0], P[2][0], P[3][0], t2),
                 bez(P[0][1], P[1][1], P[2][1], P[3][1], t2), fmaxf(.6f, w * .35f), sig);
        }
    };
    const float cw = ri == 4 ? 3.6f : .8f + ri * .6f;
    const Color cOut = !on ? dimf : (ri == 4 ? pal::red : rgb(150, 110, 66));
    const Color cSig = !on ? dimf : (ri == 4 ? pal::yellow : pal::blue);
    const float CIN[4][2] = {{62, 112}, {78, 112}, {74, 136.5f}, {88, 136.5f}};
    cable(CIN, cw, cOut, cSig);
    const float spk = .7f + .6f * q[1];  // glosnik rosnie z OUTPUT
    const float sx = 272.f, sw = 34.f * spk, sh = 46.f * spk, sy = 130.f - sh;
    const float COUT[4][2] = {{232, 136.5f}, {246, 136.5f}, {sx - sw * .5f - 14, sy + sh * .72f}, {sx - sw * .5f - 2, sy + sh * .72f}};
    cable(COUT, 1.2f, on ? rgb(150, 110, 66) : dimf, on ? pal::green : dimf);
    // wtyki w podstawie prasy
    S.frr(87, 134.5f, 7, 4, 1, rgb(20, 20, 24)); S.rr(87, 134.5f, 7, 4, 1, .8f, frame);
    S.frr(226, 134.5f, 7, 4, 1, rgb(20, 20, 24)); S.rr(226, 134.5f, 7, 4, 1, .8f, frame);
    S.rr(sx - sw * .5f - 3, sy + sh * .72f - 2, 3, 4, .8f, .8f, frame);

    // ---- interfejs audio (wejscie) ----
    S.frr(14, 96, 52, 34, 4, rgb(10, 10, 12));
    S.rr(14, 96, 52, 34, 4, 1.f, frame);
    const float kx = 30, ky = 117, ka = 3.14159265f * (.75f + 1.5f * q[0]);  // galka INPUT: w prawo do oporu
    S.ar(kx, ky, 10, 3.14159265f * .75f, 3.14159265f * 2.25f, .6f, dimf);
    S.ar(kx, ky, 10, 3.14159265f * .75f, ka, 1.2f, pal::blue);
    S.ci(kx, ky, 7, 1.f, frame);
    S.ln(kx + 2.f * cosf(ka), ky + 2.f * sinf(ka), kx + 6.f * cosf(ka), ky + 6.f * sinf(ka), 1.2f, pal::white);
    for (int k = 0; k < 5; k++) {  // diody poziomu
        const bool lit = on && lvl_ > (k + .5f) / 5.f;
        S.fci(48.5f, 123 - k * 5.f, 1.4f, lit ? (k >= 4 ? pal::red : (k >= 3 ? pal::yellow : pal::green)) : rgb(40, 40, 46));
    }
    S.ci(60, 112, 2.6f, .8f, frame);  // gniazdo kabla
    S.ln(14, 130, 14, 132, 1.f, frame); S.ln(66, 130, 66, 132, 1.f, frame);

    // ---- prasa: podstawa, kolumny z gwintem, korona z manometrem ----
    S.frr(94, BASE, 132, 5, 1.5f, rgb(10, 10, 12));
    S.rr(94, BASE, 132, 5, 1.5f, 1.f, frame);
    S.ln(98, 139, 98, 143, 1.2f, frame); S.ln(222, 139, 222, 143, 1.2f, frame);
    for (int k = 0; k < 2; k++) {
        const float x = k ? 206.f : 107.f;
        S.ln(x, 34, x, BASE, 1.f, frame);
        S.ln(x + 7, 34, x + 7, BASE, 1.f, frame);
        for (float yy = 42; yy < BASE - 6; yy += 6) S.ln(x + 1.5f, yy, x + 5.5f, yy + 2.5f, .5f, dimf);
        S.rr(x - 2, BASE - 6, 11, 6, 1, 1.f, frame);
        S.rr(x - 2, 32, 11, 6, 1, 1.f, frame);
    }
    float cr[2 * 33];
    for (int i = 0; i <= 32; i++) { const float u = i / 32.f; cr[2 * i] = 94 + 132 * u; cr[2 * i + 1] = 18 - 9.f * sinf(u * 3.14159265f); }
    S.poly(cr, 33, 1.f, frame);
    S.ln(94, 18, 94, 32, 1.f, frame); S.ln(226, 18, 226, 32, 1.f, frame);
    S.ln(94, 32, 226, 32, 1.f, frame);
    for (int k = 0; k < 2; k++) { S.fci(102 + k * 9.f, 26, .9f, frame); S.fci(218 - k * 9.f, 26, .9f, frame); }
    const float gx = 160, gy = 19, gr2 = 9;  // manometr GR
    S.frr(gx - gr2, gy - gr2, gr2 * 2, gr2 * 2, gr2, rgb(8, 8, 10));
    S.ci(gx, gy, gr2, 1.f, frame);
    for (int k = 0; k <= 10; k++) {
        const float a = 3.14159265f * (.75f + 1.5f * k / 10.f);
        S.ln(gx + (gr2 - 2.5f) * cosf(a), gy + (gr2 - 2.5f) * sinf(a), gx + (gr2 - 1) * cosf(a), gy + (gr2 - 1) * sinf(a), .5f, k >= 8 ? pal::red : frame);
    }
    const float na = 3.14159265f * (.75f + 1.5f * gaugeA);
    S.ln(gx, gy, gx + (gr2 - 2.f) * cosf(na), gy + (gr2 - 2.f) * sinf(na), .9f, pal::red);
    S.fci(gx, gy, 1.f, frame);
    snprintf(b, sizeof b, "GR -%.0f", on ? gr : 0.f);
    S.tx(173, 21, b, 6, 1.f, pal::red);

    // ---- cylinder, tloczysko, glowica ----
    S.frr(142, 34, 36, 22, 2, rgb(10, 12, 18));
    S.rr(142, 34, 36, 22, 2, 1.f, steelc);
    for (int k = 1; k < 5; k++) S.ln(143, 34 + k * 4.4f, 177, 34 + k * 4.4f, .5f, rgb(50, 66, 100));
    const float headTop = press_ - 7.f;
    // ATTACK: zawor z kolkiem na cylindrze (kreci sie stale — tym wolniej, im dluzszy atak; olej plynie rurka, gdy tlok schodzi)
    {
        static float vrot = 0.f;  // obroty: krotki atak (20 us) ~3 obr/s, dlugi (800 us) ~0,3 obr/s
        const float atkUs = (.0008f * (1.f - q[2]) + .00002f * q[2]) * 1e6f;
        if (on) vrot += dt * 6.2831853f * (.3f + 2.7f * (800.f - atkUs) / 780.f);
        const float vx = 128, vy = 40, va = vrot;
        S.ln(vx + 6, 44, 142, 44, 2.2f, rgb(60, 60, 68));
        S.ln(vx + 6, 44, 142, 44, 1.f, rgb(14, 14, 16));
        static float oil = 0.f;
        oil += dt * clampf(pressV_ * .03f, 0.f, 3.f);
        if (on && pressV_ > 2.f)
            for (int k = 0; k < 2; k++) { const float u = fmodf(oil + k * .5f, 1.f); S.ln(vx + 6 + u * 6, 44, vx + 7.5f + u * 6, 44, .9f, pal::yellow); }
        S.ln(vx, vy + 6, vx, 44, 1.f, frame);
        S.ci(vx, vy, 6, 1.f, pal::white);
        for (int k = 0; k < 4; k++) { const float a = va + k * 1.5707963f; S.ln(vx, vy, vx + 6 * cosf(a), vy + 6 * sinf(a), .7f, pal::white); }
        S.fci(vx, vy, 1.3f, pal::white);
    }
    // RELEASE: sprezyna wokol tloczyska (szybszy release = sztywniejsza, wiecej zwojow); sciska sie z plyta
    {
        const int n = 4 + (int)lrintf(q[3] * 6.f);
        const float y0 = 57.f, y1 = headTop - .5f, step = (y1 - y0) / (2 * n);
        const Color fr = on ? pal::red : dimc(pal::red, .5f), bk = dimc(fr, .4f);
        for (int k = 0; k < 2 * n; k++) {  // spirala: przod (jasny) z lewej w prawo, tyl (ciemny) z powrotem
            const bool front = !(k & 1);
            const float xa = front ? 149.f : 171.f, xb = front ? 171.f : 149.f;
            S.ln(xa, y0 + k * step, xb, y0 + (k + 1) * step, front ? 1.f : .6f, front ? fr : bk);
        }
        S.ln(147, y0, 151, y0, 1.f, fr);
        S.ln(147, y1, 151, y1, 1.f, fr);
    }
    S.ln(155, 56, 155, headTop, 1.f, steelc);
    S.ln(165, 56, 165, headTop, 1.f, steelc);
    S.ln(158, 58, 158, headTop, .5f, rgb(70, 90, 130));
    S.frr(118, headTop, 84, 7, 1.5f, rgb(20, 20, 26));
    S.rr(118, headTop, 84, 7, 1.5f, 1.f, frame);
    for (int k = 0; k < 10; k++) S.ln(123 + k * 8.f, headTop + 7, 127 + k * 8.f, headTop + 7, .7f, dimf);

    // ---- poduszka z fala: objetosc zachowana, szczyty fali scinane o plyte ----
    const float nat = 38.f + 12.f * lvl_ * (on ? 1.f : .6f);
    const float gap = BASE - press_;
    const float H = fminf(nat, gap - .5f), W = fminf(nat * nat * 1.5f / H, 90.f);
    const float bcx = 160.f, bcy = BASE - H * .5f;
    float pp[2 * 65];
    for (int i = 0; i <= 64; i++) {
        const float th = i * 6.2831853f / 64.f, ct = cosf(th), st = sinf(th);
        const float rx = copysignf(powf(fabsf(ct), .5f), ct), ry = copysignf(powf(fabsf(st), .5f), st);
        const float bul = 1.f + .02f * sinf(3.f * th + wob_ * 6.f) * lvl_;
        pp[2 * i] = bcx + rx * W * .5f * bul;
        pp[2 * i + 1] = bcy + ry * H * .5f;
    }
    S.poly(pp, 65, 1.2f, on ? col : dimc(col, .5f), true);
    const float lim = H * .5f - 2.5f;
    for (int k = 0; k < 3; k++) {
        const float A = (6.f + 18.f * lvl_) * (1.f - k * .28f);
        const float x0 = bcx - W * .5f + 4, x1 = bcx + W * .5f - 4;
        float wv[2 * 49];
        for (int i = 0; i <= 48; i++) {
            const float u = i / 48.f;
            const float y = A * sinf(6.2831853f * (u * 2.f) - wob_ * 4.f - k * .9f);
            wv[2 * i] = x0 + (x1 - x0) * u;
            wv[2 * i + 1] = bcy + clampf(y, -lim + k, lim - k);
        }
        S.poly(wv, 49, k ? .7f : 1.f, k ? dimc(col, .6f - .18f * k) : col);
    }

    // ---- glosnik (wyjscie): wielkosc = OUTPUT, membrana drga z poziomem ----
    S.frr(sx - sw * .5f, sy, sw, sh, 3, rgb(10, 10, 12));
    S.rr(sx - sw * .5f, sy, sw, sh, 3, 1.f, frame);
    const float pulse = 1.f + (on ? .08f * lvl_ * sinf(wob_ * 9.f) : 0.f);
    S.ci(sx, sy + sh * .33f, sw * .3f * pulse, 1.f, frame);
    S.ci(sx, sy + sh * .33f, sw * .12f, 1.f, frame);
    S.ci(sx, sy + sh * .76f, sw * .14f, 1.f, dimf);
    S.ln(sx - sw * .5f + 3, 130, sx - sw * .5f + 3, 132, 1.f, frame); S.ln(sx + sw * .5f - 3, 130, sx + sw * .5f - 3, 132, 1.f, frame);

    // ---- narozniki: INPUT nad RATIO (lewo), OUTPUT nad RELEASE (prawo), ATTACK (lewy dol) — tytuly i wartosci jednej wielkosci ----
    static const char* RN[5] = {"4:1", "8:1", "12:1", "20:1", "ALL"};
    S.tx(14, 8, "INPUT", 9, 1.2f, pal::blue);
    snprintf(b, sizeof b, "%+.0f DB", -10.f + 40.f * q[0]);
    S.tx(14, 18, b, 9, 1.2f, pal::white);
    S.tx(14, 31, "RATIO", 9, 1.2f, pal::dim);
    S.tx(14, 41, RN[ri], 9, 1.2f, ri == 4 && on ? pal::red : pal::white);
    S.tx(306, 8, "OUTPUT", 9, 1.2f, pal::green, ALIGN_RIGHT);
    snprintf(b, sizeof b, "%+.0f DB", -30.f + 40.f * q[1]);
    S.tx(306, 18, b, 9, 1.2f, pal::white, ALIGN_RIGHT);
    S.tx(306, 31, "RELEASE", 9, 1.2f, pal::red, ALIGN_RIGHT);
    snprintf(b, sizeof b, "%.0f MS", (1.1f * (1.f - q[3]) + .05f * q[3]) * 1000.f);
    S.tx(306, 41, b, 9, 1.2f, pal::white, ALIGN_RIGHT);
    S.tx(14, 140, "ATTACK", 9, 1.2f, pal::white);
    snprintf(b, sizeof b, "%.0f US", (.0008f * (1.f - q[2]) + .00002f * q[2]) * 1e6f);
    S.tx(14, 150, b, 9, 1.2f, pal::white);
    if (!on) {
        c.fillRect(0, 52, 640, 320, rgb(0, 0, 0, 160));
        c.fillRoundRect(250, 196, 140, 32, 16, rgb(60, 20, 20));
        drawText(c, 320, 205, "BYPASS", 13, 1.6f, pal::red, ALIGN_CENTER);
    }
}

// Wspolny interfejs audio z lewej strony scen kompresorow: galka (wartosc 0..1 w kolorze), diody poziomu, gniazdo.
static void sceneInterface(const Scn& S, float knob, Color kc, float lvl, bool on) {
    const Color frame = rgb(196, 200, 212);
    S.frr(14, 96, 52, 34, 4, rgb(10, 10, 12));
    S.rr(14, 96, 52, 34, 4, 1.f, frame);
    const float kx = 30, ky = 113, ka = 3.14159265f * (.75f + 1.5f * clampf(knob, 0.f, 1.f));
    S.ar(kx, ky, 10, 3.14159265f * .75f, 3.14159265f * 2.25f, .6f, rgb(70, 74, 86));
    S.ar(kx, ky, 10, 3.14159265f * .75f, ka, 1.2f, kc);
    S.ci(kx, ky, 7, 1.f, frame);
    S.ln(kx + 2.f * cosf(ka), ky + 2.f * sinf(ka), kx + 6.f * cosf(ka), ky + 6.f * sinf(ka), 1.2f, pal::white);
    for (int k = 0; k < 5; k++) {
        const bool lit = on && lvl > (k + .5f) / 5.f;
        S.fci(48.5f, 123 - k * 5.f, 1.4f, lit ? (k >= 4 ? pal::red : (k >= 3 ? pal::yellow : pal::green)) : rgb(40, 40, 46));
    }
    S.ci(60, 112, 2.6f, .8f, frame);
    S.ln(14, 130, 14, 132, 1.f, frame); S.ln(66, 130, 66, 132, 1.f, frame);
}
// kabel z oplotem i plynacym sygnalem (krzywa Beziera)
static void sceneCable(const Scn& S, const float P[4][2], float w, Color outer, Color sig, float flow) {
    float pts[2 * 41];
    for (int i = 0; i <= 40; i++) { const float t = i / 40.f; pts[2 * i] = bez(P[0][0], P[1][0], P[2][0], P[3][0], t); pts[2 * i + 1] = bez(P[0][1], P[1][1], P[2][1], P[3][1], t); }
    S.poly(pts, 41, w + 1.4f, outer);
    S.poly(pts, 41, w * .55f, rgb(12, 12, 14));
    for (int k = 0; k < 8; k++) {
        const float t = fmodf(k / 8.f + flow, 1.f), t2 = fminf(t + .035f, 1.f);
        S.ln(bez(P[0][0], P[1][0], P[2][0], P[3][0], t), bez(P[0][1], P[1][1], P[2][1], P[3][1], t), bez(P[0][0], P[1][0], P[2][0], P[3][0], t2),
             bez(P[0][1], P[1][1], P[2][1], P[3][1], t2), fmaxf(.6f, w * .35f), sig);
    }
}
static void sceneBypass(Canvas& c) {
    c.fillRect(0, 52, 640, 320, rgb(0, 0, 0, 160));
    c.fillRoundRect(250, 196, 140, 32, 16, rgb(60, 20, 20));
    drawText(c, 320, 205, "BYPASS", 13, 1.6f, pal::red, ALIGN_CENTER);
}

// VCA — tama (scena 320x160, skala 2). Interfejs (galka = THRESHOLD) -> rura wlewa wode do zbiornika (poziom = sygnal
// wejscia) -> TAMA: wysokosc korony = THRESHOLD, nad koroną zasuwa — im wyzsze COMPRESSION (ratio), tym nizej zjezdza
// (INF:1 = sciana, nic nie przechodzi ponad prog); OVEREASY = zaokraglona korona (woda splywa lagodnie) zamiast ostrej
// krawedzi; swiatla BELOW / ABOVE na slupkach zasuwy; lata wodowskazowa z czerwonym odcinkiem = GR -> woda za tama
// kreci kolem wodnym (szybkosc = przeplyw x OUTPUT) -> kabel -> glosnik rosnacy z OUTPUT.
void MixerView::renderVcaDam(Canvas& c, const float* q, float gr, float level, bool on, Color col) {
    const float dt = dt_ > 0.f && dt_ < .1f ? dt_ : .016f;
    const Scn S{c, 0.f, 52.f, 2.f};
    const Color frame = rgb(196, 200, 212), dimf = rgb(70, 74, 86);
    const Color water = rgb(64, 150, 255), waterD = rgb(30, 70, 130), stone = rgb(150, 150, 160);
    const float ratio = q[1] > .97f ? 1e9f : 1.f + 40.f * q[1] * q[1];
    const bool oe = q[3] >= .5f;
    const float BED = 132.f;
    // poziomy: wejscie (przed kompresja = wyjscie kanalu + GR) i wyjscie
    const float outDb = 20.f * log10f(level > 1e-5f ? level : 1e-5f);
    const float inDb = outDb + (on ? gr : 0.f);
    static float uIn = 0, uOut = 0, wav = 0, wheel = 0, fall = 0, flowC = 0, gateY = -1;
    {  // poziom wody: szybko w gore, wolno w dol (jak wskazowka VU)
        const float t = clampf((inDb + 50.f) / 50.f, 0.f, 1.f);
        uIn += (t - uIn) * (1.f - expf(-dt / (t > uIn ? .02f : .3f)));
    }
    uOut += (clampf((outDb + 50.f) / 50.f, 0.f, 1.f) - uOut) * (1.f - expf(-dt / .05f));
    wav += dt * (1.5f + 3.f * uIn);
    const float yC = BED - 8.f - 76.f * q[0];          // korona tamy = THRESHOLD
    // poziom wody na tej samej skali co korona (dB -50..0 -> wysokosc); nadwyzka ponad koroną = sygnal ponad THRESHOLD
    const float GATE_H = 22.f;
    float yW = BED - 8.f - 76.f * uIn;                  // lustro wody przed tama
    if (yW < yC - GATE_H - 3.f) yW = yC - GATE_H - 3.f; // nigdy ponad belke zasuwy
    const float over = on ? fmaxf(0.f, yC - yW) : 0.f;  // nadwyzka (jednostki sceny)
    const float inv = ratio > 1e8f ? 0.f : 1.f / ratio;
    const float open = GATE_H * inv;                    // otwor zasuwy pokazuje ratio
    const float gTarget = yC - open;
    gateY = gateY < 0 ? gTarget : gateY + (gTarget - gateY) * (1.f - expf(-dt / .12f));
    static float passS = 0;
    passS += (over * inv - passS) * (1.f - expf(-dt / .05f));
    const float passed = passS;                          // grubosc strumienia = nadwyzka / ratio
    const float yD = BED - 5.f - 16.f * uOut;           // woda za tama
    fall += dt * 1.6f;
    flowC += dt * (.2f + 1.2f * uIn);
    wheel += dt * (on ? (.4f + 3.f * fminf(passed / 10.f + uOut * .5f, 1.f)) * (.3f + 1.4f * q[2]) : .2f);

    // ---- interfejs i kabel do rury ----
    sceneInterface(S, q[0], pal::blue, uIn, on);
    const float CIN[4][2] = {{62, 112}, {74, 112}, {70, 70}, {80, 70}};
    sceneCable(S, CIN, 1.2f, on ? rgb(150, 110, 66) : dimf, on ? pal::blue : dimf, flowC);
    // rura i strumien do zbiornika
    S.rr(80, 66, 14, 8, 2, 1.f, frame);
    S.ln(94, 68, 98, 68, 1.f, frame); S.ln(94, 72, 98, 72, 1.f, frame);
    const float sw = .6f + 2.2f * uIn;
    for (int k = 0; k < 6; k++) {
        const float t = fmodf(fall * 1.5f + k / 6.f, 1.f);
        const float y0 = 72 + t * (yW - 72), y1 = fminf(y0 + 5, yW);
        S.ln(99 + t * 2.f, y0, 99 + t * 2.f + 1.f, y1, sw, on ? water : waterD);
    }
    // ---- dno i brzegi ----
    S.ln(84, BED, 290, BED, 1.f, stone);
    S.ln(84, BED, 82, 80, 1.f, stone);
    for (int k = 0; k < 18; k++) S.fci(90 + k * 11.f, BED + 3, .6f, dimf);
    // ---- woda przed tama: lustro z falami + linie glebi ----
    for (int r = 0; r < 4; r++) {
        float wp[2 * 33];
        const float yy = yW + r * 7.f;
        if (yy > BED - 1) break;
        for (int i = 0; i <= 32; i++) {
            const float x = 84 + (150 - 84) * i / 32.f;
            wp[2 * i] = x;
            wp[2 * i + 1] = yy + (r ? .6f : 1.f + 2.2f * uIn) * sinf(x * .25f - wav * (r ? 2.f : 4.f) + r);
        }
        S.poly(wp, 33, r ? .6f : 1.f, r ? waterD : (on ? water : waterD));
    }
    // ---- tama: przekroj (OVEREASY = zaokraglona korona) ----
    float dm[2 * 24];
    int m = 0;
    auto P = [&](float x, float y) { if (m < 24) { dm[2 * m] = x; dm[2 * m + 1] = y; m++; } };
    P(150, BED); P(150, yC);
    if (oe) {
        for (int i = 0; i <= 10; i++) { const float a = -1.5707963f + i * .15707963f; P(158 + 10.f * cosf(a), yC + 10.f + 10.f * sinf(a)); }
        P(172, yC + 22); P(184, BED);
    } else { P(164, yC); P(166, yC + 2); P(172, BED - 6); P(184, BED); }
    S.poly(dm, m, 1.2f, frame);
    for (int k = 0; k < 4; k++) S.ln(150, yC + 8 + k * ((BED - yC - 8) / 4.f), 160 + k * 4.f, yC + 8 + k * ((BED - yC - 8) / 4.f), .5f, dimf);  // spoiny
    // lata wodowskazowa na scianie od strony wody: czerwony odcinek = GR
    S.ln(147, BED, 147, 40, .8f, dimf);
    for (float yy = BED; yy > 42; yy -= 6) S.ln(144.5f, yy, 147, yy, .6f, dimf);
    if (on && gr > .2f) { const float l = fminf(gr * 2.2f, 60.f); S.ln(147, yW, 147, yW + l, 1.6f, pal::red); }
    // zasuwa: slupki, plyta, swiatla BELOW / ABOVE
    S.ln(150, yC, 150, yC - GATE_H - 6, 1.f, frame);
    S.ln(164, yC, 164, yC - GATE_H - 6, 1.f, frame);
    S.ln(148, yC - GATE_H - 6, 166, yC - GATE_H - 6, 1.f, frame);
    S.frr(151, yC - GATE_H - 4, 12, gateY - (yC - GATE_H - 4), 1, rgb(20, 22, 30));
    S.rr(151, yC - GATE_H - 4, 12, gateY - (yC - GATE_H - 4), 1, .9f, rgb(110, 140, 196));
    for (float yy = yC - GATE_H; yy < gateY - 2; yy += 4) S.ln(152, yy, 162, yy, .4f, dimf);
    const bool above = on && gr > .5f;
    S.fci(150, yC - GATE_H - 9, 1.8f, on && !above ? pal::green : rgb(30, 50, 30));
    S.fci(164, yC - GATE_H - 9, 1.8f, above ? pal::red : rgb(60, 26, 26));
    // przelew: woda przez otwor zasuwy, po koronie i w dol
    if (passed > .15f) {
        const float w = .5f + passed * .9f;  // grubosc strumienia
        float fp[2 * 24];
        int n = 0;
        auto A = [&](float x, float y) { if (n < 24) { fp[2 * n] = x; fp[2 * n + 1] = y; n++; } };
        if (oe) {  // splywa po zaokraglonej koronie i po skarpie
            for (int i2 = 0; i2 <= 10; i2++) { const float a = -1.5707963f + i2 * .15707963f; A(158 + (10.f + w * .5f) * cosf(a), yC + 10.f + (10.f + w * .5f) * sinf(a) - w * .5f); }
            A(172 + w * .5f, yC + 22); A(184 + w * .5f, yD);
        } else {  // przelewa sie przez ostra krawedz i spada lukiem
            A(158, yC - w * .5f); A(166, yC - w * .5f);
            for (int i2 = 1; i2 <= 10; i2++) { const float t = i2 / 10.f; A(166 + t * (6 + w * 1.5f), yC - w * .5f + t * t * (yD - yC + w * .5f)); }
        }
        S.poly(fp, n, w, waterD);
        S.poly(fp, n, fmaxf(.5f, w * .35f), water);
        const int nd = 2 + (int)fminf(passed * .5f, 6.f);
        for (int k = 0; k < nd; k++) {  // krople i piana u podnoza
            const float t = fmodf(fall * 2.f + k / (float)nd, 1.f);
            const float x = (oe ? 184.f : 172.f + w) + t * (6 + w) - 3, y = yD - 3.f * sinf(t * 3.14159265f);
            S.ln(x, y, x + .6f, y - .8f, .8f, water);
        }
    }
    // ---- woda za tama i kolo wodne (OUTPUT) ----
    for (int r = 0; r < 3; r++) {
        float wp[2 * 17];
        const float yy = yD + r * 4.f;
        if (yy > BED - 1) break;
        for (int i = 0; i <= 16; i++) { const float x = 184 + (250 - 184) * i / 16.f; wp[2 * i] = x; wp[2 * i + 1] = yy + .7f * sinf(x * .3f - wav * 3.f + r); }
        S.poly(wp, 17, r ? .6f : .9f, r ? waterD : water);
    }
    const float wx = 222, wy = BED - 18, wr = 15;
    S.ci(wx, wy, wr, 1.f, frame);
    S.ci(wx, wy, 3, 1.f, frame);
    for (int k = 0; k < 8; k++) {
        const float a = wheel + k * .78539816f;
        S.ln(wx + 3 * cosf(a), wy + 3 * sinf(a), wx + wr * cosf(a), wy + wr * sinf(a), .7f, frame);
        const float px = wx + wr * cosf(a), py = wy + wr * sinf(a);
        S.ln(px, py, px + 4 * cosf(a + 1.5707963f), py + 4 * sinf(a + 1.5707963f), 1.2f, frame);
    }
    S.ln(wx - 6, BED, wx, wy, .8f, dimf); S.ln(wx + 6, BED, wx, wy, .8f, dimf);  // kozly
    // ---- kabel z osi kola do glosnika (rosnie z OUTPUT) ----
    const float spk = .7f + .6f * q[2];
    const float sx = 276.f, sw2 = 30.f * spk, sh = 42.f * spk, sy = 130.f - sh;
    const float COUT[4][2] = {{wx, wy}, {wx + 20, wy - 22}, {sx - sw2 * .5f - 14, sy + sh * .72f}, {sx - sw2 * .5f - 2, sy + sh * .72f}};
    sceneCable(S, COUT, 1.2f, on ? rgb(150, 110, 66) : dimf, on ? pal::white : dimf, flowC);
    S.frr(sx - sw2 * .5f, sy, sw2, sh, 3, rgb(10, 10, 12));
    S.rr(sx - sw2 * .5f, sy, sw2, sh, 3, 1.f, frame);
    const float pulse = 1.f + (on ? .08f * uOut * sinf(wav * 9.f) : 0.f);
    S.ci(sx, sy + sh * .33f, sw2 * .3f * pulse, 1.f, frame);
    S.ci(sx, sy + sh * .33f, sw2 * .12f, 1.f, frame);
    S.ci(sx, sy + sh * .76f, sw2 * .14f, 1.f, dimf);
    S.rr(sx - sw2 * .5f - 3, sy + sh * .72f - 2, 3, 4, .8f, .8f, frame);
    (void)col;
    // ---- napisy ----
    char b[24];
    S.tx(14, 8, "THRESHOLD", 9, 1.2f, pal::blue);
    snprintf(b, sizeof b, "%.0f DB", -50.f + 50.f * q[0]);
    S.tx(14, 18, b, 9, 1.2f, pal::white);
    S.tx(14, 31, "COMPRESSION", 9, 1.2f, pal::green);
    if (ratio > 1e8f) snprintf(b, sizeof b, "INF:1"); else snprintf(b, sizeof b, "%.1f:1", ratio);
    S.tx(14, 41, b, 9, 1.2f, pal::white);
    S.tx(306, 8, "OUTPUT", 9, 1.2f, pal::white, ALIGN_RIGHT);
    snprintf(b, sizeof b, "%+.0f DB", -20.f + 40.f * q[2]);
    S.tx(306, 18, b, 9, 1.2f, pal::white, ALIGN_RIGHT);
    S.tx(306, 31, "OVEREASY", 9, 1.2f, oe ? pal::tan : pal::dim, ALIGN_RIGHT);
    S.tx(306, 41, oe ? "ON" : "OFF", 9, 1.2f, oe ? pal::white : pal::dim, ALIGN_RIGHT);
    snprintf(b, sizeof b, "GR -%.0f", on ? gr : 0.f);
    S.tx(167, 140, b, 7, 1.f, pal::red, ALIGN_CENTER);
    if (!on) sceneBypass(c);
}

// OPTO — mlotek i reka (scena 320x160, skala 2). Interfejs (galka = PEAK REDUCTION) -> kabel do statywu mlotka. Kazdy
// glosniejszy fragment (transjent) = zamach mlotka; rozmiar mlotka = PEAK REDUCTION. Reka odsuwa sie o tyle, ile
// kompresor sciszna (GR) i wraca powoli (jak fotoopornik OPTO), wiec kolejne uderzenia trafiaja slabiej. Krew po
// trafieniu = GAIN. COMPRESS = otwarta dlon, LIMIT = zacisnieta piesc (odskakuje dalej). MIX = polprzezroczysta reka-
// duch, ktora sie nie odsuwa (sygnal bez kompresji) — tym wyrazniejsza, im mniejszy MIX. Gwiazdki nad reka = GR.
void MixerView::renderOptoHand(Canvas& c, const float* q, float gr, float level, bool on, Color col) {
    const float dt = dt_ > 0.f && dt_ < .1f ? dt_ : .016f;
    const Scn S{c, 0.f, 52.f, 2.f};
    const Color frame = rgb(196, 200, 212), dimf = rgb(70, 74, 86), skin = rgb(226, 186, 150), blood = rgb(220, 30, 40);
    const bool lim = q[2] >= .5f;
    const float TABLE = 124.f;
    // ---- stan animacji ----
    static float fast = 0, slow = 0, swing = 0, swingV = 0, retr = 0, starA = 0, flowC = 0, hitCool = 0;
    static float px[48], py[48], vx[48], vy[48], life[48], spl[12][3];  // krople krwi; plamy: x, rozmiar, zycie
    static int nSpl = 0;
    const float ldb = 20.f * log10f(level > 1e-5f ? level : 1e-5f) + (on ? gr : 0.f);
    const float ln = clampf((ldb + 45.f) / 45.f, 0.f, 1.f);
    fast += (ln - fast) * (1.f - expf(-dt / .015f));
    slow += (ln - slow) * (1.f - expf(-dt / .25f));
    hitCool -= dt;
    if (on && swing <= 0.f && swingV <= 0.f && hitCool <= 0.f && fast - slow > .07f && fast > .25f) swingV = 1.f;  // transjent -> zamach
    if (swingV > 0.f) { swing += dt / .07f; if (swing >= 1.f) { swing = 1.f; swingV = -1.f; } }
    else if (swingV < 0.f) { swing -= dt / .3f; if (swing <= 0.f) { swing = 0.f; swingV = 0.f; hitCool = .06f; } }
    // reka: odsuniecie = GR (wraca wolno)
    const float tR = on ? clampf(gr / (lim ? 10.f : 14.f), 0.f, 1.f) : 0.f;
    retr += (tR - retr) * (1.f - expf(-dt / (tR > retr ? .04f : (.35f + .5f * retr))));
    starA += dt * 3.f;
    flowC += dt * (.2f + 1.f * fast);
    const float handX = 150.f + retr * 46.f;  // koniuszki palcow (reka lezy palcami w lewo)
    const float HIT_X = 166.f;
    // ---- interfejs, kabel do statywu ----
    sceneInterface(S, q[0], pal::blue, fast, on);
    const float CIN[4][2] = {{62, 112}, {74, 112}, {80, 126}, {92, 126}};
    sceneCable(S, CIN, 1.2f, on ? rgb(150, 110, 66) : dimf, on ? pal::blue : dimf, flowC);
    // ---- stol ----
    S.ln(88, TABLE + 8, 300, TABLE + 8, 1.2f, frame);
    S.ln(96, TABLE + 8, 96, 150, 1.f, frame); S.ln(292, TABLE + 8, 292, 150, 1.f, frame);
    // plamy krwi na stole
    for (int i = 0; i < nSpl; i++) {
        if (spl[i][2] <= 0) continue;
        spl[i][2] -= dt * .15f;
        const float a = clampf(spl[i][2], 0.f, 1.f);
        S.ln(spl[i][0] - spl[i][1], TABLE + 7.2f, spl[i][0] + spl[i][1], TABLE + 7.2f, 1.2f, dimc(blood, .3f + .5f * a));
    }
    // ---- reka (i duch dla MIX) ----
    auto hand = [&](float x0, Color cl, float lw, bool fill) {  // x0 = koniuszki palcow; reka lezy palcami w lewo
        const float cy = TABLE - 1.f;
        const Color bg = rgb(0, 0, 0);
        auto thick = [&](float ax, float ay, float bx, float by, float w) {  // kciuk: gruba kreska z obrysem
            if (!fill) { const float nx2 = -(by - ay), ny2 = bx - ax, nl = sqrtf(nx2 * nx2 + ny2 * ny2) + 1e-6f, o = w * .5f / nl;
                         S.ln(ax + nx2 * o, ay + ny2 * o, bx + nx2 * o, by + ny2 * o, lw, cl); S.ln(ax - nx2 * o, ay - ny2 * o, bx - nx2 * o, by - ny2 * o, lw, cl); return; }
            S.ln(ax, ay, bx, by, w + lw * 1.6f, cl);
            S.ln(ax, ay, bx, by, w - lw * .4f, bg);
        };
        // przedramie z mankietem
        if (fill) S.frr(x0 + 70, cy - 11, 320 - x0 - 70, 19, 2, bg);
        S.ln(x0 + 70, cy - 10, 320, cy - 12, lw, cl);
        S.ln(x0 + 70, cy + 7, 320, cy + 8, lw, cl);
        S.rr(x0 + 96, cy - 12.5f, 8, 21.5f, 1.5f, lw, dimc(cl, .75f));
        if (!lim) {
            static const float FL[4] = {28, 34, 31, 24};
            for (int f = 0; f < 4; f++) {
                const float y = cy - 9.5f + f * 5.f, xa = x0 + 40.f - FL[f] - 2.f;
                if (fill) S.frr(xa, y - 2.3f, FL[f] + 6, 4.6f, 2.3f, bg);
                S.rr(xa, y - 2.3f, FL[f] + 6, 4.6f, 2.3f, lw, cl);
                S.ln(xa + 9, y - 1.f, xa + 9, y + 1.f, .5f * lw, dimc(cl, .5f));        // zgiecie
                S.ar(xa + 2.6f, y, 1.2f, 1.5707963f, 4.712389f, .5f * lw, dimc(cl, .6f));  // paznokiec
            }
            if (fill) S.frr(x0 + 38, cy - 12, 36, 22, 7, bg);
            S.rr(x0 + 38, cy - 12, 36, 22, 7, lw, cl);
            thick(x0 + 52, cy + 7, x0 + 36, cy + 15, 4.6f);
            S.ar(x0 + 50, cy - 2, 6, 1.9f, 2.9f, .5f * lw, dimc(cl, .5f));  // linia dloni
        } else {
            if (fill) S.frr(x0 + 22, cy - 12, 52, 22, 9, bg);
            S.rr(x0 + 22, cy - 12, 52, 22, 9, lw, cl);
            for (int k = 0; k < 4; k++) {
                const float y = cy - 9.f + k * 5.f;
                S.ar(x0 + 25.5f, y, 2.4f, 1.5707963f, 4.712389f, lw * .8f, cl);   // klykcie
                if (k) S.ln(x0 + 24, y - 2.5f, x0 + 36, y - 2.5f, .5f * lw, dimc(cl, .6f));  // zgiete palce
            }
            thick(x0 + 50, cy + 7, x0 + 32, cy + 4, 4.6f);
        }
    };
    const float ghostA = (1.f - q[3]) * .55f;
    if (ghostA > .03f) hand(150.f, dimc(skin, ghostA), .7f, false);
    hand(handX, on ? skin : dimc(skin, .5f), 1.f, true);
    // ---- statyw i mlotek (rozmiar = PEAK REDUCTION) ----
    const float pvx = 100, pvy = 58;
    const float L = hypotf(HIT_X - pvx, TABLE - 16.f - pvy);
    S.ln(pvx, pvy, pvx, TABLE + 8, 1.2f, frame);
    S.ln(pvx - 3, pvy + 6, pvx - 3, TABLE + 8, .6f, dimf);
    S.ln(pvx - 8, TABLE + 8, pvx + 8, TABLE + 8, 1.6f, frame);
    S.rr(90, 122, 6, 8, 1, .8f, frame);  // gniazdo kabla na statywie
    const float aRest = -.75f, aHit = atan2f(TABLE - 16.f - pvy, HIT_X - pvx);
    const float ang = aRest + (aHit - aRest) * (swing * swing);
    const float hx = pvx + L * cosf(ang), hy = pvy + L * sinf(ang);
    S.ln(pvx, pvy, hx, hy, 1.4f, rgb(150, 110, 66));
    S.ci(pvx, pvy, 2.5f, 1.f, frame);
    const float hs = .55f + .9f * q[0];  // rozmiar glowicy
    const float hw = 11.f * hs, hh = 8.f * hs;
    const float nx = cosf(ang), ny = sinf(ang), tx2 = -ny, ty2 = nx;  // osie glowicy: wzdluz trzonka i w poprzek
    float hd[10] = {hx + tx2 * hw - nx * hh * .5f, hy + ty2 * hw - ny * hh * .5f, hx + tx2 * hw + nx * hh * .5f, hy + ty2 * hw + ny * hh * .5f,
                    hx - tx2 * hw + nx * hh * .5f, hy - ty2 * hw + ny * hh * .5f, hx - tx2 * hw - nx * hh * .5f, hy - ty2 * hw - ny * hh * .5f, 0, 0};
    hd[8] = hd[0]; hd[9] = hd[1];
    {
        float f2[8];
        for (int k = 0; k < 8; k++) f2[k] = (k & 1) ? S.oy + hd[k] * S.s : S.ox + hd[k] * S.s;
        c.fillPolygon(f2, 4, rgb(30, 30, 36));
    }
    S.poly(hd, 5, 1.2f, frame);
    S.ln(hx - tx2 * hw * .55f - nx * hh * .2f, hy - ty2 * hw * .55f - ny * hh * .2f, hx - tx2 * hw * .55f + nx * hh * .2f, hy - ty2 * hw * .55f + ny * hh * .2f, .6f, dimf);
    // trafienie: krew (ilosc = GAIN), slabsza, gdy reka odsunieta
    static bool hitDone = false;
    if (swingV > 0.f) hitDone = false;
    if (swing >= 1.f && !hitDone) {
        hitDone = true;
        const bool onHand = handX < HIT_X + 8.f;
        const float power = (onHand ? 1.f : 0.f) * (1.f - retr * .6f);
        const int n = (int)(power * (2 + 26 * q[1]));
        for (int k = 0, slot = 0; k < n && slot < 48; slot++) {
            if (life[slot] > 0) continue;
            const float r = (float)((slot * 7919 + (int)(starA * 1000)) % 1000) / 1000.f, r2 = (float)((slot * 104729 + k * 31) % 1000) / 1000.f;
            px[slot] = HIT_X; py[slot] = TABLE - 6;
            const float sp = (20.f + 50.f * q[1]) * (.4f + .6f * r);
            const float a = -3.14159265f * (.15f + .7f * r2);
            vx[slot] = sp * cosf(a); vy[slot] = sp * sinf(a);
            life[slot] = 1.f;
            k++;
        }
        if (n > 0) { const int i = nSpl < 12 ? nSpl++ : (int)(starA * 10) % 12; spl[i][0] = HIT_X + 4; spl[i][1] = 2.f + 8.f * q[1]; spl[i][2] = 1.f; }
    }
    for (int i = 0; i < 48; i++) {
        if (life[i] <= 0) continue;
        vy[i] += 120.f * dt;
        const float ox = px[i], oy = py[i];
        px[i] += vx[i] * dt; py[i] += vy[i] * dt;
        if (py[i] > TABLE + 7) { life[i] = 0; continue; }
        life[i] -= dt * .8f;
        S.ln(ox, oy, px[i], py[i], 1.1f, blood);
    }
    // gwiazdki nad reka = GR
    const int ns = on ? (int)fminf(gr / 3.f, 6.f) : 0;
    for (int k = 0; k < ns; k++) {
        const float a = starA + k * 6.2831853f / (ns ? ns : 1);
        const float sx = handX + 30 + 16 * cosf(a), sy = TABLE - 34 + 5 * sinf(a);
        for (int j = 0; j < 3; j++) { const float b2 = j * 1.0471976f + starA; S.ln(sx - 2.5f * cosf(b2), sy - 2.5f * sinf(b2), sx + 2.5f * cosf(b2), sy + 2.5f * sinf(b2), .8f, pal::yellow); }
    }
    (void)col;
    // ---- napisy ----
    char b[24];
    S.tx(14, 8, "PEAK REDUCTION", 9, 1.2f, pal::blue);
    snprintf(b, sizeof b, "%d", (int)lrintf(q[0] * 100));
    S.tx(14, 18, b, 9, 1.2f, pal::white);
    S.tx(14, 31, "MODE", 9, 1.2f, pal::dim);
    S.tx(14, 41, lim ? "LIMIT" : "COMPRESS", 9, 1.2f, lim ? pal::red : pal::white);
    S.tx(306, 8, "GAIN", 9, 1.2f, pal::green, ALIGN_RIGHT);
    snprintf(b, sizeof b, "%+.0f DB", -10.f + 35.f * q[1]);
    S.tx(306, 18, b, 9, 1.2f, pal::white, ALIGN_RIGHT);
    S.tx(306, 31, "MIX", 9, 1.2f, pal::white, ALIGN_RIGHT);
    snprintf(b, sizeof b, "%d%%", (int)lrintf(q[3] * 100));
    S.tx(306, 41, b, 9, 1.2f, pal::white, ALIGN_RIGHT);
    snprintf(b, sizeof b, "GR -%.0f", on ? gr : 0.f);
    S.tx(306, 140, b, 7, 1.f, pal::red, ALIGN_RIGHT);
    if (!on) sceneBypass(c);
}

void MixerView::renderBus(Canvas& c) {
    Project& p = app_->project();
    AudioEngine* e = app_->engine();
    const BusComp& s = p.bus;
    windowHeader(c, "BUS COMPRESSOR", "MASTER", pal::white);
    compPanel(c, "G-BUS", "SSL G BUS COMPRESSOR STYLE", rgb(120, 124, 132), s.on);
    compMeter(c, e ? e->masterGr() : 0.f, "COMPRESSION", s.on);
    static const char* RA[3] = {"2", "4", "10"};
    static const char* AT[6] = {".1", ".3", "1", "3", "10", "30"};
    static const char* RE[5] = {".1", ".3", ".6", "1.2", "AUTO"};
    const float by = MBY + 186;
    segButtons(c, MBX + 36, by, MBW - 72, 28, RA, 3, s.ratio % 3, s.on);
    drawText(c, MBX + MBW * .5f, by + 40, "Y = RATIO", 7, 1.f, pal::dim, ALIGN_CENTER);
    char b[4][24];
    snprintf(b[0], 24, "%.0f DB", -30.f + 30.f * s.thresh);
    bigKnob(c, KX[0], KY[0], KR, s.thresh, pal::blue, "THRESHOLD", b[0], s.on);
    snprintf(b[1], 24, "+%.1f DB", s.makeup);
    bigKnob(c, KX[1], KY[0], KR, s.makeup / 15.f, pal::green, "MAKE UP", b[1], s.on);
    snprintf(b[2], 24, "%s MS", AT[s.attack % 6]);
    bigKnob(c, KX[0], KY[1], KR2, s.attack / 5.f, pal::white, "ATTACK", b[2], s.on);
    snprintf(b[3], 24, s.release >= 4 ? "%s" : "%s S", RE[s.release % 5]);
    bigKnob(c, KX[1], KY[1], KR2, s.release / 4.f, pal::red, "RELEASE", b[3], s.on);
    drawText(c, 620, 394, "A = ON/OFF   B = BACK", 7, 1.f, rgb(90, 90, 90), ALIGN_RIGHT);
    char v[4][16];
    snprintf(v[0], 16, "%.0f", -30.f + 30.f * s.thresh); snprintf(v[1], 16, "+%.1f", s.makeup);
    snprintf(v[2], 16, "%s", AT[s.attack % 6]); snprintf(v[3], 16, "%s", RE[s.release % 5]);
    const char* L[4] = {"THRESHOLD", "MAKE UP", "ATTACK", "RELEASE"};
    const char* vals[4] = {v[0], v[1], v[2], v[3]};
    const float pos[4] = {s.thresh, s.makeup / 15.f, s.attack / 5.f, s.release / 4.f};
    encoderBar(c, L, vals, pos);
}

void MixerView::renderFxList(Canvas& c) {
    Project& p = app_->project();
    FxSlot* s = fxTarget(p, strip_, slot_);
    if (!s) return;
    const char* items[FX_COUNT - 1];
    for (int i = 1; i < FX_COUNT; i++) items[i - 1] = fxInfo(i).name;
    const int sel = fxSel_ < 1 ? 0 : fxSel_ - 1;
    listBox(c, 20, 60, 250, 300, items, FX_COUNT - 1, sel, 0, pal::tan);
    for (int i = 1; i < FX_COUNT; i++) if (i == s->fx) c.fillCircle(256, 74 + (i - 1) * 50, 4.f, pal::green);
    c.line(282, 60, 282, 370, 1.f, pal::steel);
    const FxInfo& fi = fxInfo(fxSel_ < 1 ? 1 : fxSel_);
    drawText(c, 300, 66, fi.name, 14, 1.7f, pal::white);
    for (int k = 0; k < 4; k++) drawText(c, 300, 100 + k * 18, fi.desc[k], 8, 1.1f, rgb(190, 190, 190));
    drawFxScreen(c, fxSel_ < 1 ? 1 : fxSel_, fxInfo(fxSel_ < 1 ? 1 : fxSel_).def, false, nullptr, fxReset_, 300, 190, 1.0f);
    fxReset_ = false;
    drawText(c, 620, 394, "UP/DOWN = CHOOSE   A = USE   B = BACK", 7, 1.f, rgb(90, 90, 90), ALIGN_RIGHT);
    const char* L[4] = {"", "", "", ""};
    const char* V[4] = {"", "", "", ""};
    const float pos[4] = {0, 0, 0, 0};
    encoderBar(c, L, V, pos);
}

void MixerView::renderFx(Canvas& c) {
    Project& p = app_->project();
    FxSlot* s = fxTarget(p, strip_, slot_);
    if (!s) return;
    char nm[16], tt[24];
    Color col;
    stripTitle(p, strip_, nm, sizeof nm, &col);
    if (strip_ < p.nTracks) snprintf(tt, sizeof tt, "INSERT %d", slot_ + 1); else snprintf(tt, sizeof tt, "AUX EFFECT");
    windowHeader(c, tt, nm, col);
    if (fxList_ || !s->fx) { renderFxList(c); return; }
    const FxInfo& cur = fxInfo(s->fx);
    drawText(c, 620, 17, cur.name, 10, 1.3f, pal::tan, ALIGN_RIGHT);
    // ekran efektu (oryginal op1r) z telemetria egzemplarza w mikserze
    AudioEngine* e = app_->engine();
    const uint8_t* tel = e ? e->fxTelemetry(strip_ < p.nTracks ? strip_ : -1 - (strip_ - p.nTracks), slot_) : nullptr;
    drawFxScreen(c, s->fx, s->w, fxPage_ != 0, tel, fxReset_, 0, 52, 2.f);
    fxReset_ = false;
    if (s->bypass) {  // jak na OP-1: przyciemnienie i napis na srodku
        c.fillRect(0, 52, 640, 320, rgb(0, 0, 0, 160));
        c.fillRoundRect(250, 196, 140, 32, 16, rgb(60, 20, 20));
        drawText(c, 320, 205, "BYPASS", 13, 1.6f, pal::red, ALIGN_CENTER);
    }
    const bool hasP2 = cur.page2[0][0] || cur.page2[1][0] || cur.page2[2][0];
    drawText(c, 20, 394, hasP2 ? (fxPage_ ? "L3 = PAGE 1" : "L3 = PAGE 2") : "", 8, 1.1f, pal::white);
    drawText(c, 620, 394, s->bypass ? "A = ON   X = CHANGE   Y = REMOVE   B = BACK" : "A = BYPASS   X = CHANGE   Y = REMOVE   B = BACK", 7, 1.f, rgb(90, 90, 90), ALIGN_RIGHT);
    char v[4][16];
    const char* L[4];
    float pos[4];
    for (int k = 0; k < 4; k++) {
        const bool p2 = fxPage_ != 0;
        L[k] = p2 ? (k < 3 ? cur.page2[k] : "") : cur.knob[k];
        const int16_t w = s->w[p2 ? 5 + k : k];
        snprintf(v[k], 16, "%d", (w * 99 + 16383) / 32767);
        if (!L[k][0]) v[k][0] = 0;
        pos[k] = w / 32767.f;
    }
    const char* vals[4] = {v[0], v[1], v[2], v[3]};
    encoderBar(c, L, vals, pos);
}

}  // namespace dp
