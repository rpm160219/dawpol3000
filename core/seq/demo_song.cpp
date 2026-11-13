#include "seq/demo_song.h"
#include "fx/fx_registry.h"
#include "seq/demo_etude_data.h"
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
    const int birds = track(p, "BIRDS", "mixtur", "BIRDS", 14, 0.4f);  // MIXTUR (undertony pod gornym glosem)
    const int horn = track(p, "HORN", "brass", "FLUGEL", 15, 0.45f);  // BRASSOPHONE (zawolanie we wstepie)
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
    // MIXTUR: gorny glos akordu polnutami — undertony f/n dopisuja pod nim akord molowy
    int pbd = p.addPattern("BIRDS", 4);
    for (int b = 0; b < 4; b++) {
        note(p.patterns[pbd], b, 0, 8, CH[b][2] + 12, 90);
        note(p.patterns[pbd], b, 8, 8, CH[b][1] + 12, 80);
    }
    p.addClip(birds, pbd, 12);
    // BRASSOPHONE: krotkie zawolanie we wstepie — tercja i kwinta akordu, z podjazdem i barwa z obwiedni
    int phn = p.addPattern("HORN", 4);
    for (int b = 0; b < 4; b++) {
        note(p.patterns[phn], b, 4, 3, CH[b][1] + 12, 95);
        note(p.patterns[phn], b, 8, 6, CH[b][2] + 12, 85);
    }
    p.addClip(horn, phn, 0);
    p.addClip(saws, pss, 8);
    p.addClip(tide, pt, 12);
    p.addClip(acid, pac, 12);
    p.addClip(chip, pch, 8);
    p.addClip(core, pr, 8);
    p.addClip(core, pr, 12);
    p.addClip(lead, pl, 8);
    p.addClip(lead, pl, 12);
}

// SYNTHWAVE: 16 taktow, 104 BPM, a-moll (Am F C G po takcie). Wstep (1-4): pad + arpeggio; 5-8: + bebny i bas;
// 9-12: + melodia; 13-16: kulminacja z akordami brass i crashem.
void buildDemoSynthwave(Project& p, bool mix, bool fxTour) {
    p = Project();
    snprintf(p.name, sizeof p.name, "SYNTHWAVE");
    p.bpm = 104.f;
    p.loopOn = true;
    p.loopIn = 0;
    p.loopOut = 16;
    const int drums = track(p, "DRUMS", "gin", "LINN + TONIC", 2, 0.7f);        // GIN
    const int bass = track(p, "BASS", "parallax", "SAW BASS", 0, 0.55f);         // PARALLAX
    const int arp = track(p, "ARP", "parallax", "PLUCK", 5, 0.32f);              // PARALLAX
    const int pad = track(p, "PAD", "juni", "ENSEMBLE", 4, 0.4f);                // JUNI
    const int lead = track(p, "LEAD", "parallax", "UNISON LEAD", 7, 0.42f);      // PARALLAX
    const int brass = track(p, "BRASS", "oxbow", "POLY BRASS", 3, 0.3f);         // OXBOW
    if (mix) {  // wersja "SYNTHWAVE MIX": mikser — chorus na ARP, poglos SISTER (AUX A), kompresory, BUS COMP
        snprintf(p.name, sizeof p.name, "SYNTHWAVE MIX");
        fxDefaults(p.tracks[arp].ins[0], FX_JUNI_CHORUS);
        p.tracks[arp].send[0] = .35f;
        p.tracks[lead].send[0] = .3f;
        p.tracks[pad].send[0] = .2f;
        p.tracks[brass].send[0] = .25f;
        p.tracks[arp].pan = -.35f;
        p.tracks[brass].pan = .3f;
        p.tracks[drums].comp.on = true; p.tracks[drums].comp.type = COMP_FET;
        p.tracks[bass].comp.on = true; p.tracks[bass].comp.type = COMP_OPTO;
        p.tracks[bass].eq.gain[0] = 2.f; p.tracks[bass].eq.freq[0] = 80.f;
        p.tracks[lead].eq.gain[2] = 2.5f;
        p.bus.on = true; p.bus.thresh = .75f; p.bus.makeup = 3.f;
    }
    if (mix && fxTour) {  // "SYNTHWAVE FX": efekty z op1r 68 / hundekraft na wszystkich AUX i insertach
        snprintf(p.name, sizeof p.name, "SYNTHWAVE FX");
        fxDefaults(p.aux[1].fx, FX_SNOWGLOBE);
        fxDefaults(p.aux[2].fx, FX_RACKVERB);
        fxDefaults(p.aux[3].fx, FX_TAPE);
        p.aux[3].fx.w[7] = 6;  // TAPE: echo 1/8 z kropka w tempie
        fxDefaults(p.aux[4].fx, FX_FATHER);
        p.tracks[lead].send[1] = .35f;   // SNOWGLOBE
        p.tracks[pad].send[2] = .3f;     // RACKVERB
        p.tracks[arp].send[3] = .4f;     // TAPE
        p.tracks[brass].send[4] = .3f;   // FATHER
        p.tracks[drums].send[4] = .15f;
        fxDefaults(p.tracks[drums].ins[1], FX_STUTTER);
        p.tracks[drums].ins[1].w[3] = 9000;  // CHANCE: co jakis czas
        fxDefaults(p.tracks[bass].ins[0], FX_CRUSH);
        p.tracks[bass].ins[0].w[3] = 9000;  // MIX: odrobina
        fxDefaults(p.tracks[pad].ins[1], FX_ENTE);
        p.tracks[pad].ins[1].w[0] = 14000; p.tracks[pad].ins[1].w[3] = fxWordOfStep(fxInfo(FX_ENTE), 3, 1);  // ducking od DRUMS (T1)
    }
    static const int SC[4][3] = {{57, 60, 64}, {53, 57, 60}, {55, 60, 64}, {55, 59, 62}};  // Am F C G
    static const int SR[4] = {33, 29, 36, 31};
    // bebny GIN (klawisze od C2): 36 KICK, 38 SNARE, 41 CLAP, 44 CLOSED HAT, 46 OPEN HAT, 51 CRASH
    int pd = p.addPattern("DRUMS", 4);
    int pdc = p.addPattern("DRUMS CRASH", 4);
    for (int v = 0; v < 2; v++) {
        Pattern& pt = p.patterns[v ? pdc : pd];
        for (int b = 0; b < 4; b++) {
            note(pt, b, 0, 1, 36, 127);
            note(pt, b, 8, 1, 36, 115);
            if (b == 3) note(pt, b, 14, 1, 36, 95);
            note(pt, b, 4, 1, 38, 120); note(pt, b, 4, 1, 41, 90);
            note(pt, b, 12, 1, 38, 120); note(pt, b, 12, 1, 41, 90);
            for (int s = 0; s < 16; s += 2) note(pt, b, s, 1, s == 14 ? 46 : 44, s % 4 ? 70 : 95);
        }
        if (v) note(pt, 0, 0, 4, 51, 110);
    }
    // bas: osemki oktawowe
    int pb = p.addPattern("BASS", 4);
    for (int b = 0; b < 4; b++)
        for (int s = 0; s < 16; s += 2) note(p.patterns[pb], b, s, 1, SR[b] + (s % 4 == 2 ? 12 : 0), s % 4 ? 90 : 115);
    // arpeggio szesnastkami o oktawe wyzej
    int pa = p.addPattern("ARP", 4);
    static const int AR[4] = {0, 1, 2, 1};
    for (int b = 0; b < 4; b++)
        for (int s = 0; s < 16; s++) note(p.patterns[pa], b, s, 1, SC[b][AR[s % 4]] + (s >= 8 ? 24 : 12), s % 4 ? 75 : 100);
    // pad: cale takty
    int pp = p.addPattern("PAD", 4);
    for (int b = 0; b < 4; b++)
        for (int k = 0; k < 3; k++) note(p.patterns[pp], b, 0, 16, SC[b][k], 85);
    // melodia
    int pl = p.addPattern("LEAD", 4);
    static const int M[][4] = {{0, 0, 6, 76}, {0, 6, 2, 74}, {0, 8, 4, 72}, {0, 12, 4, 74},
                               {1, 0, 6, 72}, {1, 6, 2, 69}, {1, 8, 8, 72},
                               {2, 0, 4, 67}, {2, 4, 4, 72}, {2, 8, 6, 76}, {2, 14, 2, 79},
                               {3, 0, 6, 77}, {3, 6, 2, 76}, {3, 8, 8, 74}};
    for (const auto& m : M) note(p.patterns[pl], m[0], m[1], m[2], m[3], 105);
    // brass: akordy na 1 i "i" dwojki (synkopa)
    int pbr = p.addPattern("BRASS", 4);
    for (int b = 0; b < 4; b++)
        for (int k = 0; k < 3; k++) {
            note(p.patterns[pbr], b, 0, 3, SC[b][k] + 12, 100);
            note(p.patterns[pbr], b, 6, 6, SC[b][k] + 12, 90);
        }
    for (int bar = 0; bar < 16; bar += 4) {
        p.addClip(pad, pp, bar);
        p.addClip(arp, pa, bar);
    }
    p.addClip(drums, pd, 4);
    p.addClip(drums, pdc, 8);
    p.addClip(drums, pdc, 12);
    for (int bar = 4; bar < 16; bar += 4) p.addClip(bass, pb, bar);
    p.addClip(lead, pl, 8);
    p.addClip(lead, pl, 12);
    p.addClip(brass, pbr, 12);
}

// REVOLUTIONARY: Chopin, Etiuda c-moll op. 10 nr 12 (domena publiczna; nuty z MIDI Mutopia Project nr 743),
// takty 1-37 (pierwsze 3 strony), 160 BPM, dwie sciezki GRAND (prawa i lewa reka — po 6 glosow), wzorce po 4 takty.
void buildDemoEtude(Project& p) {
    p = Project();
    snprintf(p.name, sizeof p.name, "REVOLUTIONARY");
    p.bpm = 160.f;
    const int BARS = 37;
    p.loopOn = true;
    p.loopIn = 0;
    p.loopOut = BARS;
    const int rh = track(p, "RIGHT HAND", "grand", "CONCERT GRAND", 3, .55f);
    const int lh = track(p, "LEFT HAND", "grand", "CONCERT GRAND", 0, .55f);
    struct Part { int t; const EtudeNote* n; int cnt; const char* nm; } parts[2] = {
        {rh, kEtudeRH, (int)(sizeof kEtudeRH / sizeof kEtudeRH[0]), "RH"}, {lh, kEtudeLH, (int)(sizeof kEtudeLH / sizeof kEtudeLH[0]), "LH"}};
    for (const Part& pt : parts)
        for (int b0 = 0; b0 < BARS; b0 += 4) {
            const int nb = BARS - b0 < 4 ? BARS - b0 : 4;
            char name[16];
            snprintf(name, sizeof name, "%s %d-%d", pt.nm, b0 + 1, b0 + nb);
            const int pi = p.addPattern(name, nb);
            if (pi < 0) continue;
            const int t0 = b0 * TICKS_PER_BAR, t1 = (b0 + nb) * TICKS_PER_BAR;
            for (int k = 0; k < pt.cnt; k++)
                if (pt.n[k].tick >= t0 && pt.n[k].tick < t1)
                    p.patterns[pi].add({(uint16_t)(pt.n[k].tick - t0), (uint16_t)(pt.n[k].len16 * 2), pt.n[k].pitch, pt.n[k].vel});
            p.addClip(pt.t, pi, b0);
        }
}

}  // namespace dp
