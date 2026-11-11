// Etap 0: ekran testowy — ton, 4 enkodery, oscyloskop, stan przyciskow i osi.
#pragma once
#include "canvas.h"
#include "input.h"
#include "tone_synth.h"

namespace dp {

struct AudioInfo {
    bool ok = false;
    int rate = 0, frames = 0, channels = 0;
    char driver[32] = "";
    char err[96] = "";
};

class HelloApp {
public:
    void init(ToneSynth* s);
    void update(const InputState& in, float dt);
    void render(Canvas& c, const InputState& in, const AudioInfo& ai, float fps);
    bool quitRequested() const { return quit_; }
    void setEncoder(int i, float v) { enc_[i] = v; }

private:
    ToneSynth* syn_ = nullptr;
    float enc_[4] = {0.5f, 0.6f, 0.5f, 0.0f};  // TON, GLOS, PAN, DRIVE (0..1)
    float sent_[TP_COUNT];
    bool prev_[BTN_COUNT] = {};
    int octave_ = 4;
    int cur_ = -1;        // indeks klawisza nuty (0..7) albo -1
    int midi_ = 69;
    bool drone_ = false, quit_ = false;
    void send(int p, float v);
};

}  // namespace dp
