#include "synth/tone_synth.h"
#include <math.h>

namespace dp {

void ToneSynth::init(float sr) {
    sr_ = sr;
    smooth_ = 1.f - expf(-1.f / (0.005f * sr));
    att_ = 1.f / (0.005f * sr);
    rel_ = 1.f / (0.120f * sr);
}

void ToneSynth::process(float* out, int frames) {
    ToneCmd c;
    while (q_.pop(c)) {
        switch (c.p) {
        case TP_FREQ: freqT_ = c.v; break;
        case TP_GATE: gate_ = c.v; break;
        case TP_VOL: volT_ = c.v; break;
        case TP_PAN: panT_ = c.v; break;
        case TP_DRIVE: driveT_ = c.v; break;
        }
    }
    uint32_t w = sw_.load(std::memory_order_relaxed);
    const float twoPi = 6.2831853f;
    for (int i = 0; i < frames; i++) {
        freq_ += (freqT_ - freq_) * smooth_;
        vol_ += (volT_ - vol_) * smooth_;
        pan_ += (panT_ - pan_) * smooth_;
        drive_ += (driveT_ - drive_) * smooth_;
        if (gate_ > 0.5f) { env_ += att_; if (env_ > 1.f) env_ = 1.f; }
        else { env_ -= rel_; if (env_ < 0.f) env_ = 0.f; }

        ph_ += freq_ / sr_;
        if (ph_ >= 1.0) ph_ -= 1.0;
        float s = sinf(twoPi * (float)ph_);
        float d = 1.f + drive_ * 9.f;
        s = tanhf(s * d) / tanhf(d);
        s *= env_ * vol_;

        float a = (pan_ + 1.f) * 0.39269908f;  // 0..pi/2
        out[i * 2] = s * cosf(a) * 1.41421356f * 0.7071f;
        out[i * 2 + 1] = s * sinf(a) * 1.41421356f * 0.7071f;
        scope_[w & (SCOPE_N - 1)] = s;
        w++;
    }
    sw_.store(w, std::memory_order_release);
}

void ToneSynth::copyScope(float* dst, int n) const {
    uint32_t w = sw_.load(std::memory_order_acquire);
    for (int i = 0; i < n; i++) dst[i] = scope_[(w - n + i) & (SCOPE_N - 1)];
}

}  // namespace dp
