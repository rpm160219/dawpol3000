/* wygenerowane przez tools/gen/svg_to_art2.py z ente.svg (grafika: autorzy op1r, GPL-3.0-or-later) */
#pragma once
static const float ENA_PTS[] = {300.00f, 10.00f, 300.00f, 100.00f};
struct ENAStroke { int first, count; unsigned rgb; float width; int closed, group; };
static const ENAStroke ENA_STROKES[] = {
    {0, 2, 0x6F7173, 2.00f, 0, -1},
};
struct ENAFill { int first, count; unsigned rgb; int group; };
static const ENAFill ENA_FILLS[] = {
    {0, 0, 0x000000, -2},  /* pusty wpis (brak elementow; poza kolejnoscia rysowania) */
};
struct ENACircle { float x, y, r; unsigned rgb; int filled; float width; int group; };
static const ENACircle ENA_CIRCLES[] = {
    {0, 0, 0, 0x000000, 0, 0, -2},  /* pusty wpis (brak elementow; poza kolejnoscia rysowania) */
};
/* kolejnosc rysowania jak w dokumencie: rodzaj (0 wypelnienie, 1 kreska, 2 okrag), indeks w swojej tablicy */
struct ENAItem { int kind, idx; };
static const ENAItem ENA_ORDER[] = {
    {1, 0},
};
