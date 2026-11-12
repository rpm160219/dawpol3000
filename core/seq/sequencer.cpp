#include "seq/sequencer.h"
#include <math.h>

namespace dp {

void Sequencer::init(float sr) {
    sr_ = sr;
    spt_ = 0;
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

void Sequencer::start(NoteSink** sinks) {
    playing_ = true;
    abs_ = 0;
    processTick(sinks);
    untilTick_ = spt_;
}

void Sequencer::playSong(NoteSink** sinks) {
    stop(sinks);
    mode_ = SONG;
    start(sinks);
}

void Sequencer::playPattern(int pattern, int track, NoteSink** sinks) {
    stop(sinks);
    mode_ = PATTERN;
    pat_ = pattern;
    track_ = track;
    start(sinks);
}

void Sequencer::stop(NoteSink** sinks) {
    for (int i = 0; i < nActive_; i++) sinks[active_[i].track]->noteOff(active_[i].pitch);
    nActive_ = 0;
    playing_ = false;
    tick_ = 0;
}

int Sequencer::samplesToNextTick() const {
    if (!playing_) return 1 << 30;
    int n = (int)ceil(untilTick_);
    return n < 1 ? 1 : n;
}

void Sequencer::advance(int n, NoteSink** sinks) {
    if (!playing_) return;
    untilTick_ -= n;
    while (untilTick_ <= 0.0 && playing_) {
        abs_++;
        processTick(sinks);
        untilTick_ += spt_;
    }
}

void Sequencer::trigger(int track, const Pattern& p, int ptick, NoteSink** sinks) {
    for (int i = 0; i < p.count; i++) {
        const Note& nt = p.notes[i];
        if (nt.start != ptick) continue;
        for (int k = 0; k < nActive_; k++)
            if (active_[k].track == track && active_[k].pitch == nt.pitch) {
                sinks[track]->noteOff(nt.pitch);
                active_[k] = active_[--nActive_];
                break;
            }
        if (nActive_ >= MAX_ACTIVE) continue;
        sinks[track]->noteOn(nt.pitch, nt.vel / 127.f);
        active_[nActive_++] = {(uint8_t)track, nt.pitch, abs_ + nt.len};
    }
}

void Sequencer::processTick(NoteSink** sinks) {
    for (int i = 0; i < nActive_;) {
        if (active_[i].end <= abs_) {
            sinks[active_[i].track]->noteOff(active_[i].pitch);
            active_[i] = active_[--nActive_];
        } else i++;
    }
    if (!prj_) return;
    if (mode_ == PATTERN) {
        if (pat_ >= prj_->nPatterns) return;
        const Pattern& p = prj_->patterns[pat_];
        int st = 0, en = p.lengthTicks;
        if (p.loopOn && p.loopOut > p.loopIn) {
            st = p.loopIn * TICKS_PER_STEP;
            en = p.loopOut * TICKS_PER_STEP;
            if (en > p.lengthTicks) en = p.lengthTicks;
            if (st >= en) { st = 0; en = p.lengthTicks; }
        }
        tick_ = st + (int)(abs_ % (en - st));
        trigger(track_, p, tick_, sinks);
    } else {
        int st, en;
        prj_->songRange(st, en);
        if (prj_->loopOn) tick_ = st + (int)(abs_ % (en - st));
        else {
            tick_ = (int)abs_;
            if (tick_ >= en) { stop(sinks); return; }  // koniec utworu bez petli
        }
        bool aud[MAX_TRACKS];
        for (int t = 0; t < prj_->nTracks; t++) aud[t] = prj_->audible(t);
        for (int i = 0; i < prj_->nClips; i++) {
            const Clip& c = prj_->clips[i];
            if (c.pattern >= prj_->nPatterns || c.track >= prj_->nTracks) continue;
            if (prj_->tracks[c.track].type != TRACK_MIDI || !aud[c.track]) continue;
            const Pattern& p = prj_->patterns[c.pattern];
            int start = c.startBar * TICKS_PER_BAR;
            if (tick_ >= start && tick_ < start + p.lengthTicks) trigger(c.track, p, tick_ - start, sinks);
        }
    }
}

}  // namespace dp
