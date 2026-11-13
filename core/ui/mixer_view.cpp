// Ekran MIXER i jego okna (zob. mixer_view.h).
#include "ui/mixer_view.h"
#include <math.h>
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
    if (b.pressed(B_L3) && c.type == COMP_FET) fetAdv_ = !fetAdv_;
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
    if (cp.type == COMP_OPTO) {
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
    } else if (!fetAdv_) {
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
    if (cp.type == COMP_FET) drawText(c, 150, 396, fetAdv_ ? "L3 = PRESS VIEW" : "L3 = ADVANCED", 7, 1.f, pal::dim);
    drawText(c, 620, 394, cp.type == COMP_FET ? "Y = RATIO   A = ON/OFF   B = BACK" : "A = ON/OFF   B = BACK", 7, 1.f, rgb(90, 90, 90), ALIGN_RIGHT);
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

    // ---- kable (pod spodem): interfejs -> prasa (RATIO), prasa -> glosnik ----
    const float cw = ri == 4 ? 5.f : 1.f + ri * .9f;
    const Color cc = !on ? dimf : (ri == 4 ? pal::red : rgb(150, 150, 160));
    float cb[2 * 33];
    for (int i = 0; i <= 32; i++) {
        const float t = i / 32.f;
        cb[2 * i] = bez(66, 84, 86, 96, t);
        cb[2 * i + 1] = bez(112, 112, 128, 128, t);
    }
    S.poly(cb, 33, cw, cc);
    const float spk = .7f + .6f * q[1];  // glosnik rosnie z OUTPUT
    const float sx = 268.f, sw = 34.f * spk, sh = 46.f * spk, sy = 130.f - sh;
    for (int i = 0; i <= 32; i++) {
        const float t = i / 32.f;
        cb[2 * i] = bez(224, 236, 240, sx - sw * .5f, t);
        cb[2 * i + 1] = bez(128, 128, sy + sh * .75f, sy + sh * .75f, t);
    }
    S.poly(cb, 33, 1.4f, on ? rgb(150, 150, 160) : dimf);

    // ---- interfejs audio (wejscie) ----
    S.frr(14, 96, 52, 34, 4, rgb(10, 10, 12));
    S.rr(14, 96, 52, 34, 4, 1.f, frame);
    S.tx(19, 99, "IN", 6, 1.f, pal::blue);
    const float kx = 30, ky = 117, ka = 3.14159265f * (.75f + 1.5f * q[0]);  // galka INPUT: w prawo do oporu
    S.ar(kx, ky, 10, 3.14159265f * .75f, 3.14159265f * 2.25f, .6f, dimf);
    S.ar(kx, ky, 10, 3.14159265f * .75f, ka, 1.2f, pal::blue);
    S.ci(kx, ky, 7, 1.f, frame);
    S.ln(kx + 2.f * cosf(ka), ky + 2.f * sinf(ka), kx + 6.f * cosf(ka), ky + 6.f * sinf(ka), 1.2f, pal::white);
    for (int k = 0; k < 5; k++) {  // diody poziomu
        const bool lit = on && lvl_ > (k + .5f) / 5.f;
        S.fci(48 + (k % 3) * 0.f, 126 - k * 5.f, 1.4f, lit ? (k >= 4 ? pal::red : (k >= 3 ? pal::yellow : pal::green)) : rgb(40, 40, 46));
    }
    S.ci(59, 112, 2.6f, .8f, frame);  // gniazdo kabla
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
    S.tx(sx + sw * .5f - 3, sy + 2, "OUT", 6, 1.f, pal::green, ALIGN_RIGHT);

    // ---- narozniki ----
    S.tx(14, 8, "INPUT", 9, 1.2f, pal::blue);
    snprintf(b, sizeof b, "%+.0f DB", -10.f + 40.f * q[0]);
    S.tx(14, 18, b, 9, 1.2f, pal::white);
    S.tx(306, 8, "OUTPUT", 9, 1.2f, pal::green, ALIGN_RIGHT);
    snprintf(b, sizeof b, "%+.0f DB", -30.f + 40.f * q[1]);
    S.tx(306, 18, b, 9, 1.2f, pal::white, ALIGN_RIGHT);
    static const char* RN[5] = {"4:1", "8:1", "12:1", "20:1", "ALL"};
    S.tx(14, 30, "RATIO", 7, 1.f, pal::dim);
    S.tx(14, 39, RN[ri], 9, 1.2f, ri == 4 && on ? pal::red : pal::white);
    S.tx(14, 140, "ATTACK", 9, 1.2f, pal::white);
    snprintf(b, sizeof b, "%.0f US", (.0008f * (1.f - q[2]) + .00002f * q[2]) * 1e6f);
    S.tx(14, 150, b, 7, 1.f, pal::dim);
    S.tx(306, 140, "RELEASE", 9, 1.2f, pal::red, ALIGN_RIGHT);
    snprintf(b, sizeof b, "%.0f MS", (1.1f * (1.f - q[3]) + .05f * q[3]) * 1000.f);
    S.tx(306, 150, b, 7, 1.f, pal::dim, ALIGN_RIGHT);
    if (!on) {
        c.fillRect(0, 52, 640, 320, rgb(0, 0, 0, 160));
        c.fillRoundRect(250, 196, 140, 32, 16, rgb(60, 20, 20));
        drawText(c, 320, 205, "BYPASS", 13, 1.6f, pal::red, ALIGN_CENTER);
    }
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
