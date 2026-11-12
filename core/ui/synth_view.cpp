#include "ui/synth_view.h"
#include "gfx/font.h"
#include "gfx/icons.h"
#include "ui/app.h"
#include "ui/encoder.h"
#include "synth/sid_dsp_wrap.h"
#include "ui/widgets.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

namespace dp {

static const int kNoteBtn[8] = {B_LEFT, B_DOWN, B_RIGHT, B_UP, B_Y, B_B, B_A, B_X};
static const int kMajor[8] = {0, 2, 4, 5, 7, 9, 11, 12};
static const char* kWaveNames[8] = {"SINE", "WARM", "TRI", "SAW", "SQUARE", "PULSE", "VOCAL", "DIGITAL"};

// Czasy obwiedni: galka 0..1 -> 1 ms .. 10 s (wykladniczo)
static float envTime(float v) { return 0.001f * powf(10000.f, v); }
static float envKnob(float t) { return t <= 0.001f ? 0.f : log10f(t / 0.001f) / 4.f; }
static float clamp01(float x) { return x < 0 ? 0.f : (x > 1 ? 1.f : x); }

static void waveName(char* out, int n, int32_t k) {
    int32_t wp = k * 7;
    int wi = wp >> 15, wf = (wp & 0x7FFF) * 100 / 32768;
    if (wf < 6 || wi == 7) snprintf(out, n, "%s", kWaveNames[wi]);
    else if (wf > 94) snprintf(out, n, "%s", kWaveNames[wi + 1]);
    else snprintf(out, n, "%.3s>%.3s", kWaveNames[wi], kWaveNames[wi + 1]);
}

SynthView::Kind SynthView::kind(int pm) const {
    static const Kind orig[8] = {K_SOUND, K_VOICE, K_AMP, K_FENV, K_MENV, K_LFO1, K_LFO2, K_PRESETS};
    static const Kind extk[10] = {K_OSC1, K_OSC2, K_FILTER, K_VOICE, K_AMP, K_FENV, K_MENV, K_LFO1, K_LFO2, K_PRESETS};
    static const Kind pewk[8] = {K_SOUND, K_BARS, K_VOICE, K_AMP, K_MENV, K_LFO1, K_LFO2, K_PRESETS};
    int n = pageCount(pm), p = page_ < n ? page_ : n - 1;
    static const Kind elgk[3] = {K_SOUND, K_AMP, K_PRESETS};
    static const Kind evak[6] = {K_SOUND, K_AMP, K_MENV, K_LFO1, K_LFO2, K_PRESETS};
    static const Kind sidk[7] = {K_SOUND, K_AMP, K_FENV, K_MENV, K_LFO1, K_LFO2, K_PRESETS};
    static const Kind monk[7] = {K_SOUND, K_VOICE, K_AMP, K_MENV, K_LFO1, K_LFO2, K_PRESETS};
    return pm == 1 ? extk[p] : (pm == 2 ? pewk[p] : (pm == 3 ? elgk[p] : (pm == 4 ? evak[p] : (pm == 5 ? sidk[p] : (pm == 6 ? monk[p] : orig[p])))));
}

void SynthView::setPageKind(int kd) {
    const Track& t = app_->project().tracks[track_];
    const int pm = pageMode(t);
    for (int p = 0; p < pageCount(pm); p++) { page_ = p; if (kind(pm) == kd) return; }
    page_ = 0;
}

void SynthView::open(int track) {
    if (track != track_) releaseAll();
    track_ = track;
}

void SynthView::releaseAll() {
    AudioEngine* e = app_ ? app_->engine() : nullptr;
    for (int i = 0; i < 8; i++)
        if (held_[i] >= 0) { if (e) e->send(EC_NOTE_OFF, track_, held_[i]); held_[i] = -1; }
}

void SynthView::update(const Buttons& b, const InputState& in, float dt) {
    dt_ = dt;
    Project& p = app_->project();
    if (track_ >= p.nTracks) return;
    Track& t = p.tracks[track_];
    AudioEngine* e = app_->engine();
    const bool wt = t.engine == ENG_WAVETABLE;
    bool ext = (wt || t.engine == ENG_OXBOW) && t.alt;  // tryb rozszerzony (WAVETABLE+, OXBOW+)
    const bool obp = t.engine == ENG_OXBOW && t.alt;
    const int pm = pageMode(t);

    if (b.pressed(B_SELECT) && !b.down(B_MENU) && !ed_.active()) {
        page_ = (page_ + 1) % pageCount(pm);
        confirm_ = 0;
        savePending_ = false;
    }
    if (page_ >= pageCount(pm)) page_ = 0;
    const Kind k = kind(pm);
    if (k == K_PRESETS) {
        releaseAll();
        updatePresets(b);
        if (b.pressed(B_START)) app_->togglePlaySong();
        return;
    }
    if (b.pressed(B_L3)) {
        if (k == K_LFO1 || k == K_LFO2) { Lfo& L = t.lfo[k - K_LFO1]; L.sync = !L.sync; app_->markDirty(); }
        else if (k == K_VOICE) { t.glideSync = !t.glideSync; app_->markDirty(); }
        else alt_ = !alt_;
    }
    if (b.pressed(B_START)) app_->togglePlaySong();

    // odsluch z przyciskow (C4, bez opisu)
    for (int i = 0; i < 8; i++) {
        if (b.pressed(kNoteBtn[i]) && e) {
            int pitch = 12 * (octave_ + 1) + kMajor[i];
            if (held_[i] >= 0) e->send(EC_NOTE_OFF, track_, held_[i]);
            e->send(EC_NOTE_ON, track_, pitch, 0, 0.8f);
            held_[i] = pitch;
        }
        if (b.released(kNoteBtn[i]) && held_[i] >= 0) {
            if (e) e->send(EC_NOTE_OFF, track_, held_[i]);
            held_[i] = -1;
        }
    }

    // galki (wzgledne): pelny zakres w ~1.5 s przy pelnym wychyleniu
    static const int axisOf[4] = {AX_LX, AX_LY, AX_RX, AX_RY};
    static const float sign[4] = {1.f, -1.f, 1.f, -1.f};
    for (int i = 0; i < 4; i++) {
        float ax = in.axis[axisOf[i]] * sign[i];
        float d = RelEncoder::delta(ax, dt, 0.7f);
        bool changed = false;
        auto knob = [&](int16_t& v) {
            if (d == 0.f) return;
            int x = v + (int)lrintf(d * 32767.f);
            x = x < 0 ? 0 : (x > 32767 ? 32767 : x);
            if (x != v) { v = (int16_t)x; changed = true; }
        };
        switch (k) {
        case K_SOUND:
            if (t.engine == ENG_ELG && alt_ && i == 3) {  // ELG + L3: czerwona galka = SYNC (OFF, 1/1 .. 1/128T)
                int st = stepMovDiv_.steps(ax, dt, 5.f);
                if (st) {
                    int pos = t.movSync ? t.movDiv + 1 : 0;  // 0 = OFF
                    pos += st;
                    pos = pos < 0 ? 0 : (pos > LFO_DIVS ? LFO_DIVS : pos);
                    t.movSync = pos > 0;
                    if (pos > 0) t.movDiv = (uint8_t)(pos - 1);
                    changed = true;
                }
            } else if (t.engine == ENG_SID && i == 0) {  // SID: fala krokowo, srodek pasma 512 (TRIANGLE .. RING)
                int st = stepSidWave_.steps(ax, dt, 4.f);
                if (st) {
                    int m = (int)sidWaveMode(t.knob[0]) + st;
                    m = m < 0 ? 0 : (m > 5 ? 5 : m);
                    const int16_t nv = (int16_t)(m * 512 + 256);
                    if (nv != t.knob[0]) { t.knob[0] = nv; changed = true; }
                }
            } else if ((t.engine == ENG_MBIRA || t.engine == ENG_OXBOW || t.engine == ENG_MONOLIT) && alt_ && i < 3) {  // MONOLIT: BODY, VEL, AGE  // OXBOW: WAVE, FILTER, DRIVE  // MBIRA + L3: strona 2 = RING, DOUBLE, WAH
                knob(t.push[i]);
            } else if (t.engine == ENG_EVA && alt_ && i == 3) {  // EVA + L3: strona 2 COM = oktawa sygnalu (3 stopnie)
                int st = stepOct_.steps(ax, dt, 3.f);
                if (st) {
                    int cur = t.push[2] <= 0 ? 1 : (t.push[2] * 3) >> 15;
                    cur = cur + st < 0 ? 0 : (cur + st > 2 ? 2 : cur + st);
                    static const int16_t mid[3] = {5461, 16384, 27306};  // srodki stopni jak OP1_PRESS_PAGE_STEPS(3)
                    t.push[2] = mid[cur];
                    changed = true;
                }
            } else knob(t.knob[i]);
            break;
        case K_BARS: if (i < 3) knob(t.push[i]); break;  // PEW: wysuniecie suwakow niskich / srodkowych / wysokich
        case K_OSC1:
        case K_OSC2:
            if (obp) {  // OXBOW+: WAVE, INTERVAL, DETUNE, LEVEL; L3: OSC 1 -> PW, NOISE; OSC 2 -> SYNC, X-MOD, PW, ENV>P2
                const int o = k - K_OSC1;
                ObExt& x = t.obx;
                auto add = [&](auto& val, int lo, int hi, float range) {
                    if (d == 0.f) return;
                    int nv = (int)lrintf(val + d * range);
                    nv = nv < lo ? lo : (nv > hi ? hi : nv);
                    if (nv != val) { val = (std::remove_reference_t<decltype(val)>)nv; changed = true; }
                };
                if (alt_ && o == 0 && i >= 2) {
                    if (i == 2) add(x.pw[0], 1, 127, 127.f);
                    else add(x.noise, 0, 126, 126.f);
                } else if (alt_ && o == 1) {
                    if (i == 0) { int st = stepObSync_.steps(ax, dt, 3.f); if (st) { bool sy = st > 0; if (sy != x.sync) { x.sync = sy; changed = true; } } }
                    else if (i == 1) add(x.xmod, 0, 127, 127.f);
                    else if (i == 2) add(x.pw[1], 1, 127, 127.f);
                    else add(x.envPitch, -64, 63, 127.f);
                } else if (i == 0) {
                    int st = stepObWave_[o].steps(ax, dt, 4.f);
                    if (st) { int w = x.wave[o] + st; x.wave[o] = (uint8_t)(w < 0 ? 0 : w > 3 ? 3 : w); changed = true; }
                } else if (i == 1) {
                    int st = stepInt_[o].steps(ax, dt, 6.f);
                    if (st) { int sm = x.semi[o] + st; x.semi[o] = (int8_t)(sm < -24 ? -24 : sm > 24 ? 24 : sm); changed = true; }
                } else if (i == 2) {
                    if (o == 0) add(x.cents1, -50, 50, 100.f);
                    else knob(t.knob[3]);  // OSC 2: galka DETUNE oryginalu (0 .. +50 centow)
                } else add(x.level[o], 0, 126, 126.f);
                break;
            }
        {  // WAVETABLE+: WAVE, INTERVAL, DETUNE, LEVEL (OSC 1 + L3: SUB)
            int o = k - K_OSC1;
            WtOsc& os = t.ext.osc[o];
            if (i == 0) knob(o == 0 ? t.knob[0] : os.wave);
            else if (i == 1) {
                int st = stepInt_[o].steps(ax, dt, 6.f);
                if (st) { int sm = os.semi + st; os.semi = (int8_t)(sm < -24 ? -24 : sm > 24 ? 24 : sm); changed = true; }
            } else if (i == 2 && d != 0.f) {
                float c = os.cents + d * 100.f;
                os.cents = c < -50 ? -50 : (c > 50 ? 50 : c);
                changed = true;
            } else if (i == 3 && d != 0.f) {
                if (o == 0 && alt_) t.ext.sub = clamp01(t.ext.sub + d);
                else os.level = clamp01(os.level + d);
                changed = true;
            }
            break;
        }
        case K_VOICE: {  // MODE, GLIDE, UNISON, AGE
            if (i == 0) {
                int st = stepVMode_.steps(ax, dt, 3.f);
                if (st) { bool m = st > 0; if (m != t.mono) { t.mono = m; changed = true; } }
            } else if (i == 1 && t.glideSync) {  // podzial nutowy, skokowo (0 = OFF przy pierwszym podziale w dol)
                int st = stepGDiv_.steps(ax, dt, 5.f);
                if (st) { int dv = t.glideDiv + st; t.glideDiv = (uint8_t)(dv < 0 ? 0 : (dv >= LFO_DIVS ? LFO_DIVS - 1 : dv)); changed = true; }
            } else if (i == 1 && d != 0.f) {  // 0 .. 2 s (wykladniczo), 0 = wylaczone
                float kk = t.glide <= 0.f ? 0.f : log10f(t.glide / 0.005f) / log10f(400.f);
                kk = clamp01(kk + d);
                t.glide = kk <= 0.01f ? 0.f : 0.005f * powf(400.f, kk);
                changed = true;
            } else if (i == 2 && d != 0.f) { float u = t.unison + d * 50.f; t.unison = u < 0 ? 0 : (u > 50 ? 50 : u); changed = true; }
            else if (i == 3 && d != 0.f && t.engine != ENG_MONOLIT) { t.age = clamp01(t.age + d); changed = true; }  // MONOLIT: AGE na L3 SOUND
            break;
        }
        case K_FILTER: {  // CUTOFF, RES, MODE, DRIVE
            if (obp) {  // OXBOW+: CUTOFF, RES, TYPE, DRIVE; L3: KEYTRK, VELOCITY, VINTAGE, DRIVE
                ObExt& x = t.obx;
                auto add = [&](uint8_t& val, int lo, int hi, float range) {
                    if (d == 0.f) return;
                    int nv = (int)lrintf(val + d * range);
                    nv = nv < lo ? lo : (nv > hi ? hi : nv);
                    if (nv != val) { val = (uint8_t)nv; changed = true; }
                };
                if (alt_ && i < 3) add(i == 0 ? x.keytrk : (i == 1 ? x.velflt : x.vint), 0, i == 0 ? 128 : 127, 127.f);
                else if (i == 0) knob(t.knob[0]);
                else if (i == 1) knob(t.knob[1]);
                else if (i == 2) {
                    int st = stepObType_.steps(ax, dt, 4.f);
                    if (st) { x.ftype = (uint8_t)((x.ftype + st % 6 + 6) % 6); changed = true; }
                } else add(x.drive, 0, 127, 127.f);
                break;
            }
            if (i == 0) knob(t.knob[2]);
            else if (i == 1) knob(t.knob[3]);
            else if (i == 2) {
                int st = stepMode_.steps(ax, dt, 4.f);
                if (st) { t.ext.fmode = (uint8_t)((t.ext.fmode + st % FLT_MODES + FLT_MODES) % FLT_MODES); changed = true; }
            } else if (d != 0.f) { t.ext.drive = clamp01(t.ext.drive + d); changed = true; }
            break;
        }
        case K_MENV: {  // A D S R (L3: S -> DEST, R -> AMOUNT)
            if (alt_ && i == 2) {
                int nd = ext ? LFO_DESTS : DEST_W1;
                int st = stepMDest_.steps(ax, dt, 4.f);
                if (st) { t.menvDest = (uint8_t)(((t.menvDest % nd) + st % nd + nd) % nd); changed = true; }
                break;
            }
            if (d == 0.f) break;
            if (alt_ && i == 3) { float a = t.menvAmt + d * 2.f; t.menvAmt = a < -1 ? -1 : (a > 1 ? 1 : a); }
            else if (i == 2) t.menv[2] = clamp01(t.menv[2] + d);
            else { float kk = clamp01(envKnob(t.menv[i]) + d); t.menv[i] = kk <= 0 ? 0.f : envTime(kk); }
            changed = true;
            break;
        }
        case K_AMP:
        case K_FENV: {  // A D S R (L3: R -> AMOUNT)
            if (d == 0.f) break;
            float* ev = k == K_AMP ? t.env : t.fenv;
            if (i == 3 && alt_ && obp && k == K_FENV) {  // OXBOW+: AMOUNT = galka ENV AMOUNT oryginalu (srodek = 0)
                knob(t.knob[2]);
                break;
            }
            if (i == 3 && alt_) {
                float& amt = k == K_AMP ? t.envAmt : t.fenvAmt;
                float a = amt + d * 2.f;
                amt = a < -1 ? -1 : (a > 1 ? 1 : a);
            } else if (i == 2) ev[2] = clamp01(ev[2] + d);
            else { float kk = clamp01(envKnob(ev[i]) + d); ev[i] = kk <= 0 ? 0.f : envTime(kk); }
            changed = true;
            break;
        }
        default: {  // LFO 1 / 2: RATE DEPTH SHAPE DEST
            int l = k - K_LFO1;
            Lfo& L = t.lfo[l];
            if (i == 0 && L.sync) {  // podzial taktu, skokowo
                int st = stepDiv_[l].steps(ax, dt, 5.f);
                if (st) {
                    int dv = L.div + st;
                    L.div = (uint8_t)(dv < 0 ? 0 : (dv >= LFO_DIVS ? LFO_DIVS - 1 : dv));
                    changed = true;
                }
            } else if (i == 0 && d != 0.f) {
                float kk = clamp01(log10f(L.rate / 0.05f) / log10f(400.f) + d);  // 0.05 .. 20 Hz
                L.rate = 0.05f * powf(400.f, kk);
                changed = true;
            } else if (i == 1 && d != 0.f) { L.depth = clamp01(L.depth + d); changed = true; }
            else if (i == 2) {
                int st = stepShape_[l].steps(ax, dt, 4.f);
                if (st) { L.shape = (uint8_t)((L.shape + st % LFO_SHAPES + LFO_SHAPES) % LFO_SHAPES); changed = true; }
            } else if (i == 3) {
                // w trybie oryginalnym tylko cele oryginalu (WAVE..VOL); w rozszerzonym wszystkie
                int nd = ext ? LFO_DESTS : DEST_W1;
                int st = stepDest_[l].steps(ax, dt, 4.f);
                if (st) { L.dest = (uint8_t)(((L.dest % nd) + st % nd + nd) % nd); changed = true; }
            }
            break;
        }
        }
        if (changed) app_->markDirty();
    }
}

// Wykres obwiedni ADSR (pod ekranem silnika)
static void drawEnvelope(Canvas& c, const float env[4], bool active, float x, float y, float w, float h) {
    float ta = envKnob(env[0]) + 0.05f, td = envKnob(env[1]) + 0.05f, tr = envKnob(env[3]) + 0.05f, ts = 0.35f;
    float tot = ta + td + ts + tr, sx = w / tot, s = env[2];
    float pts[10] = {x, y + h, x + ta * sx, y, x + (ta + td) * sx, y + h - s * h, x + (ta + td + ts) * sx, y + h - s * h,
                     x + w, y + h};
    c.line(x, y + h, x + w, y + h, 1.f, pal::faint);
    Color col[4] = {pal::blue, pal::green, pal::white, pal::red};
    for (int i = 0; i < 4; i++)
        c.line(pts[i * 2], pts[i * 2 + 1], pts[i * 2 + 2], pts[i * 2 + 3], active ? 1.8f : 1.2f,
               active ? col[i] : rgb(90, 90, 90));
    for (int i = 1; i < 4; i++) c.fillCircle(pts[i * 2], pts[i * 2 + 1], active ? 3.f : 2.f, active ? col[i - 1] : rgb(90, 90, 90));
}

void SynthView::render(Canvas& c) {
    Project& p = app_->project();
    AudioEngine* e = app_->engine();
    c.clear(pal::bg);
    if (track_ >= p.nTracks) return;
    const Track& t = p.tracks[track_];
    Color tc = pal::track[t.color % pal::TRACK_COLORS];
    bool playing = e && e->isPlaying() && e->playMode() == Sequencer::SONG;
    int tick = playing ? e->playTick() : 0;
    const bool ext = t.engine == ENG_WAVETABLE && t.alt;
    const bool obp = t.engine == ENG_OXBOW && t.alt;  // OXBOW+
    const int pm = pageMode(t);
    const Kind k = kind(pm);
    char b[64];

    if (k == K_PRESETS) {
        renderPresets(c);
    } else if (t.engine == ENG_MONOLIT) {
        bool live = e && e->wtActive(track_) > 0;
        int16_t shown[4] = {t.knob[0], t.knob[1], t.knob[2], t.knob[3]};
        if (live)
            for (int i = 0; i < 4; i++) shown[i] = e->wtModKnob(track_, i);
        mn_.draw(c, shown, t.push, k == K_SOUND && alt_, e ? e->monolitEnv(track_) : 0, live);
    } else if (t.engine == ENG_SID) {
        bool live = e && e->wtActive(track_) > 0;
        int16_t shown[4] = {t.knob[0], t.knob[1], t.knob[2], t.knob[3]};
        if (live)
            for (int i = 0; i < 4; i++) shown[i] = e->wtModKnob(track_, i);
        sd_.draw(c, shown);
    } else if (t.engine == ENG_OXBOW) {
        bool live = e && e->wtActive(track_) > 0;
        int16_t shown[4] = {t.knob[0], t.knob[1], t.knob[2], t.knob[3]};
        if (live)
            for (int i = 0; i < 4; i++) shown[i] = e->wtModKnob(track_, i);
        OxbowView ov = {};
        if (e) e->oxbowView(track_, &ov);
        const OxbowPlusShow ps = {t.obx.ftype, t.obx.envPitch};
        ob_.draw(c, shown, t.push, k == K_SOUND && alt_, ov, t.alt ? &ps : nullptr);
    } else if (t.engine == ENG_REACTOR) {
        bool live = e && e->wtActive(track_) > 0;
        int16_t shown[4] = {t.knob[0], t.knob[1], t.knob[2], t.knob[3]};
        if (live)
            for (int i = 0; i < 4; i++) shown[i] = e->wtModKnob(track_, i);
        ReactorView rv = {};
        if (e) e->reactorView(track_, &rv);
        rx_.draw(c, shown, rv, dt_);
    } else if (t.engine == ENG_MBIRA) {
        bool live = e && e->wtActive(track_) > 0;
        int16_t shown[4] = {t.knob[0], t.knob[1], t.knob[2], t.knob[3]};
        if (live)
            for (int i = 0; i < 4; i++) shown[i] = e->wtModKnob(track_, i);
        MbiraView mv = {};
        if (e) e->mbiraView(track_, &mv);
        mb_.draw(c, shown, t.push, k == K_SOUND && alt_, mv);
    } else if (t.engine == ENG_EVA) {
        bool live = e && e->wtActive(track_) > 0;
        int16_t shown[4] = {t.knob[0], t.knob[1], t.knob[2], t.knob[3]};
        if (live)
            for (int i = 0; i < 4; i++) shown[i] = e->wtModKnob(track_, i);
        EvaView vv = {};
        if (e) e->evaView(track_, &vv);
        ev_.draw(c, shown, t.push[2], k == K_SOUND && alt_, vv);
    } else if (t.engine == ENG_ELG) {
        ElgView ev = {};
        if (e) e->elgView(track_, &ev);
        el_.draw(c, t.knob, ev);
    } else if (t.engine == ENG_PEW) {
        bool live = e && e->wtActive(track_) > 0;
        int16_t shown[4] = {t.knob[0], t.knob[1], t.knob[2], t.knob[3]};
        if (live)
            for (int i = 0; i < 4; i++) shown[i] = e->wtModKnob(track_, i);
        PewView pv = {};
        if (e) e->pewView(track_, &pv);
        pw_.draw(c, shown, t.push, pv);
    } else if (t.engine == ENG_TAFFY) {
        bool live = e && e->wtActive(track_) > 0;
        int16_t shown[4] = {t.knob[0], t.knob[1], t.knob[2], t.knob[3]};
        if (live)
            for (int i = 0; i < 4; i++) shown[i] = e->wtModKnob(track_, i);
        tf_.draw(c, shown, live, e ? e->taffyEnv(track_) : 0, e ? e->voiceLevel(track_) : 0);
    } else if (t.engine == ENG_WAVETABLE) {
        // ekran pokazuje galki po modulacji (LFO, obwiednia filtra), gdy cos gra
        bool live = e && e->wtActive(track_) > 0;
        int16_t shown[4] = {t.knob[0], t.knob[1], t.knob[2], t.knob[3]};
        if (live)
            for (int i = 0; i < 4; i++) shown[i] = e->wtModKnob(track_, i);
        if (ext) shown[1] = 0;  // w trybie rozszerzonym rozstrojenie jest per oscylator
        wt_.setExtended(ext ? (live ? e->wtModKnob(track_, 4) : t.ext.osc[1].wave) : -1, ext ? t.ext.fmode : 0);
        wt_.draw(c, shown, e ? e->trackLevel(track_) : 0.f, e ? e->wtDrift(track_) : 0, e ? e->wtActive(track_) : 0, dt_);
    } else {
        drawText(c, 320, 170, "TEST SAW", 22, 1.8f, rgb(90, 90, 90), ALIGN_CENTER);
        drawText(c, 320, 210, "SIMPLE SAW FOR TESTS - NO SOUND PARAMETERS", 9, 1.2f, pal::dim, ALIGN_CENTER);
    }
    // naglowek na wierzchu (ekran silnika zaczyna sie pod nim)
    c.fillRect(0, 0, 640, 44, pal::bg);
    screenHeader(c, ICON_KEYS, tc, ext ? "WAVETABLE+" : (t.engine == ENG_OXBOW && t.alt ? "OXBOW+" : engineName(t.engine)), t.name, tc, tick / TICKS_PER_BAR + 1,
                 (tick % TICKS_PER_BAR) / PPQ + 1, playing, p.bpm);
    if (k == K_PRESETS) return;  // przegladarka ma wlasny dol ekranu

    // lewy dol: strona galek
    static const char* kPage[12] = {"SOUND", "OSC 1", "OSC 2", "FILTER", "AMP ENV", "FILTER ENV", "LFO 1", "LFO 2", "VOICE", "PRESETS", "MOD ENV", "BARS"};
    Color pc = k == K_OSC1 ? pal::green : (k == K_OSC2 ? pal::red : pal::white);
    drawText(c, 20, 342, k == K_FENV && t.engine == ENG_TAFFY ? "DCW ENV" : kPage[k], 13, 1.5f, pc);
    int np = pageCount(pm);
    for (int i = 0; i < np; i++) {
        float x = 24 + i * 12;
        if (i == page_) c.fillCircle(x, 366, 2.6f, pal::white);
        else c.circle(x, 366, 2.2f, 1.f, rgb(90, 90, 90));
    }
    drawText(c, 24 + np * 12 + 4, 362, "SELECT = NEXT PAGE", 7, 1.f, rgb(80, 80, 80));
    const char* hint = nullptr;
    Color hc = rgb(80, 80, 80);
    if (k == K_AMP || k == K_FENV) { hint = alt_ ? "L3 = RELEASE" : "L3 = AMOUNT"; if (alt_) hc = pal::red; }
    else if (k == K_MENV) { hint = alt_ ? "L3 = SUSTAIN + RELEASE" : "L3 = DEST + AMOUNT"; if (alt_) hc = pal::red; }
    else if (k == K_VOICE) { hint = t.glideSync ? "GLIDE SYNC TO BPM   L3 = FREE" : "GLIDE FREE   L3 = SYNC TO BPM"; if (t.glideSync) hc = pal::blue; }
    else if (k == K_OSC1 && obp) { hint = alt_ ? "L3 = DETUNE / LEVEL" : "L3 = PW / NOISE"; if (alt_) hc = pal::red; }
    else if (k == K_OSC2 && obp) { hint = alt_ ? "L3 = WAVE / INT / DETUNE / LEVEL" : "L3 = SYNC / X-MOD / PW / ENV>PITCH"; if (alt_) hc = pal::red; }
    else if (k == K_FILTER && obp) { hint = alt_ ? "L3 = CUTOFF / RES / TYPE" : "L3 = KEYTRACK / VELOCITY / VINTAGE"; if (alt_) hc = pal::blue; }
    else if (k == K_OSC1) { hint = alt_ ? "L3 = LEVEL" : "L3 = SUB OSC"; if (alt_) hc = pal::red; }
    else if (k == K_LFO1 || k == K_LFO2) {
        bool sy = t.lfo[k - K_LFO1].sync;
        hint = sy ? "SYNC TO BPM   L3 = FREE" : "FREE   L3 = SYNC TO BPM";
        if (sy) hc = pal::blue;
    }
    if (k == K_SOUND && t.engine == ENG_MONOLIT) { hint = alt_ ? "L3 = SHAPE / CUTOFF / CONTOUR" : "L3 = BODY / VEL / AGE"; if (alt_) hc = pal::blue; }
    if (k == K_SOUND && t.engine == ENG_OXBOW) { hint = alt_ ? "L3 = CUTOFF / RES / ENV" : "L3 = WAVE / FILTER / DRIVE"; if (alt_) hc = pal::blue; }
    if (k == K_SOUND && t.engine == ENG_MBIRA) { hint = alt_ ? "L3 = STRIKE / TINE / BOX" : "L3 = RING / DOUBLE / WAH"; if (alt_) hc = pal::blue; }
    if (k == K_SOUND && t.engine == ENG_EVA) { hint = alt_ ? "L3 = COM" : "L3 = BEEP OCTAVE"; if (alt_) hc = pal::red; }
    if (k == K_SOUND && t.engine == ENG_ELG) {
        hint = alt_ ? "L3 = MOVEMENT" : (t.movSync ? "MOVEMENT SYNC TO BPM   L3 = SYNC" : "L3 = MOVEMENT SYNC");
        if (alt_ || t.movSync) hc = alt_ ? pal::red : pal::blue;
    }
    if (k == K_VOICE && !t.mono && t.unison > 0) { hint = "UNISON WORKS IN MONO"; hc = pal::dim; }
    if (hint) drawText(c, 20, 384, hint, 7, 1.f, hc);

    // prawy dol: grafika strony
    if (k == K_MENV) {
        drawEnvelope(c, t.menv, true, 400, 342, 200, 46);
        snprintf(b, sizeof b, "-> %s %+d%%", destNameFor(t.engine, t.menvDest), (int)lrintf(t.menvAmt * 100));
        drawText(c, 600, 394, b, 8, 1.1f, t.menvAmt == 0 ? pal::dim : pal::white, ALIGN_RIGHT);
    } else if (k == K_FENV) {
        drawEnvelope(c, t.fenv, true, 400, 342, 200, 46);
        if (obp) snprintf(b, sizeof b, "AMOUNT %+d%%", (t.knob[2] - 16384) * 100 / 16384);  // OXBOW+: ENV AMOUNT
        else snprintf(b, sizeof b, "AMOUNT %+d%%", (int)lrintf(t.fenvAmt * 100));
        drawText(c, 600, 394, b, 8, 1.1f, t.fenvAmt == 0 ? pal::dim : pal::white, ALIGN_RIGHT);
    } else if (k == K_LFO1 || k == K_LFO2) {
        const Lfo& L = t.lfo[k - K_LFO1];
        float pts[2 * 81];
        uint32_t seed = 7;
        float rv = 0;
        for (int i = 0; i <= 80; i++) {
            float ph = fmodf(i / 80.f * 2.f, 1.f), v;  // dwa cykle
            switch (L.shape) {
            case LFO_SINE: v = sinf(6.2831853f * ph); break;
            case LFO_TRI: v = ph < 0.5f ? 4.f * ph - 1.f : 3.f - 4.f * ph; break;
            case LFO_SAW: v = 1.f - 2.f * ph; break;
            case LFO_SQUARE: v = ph < 0.5f ? 1.f : -1.f; break;
            default:
                if (i % 10 == 0) { seed = seed * 1664525u + 1013904223u; rv = (int32_t)(seed >> 8) / 8388608.f - 1.f; }
                v = rv;
            }
            pts[i * 2] = 400 + i * 2.5f;
            pts[i * 2 + 1] = 366 - v * 20.f * (0.25f + 0.75f * L.depth);
        }
        c.line(400, 366, 600, 366, 1.f, pal::faint);
        c.polyline(pts, 81, 1.6f, L.depth > 0 ? pal::green : rgb(90, 90, 90));
        snprintf(b, sizeof b, "-> %s", destNameFor(t.engine, L.dest));
        drawText(c, 600, 394, b, 9, 1.2f, L.depth > 0 ? pal::white : pal::dim, ALIGN_RIGHT);
    } else if (k == K_BARS || (k == K_SOUND && t.engine == ENG_PEW)) {
        // dziewiec suwakow (16' .. 1') jak brzmia: niskie niebieskie, srodkowe zielone, wysokie biale
        int32_t lv[9];
        pewRegistration(t.knob[0], t.push, lv);
        for (int i = 0; i < 9; i++) {
            const float x = 420 + i * 20, h = 6 + 40.f * lv[i] / 32768.f;
            const Color col = i < 3 ? pal::blue : (i < 6 ? pal::green : pal::white);
            c.line(x, 388, x, 388 - h, 3.f, col);
        }
        drawText(c, 600, 394, "DRAWBARS", 8, 1.1f, pal::dim, ALIGN_RIGHT);
    } else if (k == K_VOICE) {
        // glosy: POLY = 6 niezaleznych kresek, MONO = jedna (albo wachlarz unison); AGE = falowanie
        int n = t.mono ? (t.unison > 0 ? 6 : 1) : 6;
        for (int i = 0; i < n; i++) {
            float spread = t.mono && n > 1 ? (2.f * i / (n - 1) - 1.f) * t.unison / 50.f : 0.f;
            float x0 = t.mono ? 500 : 410 + i * 36, x1 = t.mono ? 500 + spread * 90 : x0;
            float wob = t.age * 4.f * sinf(i * 1.7f + (float)tick * 0.01f);
            c.line(x0, 392, x1 + wob, 344, 2.f, t.mono ? pal::green : pal::blue);
            c.fillCircle(x1 + wob, 344, 3.f, t.mono ? pal::green : pal::blue);
        }
        snprintf(b, sizeof b, "%s%s", t.mono ? "MONO" : "POLY 6", t.mono && t.unison > 0 ? " UNISON 6" : "");
        drawText(c, 600, 394, b, 8, 1.1f, pal::white, ALIGN_RIGHT);
    } else if (obp && (k == K_OSC1 || k == K_OSC2 || k == K_FILTER)) {
        // OXBOW+: poziomy OSC 1 (zielony), OSC 2 (czerwony), NOISE (bialy) i typ filtra
        static const char* F[6] = {"LP12", "HP12", "BP12", "NOTCH", "LP24", "SEM"};
        const float lv[3] = {t.obx.level[0] / 126.f, t.obx.level[1] / 126.f, t.obx.noise / 126.f};
        const Color lc[3] = {pal::green, pal::red, pal::white};
        const char* ln[3] = {"OSC1", "OSC2", "NOISE"};
        for (int i = 0; i < 3; i++) {
            float y = 344 + i * 16;
            drawText(c, 400, y, ln[i], 7, 1.f, lc[i]);
            c.line(446, y + 3.5f, 600, y + 3.5f, 1.f, pal::faint);
            if (lv[i] > 0.001f) c.line(446, y + 3.5f, 446 + 154 * lv[i], y + 3.5f, 2.5f, lc[i]);
        }
        snprintf(b, sizeof b, "%sFILTER %s", t.obx.sync ? "SYNC  " : (t.obx.xmod ? "X-MOD  " : ""), F[t.obx.ftype % 6]);
        drawText(c, 600, 394, b, 8, 1.1f, pal::white, ALIGN_RIGHT);
    } else if (k == K_OSC1 || k == K_OSC2 || k == K_FILTER) {
        // mieszanie: poziomy OSC 1 (zielony), OSC 2 (czerwony), SUB (bialy)
        const float lv[3] = {t.ext.osc[0].level, t.ext.osc[1].level, t.ext.sub};
        const Color lc[3] = {pal::green, pal::red, pal::white};
        const char* ln[3] = {"OSC1", "OSC2", "SUB"};
        for (int i = 0; i < 3; i++) {
            float y = 344 + i * 16;
            drawText(c, 400, y, ln[i], 7, 1.f, lc[i]);
            c.line(440, y + 3.5f, 600, y + 3.5f, 1.f, pal::faint);
            if (lv[i] > 0.001f) c.line(440, y + 3.5f, 440 + 160 * lv[i], y + 3.5f, 2.5f, lc[i]);
        }
        snprintf(b, sizeof b, "FILTER %s", filterModeName(t.ext.fmode));
        drawText(c, 600, 394, b, 8, 1.1f, pal::white, ALIGN_RIGHT);
    } else {
        drawEnvelope(c, t.env, k == K_AMP, 400, 342, 200, 46);
        snprintf(b, sizeof b, "AMOUNT %+d%%", (int)lrintf(t.envAmt * 100));
        drawText(c, 600, 394, b, 8, 1.1f, k == K_AMP ? pal::white : pal::dim, ALIGN_RIGHT);
    }

    // galki
    char v[4][16];
    float pos[4];
    const char* labels[4];
    auto cutoffName = [](char* out, int16_t kn) {
        float hz = 30.f * powf(16000.f / 30.f, kn / 32767.f);
        if (hz >= 1000) snprintf(out, 16, "%.1fK", hz / 1000.f);
        else snprintf(out, 16, "%.0f", hz);
    };
    if (k == K_SOUND && t.engine == ENG_MONOLIT) {  // MONOLIT: makra SHAPE, CUTOFF, CONTOUR, HEAT; L3: BODY, VEL, AGE
        static const char* L[4] = {"SHAPE", "CUTOFF", "CONTOUR", "HEAT"};
        for (int i = 0; i < 4; i++) { labels[i] = L[i]; pos[i] = t.knob[i] / 32767.f; snprintf(v[i], 16, "%d", t.knob[i] * 100 / 32767); }
        static const char* SH[5] = {"SAW", "SAW+SUB", "2 SAWS", "3 SAWS", "STACK"};
        snprintf(v[0], 16, "%s", SH[(t.knob[0] * 4 + 16383) >> 15 > 4 ? 4 : (t.knob[0] * 4 + 16383) >> 15]);
        snprintf(v[2], 16, "%s %d", t.knob[2] < 16384 ? "PLUCK" : "SWELL", t.knob[2] * 100 / 32767);
        if (alt_) {
            static const char* B[5] = {"SUB", "UNI", "5TH", "OCT", "SUB2"};
            const int bs = (t.push[0] * 5) >> 15;
            labels[0] = "BODY"; snprintf(v[0], 16, "%s", B[bs > 4 ? 4 : bs]);
            labels[1] = "VEL"; snprintf(v[1], 16, "%d", t.push[1] * 100 / 32767);
            labels[2] = "AGE"; snprintf(v[2], 16, "%d", t.push[2] * 100 / 32767);
            for (int i = 0; i < 3; i++) pos[i] = t.push[i] / 32767.f;
        }
    } else if (k == K_SOUND && t.engine == ENG_SID) {  // SID: fala, glosy / szerokosc / interwal, filtr
        static const char* W[6] = {"TRIANGLE", "SAW", "PULSE", "NOISE", "SYNC", "RING"};
        const unsigned mode = sidWaveMode(t.knob[0]);
        snprintf(v[0], 16, "%s", W[mode]);
        if (mode >= 4) { labels[1] = "INTERVAL"; snprintf(v[1], 16, "+%u ST", sidIntervalSemi(t.knob[1])); }
        else if (mode == 2) { labels[1] = "WIDTH"; snprintf(v[1], 16, "%u", 3 + (((unsigned)t.knob[1] * 94 + 16383) >> 15)); }
        else { labels[1] = "VOICES"; snprintf(v[1], 16, "%u", ((unsigned)t.knob[1] * 99 + 16383) >> 15); }
        snprintf(v[2], 16, "%u", ((unsigned)t.knob[2] * 99 + 16383) >> 15);
        snprintf(v[3], 16, "%u", ((unsigned)t.knob[3] * 99 + 16383) >> 15);
        labels[0] = "WAVE"; labels[2] = "CUTOFF"; labels[3] = "RESONANCE";
        for (int i = 0; i < 4; i++) pos[i] = t.knob[i] / 32767.f;
        pos[0] = mode / 5.f;  // fala jako przelacznik
    } else if (k == K_SOUND && t.engine == ENG_OXBOW) {  // OB: filtr, rezonans, obwiednia filtra (+/-), rozstrojenie OSC 2
        static const char* L[4] = {"CUTOFF", "RESONANCE", "ENV AMT", "DETUNE"};
        for (int i = 0; i < 4; i++) { labels[i] = L[i]; pos[i] = t.knob[i] / 32767.f; }
        snprintf(v[0], 16, "%d", t.knob[0] * 100 / 32767);
        snprintf(v[1], 16, "%d", t.knob[1] * 100 / 32767);
        snprintf(v[2], 16, "%+d", (t.knob[2] - 16384) * 100 / 16384);
        snprintf(v[3], 16, "%.0fC", (t.knob[3] * 128 >> 15) * 50.f / 128.f);
        if (alt_) {  // strona 2: WAVE, FILTER, DRIVE
            static const char* W[6] = {"SAW", "PULSE", "BOTH", "SYNC", "X-MOD", "FIFTH"};
            static const char* F[6] = {"LP12", "HP12", "BP12", "NOTCH", "LP24", "SEM"};
            const int w0 = t.push[0] < 0 ? 0 : t.push[0], w1 = t.push[1] < 0 ? 0 : t.push[1];
            labels[0] = "WAVE"; snprintf(v[0], 16, "%s", W[(w0 * 6) >> 15]);
            labels[1] = "FILTER"; snprintf(v[1], 16, "%s", F[((unsigned)w1 >> 9) % 6]);
            labels[2] = "DRIVE"; snprintf(v[2], 16, "%d", t.push[2] * 100 / 32767);
            for (int i = 0; i < 3; i++) pos[i] = t.push[i] / 32767.f;
        }
    } else if (k == K_SOUND && t.engine == ENG_REACTOR) {  // reaktor: chlodzenie, promieniowanie, prety, cieplo
        static const char* L[4] = {"COOLING", "RADIATION", "RODS", "HEAT"};
        for (int i = 0; i < 4; i++) { labels[i] = L[i]; pos[i] = t.knob[i] / 32767.f; snprintf(v[i], 16, "%d", t.knob[i] * 100 / 32767); }
    } else if (k == K_SOUND && t.engine == ENG_MBIRA) {  // pianino kciukowe: uderzenie, material, pudlo, kapsle
        static const char* L[4] = {"STRIKE", "TINE", "BOX", "BUZZ"};
        snprintf(v[0], 16, "%d", t.knob[0] * 100 / 32767);
        static const char* mat[3] = {"WOOD", "STEEL", "GLASS"};
        const int p2 = t.knob[1] * 2, mi = p2 >> 15, fr = (p2 & 0x7FFF) * 100 / 32768;
        if (mi >= 2 || fr < 8) snprintf(v[1], 16, "%s", mat[mi >= 2 ? 2 : mi]);
        else if (fr > 92) snprintf(v[1], 16, "%s", mat[mi + 1]);
        else snprintf(v[1], 16, "%.2s>%.2s", mat[mi], mat[mi + 1]);
        snprintf(v[2], 16, "%d", t.knob[2] * 100 / 32767);
        snprintf(v[3], 16, "%d", t.knob[3] * 100 / 32767);
        for (int i = 0; i < 4; i++) { labels[i] = L[i]; pos[i] = t.knob[i] / 32767.f; }
        if (alt_) {  // strona 2: RING 1,6..8 s, DOUBLE do 12 c, WAH
            static const char* L2[3] = {"RING", "DOUBLE", "WAH"};
            snprintf(v[0], 16, "%.1fS", 1.6f + 6.4f * t.push[0] / 32767.f);
            snprintf(v[1], 16, "%.1fC", 12.f * t.push[1] / 32767.f);
            snprintf(v[2], 16, "%d", t.push[2] * 100 / 32767);
            for (int i = 0; i < 3; i++) { labels[i] = L2[i]; pos[i] = t.push[i] / 32767.f; }
        }
    } else if (k == K_SOUND && t.engine == ENG_EVA) {  // chor: jasnosc/atak, samogloska, oddech, radio
        snprintf(v[0], 16, "%d", t.knob[0] * 100 / 32767);
        static const char* vw[3] = {"OO", "AH", "EE"};
        const int pos2 = t.knob[1] * 2, vi = pos2 >> 15, fr = (pos2 & 0x7FFF) * 100 / 32768;
        if (vi >= 2 || fr < 8) snprintf(v[1], 16, "%s", vw[vi >= 2 ? 2 : vi]);
        else if (fr > 92) snprintf(v[1], 16, "%s", vw[vi + 1]);
        else snprintf(v[1], 16, "%s>%s", vw[vi], vw[vi + 1]);
        snprintf(v[2], 16, "%d", t.knob[2] * 100 / 32767);
        const int c3 = t.knob[3];
        snprintf(v[3], 16, "%s %d", c3 < 13107 ? "WARM" : c3 < 24576 ? "RADIO" : "CRUSH", c3 * 100 / 32767);
        static const char* L[4] = {"PRESSURE", "FREQ", "OXYGEN", "COM"};
        for (int i = 0; i < 4; i++) { labels[i] = L[i]; pos[i] = t.knob[i] / 32767.f; }
        if (alt_) {
            static const char* oc[3] = {"DOWN", "ORIGINAL", "UP"};
            const int cur = t.push[2] <= 0 ? 1 : (t.push[2] * 3) >> 15;
            labels[3] = "BEEP OCT";
            snprintf(v[3], 16, "%s", oc[cur > 2 ? 2 : cur]);
            pos[3] = (cur > 2 ? 2 : cur) / 2.f;
        }
    } else if (k == K_SOUND && t.engine == ENG_ELG) {  // dron: interwal trzeciego glosu, rozstrojenie, barwa, ruch
        static const char* iv[5] = {"UNISON", "FIFTH", "OCTAVE", "OCT+5TH", "2 OCT"};
        int st = elgInterval(t.knob[0]);
        snprintf(v[0], 16, "%s", iv[st > 4 ? 4 : st]);
        snprintf(v[1], 16, "%.1fC", ((t.knob[1] * 393) >> 15) / 32768.f * 1731.2f);  // ulamek Q15 -> centy
        snprintf(v[2], 16, "%d", t.knob[2] * 100 / 32767);
        snprintf(v[3], 16, "%d", t.knob[3] * 100 / 32767);
        static const char* L[4] = {"INTERVAL", "DETUNE", "TIMBRE", "MOVEMENT"};
        for (int i = 0; i < 4; i++) { labels[i] = L[i]; pos[i] = t.knob[i] / 32767.f; }
        if (alt_) {  // L3: synchronizacja ruchu z tempem
            labels[3] = "SYNC";
            snprintf(v[3], 16, "%s", t.movSync ? lfoDivName(t.movDiv) : "OFF");
            pos[3] = t.movSync ? (t.movDiv + 1) / (float)LFO_DIVS : 0.f;
        } else if (t.movSync) snprintf(v[3], 16, "%d %s", t.knob[3] * 100 / 32767, lfoDivName(t.movDiv));
    } else if (k == K_SOUND && t.engine == ENG_PEW) {  // organy: rejestracja, perkusja, drive/chorus, Leslie
        snprintf(v[0], 16, "%d", t.knob[0] * 100 / 32767);  // uklad suwakow widac na grafice obok
        const int g = t.knob[1];
        if (g < 3277) snprintf(v[1], 16, "OFF");
        else if (g < 14746) snprintf(v[1], 16, "SOFT %d", (g - 3277) * 100 / (14746 - 3277));
        else if (g < 24576) snprintf(v[1], 16, "SHARP %d", (g - 14746) * 100 / (24576 - 14746));
        else snprintf(v[1], 16, "CLICK %d", (g - 24576) * 100 / (32767 - 24576));
        const int w = t.knob[2];
        if (w <= 14746) snprintf(v[2], 16, w < 300 ? "CLEAN" : "CHO %d", w * 100 / 14746);
        else snprintf(v[2], 16, "DRV %d", (w - 14746) * 100 / (32767 - 14746));
        static const char* les[3] = {"STOP", "SLOW", "FAST"};
        snprintf(v[3], 16, "%s", les[t.knob[3] < 10923 ? 0 : t.knob[3] < 21845 ? 1 : 2]);
        static const char* L[4] = {"REGISTER", "PERCUSSION", "DRIVE", "LESLIE"};
        for (int i = 0; i < 4; i++) { labels[i] = L[i]; pos[i] = t.knob[i] / 32767.f; }
    } else if (k == K_BARS) {  // PEW: wysuniecie grup suwakow
        static const char* L[4] = {"LOW BARS", "MID BARS", "HIGH BARS", ""};
        for (int i = 0; i < 3; i++) { labels[i] = L[i]; snprintf(v[i], 16, "+%d", t.push[i] * 100 / 32767); pos[i] = t.push[i] / 32767.f; }
        labels[3] = L[3]; v[3][0] = 0; pos[3] = 0;
    } else if (k == K_SOUND && t.engine == ENG_TAFFY) {  // CZ: ksztalt (saw -> square -> pulse -> saw-pulse), DCW, ENV, RES
        static const char* sh[4] = {"SAW", "SQUARE", "PULSE", "SAWPULSE"};
        int32_t sp = t.knob[0] * 3;
        int si = sp >> 15, sf = (sp & 0x7FFF) * 100 / 32768;
        if (sf < 6 || si == 3) snprintf(v[0], 16, "%s", sh[si]);
        else if (sf > 94) snprintf(v[0], 16, "%s", sh[si + 1]);
        else snprintf(v[0], 16, "%.3s>%.3s", sh[si], sh[si + 1]);
        for (int i = 1; i < 4; i++) snprintf(v[i], 16, "%d", t.knob[i] * 100 / 32767);
        static const char* L[4] = {"WAVE", "DCW", "ENV", "RESONANCE"};
        for (int i = 0; i < 4; i++) { labels[i] = L[i]; pos[i] = t.knob[i] / 32767.f; }
    } else if (k == K_SOUND && t.engine == ENG_WAVETABLE) {
        waveName(v[0], 16, t.knob[0]);
        snprintf(v[1], 16, "%.1fC", 14.f * t.knob[1] / 32767.f);
        cutoffName(v[2], t.knob[2]);
        snprintf(v[3], 16, "%d", t.knob[3] * 100 / 32767);
        static const char* L[4] = {"WAVE", "DETUNE", "CUTOFF", "RESONANCE"};
        for (int i = 0; i < 4; i++) { labels[i] = L[i]; pos[i] = t.knob[i] / 32767.f; }
    } else if (obp && (k == K_OSC1 || k == K_OSC2)) {  // OXBOW+
        static const char* W[4] = {"OFF", "SAW", "PULSE", "BOTH"};
        const int o = k - K_OSC1;
        const ObExt& x = t.obx;
        snprintf(v[0], 16, "%s", W[x.wave[o] & 3]);
        snprintf(v[1], 16, "%+d", x.semi[o]);
        if (o == 0) snprintf(v[2], 16, "%+dC", x.cents1);
        else snprintf(v[2], 16, "%.0fC", (t.knob[3] * 128 >> 15) * 50.f / 128.f);
        snprintf(v[3], 16, "%d", x.level[o] * 100 / 126);
        labels[0] = "WAVE"; labels[1] = "INTERVAL"; labels[2] = "DETUNE"; labels[3] = "LEVEL";
        pos[0] = x.wave[o] / 3.f; pos[1] = (x.semi[o] + 24) / 48.f;
        pos[2] = o == 0 ? (x.cents1 + 50) / 100.f : t.knob[3] / 32767.f; pos[3] = x.level[o] / 126.f;
        if (alt_ && o == 0) {
            labels[2] = "PW"; snprintf(v[2], 16, "%d%%", x.pw[0] * 100 / 128); pos[2] = x.pw[0] / 127.f;
            labels[3] = "NOISE"; snprintf(v[3], 16, "%d", x.noise * 100 / 126); pos[3] = x.noise / 126.f;
        } else if (alt_) {
            labels[0] = "SYNC"; snprintf(v[0], 16, "%s", x.sync ? "ON" : "OFF"); pos[0] = x.sync ? 1.f : 0.f;
            labels[1] = "X-MOD"; snprintf(v[1], 16, "%d", x.xmod * 100 / 127); pos[1] = x.xmod / 127.f;
            labels[2] = "PW"; snprintf(v[2], 16, "%d%%", x.pw[1] * 100 / 128); pos[2] = x.pw[1] / 127.f;
            labels[3] = "ENV>PITCH"; snprintf(v[3], 16, "%+.0f", x.envPitch * 24.f / 64.f); pos[3] = (x.envPitch + 64) / 127.f;
        }
    } else if (obp && k == K_FILTER) {  // OXBOW+
        static const char* F[6] = {"LP12", "HP12", "BP12", "NOTCH", "LP24", "SEM"};
        const ObExt& x = t.obx;
        snprintf(v[0], 16, "%d", t.knob[0] * 100 / 32767);
        snprintf(v[1], 16, "%d", t.knob[1] * 100 / 32767);
        snprintf(v[2], 16, "%s", F[x.ftype % 6]);
        snprintf(v[3], 16, "%d", x.drive * 100 / 127);
        labels[0] = "CUTOFF"; labels[1] = "RES"; labels[2] = "TYPE"; labels[3] = "DRIVE";
        pos[0] = t.knob[0] / 32767.f; pos[1] = t.knob[1] / 32767.f; pos[2] = x.ftype / 5.f; pos[3] = x.drive / 127.f;
        if (alt_) {
            labels[0] = "KEYTRACK"; snprintf(v[0], 16, "%d", x.keytrk * 100 / 128); pos[0] = x.keytrk / 128.f;
            labels[1] = "VELOCITY"; snprintf(v[1], 16, "%d", x.velflt * 100 / 127); pos[1] = x.velflt / 127.f;
            labels[2] = "VINTAGE"; snprintf(v[2], 16, "%d", x.vint * 100 / 127); pos[2] = x.vint / 127.f;
        }
    } else if (k == K_OSC1 || k == K_OSC2) {
        int o = k - K_OSC1;
        const WtOsc& os = t.ext.osc[o];
        int16_t w = o == 0 ? t.knob[0] : os.wave;
        waveName(v[0], 16, w);
        snprintf(v[1], 16, "%+d", os.semi);
        snprintf(v[2], 16, "%+.0fC", os.cents);
        bool sub = o == 0 && alt_;
        snprintf(v[3], 16, "%d", (int)lrintf((sub ? t.ext.sub : os.level) * 100));
        labels[0] = "WAVE"; labels[1] = "INTERVAL"; labels[2] = "DETUNE"; labels[3] = sub ? "SUB" : "LEVEL";
        pos[0] = w / 32767.f; pos[1] = (os.semi + 24) / 48.f; pos[2] = (os.cents + 50) / 100.f;
        pos[3] = sub ? t.ext.sub : os.level;
    } else if (k == K_VOICE) {
        snprintf(v[0], 16, "%s", t.mono ? "MONO" : "POLY");
        if (t.glideSync) snprintf(v[1], 16, "%s", lfoDivName(t.glideDiv));
        else if (t.glide <= 0.f) snprintf(v[1], 16, "OFF");
        else if (t.glide < 1.f) snprintf(v[1], 16, "%.0fMS", t.glide * 1000.f);
        else snprintf(v[1], 16, "%.1fS", t.glide);
        if (t.unison <= 0.f) snprintf(v[2], 16, "OFF");
        else snprintf(v[2], 16, "%.0fC%s", t.unison, t.mono ? "" : "*");
        snprintf(v[3], 16, "%d", (int)lrintf(t.age * 100));
        if (t.engine == ENG_MONOLIT) snprintf(v[3], 16, "SOUND L3");  // AGE oryginalu: SOUND + L3
        labels[0] = "MODE"; labels[1] = t.glideSync ? "GLIDE SYNC" : "GLIDE"; labels[2] = "UNISON"; labels[3] = "AGE";
        pos[0] = t.mono ? 1.f : 0.f;
        pos[1] = t.glideSync ? 1.f - t.glideDiv / (float)(LFO_DIVS - 1)
                             : (t.glide <= 0.f ? 0.f : log10f(t.glide / 0.005f) / log10f(400.f));
        pos[2] = t.unison / 50.f;
        pos[3] = t.age;
    } else if (k == K_FILTER) {
        cutoffName(v[0], t.knob[2]);
        snprintf(v[1], 16, "%d", t.knob[3] * 100 / 32767);
        snprintf(v[2], 16, "%s", filterModeName(t.ext.fmode));
        snprintf(v[3], 16, "%d", (int)lrintf(t.ext.drive * 100));
        labels[0] = "CUTOFF"; labels[1] = "RES"; labels[2] = "MODE"; labels[3] = "DRIVE";
        pos[0] = t.knob[2] / 32767.f; pos[1] = t.knob[3] / 32767.f; pos[2] = t.ext.fmode / (float)(FLT_MODES - 1);
        pos[3] = t.ext.drive;
    } else if (k == K_MENV) {
        static const char* LA[4] = {"ATTACK", "DECAY", "SUSTAIN", "RELEASE"};
        for (int i = 0; i < 4; i++) {
            labels[i] = LA[i];
            if (i == 2 && !alt_) { snprintf(v[i], 16, "%d", (int)lrintf(t.menv[2] * 100)); pos[i] = t.menv[2]; }
            else if (i == 2) { labels[i] = "DEST"; snprintf(v[i], 16, "%s", destNameFor(t.engine, t.menvDest)); pos[i] = t.menvDest / (float)(LFO_DESTS - 1); }
            else if (i == 3 && alt_) { labels[i] = "AMOUNT"; snprintf(v[i], 16, "%+d", (int)lrintf(t.menvAmt * 100)); pos[i] = 0.5f + t.menvAmt * 0.5f; }
            else {
                float s = t.menv[i];
                if (s < 1.f) snprintf(v[i], 16, "%.0fMS", s * 1000.f);
                else snprintf(v[i], 16, "%.1fS", s);
                pos[i] = envKnob(s);
            }
        }
    } else if (k == K_AMP || k == K_FENV) {
        const float* ev = k == K_FENV ? t.fenv : t.env;
        static const char* LA[4] = {"ATTACK", "DECAY", "SUSTAIN", "RELEASE"};
        for (int i = 0; i < 4; i++) {
            labels[i] = LA[i];
            if (i == 2) { snprintf(v[i], 16, "%d", (int)lrintf(ev[2] * 100)); pos[i] = ev[2]; }
            else if (i == 3 && alt_) {
                labels[i] = "AMOUNT";
                float amt = k == K_AMP ? t.envAmt : (obp ? (t.knob[2] - 16384) / 16384.f : t.fenvAmt);
                snprintf(v[i], 16, "%+d", (int)lrintf(amt * 100));
                pos[i] = 0.5f + amt * 0.5f;
            } else {
                float s = ev[i];
                if (s < 1.f) snprintf(v[i], 16, "%.0fMS", s * 1000.f);
                else snprintf(v[i], 16, "%.1fS", s);
                pos[i] = envKnob(s);
            }
        }
    } else {
        const Lfo& L = t.lfo[k - K_LFO1];
        static const char* LL[4] = {"RATE", "DEPTH", "SHAPE", "DEST"};
        for (int i = 0; i < 4; i++) labels[i] = LL[i];
        if (L.sync) labels[0] = "SYNC";
        if (L.sync) snprintf(v[0], 16, "%s", lfoDivName(L.div));
        else if (L.rate < 1.f) snprintf(v[0], 16, "%.2fHZ", L.rate);
        else snprintf(v[0], 16, "%.1fHZ", L.rate);
        snprintf(v[1], 16, "%d", (int)lrintf(L.depth * 100));
        snprintf(v[2], 16, "%s", lfoShapeName(L.shape));
        snprintf(v[3], 16, "%s", destNameFor(t.engine, L.dest));
        pos[0] = L.sync ? L.div / (float)(LFO_DIVS - 1) : log10f(L.rate / 0.05f) / log10f(400.f);
        pos[1] = L.depth;
        pos[2] = L.shape / (float)(LFO_SHAPES - 1);
        pos[3] = L.dest / (float)(LFO_DESTS - 1);
    }
    const char* vals[4] = {v[0], v[1], v[2], v[3]};
    encoderBar(c, labels, vals, pos);
}

// ---------------- PRESETS ----------------

int SynthView::filtered(int* idx, int max) {
    PresetStore& ps = app_->presetsFor(app_->project().tracks[track_].engine, app_->project().tracks[track_].alt);
    int n = 0;
    for (int i = 0; i < ps.count() && n < max; i++)
        if (catSel_ == 0 || ps.category(i) == catSel_ - 1) idx[n++] = i;
    return n;
}

void SynthView::updatePresets(const Buttons& b) {
    Project& p = app_->project();
    Track& t = p.tracks[track_];
    PresetStore& ps = app_->presetsFor(app_->project().tracks[track_].engine, app_->project().tracks[track_].alt);
    if (ed_.active()) {  // wpisywanie nazwy
        ed_.update(b);
        if (!ed_.active()) savePending_ = true;
        return;
    }
    if (savePending_) {  // potwierdzenie zapisu
        if (b.pressed(B_A)) {
            int cat = catSel_ == 0 ? CAT_OTHER : catSel_ - 1;
            if (ps.save(t, saveName_, cat)) {
                snprintf(t.preset, sizeof t.preset, "%s", saveName_);
                app_->markDirty();
                char m[48];
                snprintf(m, sizeof m, "SAVED %s", saveName_);
                app_->toast(m);
                int idx[PresetStore::MAX], n = filtered(idx, PresetStore::MAX), f = ps.find(saveName_);
                for (int i = 0; i < n; i++) if (idx[i] == f) preSel_ = i;
                focus_ = 1;
            } else app_->toast("SAVE FAILED");
            savePending_ = false;
        }
        if (b.pressed(B_B)) savePending_ = false;
        return;
    }
    int idx[PresetStore::MAX], n = filtered(idx, PresetStore::MAX);
    if (b.repeat(B_LEFT)) { focus_ = 0; confirm_ = 0; }
    if (b.repeat(B_RIGHT)) { focus_ = 1; confirm_ = 0; }
    if (focus_ == 0) {
        int old = catSel_;
        if (b.repeat(B_DOWN) && catSel_ < PRESET_CATS) catSel_++;
        if (b.repeat(B_UP) && catSel_ > 0) catSel_--;
        if (old != catSel_) { preSel_ = 0; preFirst_ = 0; confirm_ = 0; n = filtered(idx, PresetStore::MAX); }
    } else {
        if (b.repeat(B_DOWN) && preSel_ < n - 1) { preSel_++; confirm_ = 0; }
        if (b.repeat(B_UP) && preSel_ > 0) { preSel_--; confirm_ = 0; }
    }
    if (preSel_ >= n) preSel_ = n > 0 ? n - 1 : 0;
    if (b.pressed(B_A) && n > 0) {  // wczytaj
        if (ps.load(idx[preSel_], t)) {
            snprintf(t.preset, sizeof t.preset, "%s", ps.name(idx[preSel_]));
            page_ = pageCount(pageMode(t)) - 1;  // zostan na PRESETS (preset moze zmienic tryb)
            app_->markDirty();
            char m[48];
            snprintf(m, sizeof m, "LOADED %s", ps.name(idx[preSel_]));
            app_->toast(m);
        }
        focus_ = 1;
    }
    if (b.pressed(B_X)) {  // zapisz: najpierw nazwa
        snprintf(saveName_, sizeof saveName_, "%s", t.preset[0] ? t.preset : "MY SOUND");
        ed_.begin(saveName_);
        confirm_ = 0;
    }
    if (b.pressed(B_B) && n > 0 && focus_ == 1) {  // usun (dwa razy)
        if (confirm_ != 1) confirm_ = 1;
        else {
            char m[48];
            snprintf(m, sizeof m, "DELETED %s", ps.name(idx[preSel_]));
            ps.remove(idx[preSel_]);
            app_->toast(m);
            confirm_ = 0;
        }
    }
    if (b.pressed(B_Y)) {  // INIT (dwa razy)
        if (confirm_ != 2) confirm_ = 2;
        else {
            PresetStore::initInstrument(t);
            snprintf(t.preset, sizeof t.preset, "INIT");
            page_ = pageCount(pageMode(t)) - 1;
            app_->markDirty();
            app_->toast("INIT");
            confirm_ = 0;
        }
    }
}

void SynthView::renderPresets(Canvas& c) {
    PresetStore& ps = app_->presetsFor(app_->project().tracks[track_].engine, app_->project().tracks[track_].alt);
    const Track& t = app_->project().tracks[track_];
    char b[96];
    const float top = 56, rowH = 34;
    const int rows = 10;
    // lewa kolumna: kategorie z ikonami
    for (int i = 0; i <= PRESET_CATS && i < rows; i++) {
        float y = top + i * rowH;
        bool sel = i == catSel_;
        if (sel) {
            if (focus_ == 0) c.fillRoundRect(20, y - 4, 206, rowH - 6, 6, rgb(pal::tan.r, pal::tan.g, pal::tan.b, 70));
            else c.strokeRoundRect(20, y - 4, 206, rowH - 6, 6, 1.f, rgb(110, 85, 55));
        }
        Color col = sel ? rgb(245, 210, 160) : pal::tan;
        drawCategoryIcon(c, i - 1, 30, y - 1, 20, col);
        drawText(c, 62, y + 2, i == 0 ? "ALL" : presetCatName(i - 1), 14, 1.6f, col);
    }
    c.line(236, top - 6, 236, top + rows * rowH - 10, 1.5f, pal::steel);
    // prawa kolumna: presety
    int idx[PresetStore::MAX], n = filtered(idx, PresetStore::MAX);
    if (preSel_ < preFirst_) preFirst_ = preSel_;
    if (preSel_ >= preFirst_ + rows) preFirst_ = preSel_ - rows + 1;
    if (!ps.ready()) drawText(c, 256, top + 2, "NO STORAGE", 14, 1.6f, pal::dim);
    else if (n == 0) drawText(c, 256, top + 2, "EMPTY - X = SAVE", 14, 1.6f, pal::dim);
    for (int r = 0; r < rows && preFirst_ + r < n; r++) {
        int i = preFirst_ + r, pi = idx[i];
        float y = top + r * rowH;
        bool sel = i == preSel_;
        bool cur = !strcmp(ps.name(pi), t.preset);
        if (sel) {
            if (focus_ == 1) c.fillRoundRect(248, y - 4, 372, rowH - 6, 6, rgb(255, 255, 255, 40));
            else c.strokeRoundRect(248, y - 4, 372, rowH - 6, 6, 1.f, rgb(80, 80, 80));
        }
        drawCategoryIcon(c, ps.category(pi), 258, y - 1, 20, sel ? pal::white : rgb(130, 130, 130));
        drawText(c, 290, y + 2, ps.name(pi), 14, 1.6f, sel ? pal::white : rgb(190, 190, 190));
        if (cur) c.fillCircle(608, y + 9, 3.f, pal::green);  // aktualnie na sciezce
    }
    if (n > rows) {  // pasek przewijania
        float h = rows * rowH - 10, th = h * rows / n, ty = top - 6 + (h - th) * preFirst_ / (float)(n - rows);
        c.line(624, ty, 624, ty + th, 3.f, pal::white);
    }
    // okno zapisu
    if (ed_.active() || savePending_) {
        c.fillRoundRect(120, 150, 400, 150, 12, rgb(12, 12, 12));
        c.strokeRoundRect(120, 150, 400, 150, 12, 1.5f, pal::steel);
        int cat = catSel_ == 0 ? CAT_OTHER : catSel_ - 1;
        snprintf(b, sizeof b, "SAVE PRESET IN %s", presetCatName(cat));
        drawText(c, 140, 164, b, 10, 1.3f, pal::tan);
        nameBox(c, 140, 202, 360, 50, "", saveName_, &ed_, pal::steel);
        if (savePending_) {
            bool exists = app_->presetsFor(app_->project().tracks[track_].engine, app_->project().tracks[track_].alt).find(saveName_) >= 0;
            drawText(c, 140, 270, exists ? "A = OVERWRITE   B = CANCEL" : "A = SAVE   B = CANCEL", 9, 1.2f,
                     exists ? pal::red : pal::white);
        } else drawText(c, 140, 270, "UP/DOWN = LETTER  LEFT/RIGHT = MOVE  A = DONE", 8, 1.1f, pal::dim);
    }
    // dol: pigulki akcji
    const char* st;
    if (confirm_ == 1) st = "PRESS B AGAIN TO DELETE";
    else if (confirm_ == 2) st = "PRESS Y AGAIN TO RESET SOUND (INIT)";
    else {
        snprintf(b, sizeof b, "%s  -  %d PRESETS  -  LEFT/RIGHT = COLUMN  -  SELECT = NEXT PAGE",
                 catSel_ == 0 ? "ALL" : presetCatName(catSel_ - 1), n);
        st = b;
    }
    const char* labels[4] = {confirm_ == 1 ? "SURE?" : "DELETE", "LOAD", confirm_ == 2 ? "SURE?" : "INIT", "SAVE"};
    const Color cols[4] = {pal::red, pal::pill, pal::tan, pal::steel};
    const char keys[4] = {'B', 'A', 'Y', 'X'};
    actionBar(c, labels, cols, keys, st);
}

}  // namespace dp
