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
#include "synth/elg_dsp.h"
#include "synth/eva_dsp.h"
#include "synth/mbira_dsp.h"
#include "synth/reactor_dsp.h"
#include "synth/oxbow_dsp.h"
#include "synth/oxbow_plus_dsp.h"
#include "synth/monolit_dsp.h"
static inline int32_t dp_monolitEnv(const MonolitCore* p) { return monolitEnv(p); }
static inline void dp_oxbowView(const OxbowCore* p, OxbowView* v) { oxbowView(p, v); }
static inline void dp_reactorView(const ReactorCore* p, ReactorView* v) { reactorView(p, v); }
static inline void dp_mbiraView(const MbiraCore* p, MbiraView* v) { mbiraView(p, v); }
static inline void dp_evaView(const EvaCore* p, EvaView* v) { evaView(p, v); }
#include "audio/mixer.h"
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
    void elgView(int t, ElgView* v) const { if (t >= 0 && t < MAX_TRACKS && wt_[t].elg()) dp::elgView(wt_[t].elg(), v); }
    void evaView(int t, EvaView* v) const { if (t >= 0 && t < MAX_TRACKS && wt_[t].eva()) dp_evaView(wt_[t].eva(), v); }
    void mbiraView(int t, MbiraView* v) const { if (t >= 0 && t < MAX_TRACKS && wt_[t].mbira()) dp_mbiraView(wt_[t].mbira(), v); }
    void reactorView(int t, ReactorView* v) const { if (t >= 0 && t < MAX_TRACKS && wt_[t].reactor()) dp_reactorView(wt_[t].reactor(), v); }
    void oxbowView(int t, OxbowView* v) const {  // OXBOW albo OXBOW+ (wg rodzaju silnika glosow sciezki)
        if (t < 0 || t >= MAX_TRACKS) return;
        if (wt_[t].kind() == 8) oxbowPlusView(wt_[t].oxbowPlus(), v);
        else if (wt_[t].oxbow()) dp_oxbowView(wt_[t].oxbow(), v);
    }
    const ::GrandCore* grandCore(int t) const { return t >= 0 && t < MAX_TRACKS ? wt_[t].grand() : nullptr; }
    const ::FeltCore* feltCore(int t) const { return t >= 0 && t < MAX_TRACKS ? wt_[t].felt() : nullptr; }
    const ::LoungeCore* loungeCore(int t) const { return t >= 0 && t < MAX_TRACKS ? wt_[t].lounge() : nullptr; }
    const ::GinCore* ginCore(int t) const { return t >= 0 && t < MAX_TRACKS ? wt_[t].gin() : nullptr; }
    const ::OutatimeCore* outatimeCore(int t) const { return t >= 0 && t < MAX_TRACKS ? wt_[t].outatime() : nullptr; }
    const ::JuniCore* juniCore(int t) const { return t >= 0 && t < MAX_TRACKS ? wt_[t].juni() : nullptr; }
    const ::DdddCore* ddddCore(int t) const { return t >= 0 && t < MAX_TRACKS ? wt_[t].dddd() : nullptr; }
    const ::ArcCore* arcCore(int t) const { return t >= 0 && t < MAX_TRACKS ? wt_[t].arc() : nullptr; }
    const ::AetherCore* aetherCore(int t) const { return t >= 0 && t < MAX_TRACKS ? wt_[t].aether() : nullptr; }
    const ::BlotCore* blotCore(int t) const { return t >= 0 && t < MAX_TRACKS ? wt_[t].blot() : nullptr; }
    const ::HollowCore* hollowCore(int t) const { return t >= 0 && t < MAX_TRACKS ? wt_[t].hollow() : nullptr; }
    const ::MixturCore* mixturCore(int t) const { return t >= 0 && t < MAX_TRACKS ? wt_[t].mixtur() : nullptr; }
    const ::SupersawCore* supersawCore(int t) const { return t >= 0 && t < MAX_TRACKS ? wt_[t].supersaw() : nullptr; }
    const ::VaporaCore* vaporaCore(int t) const { return t >= 0 && t < MAX_TRACKS ? wt_[t].vapora() : nullptr; }
    int32_t monolitEnv(int t) const { return t >= 0 && t < MAX_TRACKS && wt_[t].monolit() ? dp_monolitEnv(wt_[t].monolit()) : 0; }
    int wtActive(int t) const { return t >= 0 && t < MAX_TRACKS ? wt_[t].activeVoices() : 0; }
    // i: 0..3 galki, 4 = fala OSC 2 (tryb rozszerzony)
    int16_t wtModKnob(int t, int i) const { return t >= 0 && t < MAX_TRACKS ? wt_[t].modKnob(i) : 0; }
    // mikser: wskazniki (watek UI czyta, audio pisze)
    float chPeak(int t) const { return t >= 0 && t < MAX_TRACKS ? mixer_.ch[t].peak.load(std::memory_order_relaxed) : 0.f; }
    float chGr(int t) const { return t >= 0 && t < MAX_TRACKS ? mixer_.ch[t].gr.load(std::memory_order_relaxed) : 0.f; }
    float auxPeak(int a) const { return a >= 0 && a < MIX_AUX ? mixer_.aux[a].peak.load(std::memory_order_relaxed) : 0.f; }
    float masterPeak() const { return mixer_.master.peak.load(std::memory_order_relaxed); }
    float masterGr() const { return mixer_.master.gr.load(std::memory_order_relaxed); }
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
    MixerDsp mixer_;
    float trkBuf_[MAX_TRACKS][512];  // bufory sciezek (stereo przeplatane, do 256 ramek)
    int nTracks_ = 0;
    float level_[MAX_TRACKS] = {};
    std::atomic<float> levelOut_[MAX_TRACKS];
    float sr_ = 48000.f;
    std::atomic<int> tickOut_{0}, modeOut_{0};
    std::atomic<bool> playingOut_{false};
    void applyProject();
    void applyMutes();
};

}  // namespace dp
