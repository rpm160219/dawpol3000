/* DAW-POL 3000: zaslepka seq/seq_text.h z op1r dla ekranu mixtur.c. sq_line idzie do funkcji zwrotnej; napisy
 * oryginalu (sq_text: n dzielnikow / F1 F2 RES na dole ekranu) pominiete — dublowalyby galki DAW-POL. */
#ifndef SEQ_TEXT_H
#define SEQ_TEXT_H
#include <stdint.h>
#define sq_line mxw_line
void sq_line(void* r, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, const uint8_t* c);
static inline int32_t sq_text(void* r, int32_t x, int32_t y, int32_t s, const char* t, int32_t w, const uint8_t* c) {
    (void)r; (void)y; (void)s; (void)t; (void)w; (void)c;
    return x;
}
static inline char* sq_num(char* p, uint32_t v, int digits) {
    char t[10]; int n = 0;
    do { t[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (n < digits) t[n++] = '0';
    while (n) *p++ = t[--n];
    *p = 0;
    return p;
}
#define sq_synth_knobs_live op1_synth_knobs_live
#endif
