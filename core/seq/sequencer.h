// Sekwencer (watek audio): odtwarza Pattern w petli z dokladnoscia do probki
// i wysyla noteOn/noteOff do NoteSink.
#pragma once
#include <stdint.h>
#include "seq/pattern.h"
#include "synth/note_sink.h"

namespace dp {

class Sequencer {
public:
    void init(float sampleRate);
    void setPattern(const Pattern* p) { pat_ = p; }
    void setTempo(float bpm);
    void play(NoteSink& sink);
    void stop(NoteSink& sink);
    bool playing() const { return playing_; }
    int tick() const { return tick_; }
    float tempo() const { return bpm_; }

    // Ile probek mozna wyrenderowac, zanim wypadnie nastepny tick.
    int samplesToNextTick() const;
    // Przesun czas o n probek; obsluz ticki, ktore minely.
    void advance(int n, NoteSink& sink);

private:
    struct Active { uint8_t pitch; int64_t end; };
    static const int MAX_ACTIVE = 32;
    const Pattern* pat_ = nullptr;
    float sr_ = 48000.f, bpm_ = 120.f;
    double spt_ = 250.0;      // probek na tick
    double untilTick_ = 0.0;  // probek do nastepnego ticka
    int64_t abs_ = 0;         // tick bezwzgledny od startu
    int tick_ = 0;            // tick w patternie
    bool playing_ = false;
    Active active_[MAX_ACTIVE];
    int nActive_ = 0;
    void processTick(NoteSink& sink);
};

}  // namespace dp
