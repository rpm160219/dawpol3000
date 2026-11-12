/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE.
 * Kennel level stage (2026-10-28, Hundekraft): GIN_FW_OUTPUT_LEVEL_Q12. */
#ifndef GIN_GIN_FW_LEVEL_H
#define GIN_GIN_FW_LEVEL_H
#include <stdint.h>




#ifndef GIN_FW_OUTPUT_SHIFT
#define GIN_FW_OUTPUT_SHIFT 14
#endif
#if GIN_FW_OUTPUT_SHIFT != 14 && GIN_FW_OUTPUT_SHIFT != 15 && GIN_FW_OUTPUT_SHIFT != 16
#error "Gin output shift must be baseline14, diagnostic15 or headroom16"
#endif

/* Kennel level (tester feedback 2026-10-28: Gin much quieter than D-Box and the sampler kits). With
 * GIN_FW_OUTPUT_LEVEL_Q12 defined the stage is (sample * native gain) >> 14, times LEVEL / 4096, then a soft knee:
 * linear up to 24576, a quadratic bend above it (slope 1 at the knee, 0 at full scale, monotone), clamped to int16.
 * Factory kit pads stay under the knee, so their balance does not change; only loud edits reach the bend.
 * Integers only, no division: (sample * gain) < 2^30, >> 14 <= 65534, * LEVEL (< 2^15) < 2^31; e * e <= 2^28. */
#define GIN_FW_KNEE 24576
#define GIN_FW_KNEE_SPAN 16384                  /* e = |v| - knee in 0..16384: knee + e - e^2 / 32768 */
#ifdef GIN_FW_OUTPUT_LEVEL_Q12
#if GIN_FW_OUTPUT_SHIFT != 14 || GIN_FW_OUTPUT_LEVEL_Q12 < 4096 || GIN_FW_OUTPUT_LEVEL_Q12 > 8191
#error "Gin kennel level needs shift 14 and a Q12 level in 1.0 .. 2.0"
#endif
static inline int32_t gin_fw_soft_limit(int32_t v){
 int32_t a=v<0?-v:v,e;
 if(a<=GIN_FW_KNEE)return v;
 e=a-GIN_FW_KNEE;
 a=e>=GIN_FW_KNEE_SPAN?32767:GIN_FW_KNEE+e-((e*e)>>15);
 if(a>32767)a=32767;
 return v<0?-a:a;
}
#endif





static inline int16_t gin_fw_output_sample(int16_t sample,int16_t native_gain){
 int32_t value=((int32_t)sample*native_gain)>>GIN_FW_OUTPUT_SHIFT;
#ifdef GIN_FW_OUTPUT_LEVEL_Q12
 value=gin_fw_soft_limit((value*GIN_FW_OUTPUT_LEVEL_Q12)>>12);
#elif GIN_FW_OUTPUT_SHIFT == 14
 if(value>32767)value=32767;else if(value< -32768)value= -32768;
#endif
 return(int16_t)value;
}
#endif
