// Testowy syntezator: polifoniczna pila (polyBLEP), lagodny filtr, obwiednia ADSR.
#pragma once
#include <stdint.h>
#include "synth/note_sink.h"

namespace dp {

class SawSynth : public NoteSink {
public:
    static const int VOICES = 8;
    void init(float sampleRate);
    void setVolume(float v) { volT_ = v; }
    void noteOn(int pitch, float vel) override;
    void noteOff(int pitch) override;
    void allOff() override;
    // Dodaje n ramek stereo do out (out wyzerowany przez wolajacego).
    void render(float* out, int n);

private:
    enum { OFF, ATTACK, DECAY, RELEASE };
    struct Voice {
        int pitch = -1, stage = OFF;
        float dt = 0, phase = 0, env = 0, vel = 0, lp1 = 0, lp2 = 0;
        uint32_t age = 0;
    };
    Voice v_[VOICES];
    float sr_ = 48000.f, attack_ = 0, decay_ = 0, release_ = 0, sustain_ = 0.6f, lpK_ = 0;
    float vol_ = 0.7f, volT_ = 0.7f, volK_ = 0;
    uint32_t age_ = 0;
};

}  // namespace dp
