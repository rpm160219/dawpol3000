/* Maiko #250 algorithm port, authorized by the project maintainer 2026-10-16. See README.md. */
#ifndef CRUSH_DSP_H
#define CRUSH_DSP_H
#include <stdint.h>
typedef struct {
    uint32_t phase;
    int32_t held[2], gain, mix, peak;
} crush_state;
void crush_reset(crush_state *s);
void crush_process(crush_state *s, int16_t *left, int16_t *right, const int16_t q15[4]);
int32_t crush_exp2(int32_t exponent_q11);
int crush_bits(int q15);
#endif
