/* wygenerowane przez tools/gen/svg_to_art2.py z monolit.svg (grafika: Hundekraft, GPL-3.0-or-later) */
#pragma once
static const float MN_PTS[] = {14.00f, 104.00f, 14.00f, 99.00f, 26.00f, 95.00f, 38.00f, 98.00f, 52.00f, 92.00f, 66.00f, 97.00f, 84.00f, 99.00f, 104.00f, 101.00f, 130.00f, 102.00f, 146.00f, 104.00f, 174.00f, 104.00f, 190.00f, 102.00f, 214.00f, 100.00f, 232.00f, 96.00f, 246.00f, 99.00f, 262.00f, 93.00f, 278.00f, 97.00f, 294.00f, 95.00f, 306.00f, 99.00f, 306.00f, 104.00f, 14.00f, 104.00f, 306.00f, 104.00f, 160.00f, 32.00f, 155.00f, 42.00f, 150.00f, 116.00f, 160.00f, 116.00f, 160.00f, 32.00f, 160.00f, 32.00f, 165.00f, 42.00f, 170.00f, 116.00f, 160.00f, 116.00f, 160.00f, 32.00f, 155.00f, 42.00f, 165.00f, 42.00f};
struct MNStroke { int first, count; unsigned rgb; float width; int closed, group; };
static const MNStroke MN_STROKES[] = {
    {0, 10, 0x383572, 1.50f, 0, -1},
    {10, 10, 0x383572, 1.50f, 0, -1},
    {20, 2, 0x656579, 1.00f, 0, -1},
    {22, 5, 0xDED7FF, 1.50f, 0, -1},
    {27, 5, 0xDED7FF, 1.50f, 0, -1},
    {32, 2, 0xDED7FF, 1.00f, 0, -1},
};
struct MNFill { int first, count; unsigned rgb; int group; };
static const MNFill MN_FILLS[] = {
    {0, 10, 0x1D2040, -1},
    {10, 10, 0x1D2040, -1},
    {22, 5, 0x1D2040, -1},
    {27, 5, 0x383572, -1},
};
struct MNCircle { float x, y, r; unsigned rgb; int filled; float width; int group; };
static const MNCircle MN_CIRCLES[] = {
    {40.00f, 36.00f, 11.00f, 0xDED7FF, 1, 1.00f, -1},
    {286.00f, 64.00f, 4.00f, 0x383572, 1, 1.00f, -1},
};
/* kolejnosc rysowania jak w dokumencie: rodzaj (0 wypelnienie, 1 kreska, 2 okrag), indeks w swojej tablicy */
struct MNItem { int kind, idx; };
static const MNItem MN_ORDER[] = {
    {2, 0},
    {2, 1},
    {0, 0},
    {1, 0},
    {0, 1},
    {1, 1},
    {1, 2},
    {0, 2},
    {1, 3},
    {0, 3},
    {1, 4},
    {1, 5},
};
