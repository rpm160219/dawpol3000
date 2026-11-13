/* Maiko #250 original custom static art and recovered procedural plot geometry.
 * Re-rendered through kennel's raster; no TE font data is bundled. README.md. */
#include "op1_synth.h"
#include "crush_dsp.h"
/* OLED correction: total lift 12px; lower 40px readouts get another 4px. */
#define SCREEN_LIFT 12
#ifdef OP1_HOST
extern void crush_host_knobs(int16_t *);
extern void *crush_host_raster(void);
extern void crush_host_line(void *,const op1_pt *,const op1_pt *,int32_t,const uint8_t *);
extern void crush_host_text(void *,int,int,int,const char *,const uint8_t *);
extern int16_t crush_host_sine(uint32_t);
#define op1_fx_knobs_live crush_host_knobs
#undef op1_raster
#undef op1_raster_line
#undef op1_screen_draw
#define op1_raster crush_host_raster
#define op1_raster_line crush_host_line
#define op1_screen_draw(s,d) ((void)(s),(void)(d))
#define op1_fx_screen_pos() 0
#endif
extern volatile uint32_t crush_blocks[3];
static uint32_t frame,previous_blocks;
static unsigned idle_frames,text_slot;
#ifndef OP1_HOST
static uint8_t texts[10][OP1_TEXT_SIZE] __attribute__((aligned(4)));
static uint8_t made[10];
static char last_text[10][16];
#endif
static const uint8_t blue[4]={107,142,255,255},green[4]={0,239,148,255},white[4]={255,255,255,255},red[4]={255,65,57,255};
static void text(void *r,int x,int y,int size,const char *str,const uint8_t *c) {
#ifndef OP1_HOST
    unsigned n=text_slot++;if(n>=10)return;
    static const int32_t identity[9]={1024,0,0,0,1024,0,0,0,1024};
    if(!made[n]){op1_text_ctor(texts[n],OP1_Q10(x),y-SCREEN_LIFT,size,c);made[n]=1;}
    unsigned i=0;while(str[i] && i<15 && last_text[n][i]==str[i])i++;
    if(str[i] || last_text[n][i]) {
        i=0;do{last_text[n][i]=str[i];}while(str[i] && ++i<15);
        last_text[n][15]=0;op1_text_set(texts[n],last_text[n]);
    }
    op1_text_draw(texts[n],r,identity);
#else
    crush_host_text(r,x,y-SCREEN_LIFT,size,str,c);
#endif
}
static char *unsigned_text(unsigned n,char *b) {
    char reverse[12];unsigned k=0;do{reverse[k++]=(char)('0'+n%10);n/=10;}while(n);
    unsigned i=0;while(k)b[i++]=reverse[--k];b[i]=0;return b;
}
static void number(void *r,int x,int y,unsigned n,const uint8_t *c) {
    char b[12];text(r,x,y,40,unsigned_text(n,b),c);
}
static void line(void *r,int x,int y,int xx,int yy,const uint8_t *c) {
    op1_pt a={x*64,(y-SCREEN_LIFT*16)*64},b={xx*64,(yy-SCREEN_LIFT*16)*64};op1_raster_line(r,&a,&b,768,c);
}
static int sine(uint32_t phase) {
#ifdef OP1_HOST
    return crush_host_sine(phase);
#else
    return OP1_SINE_TABLE[(phase>>19)&8191];
#endif
}
static void init(void *scr) {(void)scr;frame=idle_frames=0;previous_blocks=crush_blocks[(unsigned)op1_fx_screen_pos()%3];}
static void update(void *scr,int dt) {
    int16_t q[4];op1_fx_knobs_live(q);text_slot=0;frame++;
    op1_screen_draw(scr,dt);void *r=op1_raster();
    int bits=crush_bits(q[0]),drive=(q[2]*24+16383)>>15;
    unsigned rate=(unsigned)((uint32_t)crush_exp2(((q[1]-32767)*14336)>>15)*44100u>>16);
    text(r,14,16,14,"bits",blue);number(r,14,34,(unsigned)bits,white);
    char value[12];unsigned tenth=(rate+50)/100;
    if(tenth>99)unsigned_text((tenth+5)/10,value);
    else {unsigned_text(tenth/10,value);unsigned n=0;while(value[n])n++;
        value[n++]='.';value[n++]=(char)('0'+tenth%10);value[n]=0;}
    text(r,14,92,14,"rate",green);text(r,14,110,40,value,white);
    text(r,246,16,14,"drive",white);number(r,246,34,(unsigned)drive,white);
    text(r,246,92,14,"mix",red);number(r,246,110,((unsigned)q[3]*99+16383)>>15,white);
    /* Original drive rails and waveform viewport: fixed 1/16-pixel geometry. */
    const uint8_t shadow[4]={57,52,115,255};
    line(r,106*16,(33+drive)*16,214*16,(33+drive)*16,shadow);
    line(r,106*16,(120-drive)*16,214*16,(120-drive)*16,shadow);
    uint32_t phase=frame*0x05c28f60u;
    int previous=80*16-((sine(phase+110u*16u*0x03d70a4u)*30)>>11);
    for(int x=112;x<=212;x+=2) {
        int yy=80*16-((sine(phase+(unsigned)x*16u*0x03d70a4u)*30)>>11);
        line(r,(x-2)*16,previous,x*16,yy,white);previous=yy;
    }
    int spacing=rate?524288/(int)rate:1520;if(spacing<16)spacing=16;
    int quant=bits<10?1024>>bits:1,limit=(40-drive)*16;
    int oldx=110*16,oldy=80*16;
    for(int x=110*16;x<=210*16;x+=spacing) {
        int sample=(sine(phase+(uint32_t)x*0x003d70a4u)*30)>>11;
        if(sample>limit)sample=limit;
        if(sample< -limit)sample=-limit;
        if(quant>1)sample=((sample+(quant>>1))>>(10-bits))*quant;
        int yy=80*16-sample;
        if(x>110*16){line(r,oldx,oldy,x,oldy,green);line(r,x,oldy,x,yy,green);}
        oldx=x;oldy=yy;
    }
    if(oldx<210*16)line(r,oldx,oldy,210*16,oldy,green);
    unsigned pos=(unsigned)op1_fx_screen_pos()%3;uint32_t blocks=crush_blocks[pos];
    idle_frames=blocks==previous_blocks?idle_frames+1:0;previous_blocks=blocks;
    if(idle_frames>3) {
        const uint8_t dim[4]={0,0,0,150};for(int y=1;y<160;y++)line(r,0,(y+SCREEN_LIFT)*16,319*16,(y+SCREEN_LIFT)*16,dim);
        text(r,112,76,14,"fx off",blue);
    }
}
const op1_screen_t SCREEN_crush={init,update};
