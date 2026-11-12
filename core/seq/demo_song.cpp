#include "seq/demo_song.h"
#include <stdio.h>

namespace dp {

namespace {
const int T = TICKS_PER_STEP, BAR = TICKS_PER_BAR;
// akordy (Am, F, C, G) — glosy srodkowe i pryma basu
const int CH[4][3] = {{57, 60, 64}, {57, 60, 65}, {55, 60, 64}, {55, 59, 62}};
const int ROOT[4] = {45, 41, 48, 43};

void note(Pattern& p, int bar, int step, int len, int pitch, int vel) {
    p.add({(uint16_t)(bar * BAR + step * T), (uint16_t)(len * T), (uint8_t)pitch, (uint8_t)vel});
}

int track(Project& p, const char* name, const char* engDir, const char* preset, int color, float vol) {
    int t = p.addTrack(TRACK_MIDI, name);
    if (t < 0) return t;
    Track& tr = p.tracks[t];
    if (!factoryInstrument(engDir, preset, tr)) fprintf(stderr, "demo: brak presetu %s/%s\\n", engDir, preset);
    tr.color = (uint8_t)color;
    tr.volume = vol;
    return t;
}
}  // namespace

void buildDemoAllEngines(Project& p) {
    p = Project();
    snprintf(p.name, sizeof p.name, "ALL ENGINES");
    p.bpm = 96.f;
    p.loopOn = true;
    p.loopIn = 0;
    p.loopOut = 16;

    const int drone = track(p, "DRONE", "elg", "NORTHERN LIGHTS", 4, 0.45f);    // ELG
    const int choir = track(p, "CHOIR", "eva", "SPACE CHOIR", 5, 0.5f);         // EVA
    const int kal = track(p, "KALIMBA", "mbira", "KALIMBA", 1, 0.55f);         // MBIRA
    const int bass = track(p, "BASS", "wavetable", "ROUND BASS", 0, 0.8f);      // WAVETABLE
    const int kick = track(p, "KICK", "wavetableplus", "KICK DRUM", 2, 0.85f); // WAVETABLE+
    const int organ = track(p, "ORGAN", "pew", "JAZZ CLUB", 3, 0.4f);          // PEW
    const int pluck = track(p, "PLUCK", "wavetableplus", "SYNC PLUCK", 6, 0.4f);
    const int lead = track(p, "LEAD", "taffy", "CZ LEAD", 7, 0.5f);            // TAFFY
    const int core = track(p, "CORE", "reactor", "SCRAM", 8, 0.35f);          // REACTOR
    const int brass = track(p, "BRASS", "oxbow", "POLY BRASS", 9, 0.4f);     // OXBOW
    const int chip = track(p, "CHIP", "sid", "PULSE KEYS", 10, 0.3f);        // SID
    const int acid = track(p, "ACID", "monolit", "ACID GLIDE", 11, 0.35f);  // MONOLIT (MONO + glide)
    const int tide = track(p, "TIDE", "vapora", "A FIFTH TIDE", 12, 0.45f);  // VAPORA (pad z kwinta)
    const int saws = track(p, "SAWS", "supersaw", "TRANCE PLUCK", 13, 0.45f);  // SUPERSAW (akordy na off-beat, filtr z obwiednia)

    // dron: pryma A przez 4 takty (interwal oktawy w presecie)
    int pd = p.addPattern("DRONE", 4);
    note(p.patterns[pd], 0, 0, 64, 45, 90);
    // chor: akordy calymi taktami
    int pc = p.addPattern("CHOIR", 4);
    for (int b = 0; b < 4; b++)
        for (int k = 0; k < 3; k++) note(p.patterns[pc], b, 0, 16, CH[b][k], 80);
    // kalimba: arpeggio osemkami o oktawe wyzej
    int pk = p.addPattern("KALIMBA", 4);
    static const int ARP[8] = {0, 1, 2, 3, 2, 1, 2, 3};  // 3 = pryma oktawe wyzej
    for (int b = 0; b < 4; b++)
        for (int i = 0; i < 8; i++) {
            const int a = ARP[i];
            note(p.patterns[pk], b, i * 2, 2, (a == 3 ? CH[b][0] + 12 : CH[b][a]) + 12, i % 2 ? 70 : 100);
        }
    // bas: pryma z synkopa, oktawa w gore na koncu taktu
    int pb = p.addPattern("BASS", 4);
    for (int b = 0; b < 4; b++) {
        const int r = ROOT[b] - 12;
        note(p.patterns[pb], b, 0, 3, r, 120);
        note(p.patterns[pb], b, 6, 2, r, 95);
        note(p.patterns[pb], b, 8, 3, r, 110);
        note(p.patterns[pb], b, 14, 2, r + 12, 90);
    }
    // stopa: cwiercnuty
    int pkk = p.addPattern("KICK", 4);
    for (int b = 0; b < 4; b++)
        for (int s = 0; s < 16; s += 4) note(p.patterns[pkk], b, s, 2, 36, s == 0 ? 127 : 105);
    // organy: akordy na "dwa" i "cztery" (offbeat)
    int po = p.addPattern("ORGAN", 4);
    for (int b = 0; b < 4; b++)
        for (int s = 4; s < 16; s += 8)
            for (int k = 0; k < 3; k++) note(p.patterns[po], b, s, 2, CH[b][k] + 12, 90);
    // pluck: synkopy szesnastkowe — najwyzszy dzwiek akordu i pryma
    int pp = p.addPattern("PLUCK", 4);
    for (int b = 0; b < 4; b++) {
        note(p.patterns[pp], b, 3, 1, CH[b][2] + 12, 100);
        note(p.patterns[pp], b, 6, 1, CH[b][0] + 12, 85);
        note(p.patterns[pp], b, 10, 1, CH[b][2] + 12, 95);
        note(p.patterns[pp], b, 13, 1, CH[b][1] + 12, 80);
    }
    // melodia (TAFFY)
    int pl = p.addPattern("MELODY", 4);
    struct M { int bar, step, len, pitch, vel; };
    static const M MEL[] = {
        {0, 0, 6, 76, 110}, {0, 6, 2, 74, 90}, {0, 8, 4, 72, 100}, {0, 12, 4, 71, 85},
        {1, 0, 6, 72, 105}, {1, 6, 2, 69, 85}, {1, 8, 4, 72, 95}, {1, 12, 4, 74, 100},
        {2, 0, 8, 76, 115}, {2, 8, 4, 79, 105}, {2, 12, 4, 76, 90},
        {3, 0, 6, 74, 100}, {3, 6, 2, 71, 85}, {3, 8, 8, 69, 110},
    };
    for (const M& m : MEL) note(p.patterns[pl], m.bar, m.step, m.len, m.pitch, m.vel);
    // REACTOR: gorace akcenty na "raz" i synkopa przed nastepnym taktem (prima o oktawe nizej niz akord)
    int pr = p.addPattern("CORE", 4);
    for (int b = 0; b < 4; b++) {
        note(p.patterns[pr], b, 0, 3, ROOT[b], 120);
        note(p.patterns[pr], b, 14, 2, ROOT[b] + 7, 90);
    }

    for (int s = 0; s < 16; s += 4) {
        p.addClip(drone, pd, s);
        p.addClip(choir, pc, s);
        p.addClip(kal, pk, s);
    }
    for (int s = 4; s <= 8; s += 4) {
        p.addClip(bass, pb, s);
        p.addClip(kick, pkk, s);
        p.addClip(organ, po, s);
    }
    p.addClip(pluck, pp, 8);
    // OXBOW: akordy dete — dluga "raz" i synkopa na "trzy i"
    int pbr = p.addPattern("BRASS", 4);
    for (int b = 0; b < 4; b++)
        for (int k = 0; k < 3; k++) {
            note(p.patterns[pbr], b, 0, 6, CH[b][k] + 12, 105);
            note(p.patterns[pbr], b, 10, 4, CH[b][k] + 12, 90);
        }
    p.addClip(brass, pbr, 8);
    // SID: szesnastkowe arpeggio akordu dwie oktawy wyzej (groove i calosc)
    int pch = p.addPattern("CHIP", 4);
    for (int b = 0; b < 4; b++)
        for (int st = 0; st < 16; st++) note(p.patterns[pch], b, st, 1, CH[b][st % 3] + (st % 6 < 3 ? 12 : 24), st % 4 ? 70 : 100);
    p.addClip(chip, pch, 4);
    // MONOLIT: linia acid szesnastkami (MONO z glide), pryma z oktawa i kwinta
    int pac = p.addPattern("ACID", 4);
    static const int AC[16] = {0, 0, 12, 0, 7, 0, 12, 10, 0, 0, 12, 0, 7, 12, 0, 5};
    for (int b = 0; b < 4; b++)
        for (int st = 0; st < 16; st++)
            if (st % 4 != 3) note(p.patterns[pac], b, st, (st % 8 == 6) ? 2 : 1, ROOT[b] - 12 + AC[st], st % 4 ? 85 : 115);
    p.addClip(acid, pac, 8);
    // VAPORA: pad — pryma i kwinta akordu (preset dodaje kwinte wyzej), calymi taktami; wstep i zakonczenie
    int pt = p.addPattern("TIDE", 4);
    for (int b = 0; b < 4; b++) {
        note(p.patterns[pt], b, 0, 16, ROOT[b] + 12, 80);
        note(p.patterns[pt], b, 0, 16, CH[b][2], 70);
    }
    p.addClip(tide, pt, 0);
    // SUPERSAW: krotkie akordy na "i" (off-beat osemek), oktawe wyzej
    int pss = p.addPattern("SAWS", 4);
    for (int b = 0; b < 4; b++)
        for (int st = 2; st < 16; st += 4)
            for (int k = 0; k < 3; k++) note(p.patterns[pss], b, st, 1, CH[b][k] + 12, st == 2 ? 100 : 85);
    p.addClip(saws, pss, 4);
    p.addClip(saws, pss, 8);
    p.addClip(tide, pt, 12);
    p.addClip(acid, pac, 12);
    p.addClip(chip, pch, 8);
    p.addClip(core, pr, 8);
    p.addClip(core, pr, 12);
    p.addClip(lead, pl, 8);
    p.addClip(lead, pl, 12);
}

}  // namespace dp
