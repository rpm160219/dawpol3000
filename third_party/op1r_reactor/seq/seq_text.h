/* DAW-POL 3000: zaslepka seq/seq_text.h dla reactor.c (tekst na ekranie oryginalu; nasz ekran rysuje odczyty
 * wlasna czcionka w core/ui/reactor_screen.cpp). Tylko funkcje, ktorych uzywa reactor.c. */
#ifndef SEQ_TEXT_STUB_H
#define SEQ_TEXT_STUB_H
#include <stdint.h>
static inline char* sq_cat(char* p, const char* s) { while (*s) *p++ = *s++; *p = 0; return p; }
static inline char* sq_num(char* p, uint32_t v, int digits) { (void)v; (void)digits; *p = 0; return p; }
static inline int32_t sq_text(void* r, int32_t x, int32_t y, int32_t s, const char* t, int32_t w, const uint8_t* c) {
    (void)r; (void)y; (void)s; (void)t; (void)w; (void)c; return x;
}
#endif
