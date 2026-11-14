#include "ui/synth_view.h"
#include "ui/app.h"
#include "gfx/font.h"
#include "gfx/icons.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
namespace dp {
namespace {
const int keys[] = {B_LEFT, B_DOWN, B_RIGHT, B_UP, B_Y, B_B, B_A, B_X}, scale[] = {0, 2, 4, 5, 7, 9, 11, 12},
          axes[] = {AX_LX, AX_LY, AX_RX, AX_RY};
const float signs[] = {1, -1, 1, -1};
const char *pages[] = {"OSC A",    "OSC B", "FILTER",    "MIXER", "FILTER ENV", "AMP ENV",
                       "POLY MOD", "LFO",   "WHEEL MOD", "VOICE", "PRESETS"};
const int ids[10][2][4] = {
    {{P5_AWAVE, P5_AFREQ, P5_APW, P5_MIXA}, {P5_SYNC, -1, -1, -1}},
    {{P5_BWAVE, P5_BFREQ, P5_BFINE, P5_BPW}, {P5_BLOW, P5_BKEY, P5_MIXB, -1}},
    {{P5_CUTOFF, P5_RESONANCE, P5_ENVAMT, P5_KEYTRACK}, {-1, -1, -1, -1}},
    {{P5_MIXA, P5_MIXB, P5_NOISE, P5_VINTAGE}, {-1, -1, -1, -1}},
    {{P5_FA, P5_FD, P5_FS, P5_FR}, {-1, -1, -1, -1}},
    {{P5_AA, P5_AD, P5_AS, P5_AR}, {P5_RELEASE, P5_VELOCITY, -1, -1}},
    {{P5_PMENV, P5_PMB, P5_PMFREQ, P5_PMPW}, {P5_PMFILTER, -1, -1, -1}},
    {{P5_LFORATE, P5_LFOWAVE, P5_WHEEL, P5_WMIX}, {P5_LFOSYNC, P5_LFODIV, -1, -1}},
    {{P5_WFREQA, P5_WFREQB, P5_WPWA, P5_WPWB}, {P5_WFILTER, -1, -1, -1}},
    {{P5_UNISON, P5_GLIDE, P5_TUNE, P5_VINTAGE}, {P5_RELEASE, P5_VELOCITY, P5_BEND, P5_A440}}};
const char *labels[10][2][4] = {
    {{"WAVE", "FREQUENCY", "PULSE WIDTH", "LEVEL"}, {"HARD SYNC", "", "", ""}},
    {{"WAVE", "FREQUENCY", "FINE", "PULSE WIDTH"}, {"LO FREQ", "KEYBOARD", "LEVEL", ""}},
    {{"CUTOFF", "RESONANCE", "ENV AMOUNT", "KEYBOARD"}, {"", "", "", ""}},
    {{"OSC A LVL", "OSC B LVL", "WHITE NOISE", "VINTAGE"}, {"", "", "", ""}},
    {{"ATTACK", "DECAY", "SUSTAIN", "RELEASE"}, {"", "", "", ""}},
    {{"ATTACK", "DECAY", "SUSTAIN", "RELEASE"}, {"RELEASE", "VELOCITY", "", ""}},
    {{"FILTER ENV", "OSC B", "FREQ A", "PW A"}, {"FILTER", "", "", ""}},
    {{"RATE", "SHAPES", "MOD WHEEL", "LFO / PINK"}, {"BPM SYNC", "DIVISION", "", ""}},
    {{"FREQ A", "FREQ B", "PW A", "PW B"}, {"FILTER", "", "", ""}},
    {{"UNISON", "GLIDE", "MASTER TUNE", "VINTAGE"}, {"RELEASE", "VELOCITY", "PITCH WHEEL", "A-440"}}};
const char *hints[] = {"L3 = HARD SYNC   OSC B RESETS OSC A",
                       "L3 = LO FREQ / KEYBOARD / LEVEL",
                       "24 DB LOW-PASS   KEYBOARD = FULL TRACK",
                       "WHITE NOISE = AUDIO   PINK NOISE = WHEEL MOD",
                       "FILTER ENVELOPE ALSO FEEDS POLY MOD",
                       "L3 = RELEASE SWITCH / VELOCITY EXTRA",
                       "L3 = FILTER DESTINATION   ROUTES CAN COMBINE",
                       "L3 = BPM SYNC / DIVISION TO 1/128",
                       "L3 = FILTER DESTINATION   ALL FIVE ROUTES CAN COMBINE",
                       "L3 = RELEASE / VELOCITY / PITCH / A-440   R3 = TUNE"};
int layer(int page, bool alt) { return alt && (page == 0 || page == 1 || page == 5 || page >= 6) ? 1 : 0; }
void diagram(Canvas &c, int page, const int16_t *p, const ProphetView &v) {
    const float x = 447, y = 350, w = 172, h = 40;
    if (page == 4 || page == 5) {
        int b = page == 4 ? P5_FA : P5_AA;
        float ta = .05f + p[b] / 32767.f, td = .05f + p[b + 1] / 32767.f, tr = .05f + p[b + 3] / 32767.f,
              st = p[b + 2] / 32767.f, z = w / (ta + td + tr + .3f);
        float xy[] = {x,
                      y + h,
                      x + ta * z,
                      y,
                      x + (ta + td) * z,
                      y + h * (1 - st),
                      x + (ta + td + .3f) * z,
                      y + h * (1 - st),
                      x + w,
                      y + h};
        c.line(x, y + h, x + w, y + h, 1, pal::faint);
        for (int i = 0; i < 4; i++)
            c.line(xy[2 * i], xy[2 * i + 1], xy[2 * i + 2], xy[2 * i + 3], 1.6f, pal::enc[i]);
        for (int i = 1; i < 4; i++)
            c.fillCircle(xy[2 * i], xy[2 * i + 1], 3, pal::enc[i - 1]);
    } else if (page == 7 || page == 8) {
        int bits = p[P5_LFOWAVE] * 8 / 32768;
        float xy[256];
        for (int j = 0; j < 128; j++) {
            float t = j / 127.f, l = 0;
            if (bits & 1)
                l += 2 * t - 1;
            if (bits & 2)
                l += 1 - 4 * std::fabs(t - .5f);
            if (bits & 4)
                l += t < .5f ? 1 : -1;
            xy[j * 2] = x + w * t;
            xy[j * 2 + 1] = y + 20 - 7 * l;
        }
        c.polyline(xy, 128, 1.25f, pal::green);
        c.fillCircle(x + w * v.state.lfoPhase, y + 20 - 7 * v.state.lfo, 2.5f, pal::white);
        char routes[80];
        int depth = p[P5_WHEEL] * 100 / 32767;
        std::snprintf(routes, sizeof(routes), "OSC A:%d OSC B:%d PW A:%d PW B:%d FLT:%d",
                      p[P5_WFREQA] >= 16384 ? depth : 0, p[P5_WFREQB] >= 16384 ? depth : 0,
                      p[P5_WPWA] >= 16384 ? depth : 0, p[P5_WPWB] >= 16384 ? depth : 0,
                      p[P5_WFILTER] >= 16384 ? depth : 0);
        drawText(c, x + w, y + 49, routes, 5.5f, 1, pal::dim, ALIGN_RIGHT);
    } else if (page == 2) {
        float knee = x + 15 + 130 * p[P5_CUTOFF] / 32767.f, r = p[P5_RESONANCE] / 32767.f;
        float xy[] = {x, y + 15, knee - 8, y + 15, knee, y + 15 - 13 * r, knee + 10, y + 25, x + w, y + h};
        c.polyline(xy, 5, 1.5f, pal::silver);
        drawText(c, x + w, y + 48, "FOUR POLES / CEM REV 3", 7, 1, pal::dim, ALIGN_RIGHT);
    } else if (page == 6) {
        drawText(c, x + w, y + 4, "ENV + OSC B", 9, 1, pal::tan, ALIGN_RIGHT);
        char routes[40];
        std::snprintf(routes, sizeof(routes), "A:%s  PW:%s  FLT:%s", p[P5_PMFREQ] >= 16384 ? "ON" : "--",
                      p[P5_PMPW] >= 16384 ? "ON" : "--", p[P5_PMFILTER] >= 16384 ? "ON" : "--");
        drawText(c, x + w, y + 27, routes, 8, 1, pal::silver, ALIGN_RIGHT);
    } else {
        for (int i = 0; i < 5; i++) {
            float a = v.state.amp[i];
            c.fillRect(x + 20 + i * 30, y + h - 36 * a, 12, 36 * a, pal::enc[i % 4]);
        }
        drawText(c, x + w, y + 49, "FIVE INDEPENDENT VOICES", 7, 1, pal::dim, ALIGN_RIGHT);
    }
}
} // namespace
void SynthView::updateProphet(const Buttons &b, const InputState &in, float dt, Track &t) {
    prophetEnsure(t);
    AudioEngine *e = app_->engine();
    if (p5UiPage_ != page_ || p5UiTrack_ != track_) {
        p5UiPage_ = page_;
        p5UiTrack_ = track_;
        for (auto &v : stepP5_)
            v = RelEncoder();
    }
    if (b.pressed(B_START)) {
        app_->togglePlaySong();
    }
    if (page_ == 9 && b.pressed(B_R3) && e) {
        e->send(EC_PROPHET_TUNE, track_);
        app_->toast("OSCILLATORS TUNED");
    }
    if (b.pressed(B_L3)) {
        alt_ = !alt_;
        for (auto &v : stepP5_)
            v = RelEncoder();
    }
    if (b.pressed(B_L1) && t.octave > -3) {
        --t.octave;
        app_->markDirty();
    }
    if (b.pressed(B_R1) && t.octave < 3) {
        ++t.octave;
        app_->markDirty();
    }
    for (int i = 0; i < 8; i++) {
        if (b.pressed(keys[i]) && e) {
            int pitch = 12 * (octave_ + 1) + scale[i];
            if (held_[i] >= 0)
                e->send(EC_NOTE_OFF, track_, held_[i]);
            e->send(EC_NOTE_ON, track_, pitch, 0, .85f);
            held_[i] = pitch;
        }
        if (b.released(keys[i]) && held_[i] >= 0) {
            if (e)
                e->send(EC_NOTE_OFF, track_, held_[i]);
            held_[i] = -1;
        }
    }
    if (page_ < 0 || page_ >= 10) {
        return;
    }
    for (int i = 0; i < 4; i++) {
        int id = ids[page_][layer(page_, alt_)][i];
        if (id < 0)
            continue;
        if (page_ == 7 && !alt_ && i == 0 && t.prophet[P5_LFOSYNC] >= 16384)
            id = P5_LFODIV;
        int count = prophetChoices(id), word = t.prophet[id], next = word;
        if (id == P5_BFREQ && t.prophet[P5_BLOW] >= 16384)
            count = 0;
        float axis = in.axis[axes[i]] * signs[i];
        if (count) {
            int d = stepP5_[i].steps(axis, dt, 5);
            if (d)
                next = prophetChoice(std::clamp(word * count / 32768 + d, 0, count - 1), count);
        } else
            next = std::clamp(word + (int)std::lround(RelEncoder::delta(axis, dt, .7f) * 32767), 0, 32767);
        if (next != word) {
            t.prophet[id] = (int16_t)next;
            app_->markDirty();
        }
    }
}
void SynthView::renderProphet(Canvas &c, const Track &t) {
    int16_t defaults[PROPHET_PARAMS];
    if (!t.prophetSet)
        prophetPreset(0, defaults);
    const int16_t *p = t.prophetSet ? t.prophet : defaults;
    auto *e = app_->engine();
    if (e)
        e->prophetView(track_, p5View_);
    p5_.draw(c, p, p5View_, dt_);
    bool playing = e && e->isPlaying();
    int tick = playing ? e->playTick() : 0;
    c.fillRect(0, 0, 640, 44, pal::bg);
    screenHeader(c, ICON_KEYS, pal::green, "PROPHET-5", t.name, pal::track[t.color % pal::TRACK_COLORS],
                 tick / TICKS_PER_BAR + 1, (tick % TICKS_PER_BAR) / PPQ + 1, playing, app_->project().bpm);
    c.fillRect(0, 334, 640, 146, pal::bg);
    int page = std::clamp(page_, 0, 9), l = layer(page, alt_);
    drawText(c, 20, 334, t.preset[0] ? t.preset : "PROPHET BRASS", 8, 1, pal::tan);
    drawText(c, 20, 350, pages[page], 13, 1.5f, pal::white);
    for (int i = 0; i < 11; i++) {
        if (i == page)
            c.fillCircle(24 + i * 12, 376, 2.6f, pal::white);
        else
            c.circle(24 + i * 12, 376, 2.2f, 1, pal::dim);
    }
    drawText(c, 164, 372, "SELECT = NEXT PAGE", 7, 1, pal::dim);
    drawText(c, 20, 405, hints[page], 6.5f, 1, l ? pal::blue : pal::dim);
    diagram(c, page, p, p5View_);
    if (t.octave) {
        char text[20];
        std::snprintf(text, sizeof(text), "OCT %+d", t.octave);
        drawText(c, 410, 354, text, 8, 1, pal::white, ALIGN_RIGHT);
    }
    char text[4][24];
    const char *values[4], *names[4];
    float positions[4];
    for (int i = 0; i < 4; i++) {
        int id = ids[page][l][i];
        names[i] = labels[page][l][i];
        if (page == 7 && !l && i == 0 && p[P5_LFOSYNC] >= 16384) {
            id = P5_LFODIV;
            names[i] = "SYNC";
        }
        if (id < 0) {
            text[i][0] = 0;
            positions[i] = -1;
        } else {
            prophetValue(id, p[id], text[i], sizeof(text[i]));
            int n = prophetChoices(id);
            if (id == P5_BFREQ && p[P5_BLOW] >= 16384) {
                n = 0;
                std::snprintf(text[i], sizeof(text[i]), "%.2f HZ", .4f * std::pow(75.f, p[id] / 32767.f));
            }
            positions[i] = n > 1 ? (p[id] * n / 32768) / (float)(n - 1) : p[id] / 32767.f;
        }
        values[i] = text[i];
    }
    encoderBar(c, names, values, positions);
}
} // namespace dp
