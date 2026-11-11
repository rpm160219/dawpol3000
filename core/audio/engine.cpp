#include "audio/engine.h"
#include <string.h>

namespace dp {

void AudioEngine::init(float sr) {
    seq_.init(sr);
    synth_.init(sr);
    seq_.setPattern(&pat_.front());
}

void AudioEngine::process(float* out, int frames) {
    memset(out, 0, sizeof(float) * 2 * frames);
    if (pat_.consume()) seq_.setPattern(&pat_.front());
    EngineCmd c;
    while (q_.pop(c)) {
        switch (c.type) {
        case EC_NOTE_ON: synth_.noteOn(c.pitch, c.value); break;
        case EC_NOTE_OFF: synth_.noteOff(c.pitch); break;
        case EC_PLAY: seq_.play(synth_); break;
        case EC_STOP: seq_.stop(synth_); break;
        case EC_TEMPO: seq_.setTempo(c.value); break;
        case EC_VOLUME: synth_.setVolume(c.value); break;
        }
    }
    int pos = 0;
    while (pos < frames) {
        int n = seq_.samplesToNextTick();
        if (n > frames - pos) n = frames - pos;
        synth_.render(out + pos * 2, n);
        seq_.advance(n, synth_);
        pos += n;
    }
    tickOut_.store(seq_.tick(), std::memory_order_relaxed);
    playingOut_.store(seq_.playing(), std::memory_order_relaxed);
}

}  // namespace dp
