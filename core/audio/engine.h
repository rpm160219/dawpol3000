// Silnik audio: sekwencer + instrument na kazda sciezke + miks. UI rozmawia z nim tylko przez
// kolejke komend (Spsc) i migawki calego projektu (TripleBuffer) — zero lockow w watku audio.
// Tempo, glosnosci sciezek i master sa brane z migawki projektu.
#pragma once
#include <atomic>
#include <stdint.h>
#include "seq/project.h"
#include "seq/sequencer.h"
#include "synth/saw_synth.h"
#include "synth/wavetable_synth.h"
#include "synth/pew_dsp.h"
#include "util/spsc.h"
#include "util/triple_buffer.h"

namespace dp {

enum EngineCmdType : uint8_t { EC_NOTE_ON, EC_NOTE_OFF, EC_PLAY_SONG, EC_PLAY_PATTERN, EC_STOP };
struct EngineCmd { uint8_t type, track, pitch, pattern; float value; };

class AudioEngine {
public:
    void init(float sampleRate);
    // --- watek UI ---
    bool send(EngineCmdType t, int track = 0, int pitch = 0, int pattern = 0, float value = 0.f) {
        return q_.push({(uint8_t)t, (uint8_t)track, (uint8_t)pitch, (uint8_t)pattern, value});
    }
    Project& projectBack() { return prj_.back(); }
    void publishProject() { prj_.publish(); }
    int playTick() const { return tickOut_.load(std::memory_order_relaxed); }
    bool isPlaying() const { return playingOut_.load(std::memory_order_relaxed); }
    int playMode() const { return modeOut_.load(std::memory_order_relaxed); }  // Sequencer::Mode
    // Poziom sciezki do VU (0..1, szczyt z opadaniem ~24 dB/s, po glosnosci sciezki, przed masterem).
    // Dla ekranu instrumentu: roznica faz oscylatorow i liczba grajacych glosow WAVETABLE na sciezce
    uint32_t wtDrift(int t) const { return t >= 0 && t < MAX_TRACKS ? wt_[t].drift() : 0; }
    int16_t voiceLevel(int t) const { return t >= 0 && t < MAX_TRACKS ? wt_[t].voiceLevel() : 0; }
    int16_t taffyEnv(int t) const { return t >= 0 && t < MAX_TRACKS ? wt_[t].taffyEnv() : 0; }
    void pewView(int t, PewView* v) const { if (t >= 0 && t < MAX_TRACKS && wt_[t].pew()) dp::pewView(wt_[t].pew(), v); }
    int wtActive(int t) const { return t >= 0 && t < MAX_TRACKS ? wt_[t].activeVoices() : 0; }
    // i: 0..3 galki, 4 = fala OSC 2 (tryb rozszerzony)
    int16_t wtModKnob(int t, int i) const { return t >= 0 && t < MAX_TRACKS ? wt_[t].modKnob(i) : 0; }
    float trackLevel(int t) const { return t >= 0 && t < MAX_TRACKS ? levelOut_[t].load(std::memory_order_relaxed) : 0.f; }
    // --- watek audio --- out = stereo przeplecione
    void process(float* out, int frames);

private:
    Spsc<EngineCmd, 256> q_;
    TripleBuffer<Project> prj_;
    Sequencer seq_;
    SawSynth synth_[MAX_TRACKS];
    WavetableSynth wt_[MAX_TRACKS];
    uint8_t eng_[MAX_TRACKS] = {};  // biezacy silnik sciezki
    NoteSink* sinks_[MAX_TRACKS];
    float master_ = 0.8f;
    float level_[MAX_TRACKS] = {};
    std::atomic<float> levelOut_[MAX_TRACKS];
    float sr_ = 48000.f;
    std::atomic<int> tickOut_{0}, modeOut_{0};
    std::atomic<bool> playingOut_{false};
    void applyProject();
    void applyMutes();
};

}  // namespace dp
