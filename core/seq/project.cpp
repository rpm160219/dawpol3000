#include "seq/project.h"
#include <stdio.h>
#include <string.h>

namespace dp {

int Project::countTracks(int type) const {
    int n = 0;
    for (int i = 0; i < nTracks; i++) if (tracks[i].type == type) n++;
    return n;
}

bool Project::canAddTrack(int type) const {
    if (nTracks >= MAX_TRACKS) return false;
    return countTracks(type) < (type == TRACK_MIDI ? MAX_MIDI_TRACKS : MAX_AUDIO_TRACKS);
}

int Project::addTrack(int type, const char* nm) {
    if (!canAddTrack(type)) return -1;
    Track& t = tracks[nTracks];
    t = Track();
    t.type = (uint8_t)type;
    t.color = (uint8_t)(nTracks % 8);
    snprintf(t.name, sizeof t.name, "%s", nm);
    return nTracks++;
}

int Project::addPattern(const char* nm, int bars) {
    if (nPatterns >= MAX_PATTERNS) return -1;
    Pattern& p = patterns[nPatterns];
    p.clear();
    snprintf(p.name, sizeof p.name, "%s", nm);
    p.setBars(bars);
    return nPatterns++;
}

int Project::clipAt(int track, int bar) const {
    for (int i = 0; i < nClips; i++) {
        const Clip& c = clips[i];
        if (c.track == track && bar >= c.startBar && bar < c.startBar + clipBars(c)) return i;
    }
    return -1;
}

bool Project::fits(int track, int start, int bars, int ignore) const {
    if (start < 0 || start + bars > MAX_SONG_BARS) return false;
    for (int i = 0; i < nClips; i++) {
        if (i == ignore || clips[i].track != track) continue;
        int s = clips[i].startBar, e = s + clipBars(clips[i]);
        if (start < e && start + bars > s) return false;
    }
    return true;
}

int Project::addClip(int track, int pattern, int start) {
    if (nClips >= MAX_CLIPS || pattern < 0 || pattern >= nPatterns) return -1;
    if (!fits(track, start, patterns[pattern].bars())) return -1;
    clips[nClips] = {(uint8_t)track, (uint8_t)pattern, (uint16_t)start};
    tracks[track].lastPattern = pattern;
    return nClips++;
}

void Project::removeClip(int i) {
    if (i < 0 || i >= nClips) return;
    clips[i] = clips[--nClips];
}

int Project::songBars() const {
    int e = 1;
    for (int i = 0; i < nClips; i++) {
        int ce = clips[i].startBar + clipBars(clips[i]);
        if (ce > e) e = ce;
    }
    return e;
}

void Project::nextPatternName(char* out, int n) const { snprintf(out, n, "PATTERN %02d", nPatterns + 1); }

void Project::nextTrackName(int type, char* out, int n) const {
    snprintf(out, n, "%s %02d", type == TRACK_MIDI ? "MIDI" : "AUDIO", countTracks(type) + 1);
}

}  // namespace dp
