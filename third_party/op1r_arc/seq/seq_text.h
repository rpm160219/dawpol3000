/* DAW-POL 3000: zaslepka seq/seq_text.h z op1r dla ekranu arc.c: sq_line = linia; napis proporcji (sq_text) i obrot
 * wskazowek tarcz (sq_node_rotate) pominiete — tarcze z dolu oryginalu dublowalyby galki DAW-POL. */
#ifndef SEQ_TEXT_H
#define SEQ_TEXT_H
#include <stdint.h>
static inline void sq_line(void* r, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, const uint8_t* c) {
    op1_pt a = {x0, y0}, b = {x1, y1};
    op1_raster_line(r, &a, &b, w, c);
}
static inline int32_t sq_text(void* r, int32_t x, int32_t y, int32_t s, const char* t, int32_t w, const uint8_t* c) {
    (void)r; (void)y; (void)s; (void)t; (void)w; (void)c;
    return x;
}
static inline void sq_node_rotate(void* n, int32_t cx, int32_t cy, int32_t s15, int32_t c15) {
    (void)n; (void)cx; (void)cy; (void)s15; (void)c15;
}
#define sq_synth_knobs_live op1_synth_knobs_live
#endif
