#include "ui/synth_view.h"
#include "gfx/font.h"
#include "gfx/icons.h"
#include "ui/app.h"
#include "ui/encoder.h"
#include "ui/widgets.h"
#include <math.h>
#include <stdio.h>

namespace dp {

static const int kNoteBtn[8] = {B_LEFT, B_DOWN, B_RIGHT, B_UP, B_Y, B_B, B_A, B_X};
static const int kMajor[8] = {0, 2, 4, 5, 7, 9, 11, 12};
static const char* kWaveNames[8] = {"SINE", "WARM", "TRI", "SAW", "SQUARE", "PULSE", "VOCAL", "DIGITAL"};

// Czasy obwiedni: galka 0..1 -> 1 ms .. 10 s (wykladniczo)
static float envTime(float v) { return 0.001f * powf(10000.f, v); }
static float envKnob(float t) { return t <= 0.001f ? 0.f : log10f(t / 0.001f) / 4.f; }

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

    if (b.pressed(B_L1) && octave_ > 1) octave_--;
    if (b.pressed(B_R1) && octave_ < 7) octave_++;
    if (b.pressed(B_SELECT) && !b.down(B_MENU)) page_ = 1 - page_;
    if (b.pressed(B_START)) app_->togglePlaySong();

    // granie nut z przyciskow
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
        float d = RelEncoder::delta(in.axis[axisOf[i]] * sign[i], dt, 0.7f);
        if (d == 0.f) continue;
        if (page_ == 0) {
            int v = t.knob[i] + (int)lrintf(d * 32767.f);
            v = v < 0 ? 0 : (v > 32767 ? 32767 : v);
            if (v != t.knob[i]) { t.knob[i] = (int16_t)v; app_->markDirty(); }
        } else {
            float k = i == 2 ? t.env[2] : envKnob(t.env[i]);
            k += d;
            k = k < 0 ? 0 : (k > 1 ? 1 : k);
            t.env[i] = i == 2 ? k : (k <= 0 ? 0.f : envTime(k));
            app_->markDirty();
        }
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
    char b[64];

    if (t.engine == ENG_WAVETABLE) {
        wt_.draw(c, t.knob, e ? e->trackLevel(track_) : 0.f, e ? e->wtDrift(track_) : 0, e ? e->wtActive(track_) : 0, dt_);
    } else {
        drawText(c, 320, 170, "TEST SAW", 22, 1.8f, rgb(90, 90, 90), ALIGN_CENTER);
        drawText(c, 320, 210, "SIMPLE SAW FOR TESTS - NO SOUND PARAMETERS", 9, 1.2f, pal::dim, ALIGN_CENTER);
    }
    // naglowek na wierzchu (ekran silnika zaczyna sie pod nim)
    c.fillRect(0, 0, 640, 44, pal::bg);
    screenHeader(c, ICON_KEYS, tc, engineName(t.engine), t.name, tc, tick / TICKS_PER_BAR + 1,
                 (tick % TICKS_PER_BAR) / PPQ + 1, playing, p.bpm);

    // obwiednia + oktawa
    drawEnvelope(c, t.env, page_ == 1, 400, 352, 200, 50);
    drawText(c, 400, 336, "ENVELOPE", 8, 1.1f, page_ == 1 ? pal::white : pal::dim);
    snprintf(b, sizeof b, "OCTAVE %d", octave_);
    drawText(c, 20, 392, b, 9, 1.2f, pal::dim);
    drawText(c, 20, 406, page_ == 0 ? "SELECT = ENVELOPE PAGE" : "SELECT = SOUND PAGE", 7, 1.f, rgb(70, 70, 70));

    // galki
    char v[4][16];
    float pos[4];
    const char* labels[4];
    if (page_ == 0 && t.engine == ENG_WAVETABLE) {
        int32_t wp = t.knob[0] * 7;
        int wi = wp >> 15, wf = (wp & 0x7FFF) * 100 / 32768;
        if (wf < 6 || wi == 7) snprintf(v[0], 16, "%s", kWaveNames[wi]);
        else if (wf > 94) snprintf(v[0], 16, "%s", kWaveNames[wi + 1]);
        else snprintf(v[0], 16, "%.3s>%.3s", kWaveNames[wi], kWaveNames[wi + 1]);
        snprintf(v[1], 16, "%.1fC", 14.f * t.knob[1] / 32767.f);
        float hz = 30.f * powf(16000.f / 30.f, t.knob[2] / 32767.f);
        if (hz >= 1000) snprintf(v[2], 16, "%.1fK", hz / 1000.f);
        else snprintf(v[2], 16, "%.0f", hz);
        snprintf(v[3], 16, "%d", t.knob[3] * 100 / 32767);
        static const char* L[4] = {"WAVE", "DETUNE", "CUTOFF", "RESONANCE"};
        for (int i = 0; i < 4; i++) { labels[i] = L[i]; pos[i] = t.knob[i] / 32767.f; }
    } else {
        static const char* L[4] = {"ATTACK", "DECAY", "SUSTAIN", "RELEASE"};
        for (int i = 0; i < 4; i++) {
            labels[i] = L[i];
            if (i == 2) { snprintf(v[i], 16, "%d", (int)lrintf(t.env[2] * 100)); pos[i] = t.env[2]; }
            else {
                float s = t.env[i];
                if (s < 1.f) snprintf(v[i], 16, "%.0fMS", s * 1000.f);
                else snprintf(v[i], 16, "%.1fS", s);
                pos[i] = envKnob(s);
            }
        }
    }
    const char* vals[4] = {v[0], v[1], v[2], v[3]};
    encoderBar(c, labels, vals, pos);
}

}  // namespace dp
