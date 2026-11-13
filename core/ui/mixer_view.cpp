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
#include "ui/widgets.h"

namespace dp {

namespace {
const int PER_PAGE = 5;
const float VU_REF = 12.f;  // 0 VU = -12 dBFS (czerwona strefa = ostatnie 3 dB przed przesterowaniem)

const float SW = 100.f, SX0 = 8.f, MX = 516.f, MW = 116.f;
inline Color dimc(Color c, float a) { return rgb((uint8_t)(c.r * a), (uint8_t)(c.g * a), (uint8_t)(c.b * a)); }
Color auxColor(int a) { static const Color C[MIX_AUX] = {pal::cyan, pal::purple, pal::yellow, pal::pink, pal::tan, pal::silver}; return C[a % MIX_AUX]; }
const int axisOf[4] = {AX_LX, AX_LY, AX_RX, AX_RY};
const float sgn[4] = {1.f, -1.f, 1.f, -1.f};
float stick(const InputState& in, int i, float dt) { return RelEncoder::delta(in.axis[axisOf[i]] * sgn[i], dt, .7f); }
float clampf(float x, float a, float b) { return x < a ? a : (x > b ? b : x); }
const char* compName(int t) { static const char* N[COMP_TYPES] = {"OPTO", "VCA", "FET"}; return N[t % COMP_TYPES]; }

// luk VU: dB -20..+3 (0 dB przy 87 %), czerwona strefa powyzej 0
void drawVu(Canvas& c, float cx, float cy, float r, float db, Color pivot, bool labels) {
    auto ang = [](float d) { const float u = clampf((d + 20.f) / 23.f, 0.f, 1.f); return -1.5707963f + (u - .5f) * 1.7f; };
    float pts[2 * 25];
    for (int i = 0; i <= 24; i++) { const float a = ang(-20.f + 20.f * i / 24.f); pts[2 * i] = cx + r * cosf(a); pts[2 * i + 1] = cy + r * sinf(a); }
    c.polyline(pts, 25, 1.2f, rgb(150, 150, 160));
    for (int i = 0; i <= 8; i++) { const float a = ang(3.f * i / 8.f); pts[2 * i] = cx + r * cosf(a); pts[2 * i + 1] = cy + r * sinf(a); }
    c.polyline(pts, 9, 3.f, pal::red);
    static const float T[5] = {-20.f, -10.f, -5.f, 0.f, 3.f};
    static const char* TL[5] = {"20", "10", "5", "0", "3"};
    for (int k = 0; k < 5; k++) {
        const float a = ang(T[k]);
        c.line(cx + (r - 5) * cosf(a), cy + (r - 5) * sinf(a), cx + r * cosf(a), cy + r * sinf(a), 1.f, k >= 3 ? pal::red : rgb(150, 150, 160));
        if (labels) drawText(c, cx + (r + 7) * cosf(a), cy + (r + 7) * sinf(a) - 4, TL[k], 6, 1.f, k >= 3 ? pal::red : pal::dim, ALIGN_CENTER);
    }
    const float a = ang(db);
    c.line(cx, cy, cx + (r + 2) * cosf(a), cy + (r + 2) * sinf(a), 1.6f, rgb(255, 120, 110));
    c.fillCircle(cx, cy, 3.f, pivot);
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
    if (w == W_FX) {
        const Project& p = app_->project();
        const FxSlot& s = strip_ >= p.nTracks ? p.aux[strip_ - p.nTracks].fx : p.tracks[strip_].ins[slot];
        fxSel_ = s.fx;
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
        else if (row_ == R_INS0 || row_ == R_INS1) openWindow(W_FX, row_ - R_INS0);
        else if (row_ == R_PAN && isTrack) { p.tracks[strip_].pan = 0.f; app_->markDirty(); }
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
    if (!s || b.pressed(B_B)) { win_ = W_NONE; return; }
    if (b.repeat(B_UP) && fxSel_ > 0) fxSel_--;
    if (b.repeat(B_DOWN) && fxSel_ < FX_COUNT - 1) fxSel_++;
    bool ch = false;
    if (b.pressed(B_A) && fxSel_ != s->fx) { fxDefaults(*s, fxSel_); fxPage_ = 0; ch = true; }
    const FxInfo& fi = fxInfo(s->fx);
    const bool hasP2 = fi.page2[0][0] || fi.page2[1][0] || fi.page2[2][0];
    if (b.pressed(B_L3) && hasP2) fxPage_ ^= 1;
    if (s->fx != FX_NONE)
        for (int i = 0; i < 4; i++) {
            const float d = stick(in, i, dt);
            if (d == 0.f) continue;
            const int w = fxPage_ ? 5 + i : i;
            if (fxPage_ && (i > 2 || !fi.page2[i][0])) continue;
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
    if (sel) c.fillRoundRect(x + 2, 50, w - 4, 300, 8, rgb(16, 16, 20));
    // VU
    const float pk = e ? (isTrack ? e->chPeak(strip) : e->auxPeak(a)) : 0.f;
    const float db = 20.f * log10f(pk > 1e-5f ? pk : 1e-5f) + VU_REF;
    float& v = vu_[strip & 63];
    v = db > v ? db : v - 24.f * dt;
    if (v < -40.f) v = -40.f;
    drawVu(c, cx, 104, 34, v, col, true);
    auto hl = [&](int row, float y, float h) {
        if (sel && row_ == row && win_ == W_NONE) c.strokeRoundRect(x + 3, y - 2, w - 6, h + 4, 5, 1.4f, pal::white);
    };
    // podglad EQ
    const MixEq& eq = isTrack ? t->eq : p.aux[a].eq;
    c.strokeRoundRect(x + 7, 112, w - 14, 22, 4, 1.f, pal::faint);
    eqCurve(c, eq, x + 9, 114, w - 18, 18, 12.f, eq.on && !eq.flat() ? col : rgb(80, 80, 80), 1.3f);
    drawText(c, x + 10, 115, "EQ", 5, .9f, pal::dim);
    hl(R_EQ, 112, 22);
    if (isTrack) {
        // podglad kompresora
        const MixComp& cp = t->comp;
        c.strokeRoundRect(x + 7, 138, w - 14, 18, 4, 1.f, pal::faint);
        drawText(c, x + 12, 143, cp.on ? compName(cp.type) : "COMP", 7, 1.1f, cp.on ? pal::white : pal::dim);
        if (cp.on) {
            const float gr = e ? e->chGr(strip) : 0.f, bw = (w - 60) * clampf(gr / 12.f, 0.f, 1.f);
            c.line(x + 48, 147, x + w - 12, 147, 3.f, pal::faint);
            if (bw > .5f) c.line(x + w - 12 - bw, 147, x + w - 12, 147, 3.f, pal::red);
        }
        hl(R_COMP, 138, 18);
        // wysylki A..F
        for (int k = 0; k < MIX_AUX; k++) {
            const float y = 165 + k * 10;
            char L[2] = {(char)('A' + k), 0};
            drawText(c, x + 10, y - 3, L, 6, 1.f, t->send[k] > 0 ? auxColor(k) : pal::dim);
            c.line(x + 22, y, x + w - 10, y, 2.f, pal::faint);
            if (t->send[k] > 0) c.line(x + 22, y, x + 22 + (w - 32) * t->send[k], y, 2.f, auxColor(k));
            hl(R_SEND0 + k, y - 5, 10);
        }
        // inserty
        for (int k = 0; k < MIX_INSERTS; k++) {
            const float y = 226 + k * 16;
            const FxSlot& s = t->ins[k];
            if (s.fx) c.fillRoundRect(x + 7, y, w - 14, 13, 6, rgb(40, 34, 26));
            c.strokeRoundRect(x + 7, y, w - 14, 13, 6, 1.f, s.fx ? pal::tan : pal::faint);
            drawText(c, cx, y + 3, s.fx ? fxInfo(s.fx).shortName : "INSERT", 6, 1.f, s.fx ? pal::tan : pal::dim, ALIGN_CENTER);
            hl(R_INS0 + k, y, 13);
        }
        // panorama
        const float py = 272, ang = -1.5707963f + t->pan * 2.3f;
        c.fillCircle(cx, py, 10.f, dimc(col, .55f));
        c.line(cx, py, cx + 9 * cosf(ang), py + 9 * sinf(ang), 2.f, pal::white);
        drawText(c, cx - 26, py - 2, "L", 7, 1.f, pal::dim);
        drawText(c, cx + 20, py - 2, "R", 7, 1.f, pal::dim);
        c.fillCircle(cx - 15, py + 7, 1.2f, pal::dim);
        c.fillCircle(cx + 15, py + 7, 1.2f, pal::dim);
        hl(R_PAN, 260, 24);
    } else {
        // powrot AUX: efekt w miejscu insertu
        drawText(c, cx, 180, "RETURN", 7, 1.f, pal::dim, ALIGN_CENTER);
        const FxSlot& s = p.aux[a].fx;
        if (s.fx) c.fillRoundRect(x + 7, 226, w - 14, 13, 6, rgb(40, 34, 26));
        c.strokeRoundRect(x + 7, 226, w - 14, 13, 6, 1.f, s.fx ? pal::tan : pal::faint);
        drawText(c, cx, 229, s.fx ? fxInfo(s.fx).shortName : "EFFECT", 6, 1.f, s.fx ? pal::tan : pal::dim, ALIGN_CENTER);
        hl(R_INS0, 226, 13);
    }
    // glosnosc i nazwa
    char b[16];
    const float vol = isTrack ? t->volume : p.aux[a].level;
    snprintf(b, sizeof b, "%02d", (int)lrintf(vol * 99.f));
    const bool muted = isTrack && !p.audible(strip);
    drawText(c, cx, 292, b, 30, 1.6f, muted ? dimc(col, .4f) : col, ALIGN_CENTER);
    hl(R_VOL, 290, 34);
    char nm[16];
    if (isTrack) snprintf(nm, sizeof nm, "%s", t->name);
    else snprintf(nm, sizeof nm, "AUX %c", 'A' + a);
    float tw = textWidth(nm, 8);
    if (tw > w - 8) tw = w - 8;
    drawTextFit(c, cx - tw * .5f, 334, nm, 8, 1.1f, pal::white, w - 8);
    if (isTrack && t->mute) drawText(c, x + 8, 334, "M", 7, 1.f, pal::red);
    if (isTrack && t->solo) drawText(c, x + w - 14, 334, "S", 7, 1.f, pal::yellow);
}

void MixerView::renderMaster(Canvas& c, float x, float w) {
    const Project& p = app_->project();
    AudioEngine* e = app_->engine();
    const bool sel = strip_ < 0;
    const float cx = x + w * .5f;
    if (sel) c.fillRoundRect(x + 2, 50, w - 4, 300, 8, rgb(16, 16, 20));
    const float pk = e ? e->masterPeak() : 0.f;
    const float db = 20.f * log10f(pk > 1e-5f ? pk : 1e-5f) + VU_REF;
    vuM_ = db > vuM_ ? db : vuM_ - 24.f * dt_;
    if (vuM_ < -40.f) vuM_ = -40.f;
    drawVu(c, cx, 104, 38, vuM_, pal::white, true);
    // BUS COMP
    c.strokeRoundRect(x + 8, 116, w - 16, 40, 5, 1.f, pal::faint);
    drawText(c, cx, 121, "BUS COMP", 7, 1.1f, p.bus.on ? pal::white : pal::dim, ALIGN_CENTER);
    if (p.bus.on) {
        static const char* R[3] = {"2:1", "4:1", "10:1"};
        drawText(c, x + 14, 137, R[p.bus.ratio % 3], 7, 1.f, pal::dim);
        const float gr = e ? e->masterGr() : 0.f, bw = (w - 64) * clampf(gr / 12.f, 0.f, 1.f);
        c.line(x + 48, 141, x + w - 14, 141, 3.f, pal::faint);
        if (bw > .5f) c.line(x + w - 14 - bw, 141, x + w - 14, 141, 3.f, pal::red);
    }
    if (sel && row_ == R_COMP) c.strokeRoundRect(x + 4, 113, w - 8, 46, 6, 1.4f, pal::white);
    char b[8];
    snprintf(b, sizeof b, "%02d", (int)lrintf(p.masterVol * 99.f));
    drawText(c, cx, 292, b, 30, 1.6f, pal::white, ALIGN_CENTER);
    if (sel && row_ == R_VOL) c.strokeRoundRect(x + 4, 288, w - 8, 38, 6, 1.4f, pal::white);
    drawText(c, cx, 334, "MASTER", 8, 1.1f, pal::white, ALIGN_CENTER);
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
        if (i) c.line(SX0 + i * SW, 60, SX0 + i * SW, 340, 1.f, rgb(28, 28, 32));
    }
    c.line(MX - 4, 56, MX - 4, 346, 1.f, rgb(60, 60, 66));
    renderMaster(c, MX, MW);
    // podpowiedz i pasek galek
    drawText(c, 20, 362, "A = OPEN   Y = MUTE   X = SOLO   L1/R1 = PAGE   D-PAD = MOVE", 7, 1.f, rgb(80, 80, 80));
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

void MixerView::renderComp(Canvas& c) {
    Project& p = app_->project();
    if (strip_ < 0 || strip_ >= p.nTracks) return;
    MixComp& cp = p.tracks[strip_].comp;
    AudioEngine* e = app_->engine();
    char nm[16];
    Color col;
    stripTitle(p, strip_, nm, sizeof nm, &col);
    windowHeader(c, "COMPRESSOR", nm, col);
    // wybor modelu (L1/R1)
    static const char* MN[COMP_TYPES] = {"OPTO", "VCA", "FET"};
    static const char* MS[COMP_TYPES] = {"LA-2A STYLE", "DBX 160 STYLE", "1176 STYLE"};
    for (int m = 0; m < COMP_TYPES; m++) {
        const float x = 330 + m * 98;
        if (m == cp.type) c.fillRoundRect(x, 12, 90, 24, 12, pal::silver);
        else c.strokeRoundRect(x, 12, 90, 24, 12, 1.f, pal::dim);
        drawText(c, x + 45, 18, MN[m], 10, 1.3f, m == cp.type ? rgb(20, 20, 20) : pal::dim, ALIGN_CENTER);
    }
    const bool on = cp.on;
    const float gr = e ? e->chGr(strip_) : 0.f;
    const float* q = cp.p[cp.type];
    const float px = 20, py = 56, pw = 600, ph = 330;
    char b[24], b2[24], b3[24], b4[24];
    if (cp.type == COMP_OPTO) {  // panel jasny metal: miernik, GAIN, PEAK REDUCTION, przelacznik, MIX
        c.fillRoundRect(px, py, pw, ph, 12, rgb(24, 24, 28));
        c.strokeRoundRect(px, py, pw, ph, 12, 1.4f, rgb(150, 150, 160));
        for (int k = 0; k < 2; k++) { c.circle(px + 16, py + 20 + k * 290, 4, 1.f, pal::dim); c.circle(px + pw - 16, py + 20 + k * 290, 4, 1.f, pal::dim); }
        drawText(c, px + 30, py + 14, "OPTO-2", 14, 1.8f, pal::silver);
        drawText(c, px + 30, py + 36, MS[0], 7, 1.f, pal::dim);
        c.strokeRoundRect(70, 140, 220, 150, 8, 1.2f, rgb(90, 90, 96));
        c.fillRoundRect(72, 142, 216, 146, 7, rgb(40, 34, 22));
        grMeter(c, 180, 262, 92, gr, "GAIN REDUCTION", on);
        snprintf(b, sizeof b, "%+.0f DB", -10.f + 35.f * q[1]);
        bigKnob(c, 380, 170, 36, q[1], pal::green, "GAIN", b, on);
        snprintf(b2, sizeof b2, "%d", (int)lrintf(q[0] * 100));
        bigKnob(c, 520, 170, 36, q[0], pal::blue, "PEAK REDUCTION", b2, on);
        const bool lim = q[2] >= .5f;  // przelacznik COMPRESS / LIMIT
        c.strokeRoundRect(350, 285, 60, 26, 13, 1.2f, pal::dim);
        c.fillCircle(lim ? 397 : 363, 298, 10, pal::tan);
        drawText(c, 340, 318, "COMPRESS", 7, 1.f, !lim ? pal::white : pal::dim, ALIGN_RIGHT);
        drawText(c, 420, 318, "LIMIT", 7, 1.f, lim ? pal::white : pal::dim);
        drawText(c, 380, 318, "Y", 7, 1.f, pal::dim, ALIGN_CENTER);
        snprintf(b3, sizeof b3, "%d%%", (int)lrintf(q[3] * 100));
        bigKnob(c, 520, 300, 22, q[3], pal::white, "MIX", b3, on);
    } else if (cp.type == COMP_VCA) {  // ciemny panel z diodami progu i rzedem diod sciszania
        c.fillRoundRect(px, py, pw, ph, 12, rgb(16, 16, 18));
        c.strokeRoundRect(px, py, pw, ph, 12, 1.4f, rgb(110, 110, 118));
        drawText(c, px + 30, py + 14, "VCA-160", 14, 1.8f, pal::white);
        drawText(c, px + 30, py + 36, MS[1], 7, 1.f, pal::dim);
        const float thr = -50.f + 50.f * q[0];
        const bool above = gr > .5f;  // dioda progu: sygnal powyzej progu = kompresor pracuje
        c.fillCircle(470, 100, 6, on && !above ? pal::green : rgb(30, 50, 30));
        c.fillCircle(520, 100, 6, on && above ? pal::red : rgb(60, 26, 26));
        drawText(c, 470, 112, "BELOW", 6, 1.f, pal::dim, ALIGN_CENTER);
        drawText(c, 520, 112, "ABOVE", 6, 1.f, pal::dim, ALIGN_CENTER);
        snprintf(b, sizeof b, "%.0f DB", thr);
        bigKnob(c, 130, 190, 36, q[0], pal::blue, "THRESHOLD", b, on);
        const float ratio = q[1] > .97f ? 0.f : 1.f + 40.f * q[1] * q[1];
        if (ratio == 0.f) snprintf(b2, sizeof b2, "INF:1"); else snprintf(b2, sizeof b2, "%.1f:1", ratio);
        bigKnob(c, 270, 190, 36, q[1], pal::green, "COMPRESSION", b2, on);
        snprintf(b3, sizeof b3, "%+.0f DB", -20.f + 40.f * q[2]);
        bigKnob(c, 410, 190, 36, q[2], pal::white, "OUTPUT GAIN", b3, on);
        const bool oe = q[3] >= .5f;
        if (oe) c.fillRoundRect(470, 168, 110, 28, 6, pal::tan); else c.strokeRoundRect(470, 168, 110, 28, 6, 1.2f, pal::tan);
        drawText(c, 525, 177, "OVEREASY", 10, 1.3f, oe ? rgb(20, 20, 20) : pal::tan, ALIGN_CENTER);
        drawText(c, 525, 204, "Y", 7, 1.f, pal::dim, ALIGN_CENTER);
        for (int k = 0; k < 12; k++) {  // diody sciszania co 2 dB
            const float x = 90 + k * 38;
            const bool lit = on && gr >= 1.f + k * 2.f;
            c.fillRoundRect(x, 300, 28, 12, 3, lit ? (k < 4 ? pal::green : (k < 8 ? pal::yellow : pal::red)) : rgb(36, 36, 40));
        }
        drawText(c, 320, 320, "GAIN REDUCTION", 8, 1.1f, pal::dim, ALIGN_CENTER);
    } else {  // FET: czarny panel, miernik, INPUT / OUTPUT, ATTACK / RELEASE, kolumna przyciskow ratio
        c.fillRoundRect(px, py, pw, ph, 12, rgb(10, 10, 10));
        c.strokeRoundRect(px, py, pw, ph, 12, 1.4f, rgb(90, 90, 96));
        drawText(c, px + 30, py + 14, "FET-76", 14, 1.8f, pal::white);
        drawText(c, px + 30, py + 36, MS[2], 7, 1.f, pal::dim);
        c.strokeRoundRect(50, 130, 200, 140, 8, 1.2f, rgb(90, 90, 96));
        c.fillRoundRect(52, 132, 196, 136, 7, rgb(42, 36, 24));
        grMeter(c, 150, 248, 84, gr, "GAIN REDUCTION", on);
        snprintf(b, sizeof b, "%+.0f DB", -10.f + 40.f * q[0]);
        bigKnob(c, 320, 150, 32, q[0], pal::blue, "INPUT", b, on);
        snprintf(b2, sizeof b2, "%+.0f DB", -30.f + 40.f * q[1]);
        bigKnob(c, 450, 150, 32, q[1], pal::green, "OUTPUT", b2, on);
        snprintf(b3, sizeof b3, "%.0f US", (.0008f * (1.f - q[2]) + .00002f * q[2]) * 1e6f);
        bigKnob(c, 320, 290, 22, q[2], pal::white, "ATTACK", b3, on);
        const float rel = 1.1f * (1.f - q[3]) + .05f * q[3];
        snprintf(b4, sizeof b4, "%.0f MS", rel * 1000.f);
        bigKnob(c, 450, 290, 22, q[3], pal::red, "RELEASE", b4, on);
        static const char* RN[5] = {"4", "8", "12", "20", "ALL"};
        const int ri = (int)(q[4] * 4.999f);
        drawText(c, 560, 82, "RATIO", 8, 1.1f, pal::dim, ALIGN_CENTER);
        for (int k = 0; k < 5; k++) {
            const float y = 100 + k * 42;
            if (k == ri) c.fillRoundRect(535, y, 50, 30, 5, pal::white); else c.strokeRoundRect(535, y, 50, 30, 5, 1.2f, pal::dim);
            drawText(c, 560, y + 9, RN[k], 10, 1.3f, k == ri ? rgb(10, 10, 10) : pal::dim, ALIGN_CENTER);
        }
        drawText(c, 560, 312, "Y", 7, 1.f, pal::dim, ALIGN_CENTER);
    }
    if (!on) { c.fillRoundRect(250, 64, 140, 26, 13, rgb(60, 20, 20)); drawText(c, 320, 71, "BYPASS", 11, 1.4f, pal::red, ALIGN_CENTER); }
    drawText(c, 20, 394, "L1/R1 = MODEL", 8, 1.1f, pal::white);
    drawText(c, 620, 394, "A = ON/OFF   B = BACK", 7, 1.f, rgb(90, 90, 90), ALIGN_RIGHT);
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

void MixerView::renderBus(Canvas& c) {
    Project& p = app_->project();
    AudioEngine* e = app_->engine();
    const BusComp& s = p.bus;
    windowHeader(c, "BUS COMPRESSOR", "MASTER", pal::white);
    const float px = 20, py = 56, pw = 600, ph = 330;
    c.fillRoundRect(px, py, pw, ph, 12, rgb(30, 32, 36));
    c.strokeRoundRect(px, py, pw, ph, 12, 1.4f, rgb(120, 124, 132));
    drawText(c, px + 30, py + 14, "G-BUS", 14, 1.8f, pal::white);
    drawText(c, px + 30, py + 36, "SSL G BUS COMPRESSOR STYLE", 7, 1.f, pal::dim);
    c.strokeRoundRect(60, 120, 200, 150, 8, 1.2f, rgb(90, 90, 96));
    c.fillRoundRect(62, 122, 196, 146, 7, rgb(44, 38, 26));
    grMeter(c, 160, 244, 86, e ? e->masterGr() : 0.f, "COMPRESSION", s.on);
    char b[24], b2[24], b3[24], b4[24];
    static const char* RA[3] = {"2", "4", "10"};
    static const char* AT[6] = {".1", ".3", "1", "3", "10", "30"};
    static const char* RE[5] = {".1", ".3", ".6", "1.2", "AUTO"};
    snprintf(b, sizeof b, "%.0f DB", -30.f + 30.f * s.thresh);
    bigKnob(c, 340, 150, 32, s.thresh, pal::blue, "THRESHOLD", b, s.on);
    snprintf(b2, sizeof b2, "+%.1f DB", s.makeup);
    bigKnob(c, 480, 150, 32, s.makeup / 15.f, pal::green, "MAKE UP", b2, s.on);
    snprintf(b3, sizeof b3, "%s MS", AT[s.attack % 6]);
    bigKnob(c, 340, 290, 24, s.attack / 5.f, pal::white, "ATTACK", b3, s.on);
    snprintf(b4, sizeof b4, s.release >= 4 ? "%s" : "%s S", RE[s.release % 5]);
    bigKnob(c, 480, 290, 24, s.release / 4.f, pal::red, "RELEASE", b4, s.on);
    drawText(c, 572, 92, "RATIO", 8, 1.1f, pal::dim, ALIGN_CENTER);
    for (int k = 0; k < 3; k++) {
        const float y = 110 + k * 46;
        if (k == s.ratio) c.fillRoundRect(548, y, 48, 32, 5, pal::white); else c.strokeRoundRect(548, y, 48, 32, 5, 1.2f, pal::dim);
        drawText(c, 572, y + 10, RA[k], 11, 1.4f, k == s.ratio ? rgb(10, 10, 10) : pal::dim, ALIGN_CENTER);
    }
    drawText(c, 572, 252, "Y", 7, 1.f, pal::dim, ALIGN_CENTER);
    if (!s.on) { c.fillRoundRect(250, 64, 140, 26, 13, rgb(60, 20, 20)); drawText(c, 320, 71, "BYPASS", 11, 1.4f, pal::red, ALIGN_CENTER); }
    drawText(c, 620, 394, "A = ON/OFF   B = BACK", 7, 1.f, rgb(90, 90, 90), ALIGN_RIGHT);
    char v[4][16];
    snprintf(v[0], 16, "%.0f", -30.f + 30.f * s.thresh); snprintf(v[1], 16, "+%.1f", s.makeup);
    snprintf(v[2], 16, "%s", AT[s.attack % 6]); snprintf(v[3], 16, "%s", RE[s.release % 5]);
    const char* L[4] = {"THRESHOLD", "MAKE UP", "ATTACK", "RELEASE"};
    const char* vals[4] = {v[0], v[1], v[2], v[3]};
    const float pos[4] = {s.thresh, s.makeup / 15.f, s.attack / 5.f, s.release / 4.f};
    encoderBar(c, L, vals, pos);
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
    // lista efektow
    const char* items[FX_COUNT];
    for (int i = 0; i < FX_COUNT; i++) items[i] = fxInfo(i).name;
    listBox(c, 20, 60, 250, 300, items, FX_COUNT, fxSel_, 0, pal::tan);
    for (int i = 0; i < FX_COUNT; i++) if (i == s->fx) c.fillCircle(256, 74 + i * 50, 4.f, pal::green);
    c.line(282, 60, 282, 370, 1.f, pal::steel);
    const FxInfo& fi = fxInfo(fxSel_);
    drawText(c, 300, 66, fi.name, 14, 1.7f, pal::white);
    for (int k = 0; k < 4; k++) drawText(c, 300, 100 + k * 18, fi.desc[k], 8, 1.1f, rgb(190, 190, 190));
    if (fxSel_ != s->fx) drawText(c, 300, 190, "A = USE THIS EFFECT", 9, 1.2f, pal::green);
    else if (s->fx) {
        const FxInfo& cur = fxInfo(s->fx);
        drawText(c, 300, 190, fxPage_ ? "PAGE 2" : "PAGE 1", 9, 1.2f, pal::white);
        const bool hasP2 = cur.page2[0][0] || cur.page2[1][0] || cur.page2[2][0];
        if (hasP2) drawText(c, 380, 192, "L3 = PAGE", 7, 1.f, pal::dim);
        for (int k = 0; k < 4; k++) {  // slupki galek
            const bool p2 = fxPage_ != 0;
            const char* kn = p2 ? (k < 3 ? cur.page2[k] : "") : cur.knob[k];
            if (!kn[0]) continue;
            const int16_t w = s->w[p2 ? 5 + k : k];
            const float x = 300 + k * 80;
            c.line(x, 230, x + 60, 230, 3.f, pal::faint);
            c.line(x, 230, x + 60 * w / 32767.f, 230, 3.f, pal::enc[k]);
            drawText(c, x, 240, kn, 8, 1.1f, pal::enc[k]);
        }
    }
    drawText(c, 620, 394, "UP/DOWN = CHOOSE   A = USE   B = BACK", 7, 1.f, rgb(90, 90, 90), ALIGN_RIGHT);
    const FxInfo& cur = fxInfo(s->fx);
    char v[4][16];
    const char* L[4];
    float pos[4];
    for (int k = 0; k < 4; k++) {
        const bool p2 = fxPage_ != 0;
        L[k] = s->fx ? (p2 ? (k < 3 ? cur.page2[k] : "") : cur.knob[k]) : "";
        const int16_t w = s->w[p2 ? 5 + k : k];
        snprintf(v[k], 16, "%d", w * 99 / 32767);
        if (!L[k][0]) v[k][0] = 0;
        pos[k] = w / 32767.f;
    }
    const char* vals[4] = {v[0], v[1], v[2], v[3]};
    encoderBar(c, L, vals, pos);
}

}  // namespace dp
