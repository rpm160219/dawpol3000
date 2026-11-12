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
    if (b.pressed(B_SELECT) && !b.down(B_MENU)) page_ = (page_ + 1) % PAGES;
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
        float ax = in.axis[axisOf[i]] * sign[i];
        float d = RelEncoder::delta(ax, dt, 0.7f);
        auto clamp01 = [](float x) { return x < 0 ? 0.f : (x > 1 ? 1.f : x); };
        bool changed = false;
        switch (page_) {
        case 0: {  // SOUND
            if (d == 0.f) break;
            int v = t.knob[i] + (int)lrintf(d * 32767.f);
            v = v < 0 ? 0 : (v > 32767 ? 32767 : v);
            if (v != t.knob[i]) { t.knob[i] = (int16_t)v; changed = true; }
            break;
        }
        case 1: {  // AMP ENV
            if (d == 0.f) break;
            float k = i == 2 ? t.env[2] : envKnob(t.env[i]);
            k = clamp01(k + d);
            t.env[i] = i == 2 ? k : (k <= 0 ? 0.f : envTime(k));
            changed = true;
            break;
        }
        case 2: {  // FILTER ENV: A D S AMOUNT
            if (d == 0.f) break;
            if (i == 3) { float a = t.fenv[3] + d * 2.f; t.fenv[3] = a < -1 ? -1 : (a > 1 ? 1 : a); }
            else if (i == 2) t.fenv[2] = clamp01(t.fenv[2] + d);
            else { float k = clamp01(envKnob(t.fenv[i]) + d); t.fenv[i] = k <= 0 ? 0.f : envTime(k); }
            changed = true;
            break;
        }
        default: {  // LFO 1 / 2: RATE DEPTH SHAPE DEST
            Lfo& L = t.lfo[page_ - 3];
            if (i == 0 && d != 0.f) {
                float k = clamp01(log10f(L.rate / 0.05f) / log10f(400.f) + d);  // 0.05 .. 20 Hz
                L.rate = 0.05f * powf(400.f, k);
                changed = true;
            } else if (i == 1 && d != 0.f) { L.depth = clamp01(L.depth + d); changed = true; }
            else if (i == 2) {
                int st = stepShape_[page_ - 3].steps(ax, dt, 4.f);
                if (st) { L.shape = (uint8_t)((L.shape + st % LFO_SHAPES + LFO_SHAPES) % LFO_SHAPES); changed = true; }
            } else if (i == 3) {
                int st = stepDest_[page_ - 3].steps(ax, dt, 4.f);
                if (st) { L.dest = (uint8_t)((L.dest + st % LFO_DESTS + LFO_DESTS) % LFO_DESTS); changed = true; }
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
    char b[64];

    if (t.engine == ENG_WAVETABLE) {
        // ekran pokazuje galki po modulacji (LFO, obwiednia filtra), gdy cos gra
        int16_t shown[4] = {t.knob[0], t.knob[1], t.knob[2], t.knob[3]};
        if (e && e->wtActive(track_) > 0)
            for (int i = 0; i < 4; i++) shown[i] = e->wtModKnob(track_, i);
        wt_.draw(c, shown, e ? e->trackLevel(track_) : 0.f, e ? e->wtDrift(track_) : 0, e ? e->wtActive(track_) : 0, dt_);
    } else {
        drawText(c, 320, 170, "TEST SAW", 22, 1.8f, rgb(90, 90, 90), ALIGN_CENTER);
        drawText(c, 320, 210, "SIMPLE SAW FOR TESTS - NO SOUND PARAMETERS", 9, 1.2f, pal::dim, ALIGN_CENTER);
    }
    // naglowek na wierzchu (ekran silnika zaczyna sie pod nim)
    c.fillRect(0, 0, 640, 44, pal::bg);
    screenHeader(c, ICON_KEYS, tc, engineName(t.engine), t.name, tc, tick / TICKS_PER_BAR + 1,
                 (tick % TICKS_PER_BAR) / PPQ + 1, playing, p.bpm);

    // lewy dol: oktawa klawiszy i strona galek
    static const char* kPage[PAGES] = {"SOUND", "AMP ENV", "FILTER ENV", "LFO 1", "LFO 2"};
    drawText(c, 20, 340, kPage[page_], 13, 1.5f, pal::white);
    for (int i = 0; i < PAGES; i++) {
        float x = 24 + i * 12;
        if (i == page_) c.fillCircle(x, 364, 2.6f, pal::white);
        else c.circle(x, 364, 2.2f, 1.f, rgb(90, 90, 90));
    }
    drawText(c, 86, 360, "SELECT = NEXT PAGE", 7, 1.f, rgb(80, 80, 80));
    int lo = 12 * (octave_ + 1);
    char nn[8];
    static const char* NN[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    snprintf(nn, sizeof nn, "%s%d", NN[lo % 12], lo / 12 - 1);
    snprintf(b, sizeof b, "KEYS: D-PAD + ABXY = %s MAJOR SCALE   L1/R1 = OCTAVE %d", nn, octave_);
    drawText(c, 20, 384, b, 7, 1.f, pal::dim);

    // prawy dol: grafika strony (obwiednie / LFO)
    if (page_ == 2) {
        float fe[4] = {t.fenv[0], t.fenv[1], t.fenv[2], t.env[3]};
        drawEnvelope(c, fe, true, 400, 342, 200, 46);
        snprintf(b, sizeof b, "CUTOFF %+d%%", (int)lrintf(t.fenv[3] * 100));
        drawText(c, 600, 394, b, 8, 1.1f, t.fenv[3] == 0 ? pal::dim : pal::white, ALIGN_RIGHT);
    } else if (page_ >= 3) {
        const Lfo& L = t.lfo[page_ - 3];
        float pts[2 * 81];
        uint32_t seed = 7;
        float rv = 0;
        for (int i = 0; i <= 80; i++) {
            float p = fmodf(i / 80.f * 2.f, 1.f), v;  // dwa cykle
            switch (L.shape) {
            case LFO_SINE: v = sinf(6.2831853f * p); break;
            case LFO_TRI: v = p < 0.5f ? 4.f * p - 1.f : 3.f - 4.f * p; break;
            case LFO_SAW: v = 1.f - 2.f * p; break;
            case LFO_SQUARE: v = p < 0.5f ? 1.f : -1.f; break;
            default:
                if (i % 10 == 0) { seed = seed * 1664525u + 1013904223u; rv = (int32_t)(seed >> 8) / 8388608.f - 1.f; }
                v = rv;
            }
            pts[i * 2] = 400 + i * 2.5f;
            pts[i * 2 + 1] = 366 - v * 20.f * (0.25f + 0.75f * L.depth);
        }
        c.line(400, 366, 600, 366, 1.f, pal::faint);
        c.polyline(pts, 81, 1.6f, L.depth > 0 ? pal::green : rgb(90, 90, 90));
        snprintf(b, sizeof b, "-> %s", lfoDestName(L.dest));
        drawText(c, 600, 394, b, 9, 1.2f, L.depth > 0 ? pal::white : pal::dim, ALIGN_RIGHT);
    } else {
        drawEnvelope(c, t.env, page_ == 1, 400, 342, 200, 46);
        drawText(c, 600, 394, "AMP ENV", 8, 1.1f, page_ == 1 ? pal::white : pal::dim, ALIGN_RIGHT);
    }

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
    } else if (page_ <= 2) {
        const float* ev = page_ == 2 ? t.fenv : t.env;
        static const char* LA[4] = {"ATTACK", "DECAY", "SUSTAIN", "RELEASE"};
        static const char* LF[4] = {"F.ATTACK", "F.DECAY", "F.SUSTAIN", "AMOUNT"};
        for (int i = 0; i < 4; i++) {
            labels[i] = page_ == 2 ? LF[i] : LA[i];
            if (i == 2) { snprintf(v[i], 16, "%d", (int)lrintf(ev[2] * 100)); pos[i] = ev[2]; }
            else if (i == 3 && page_ == 2) { snprintf(v[i], 16, "%+d", (int)lrintf(ev[3] * 100)); pos[i] = 0.5f + ev[3] * 0.5f; }
            else {
                float s = ev[i];
                if (s < 1.f) snprintf(v[i], 16, "%.0fMS", s * 1000.f);
                else snprintf(v[i], 16, "%.1fS", s);
                pos[i] = envKnob(s);
            }
        }
    } else {
        const Lfo& L = t.lfo[page_ - 3];
        static const char* LL[4] = {"RATE", "DEPTH", "SHAPE", "DEST"};
        for (int i = 0; i < 4; i++) labels[i] = LL[i];
        if (L.rate < 1.f) snprintf(v[0], 16, "%.2fHZ", L.rate);
        else snprintf(v[0], 16, "%.1fHZ", L.rate);
        snprintf(v[1], 16, "%d", (int)lrintf(L.depth * 100));
        snprintf(v[2], 16, "%s", lfoShapeName(L.shape));
        snprintf(v[3], 16, "%s", lfoDestName(L.dest));
        pos[0] = log10f(L.rate / 0.05f) / log10f(400.f);
        pos[1] = L.depth;
        pos[2] = L.shape / (float)(LFO_SHAPES - 1);
        pos[3] = L.dest / (float)(LFO_DESTS - 1);
    }
    const char* vals[4] = {v[0], v[1], v[2], v[3]};
    encoderBar(c, labels, vals, pos);
}

}  // namespace dp
