// Sekwencer (watek audio). Dwa tryby:
//   SONG    — gra cala aranzacje (klipy na sciezkach), petla do konca ostatniego klipu,
//   PATTERN — gra jeden pattern w petli na wybranej sciezce (edycja w piano rollu).
// Dokladnosc do probki; sam pamieta grajace nuty, wiec edycja w trakcie nie zawiesza dzwieku.
#pragma once
#include <stdint.h>
#include "seq/project.h"
#include "synth/note_sink.h"

namespace dp {

class Sequencer {
public:
    enum Mode { SONG, PATTERN };
    void init(float sampleRate);
    void setProject(const Project* p) { prj_ = p; }
    void setTempo(float bpm);
    void playSong(NoteSink** sinks);
    void playPattern(int pattern, int track, NoteSink** sinks);
    void stop(NoteSink** sinks);
    bool playing() const { return playing_; }
    Mode mode() const { return mode_; }
    int tick() const { return tick_; }  // pozycja w petli (song: od poczatku utworu)

    int samplesToNextTick() const;
    void advance(int n, NoteSink** sinks);

private:
    struct Active { uint8_t track, pitch; int64_t end; };
    static const int MAX_ACTIVE = 128;
    const Project* prj_ = nullptr;
    Mode mode_ = SONG;
    int pat_ = 0, track_ = 0;
    float sr_ = 48000.f, bpm_ = 120.f;
    double spt_ = 250.0, untilTick_ = 0.0;
    int64_t abs_ = 0;
    int tick_ = 0;
    bool playing_ = false;
    Active active_[MAX_ACTIVE];
    int nActive_ = 0;
    void start(NoteSink** sinks);
    void processTick(NoteSink** sinks);
    void trigger(int track, const Pattern& p, int ptick, NoteSink** sinks);
};

}  // namespace dp
