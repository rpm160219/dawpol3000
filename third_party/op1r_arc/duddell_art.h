/* wygenerowane przez tools/gen/svg_to_art2.py z duddell.svg (grafika: autorzy op1r, GPL-3.0-or-later) */
#pragma once
static const float ARA_PTS[] = {30.00f, 54.00f, 40.00f, 54.00f, 40.00f, 70.00f, 30.00f, 70.00f, 35.00f, 70.00f, 35.00f, 96.00f, 29.00f, 96.00f, 41.00f, 96.00f, 290.00f, 54.00f, 280.00f, 54.00f, 280.00f, 70.00f, 290.00f, 70.00f, 285.00f, 70.00f, 285.00f, 96.00f, 279.00f, 96.00f, 291.00f, 96.00f, 62.20f, 147.80f, 60.80f, 149.20f, 70.00f, 129.00f, 70.00f, 127.00f, 77.80f, 147.80f, 79.20f, 149.20f, 122.20f, 147.80f, 120.80f, 149.20f, 130.00f, 129.00f, 130.00f, 127.00f, 137.80f, 147.80f, 139.20f, 149.20f, 182.20f, 147.80f, 180.80f, 149.20f, 190.00f, 129.00f, 190.00f, 127.00f, 197.80f, 147.80f, 199.20f, 149.20f, 242.20f, 147.80f, 240.80f, 149.20f, 250.00f, 129.00f, 250.00f, 127.00f, 257.80f, 147.80f, 259.20f, 149.20f, 70.00f, 140.00f, 70.00f, 131.00f, 130.00f, 140.00f, 130.00f, 131.00f, 190.00f, 140.00f, 190.00f, 131.00f, 250.00f, 140.00f, 250.00f, 131.00f};
struct ARAStroke { int first, count; unsigned rgb; float width; int closed, group; };
static const ARAStroke ARA_STROKES[] = {
    {0, 4, 0x6F7173, 1.50f, 0, -1},
    {4, 2, 0x6F7173, 1.50f, 0, -1},
    {6, 2, 0x6F7173, 1.50f, 0, -1},
    {8, 4, 0x6F7173, 1.50f, 0, -1},
    {12, 2, 0x6F7173, 1.50f, 0, -1},
    {14, 2, 0x6F7173, 1.50f, 0, -1},
    {16, 2, 0x6F7173, 2.00f, 0, -1},
    {18, 2, 0x6F7173, 2.00f, 0, -1},
    {20, 2, 0x6F7173, 2.00f, 0, -1},
    {22, 2, 0x6F7173, 2.00f, 0, -1},
    {24, 2, 0x6F7173, 2.00f, 0, -1},
    {26, 2, 0x6F7173, 2.00f, 0, -1},
    {28, 2, 0x6F7173, 2.00f, 0, -1},
    {30, 2, 0x6F7173, 2.00f, 0, -1},
    {32, 2, 0x6F7173, 2.00f, 0, -1},
    {34, 2, 0x6F7173, 2.00f, 0, -1},
    {36, 2, 0x6F7173, 2.00f, 0, -1},
    {38, 2, 0x6F7173, 2.00f, 0, -1},
    {40, 2, 0x4D9EFF, 2.00f, 0, -1},
    {42, 2, 0x00ED95, 2.00f, 0, -1},
    {44, 2, 0xFFFFFF, 2.00f, 0, -1},
    {46, 2, 0xFF3A5D, 2.00f, 0, -1},
};
struct ARAFill { int first, count; unsigned rgb; int group; };
static const ARAFill ARA_FILLS[] = {
    {0, 0, 0x000000, -2},  /* pusty wpis (brak elementow; poza kolejnoscia rysowania) */
};
struct ARACircle { float x, y, r; unsigned rgb; int filled; float width; int group; };
static const ARACircle ARA_CIRCLES[] = {
    {70.00f, 140.00f, 2.80f, 0x4D9EFF, 1, 1.00f, -1},
    {130.00f, 140.00f, 2.80f, 0x00ED95, 1, 1.00f, -1},
    {190.00f, 140.00f, 2.80f, 0xFFFFFF, 1, 1.00f, -1},
    {250.00f, 140.00f, 2.80f, 0xFF3A5D, 1, 1.00f, -1},
};
/* kolejnosc rysowania jak w dokumencie: rodzaj (0 wypelnienie, 1 kreska, 2 okrag), indeks w swojej tablicy */
struct ARAItem { int kind, idx; };
static const ARAItem ARA_ORDER[] = {
    {1, 0},
    {1, 1},
    {1, 2},
    {1, 3},
    {1, 4},
    {1, 5},
    {1, 6},
    {1, 7},
    {1, 8},
    {1, 9},
    {1, 10},
    {1, 11},
    {1, 12},
    {1, 13},
    {1, 14},
    {1, 15},
    {1, 16},
    {1, 17},
    {2, 0},
    {2, 1},
    {2, 2},
    {2, 3},
    {1, 18},
    {1, 19},
    {1, 20},
    {1, 21},
};
