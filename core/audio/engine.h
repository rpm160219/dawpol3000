// Silnik audio: laczy sekwencer i instrument. UI rozmawia z nim tylko przez
// kolejke komend (Spsc) i migawki patternu (TripleBuffer) — zero lockow.
#pragma once
#include <atomic>
#include <stdint.h>
#include "seq/pattern.h"
#include "seq/sequencer.h"
#include "synth/saw_synth.h"
#include "util/spsc.h"
#include "util/triple_buffer.h"

namespace dp {

enum EngineCmdType : uint8_t { EC_NOTE_ON, EC_NOTE_OFF, EC_PLAY, EC_STOP, EC_TEMPO, EC_VOLUME };
struct EngineCmd { uint8_t type; uint8_t pitch; float value; };

class AudioEngine {
public:
    void init(float sampleRate);
    // --- watek UI ---
    bool send(EngineCmdType t, int pitch = 0, float value = 0.f) { return q_.push({(uint8_t)t, (uint8_t)pitch, value}); }
    Pattern& patternBack() { return pat_.back(); }
    void publishPattern() { pat_.publish(); }
    int playTick() const { return tickOut_.load(std::memory_order_relaxed); }
    bool isPlaying() const { return playingOut_.load(std::memory_order_relaxed); }
    // --- watek audio --- out = stereo przeplecione
    void process(float* out, int frames);

private:
    Spsc<EngineCmd, 256> q_;
    TripleBuffer<Pattern> pat_;
    Sequencer seq_;
    SawSynth synth_;
    std::atomic<int> tickOut_{0};
    std::atomic<bool> playingOut_{false};
};

}  // namespace dp
