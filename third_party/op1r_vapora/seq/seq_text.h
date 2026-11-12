/* DAW-POL 3000: zaslepka seq/seq_text.h z op1r (tylko sq_cat / sq_num, ten sam kod) */
#ifndef SEQ_TEXT_H
#define SEQ_TEXT_H
#include <stdint.h>
static inline char *sq_cat(char *p, const char *s) { while (*s) *p++ = *s++; *p = 0; return p; }
static inline char *sq_num(char *p, uint32_t v, int digits)
{
    char t[10]; int n = 0;
    do { t[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (n < digits) t[n++] = '0';
    while (n) *p++ = t[--n];
    *p = 0;
    return p;
}
#endif
