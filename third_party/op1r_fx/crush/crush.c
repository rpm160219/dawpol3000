/* Copyright (C) 2026 Hundekraft. Maiko #250 port authorized by owner; README.md. */
#include "op1_plugin.h"
#include "crush_dsp.h"
static crush_state state[3];
volatile uint32_t crush_blocks[3];
static void init(int pos) { crush_reset(&state[pos%3]); }
static void render(int pos,int16_t *left,int16_t *right,const int16_t *p) {
    int16_t q[4];for(int i=0;i<4;i++)q[i]=(int16_t)op1_fx_param_q15(p,i);
    crush_process(&state[pos%3],left,right,q);crush_blocks[pos%3]++;
}
static void mono(int pos,int16_t *buffer,const int16_t *p) { render(pos,buffer,0,p); }
static void stereo(int pos,int16_t *left,int16_t *right,const int16_t *p) { render(pos,left,right,p); }
const op1_fx_stereo_t FX_crush={{init,mono,{OP1_PRESS_NONE,OP1_PRESS_NONE,OP1_PRESS_NONE,OP1_PRESS_NONE}},stereo};
