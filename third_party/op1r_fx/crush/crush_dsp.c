/* Maiko #250 Crush DSP, reconstructed from the permission-authorized custom
 * region (init 0x01e4fd3c/render 0x01e4fd60). No TE code/data is imported.
 * Explicit scalar fixed-point operations replace Blackfin saturating intrinsics;
 * block ramps and per-channel sample holds retain the original algorithm.
 * Copyright (C) 2026 Hundekraft. GPL-3.0-or-later with OP-1 firmware permission. */
#include "crush_dsp.h"
static const int32_t exp2_table[33] = {65536, 66971, 68438, 69936, 71468, 73032, 74632, 76266, 77936, 79642, 81386, 83169, 84990, 86851, 88752, 90696, 92682, 94711, 96785, 98905, 101070, 103283, 105545, 107856, 110218, 112631, 115098, 117618, 120194, 122825, 125515, 128263, 131072};
int32_t crush_exp2(int32_t x) {
    unsigned i = ((uint32_t)x >> 6) & 31, f = (uint32_t)x & 63;
    uint32_t y = exp2_table[i] + ((exp2_table[i+1]-exp2_table[i]) * f >> 6);
    int oct = x >> 11;
    return (int32_t)(oct < 0 ? y >> -oct : y << oct);
}
int crush_bits(int q) { return 1 + ((q * 15 + 16384) >> 15); }
void crush_reset(crush_state *s) {
    s->phase=65535;s->held[0]=s->held[1]=0;s->gain=-1;s->mix=s->peak=0;
}
static int32_t sat16(int32_t x) { return x>32767?32767:x< -32768?-32768:x; }
static int32_t driven(int32_t x,int32_t gain,int bits) {
    x=(x*gain)>>8;
    int sign=x<0,mag=sign?-x:x;
    if(mag>24576) {
        int t=mag-24576;if(t>16384)t=16384;
        mag=24576+t-((t*t)>>15);if(mag>32767)mag=32767;
        x=sign?-mag:mag;
    }
    if(bits<16) {
        int shift=16-bits,unit=1<<shift;
        x=sat16(((x+(unit>>1))>>shift)*unit);
    }
    return x;
}
void crush_process(crush_state *s,int16_t *left,int16_t *right,const int16_t q[4]) {
    int bits=crush_bits(q[0]);
    uint32_t increment=(uint32_t)crush_exp2(((q[1]-32767)*14336)>>15);
    int gain=crush_exp2(q[2]>>2)>>8,mix=q[3];
    if(s->gain<0){s->gain=gain;s->mix=mix;}
    int dg=gain-s->gain,dm=mix-s->mix,peak=0;
    uint32_t end_phase=s->phase;
    for(int channel=0;channel<(right?2:1);channel++) {
        int16_t *buffer=channel?right:left;
        uint32_t phase=s->phase;int held=s->held[channel];
        int gain64=s->gain*64,mix64=s->mix*64;
        for(int n=0;n<64;n++) {
            int dry=buffer[n];gain64+=dg;mix64+=dm;phase+=increment;
            if(phase>=65536){phase-=65536;held=driven(dry,gain64>>6,bits);}
            int magnitude=held<0?-held:held;if(magnitude>peak)peak=magnitude;
            /* The original halves the difference before its Q14 multiply. */
            int difference=(held-dry)>>1;
            buffer[n]=(int16_t)sat16(dry+((difference*(mix64>>6))>>14));
        }
        s->held[channel]=held;end_phase=phase;
    }
    s->phase=end_phase;s->gain=gain;s->mix=mix;s->peak=peak;
}
