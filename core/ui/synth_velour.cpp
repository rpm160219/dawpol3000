// SYNTH: strony silnika VELOUR (string ensemble). Select = nastepna strona:
//   STRINGS  CONTRABASS / CELLO / VIOLA / VIOLIN   (poziom kazdego rejestru, 0 = wylaczony, 99 = oryginal)
//   BRASS    TRUMPET / HORN / BASS VOL / VOLUME
//   SWELL    CRESCENDO / SUSTAIN / TUNE            (obwiednia kazdego klawisza jak w oryginale)
//   ENSEMBLE TREMOLO RATE / TREMOLO DEPTH / CHORUS RATE / CHORUS DEPTH; L3: ENSEMBLE (OFF/ON) / TONE / WIDTH
//   PHASER   PHASER (OFF/ON) / RATE / COLOR
//   VOICING  LOWPASS / HIGHPASS / SHELF / FORMANT; L3: SHAPER
//   PRESETS
// L1 / R1 = OCTAVE, D-pad + ABXY = odsluch, Start = play.
#include "ui/synth_view.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "audio/engine.h"
#include "gfx/font.h"
#include "gfx/icons.h"
#include "ui/app.h"

namespace dp {

namespace {
const int kBtn[8] = {B_LEFT, B_DOWN, B_RIGHT, B_UP, B_Y, B_B, B_A, B_X};
const int kMaj[8] = {0, 2, 4, 5, 7, 9, 11, 12};
// strona -> parametry galek (L3 = druga warstwa); -1 = brak
const int kPar[6][2][4] = {
    {{VL_CONTRABASS, VL_CELLO, VL_VIOLA, VL_VIOLIN}, {VL_CONTRABASS, VL_CELLO, VL_VIOLA, VL_VIOLIN}},
    {{VL_TRUMPET, VL_HORN, VL_BASS_VOL, VL_VOLUME}, {VL_TRUMPET, VL_HORN, VL_BASS_VOL, VL_VOLUME}},
    {{VL_CRESCENDO, VL_SUSTAIN, VL_TUNE, -1}, {VL_CRESCENDO, VL_SUSTAIN, VL_TUNE, -1}},
    {{VL_TREM_RATE, VL_TREM_DEPTH, VL_CHORUS_RATE, VL_CHORUS_DEPTH}, {VL_ENSEMBLE, VL_ENS_TONE, VL_ENS_WIDTH, -1}},
    {{VL_PHASER, VL_PHASER_RATE, VL_PHASER_COLOR, -1}, {VL_PHASER, VL_PHASER_RATE, VL_PHASER_COLOR, -1}},
    {{VL_TONE_LP, VL_TONE_HP, VL_TONE_SHELF, VL_FORMANT}, {VL_SHAPER, -1, -1, -1}},
};
const char* const kPageName[6] = {"STRINGS", "BRASS", "SWELL", "ENSEMBLE", "PHASER", "VOICING"};
bool isSwitch(int p) { return p == VL_ENSEMBLE || p == VL_PHASER; }
const char* parName(int p) {
    static const char* const N[VELOUR_PARAMS] = {"CONTRABASS", "CELLO", "VIOLA", "VIOLIN", "TRUMPET", "HORN", "BASS VOL", "CRESCENDO",
                                                 "SUSTAIN", "VOLUME", "TUNE", "ENSEMBLE", "TREM RATE", "TREM DEPTH", "CHOR RATE", "CHOR DEPTH",
                                                 "TONE", "WIDTH", "PHASER", "RATE", "COLOR", "LOWPASS", "HIGHPASS", "SHELF", "FORMANT", "SHAPER"};
    return p >= 0 && p < VELOUR_PARAMS ? N[p] : "";
}
// wartosc jak w oryginale (getParameterDisplay)
void parValue(int p, int16_t w, char* o, int n) {
    const float v = velourWordToFloat(w);
    auto tm = [](float t, float lo, float hi) { return lo * powf(hi / lo, t); };
    switch (p) {
    case VL_CONTRABASS: case VL_CELLO: case VL_VIOLA: case VL_VIOLIN: case VL_TRUMPET: case VL_HORN:
        if (v <= 0.f) snprintf(o, n, "OFF"); else snprintf(o, n, "%d", (int)(v * 99.f + .5f)); break;
    case VL_ENSEMBLE: case VL_PHASER: snprintf(o, n, v > 0.f ? "ON" : "OFF"); break;
    case VL_CRESCENDO: { const float s = tm(v, .005f, 1.5f); if (s < 1.f) snprintf(o, n, "%.0f MS", s * 1000.f); else snprintf(o, n, "%.2f S", s); } break;
    case VL_SUSTAIN: { const float s = tm(v, .02f, 4.f); if (s < 1.f) snprintf(o, n, "%.0f MS", s * 1000.f); else snprintf(o, n, "%.2f S", s); } break;
    case VL_TREM_RATE: snprintf(o, n, "%.2f HZ", 3.f + 6.f * v); break;
    case VL_CHORUS_RATE: snprintf(o, n, "%.2f HZ", .3f + .6f * v); break;
    case VL_PHASER_RATE: snprintf(o, n, "%.2f HZ", tm(v, .05f, 8.f)); break;
    case VL_TUNE: snprintf(o, n, "%+.2f", v * 2.f - 1.f); break;
    case VL_ENS_TONE: snprintf(o, n, "%.0f HZ", 5883.f * powf(2.f, (v - .5f) * 2.f)); break;
    case VL_TONE_LP: case VL_TONE_HP: case VL_TONE_SHELF: case VL_FORMANT: snprintf(o, n, "%+.1f ST", -20.f + 80.f * v); break;
    default: snprintf(o, n, "%d", (int)(v * 100.f + .5f)); break;
    }
}
}  // namespace

void SynthView::updateVelour(const Buttons& b, const InputState& in, float dt, Track& t) {
    AudioEngine* e = app_->engine();
    if (!t.velourSet) { velourEnsure(t); app_->markDirty(); }
    if (b.pressed(B_START)) app_->togglePlaySong();
    if (b.pressed(B_L3)) alt_ = !alt_;
    if (b.pressed(B_L1) && t.octave > -3) { t.octave--; app_->markDirty(); }
    if (b.pressed(B_R1) && t.octave < 3) { t.octave++; app_->markDirty(); }
    for (int i = 0; i < 8; i++) {  // odsluch: skala C-dur od C4
        if (b.pressed(kBtn[i]) && e) {
            const int pitch = 12 * (octave_ + 1) + kMaj[i];
            if (held_[i] >= 0) e->send(EC_NOTE_OFF, track_, held_[i]);
            e->send(EC_NOTE_ON, track_, pitch, 0, 0.8f);
            held_[i] = pitch;
        }
        if (b.released(kBtn[i]) && held_[i] >= 0) {
            if (e) e->send(EC_NOTE_OFF, track_, held_[i]);
            held_[i] = -1;
        }
    }
    if (page_ > 5) return;
    static const int axisOf[4] = {AX_LX, AX_LY, AX_RX, AX_RY};
    static const float sign[4] = {1.f, -1.f, 1.f, -1.f};
    bool ch = false;
    for (int i = 0; i < 4; i++) {
        const int p = kPar[page_][alt_ ? 1 : 0][i];
        if (p < 0) continue;
        if (isSwitch(p)) {  // OFF / ON krokowo
            const int st = stepVl_[i].steps(in.axis[axisOf[i]] * sign[i], dt, 4.f);
            if (st) { t.velour[p] = st > 0 ? 32767 : 0; ch = true; }
            continue;
        }
        const float d = RelEncoder::delta(in.axis[axisOf[i]] * sign[i], dt, .7f);
        if (d == 0.f) continue;
        int x = t.velour[p] + (int)lrintf(d * 32767.f);
        t.velour[p] = (int16_t)(x < 0 ? 0 : (x > 32767 ? 32767 : x));
        ch = true;
    }
    if (ch) app_->markDirty();
}

void SynthView::renderVelour(Canvas& c, const Track& t) {
    Project& p = app_->project();
    AudioEngine* e = app_->engine();
    const Color tc = pal::track[t.color % pal::TRACK_COLORS];
    const bool playing = e && e->isPlaying() && e->playMode() == Sequencer::SONG;
    const int tick = playing ? e->playTick() : 0;
    int16_t w[VELOUR_PARAMS];
    if (t.velourSet) memcpy(w, t.velour, sizeof w); else velourProgramWords(2, w);
    vl_.draw(c, w, e ? e->velourCore(track_) : nullptr, dt_);
    c.fillRect(0, 0, 640, 44, pal::bg);
    screenHeader(c, ICON_KEYS, tc, "VELOUR", t.name, tc, tick / TICKS_PER_BAR + 1, (tick % TICKS_PER_BAR) / PPQ + 1, playing, p.bpm);
    c.fillRect(0, 334, 640, 146, pal::bg);
    drawText(c, 20, 342, kPageName[page_ < 6 ? page_ : 0], 13, 1.5f, pal::white);
    const int np = pageCount(12);
    for (int i = 0; i < np; i++) {
        const float x = 24 + i * 12;
        if (i == page_) c.fillCircle(x, 366, 2.6f, pal::white);
        else c.circle(x, 366, 2.2f, 1.f, rgb(90, 90, 90));
    }
    drawText(c, 24 + np * 12 + 4, 362, "SELECT = NEXT PAGE", 7, 1.f, rgb(80, 80, 80));
    char b[24];
    if (t.octave) { snprintf(b, sizeof b, "OCT %+d", t.octave); drawText(c, 380, 342, b, 9, 1.2f, pal::white, ALIGN_RIGHT); }
    if (page_ == 3) drawText(c, 20, 384, alt_ ? "L3 = TREMOLO / CHORUS" : "L3 = ENSEMBLE / TONE / WIDTH", 7, 1.f, alt_ ? pal::blue : rgb(80, 80, 80));
    if (page_ == 5) drawText(c, 20, 384, alt_ ? "L3 = FILTERS / FORMANT" : "L3 = SHAPER", 7, 1.f, alt_ ? pal::blue : rgb(80, 80, 80));
    if (t.preset[0]) drawText(c, 600, 342, t.preset, 9, 1.2f, rgb(150, 150, 150), ALIGN_RIGHT);
    const char* labels[4];
    char v[4][24];
    const char* vals[4];
    float pos[4];
    for (int i = 0; i < 4; i++) {
        const int pr = kPar[page_ < 6 ? page_ : 0][alt_ ? 1 : 0][i];
        labels[i] = parName(pr);
        if (pr >= 0) parValue(pr, w[pr], v[i], sizeof v[i]); else v[i][0] = 0;
        vals[i] = v[i];
        pos[i] = pr >= 0 ? velourWordToFloat(w[pr]) : 0.f;
    }
    encoderBar(c, labels, vals, pos);
}

}  // namespace dp
