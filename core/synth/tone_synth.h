// Testowy syntezator: jeden ton sinus z drive, obwiednia, panorama. Etap 0.
#pragma once
#include <atomic>
#include <stdint.h>
#include "util/spsc.h"

namespace dp {

enum ToneParam : uint8_t { TP_FREQ, TP_GATE, TP_VOL, TP_PAN, TP_DRIVE, TP_COUNT };
struct ToneCmd { uint8_t p; float v; };

class ToneSynth {
public:
    static const int SCOPE_N = 2048;
    void init(float sampleRate);
    // Watek UI:
    bool send(uint8_t p, float v) { return q_.push({p, v}); }
    void copyScope(float* dst, int n) const;
    // Watek audio (bez alokacji i lockow). out = stereo przeplecione.
    void process(float* out, int frames);

private:
    Spsc<ToneCmd, 256> q_;
    float sr_ = 48000.f, smooth_ = 0.f, att_ = 0.f, rel_ = 0.f;
    float freqT_ = 440.f, freq_ = 440.f, gate_ = 0.f, env_ = 0.f;
    float volT_ = 0.6f, vol_ = 0.6f, panT_ = 0.f, pan_ = 0.f, driveT_ = 0.f, drive_ = 0.f;
    double ph_ = 0.0;
    float scope_[SCOPE_N] = {};
    std::atomic<uint32_t> sw_{0};
};

}  // namespace dp
