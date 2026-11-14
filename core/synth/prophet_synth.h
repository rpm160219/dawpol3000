// Independent Prophet host adapter. Never passes through WavetableSynth DSP.
#pragma once
#include "synth/note_sink.h"
#include "synth/prophet_dsp.h"
#include "util/triple_buffer.h"
#include <atomic>

namespace dp {
struct ProphetView {
    ProphetState state = {};
    float peak = 0;
    float scope[128] = {};
};
class ProphetSynth : public NoteSink {
  public:
    ~ProphetSynth() override { prophetDestroy(core_); }
    void init(float rate);
    void seed(uint32_t value) { prophetSeed(core_, value); }
    void configure(const int16_t *params, int octave);
    void noteOn(int pitch, float velocity) override;
    void noteOff(int pitch) override;
    void allOff() override;
    void reset();
    void setTransport(bool playing, double beat, float bpm) {
        (void)playing;
        (void)beat;
        tempo_ = bpm;
    }
    void restartLfo() { prophetResetLfo(core_); }
    void tune() { prophetTune(core_); }
    void render(float *stereo, int frames);
    int active() const { return active_.load(std::memory_order_relaxed); }
    bool view(ProphetView &out) {
        if (!view_.consume())
            return false;
        out = view_.front();
        return true;
    }

  private:
    ProphetCore *core_ = nullptr;
    float rate_ = 48000;
    int16_t params_[PROPHET_PARAMS] = {};
    int octave_ = 0, map_[128] = {}, references_[128] = {};
    float scope_[128] = {};
    unsigned scopePos_ = 0, scopeClock_ = 0;

    float tempo_ = 120;
    std::atomic<int> active_{0};
    TripleBuffer<ProphetView> view_;
};
} // namespace dp
