// Pattern: lista nut w czasie (ticki). Na razie jeden kanal, jeden takt 4/4.
// Czas w tickach: PPQ = 96 na cwiercnute, siatka edycji = szesnastki (24 ticki).
#pragma once
#include <stdint.h>

namespace dp {

static const int PPQ = 96;
static const int TICKS_PER_STEP = PPQ / 4;  // szesnastka
static const int STEPS_PER_BAR = 16;        // 4/4

struct Note {
    uint16_t start;  // tick poczatku
    uint16_t len;    // dlugosc w tickach
    uint8_t pitch;   // MIDI 0..127
    uint8_t vel;     // 1..127
};

struct Pattern {
    static const int MAX_NOTES = 256;
    int lengthTicks = STEPS_PER_BAR * TICKS_PER_STEP;
    int count = 0;
    Note notes[MAX_NOTES];

    void clear() { count = 0; }
    // Nuta o danej wysokosci, ktora obejmuje tick (albo -1).
    int find(int tick, int pitch) const;
    // Dodaje nute (przycina, by nie nachodzila na nastepna o tej samej wysokosci). Zwraca indeks albo -1.
    int add(Note n);
    void remove(int i);
    // Maksymalna dlugosc nuty od 'start' na wysokosci 'pitch' (do nastepnej nuty albo konca patternu).
    int maxLen(int start, int pitch, int ignore = -1) const;
};

}  // namespace dp
