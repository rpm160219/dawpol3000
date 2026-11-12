// Pattern: nazwana lista nut w czasie (ticki), dlugosc 1..16 taktow 4/4.
// Czas w tickach: PPQ = 96 na cwiercnute, siatka edycji = szesnastki (24 ticki).
#pragma once
#include <stdint.h>

namespace dp {

static const int PPQ = 96;
static const int TICKS_PER_STEP = PPQ / 4;  // szesnastka
static const int STEPS_PER_BAR = 16;        // 4/4
static const int TICKS_PER_BAR = STEPS_PER_BAR * TICKS_PER_STEP;
static const int MAX_PATTERN_BARS = 16;

struct Note {
    uint16_t start;  // tick poczatku
    uint16_t len;    // dlugosc w tickach
    uint8_t pitch;   // MIDI 0..127
    uint8_t vel;     // 1..127
};

struct Pattern {
    static const int MAX_NOTES = 512;
    char name[12] = "PATTERN";
    // Petla w PIANO ROLL (kroki, OUT wylacznie). Wylaczona = caly pattern.
    bool loopOn = false;
    uint16_t loopIn = 0, loopOut = 0;
    int lengthTicks = TICKS_PER_BAR;
    int count = 0;
    Note notes[MAX_NOTES];

    void clear() { count = 0; }
    int bars() const { return (lengthTicks + TICKS_PER_BAR - 1) / TICKS_PER_BAR; }
    // Zmiana dlugosci; nuty za koncem znikaja, wystajace sa przycinane.
    void setBars(int bars);
    // Nuta o danej wysokosci, ktora obejmuje tick (albo -1).
    int find(int tick, int pitch) const;
    // Dodaje nute (przycina, by nie nachodzila na nastepna o tej samej wysokosci). Zwraca indeks albo -1.
    int add(Note n);
    void remove(int i);
    // Maksymalna dlugosc nuty od 'start' na wysokosci 'pitch' (do nastepnej nuty albo konca patternu).
    int maxLen(int start, int pitch, int ignore = -1) const;
};

}  // namespace dp
