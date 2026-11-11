#include "seq/sequencer.h"
#include <math.h>

namespace dp {

void Sequencer::init(float sr) {
    sr_ = sr;
    setTempo(bpm_);
}

void Sequencer::setTempo(float bpm) {
    if (bpm < 20.f) bpm = 20.f;
    if (bpm > 400.f) bpm = 400.f;
    double old = spt_;
    bpm_ = bpm;
    spt_ = sr_ * 60.0 / (bpm * PPQ);
    if (old > 0) untilTick_ *= spt_ / old;
}

void Sequencer::play(NoteSink& sink) {
    stop(sink);
    playing_ = true;
    abs_ = 0;
    tick_ = 0;
    processTick(sink);
    untilTick_ = spt_;
}

void Sequencer::stop(NoteSink& sink) {
    for (int i = 0; i < nActive_; i++) sink.noteOff(active_[i].pitch);
    nActive_ = 0;
    playing_ = false;
}

int Sequencer::samplesToNextTick() const {
    if (!playing_) return 1 << 30;
    int n = (int)ceil(untilTick_);
    return n < 1 ? 1 : n;
}

void Sequencer::advance(int n, NoteSink& sink) {
    if (!playing_) return;
    untilTick_ -= n;
    while (untilTick_ <= 0.0) {
        abs_++;
        processTick(sink);
        untilTick_ += spt_;
    }
}

void Sequencer::processTick(NoteSink& sink) {
    int len = pat_ ? pat_->lengthTicks : STEPS_PER_BAR * TICKS_PER_STEP;
    tick_ = (int)(abs_ % len);
    // Najpierw konce nut (z wlasnej listy — usuniecie nuty z patternu nie zawiesi dzwieku).
    for (int i = 0; i < nActive_;) {
        if (active_[i].end <= abs_) {
            sink.noteOff(active_[i].pitch);
            active_[i] = active_[--nActive_];
        } else i++;
    }
    if (!pat_) return;
    for (int i = 0; i < pat_->count; i++) {
        const Note& nt = pat_->notes[i];
        if (nt.start != tick_) continue;
        for (int k = 0; k < nActive_; k++)
            if (active_[k].pitch == nt.pitch) { sink.noteOff(nt.pitch); active_[k] = active_[--nActive_]; break; }
        if (nActive_ >= MAX_ACTIVE) continue;
        sink.noteOn(nt.pitch, nt.vel / 127.f);
        active_[nActive_++] = {nt.pitch, abs_ + nt.len};
    }
}

}  // namespace dp
