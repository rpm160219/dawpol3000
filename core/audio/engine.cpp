#include "audio/engine.h"
#include <math.h>
#include <string.h>

namespace dp {

void AudioEngine::init(float sr) {
    sr_ = sr;
    for (int i = 0; i < MAX_TRACKS; i++) { level_[i] = 0; levelOut_[i].store(0.f); }
    seq_.init(sr);
    for (int i = 0; i < MAX_TRACKS; i++) {
        synth_[i].init(sr);
        wt_[i].init(sr);
        eng_[i] = ENG_WAVETABLE;
        sinks_[i] = &wt_[i];
    }
    seq_.setProject(&prj_.front());
}

void AudioEngine::applyProject() {
    const Project& p = prj_.front();
    seq_.setProject(&p);
    seq_.setTempo(p.bpm);
    for (int i = 0; i < MAX_TRACKS; i++) {
        float v = i < p.nTracks ? p.tracks[i].volume : 0.f;
        synth_[i].setVolume(v);
        wt_[i].setVolume(v);
        if (i >= p.nTracks) continue;
        const Track& t = p.tracks[i];
        uint8_t e = t.engine < ENG_COUNT ? t.engine : (uint8_t)ENG_WAVETABLE;
        if (e != eng_[i]) {  // zmiana instrumentu: wycisz stary
            sinks_[i]->allOff();
            eng_[i] = e;
            sinks_[i] = e == ENG_SAW ? (NoteSink*)&synth_[i] : (NoteSink*)&wt_[i];
        }
        wt_[i].setKnobs(t.knob);
        wt_[i].setEnvelope(t.env[0], t.env[1], t.env[2], t.env[3]);
    }
    applyMutes();
    master_ = p.masterVol;
}

// Mute/solo dziala w trybie SONG (w piano rollu slychac edytowana sciezke zawsze).
void AudioEngine::applyMutes() {
    const Project& p = prj_.front();
    bool song = seq_.mode() == Sequencer::SONG;
    for (int i = 0; i < p.nTracks && i < MAX_TRACKS; i++) {
        float v = !song || p.audible(i) ? p.tracks[i].volume : 0.f;
        synth_[i].setVolume(v);
        wt_[i].setVolume(v);
    }
}

void AudioEngine::process(float* out, int frames) {
    memset(out, 0, sizeof(float) * 2 * frames);
    if (prj_.consume()) applyProject();
    EngineCmd c;
    while (q_.pop(c)) {
        switch (c.type) {
        case EC_NOTE_ON: if (c.track < MAX_TRACKS) sinks_[c.track]->noteOn(c.pitch, c.value); break;
        case EC_NOTE_OFF: if (c.track < MAX_TRACKS) sinks_[c.track]->noteOff(c.pitch); break;
        case EC_PLAY_SONG: seq_.playSong(sinks_); applyMutes(); break;
        case EC_PLAY_PATTERN: seq_.playPattern(c.pattern, c.track, sinks_); applyMutes(); break;
        case EC_STOP: seq_.stop(sinks_); break;
        }
    }
    int pos = 0;
    while (pos < frames) {
        int n = seq_.samplesToNextTick();
        if (n > frames - pos) n = frames - pos;
        for (int i = 0; i < MAX_TRACKS; i++) {
            synth_[i].render(out + pos * 2, n);
            wt_[i].render(out + pos * 2, n);
        }
        seq_.advance(n, sinks_);
        pos += n;
    }
    // Master + lagodny ogranicznik (powyzej 0.8 miekko dobija do 1.0, zamiast trzeszczec)
    for (int i = 0; i < frames * 2; i++) {
        float x = out[i] * master_, a = fabsf(x);
        if (a > 0.8f) x = copysignf(0.8f + 0.2f * tanhf((a - 0.8f) * 5.f), x);
        out[i] = x;
    }
    // VU: szczyt bloku, opadanie ~24 dB/s
    float fall = powf(10.f, -24.f / 20.f * frames / sr_);
    for (int i = 0; i < MAX_TRACKS; i++) {
        float pk = synth_[i].takePeak(), pw = wt_[i].takePeak();
        if (pw > pk) pk = pw;
        level_[i] = pk > level_[i] * fall ? pk : level_[i] * fall;
        levelOut_[i].store(level_[i], std::memory_order_relaxed);
    }
    tickOut_.store(seq_.tick(), std::memory_order_relaxed);
    modeOut_.store((int)seq_.mode(), std::memory_order_relaxed);
    playingOut_.store(seq_.playing(), std::memory_order_relaxed);
}

}  // namespace dp
