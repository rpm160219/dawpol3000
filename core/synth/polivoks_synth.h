// Independent Polivoks host adapter. Never passes through WavetableSynth DSP.
#pragma once
#include "synth/note_sink.h"
#include "synth/polivoks_dsp.h"
#include "util/triple_buffer.h"
#include <atomic>

namespace dp {
struct PolivoksView {
    PolivoksState state = {};
    float peak = 0;
    float scope[128] = {};
};
class PolivoksSynth : public NoteSink {
public:
    ~PolivoksSynth() override { polivoksDestroy(core_); }
    void init(float rate);
    void configure(const int16_t* params, int octave);
    void noteOn(int pitch, float velocity) override;
    void noteOff(int pitch) override;
    void allOff() override;
    void reset();
    void setTransport(bool playing,double beat,float bpm) { playing_=playing;beat_=beat;tempo_=bpm; }
    void restartLfo() { polivoksResetLfoClock(core_); }
    void render(float* stereo, int frames);
    int active() const { return active_.load(std::memory_order_relaxed); }
    bool view(PolivoksView& out) { if (!view_.consume()) return false; out=view_.front(); return true; }
private:
    PolivoksCore* core_ = nullptr;
    float rate_ = 48000;
    int16_t params_[POLIVOKS_NUM_PARAMS] = {};
    int octave_ = 0, map_[128] = {}, references_[128] = {};
    float scope_[128] = {};
    unsigned scopePos_ = 0, scopeClock_ = 0;
    bool live_ = false, playing_ = false;
    double beat_ = 0; float tempo_ = 120;
    std::atomic<int> active_{0};
    TripleBuffer<PolivoksView> view_;
};
} // namespace dp
