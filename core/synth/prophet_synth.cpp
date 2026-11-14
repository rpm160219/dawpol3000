#include "synth/prophet_synth.h"
#include <cmath>
#include <cstring>

namespace dp {
void ProphetSynth::init(float rate) {
    rate_ = rate;
    if (!core_)
        core_ = prophetCreate();
    prophetDefault(params_);
    for (int &key : map_)
        key = -1;
}
void ProphetSynth::configure(const int16_t *params, int octave) {
    std::memcpy(params_, params, sizeof(params_));
    octave_ = octave < -3 ? -3 : octave > 3 ? 3 : octave;
    prophetConfigure(core_, params_);
}
void ProphetSynth::noteOn(int pitch, float velocity) {
    if (!core_ || pitch < 0 || pitch > 127)
        return;
    if (velocity <= 0) {
        noteOff(pitch);
        return;
    }
    if (map_[pitch] >= 0)
        noteOff(pitch);
    int note = pitch + 12 * octave_;
    if (note < 0 || note > 127)
        return;
    map_[pitch] = note;
    ++references_[note];
    prophetKeyDown(core_, note, (int)std::lround(127 * std::fmin(velocity, 1.f)), params_);
}
void ProphetSynth::noteOff(int pitch) {
    if (pitch < 0 || pitch > 127 || map_[pitch] < 0)
        return;
    const int note = map_[pitch];
    map_[pitch] = -1;
    if (--references_[note] == 0)
        prophetKeyUp(core_, note, params_);
}
void ProphetSynth::allOff() {
    for (int note = 0; note < 128; ++note) {
        if (references_[note])
            prophetKeyUp(core_, note, params_);
        references_[note] = 0;
        map_[note] = -1;
    }
}
void ProphetSynth::reset() {
    allOff();
    prophetPanic(core_);
    active_.store(0, std::memory_order_relaxed);
    std::memset(scope_, 0, sizeof(scope_));
    view_.back() = ProphetView();
    view_.publish();
}
void ProphetSynth::render(float *stereo, int frames) {
    if (!core_)
        return;
    float peak = 0;
    for (int pos = 0; pos < frames;) {
        const int n = frames - pos < 128 ? frames - pos : 128;
        float mono[128];
        prophetRender(core_, mono, n, params_, rate_, tempo_);
        for (int i = 0; i < n; ++i) {
            float x = mono[i];
            stereo[2 * (pos + i)] += x;
            stereo[2 * (pos + i) + 1] += x;
            peak = std::fmax(peak, std::fabs(x));
            if (++scopeClock_ == 4) {
                scopeClock_ = 0;
                scope_[scopePos_] = x;
                scopePos_ = (scopePos_ + 1) % 128;
            }
        }
        pos += n;
    }
    ProphetView &v = view_.back();
    prophetGetState(core_, &v.state);
    v.peak = peak;
    for (unsigned i = 0; i < 128; ++i)
        v.scope[i] = scope_[(scopePos_ + i) % 128];
    bool sounding = v.state.held || params_[P5_A440] >= 16384;
    for (float amp : v.state.amp)
        sounding = sounding || amp > 0.00001f;
    active_.store(sounding, std::memory_order_relaxed);
    view_.publish();
}
} // namespace dp
