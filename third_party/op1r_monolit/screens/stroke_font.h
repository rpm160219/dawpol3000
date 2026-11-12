/* DAW-POL 3000: zaslepka screens/stroke_font.h dla monolit.c (tekst na ekranie oryginalu; nasz ekran rysuje napisy
 * wlasna czcionka w core/ui/monolit_screen.cpp). */
#pragma once
#include <stdint.h>
static inline int32_t sf_text(void* r, int32_t x, int32_t y, int32_t s, const char* t, int32_t w, const uint8_t* c) {
    (void)r; (void)y; (void)s; (void)t; (void)w; (void)c; return x;
}
