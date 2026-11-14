#include "ui/synth_view.h"
#include "ui/app.h"
#include "gfx/font.h"
#include "gfx/icons.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
namespace dp {
namespace {
const int keys[]={B_LEFT,B_DOWN,B_RIGHT,B_UP,B_Y,B_B,B_A,B_X};
const int scale[]={0,2,4,5,7,9,11,12};
const int axes[]={AX_LX,AX_LY,AX_RX,AX_RY};
const float signs[]={1,-1,1,-1};
const char* pages[]={"SOUND / FILTER","OSCILLATORS","MIXER","FILTER ENV","AMP ENV","MODULATOR","CONTROL","PRESETS"};
const char* hints[7][2]={
    {"L3 = KEYTRACK / AUTO-OSC / GLIDE / DUO","L3 = CUTOFF / RESONANCE / ENV / MODE"},
    {"L3 = REGISTERS / FINE / MASTER TUNE","L3 = WAVES / DETUNE / CROSS-MOD"},
    {"L3 = OUTPUT / EXTERNAL / PWM / VELOCITY","L3 = OSC 1 / OSC 2 / RING / NOISE"},
    {"L3 = ENV DEPTH / REPEAT","REPEAT SETS SUSTAIN TO ZERO"},
    {"L3 = VCA HOLD / REPEAT","REPEAT SETS SUSTAIN TO ZERO"},
    {"L3 = OSC 2 / TREMOLO / PWM","L3 = RATE / WAVE / FILTER / OSC 1"},
    {"L3 = KEYTRACK / VELOCITY / HOLD / AUTO-OSC","L3 = TUNE / GLIDE / MONO-DUO / LEVEL"}
};
const int params[7][2][4]={
    {{PVX_FILT_CUTOFF,PVX_FILT_RES,PVX_FILT_ENV_AMT,PVX_FILT_MODE},{PVX_FILT_KEYTRACK,PVX_FILT_AUTOOSC,PVX_GLIDE,PVX_DUOPHONY}},
    {{PVX_OSC1_WAVE,PVX_OSC2_WAVE,PVX_OSC2_DETUNE,PVX_XMOD_DEPTH},{PVX_OSC1_OCT,PVX_OSC2_OCT,PVX_OSC2_FINE,PVX_MASTER_TUNE}},
    {{PVX_MIX_OSC1,PVX_MIX_OSC2,PVX_MIX_RING,PVX_MIX_NOISE},{PVX_MASTER_LEVEL,PVX_MIX_EXTERNAL,PVX_OSC2_PWM,PVX_VELOCITY}},
    {{PVX_FENV_ATTACK,PVX_FENV_DECAY,PVX_FENV_SUSTAIN,PVX_FENV_RELEASE},{PVX_FENV_ATTACK,PVX_FENV_DECAY,PVX_FILT_ENV_AMT,PVX_FENV_REPEAT}},
    {{PVX_AENV_ATTACK,PVX_AENV_DECAY,PVX_AENV_SUSTAIN,PVX_AENV_RELEASE},{PVX_AENV_ATTACK,PVX_AENV_DECAY,PVX_VCA_HOLD,PVX_AENV_REPEAT}},
    {{PVX_LFO_RATE,PVX_LFO_WAVE,PVX_LFO_TO_FILT,PVX_LFO_TO_PITCH},{PVX_LFO_RATE,PVX_LFO_TO_OSC2,PVX_LFO_TO_AMP,PVX_OSC2_PWM}},
    {{PVX_MASTER_TUNE,PVX_GLIDE,PVX_DUOPHONY,PVX_MASTER_LEVEL},{PVX_FILT_KEYTRACK,PVX_VELOCITY,PVX_VCA_HOLD,PVX_FILT_AUTOOSC}}
};
const char* labels[7][2][4]={
    {{"CUTOFF","RESONANCE","ENV MOD","MODE"},{"KEYTRACK","AUTO-OSC","GLIDE","DUOPHONY"}},
    {{"OSC 1 WAVE","OSC 2 WAVE","DETUNE 2","CROSS-MOD"},{"OSC 1 OCT","OSC 2 OCT","FINE 2","TUNE"}},
    {{"OSC 1 LVL","OSC 2 LVL","RING MOD","NOISE"},{"OUTPUT","EXTERNAL","OSC 2 PWM","VELOCITY"}},
    {{"ATTACK","DECAY","SUSTAIN","RELEASE"},{"ATTACK","DECAY","ENV MOD","ENV MODE"}},
    {{"ATTACK","DECAY","SUSTAIN","RELEASE"},{"ATTACK","DECAY","VCA HOLD","AMP MODE"}},
    {{"LFO RATE","LFO WAVE","TO FILTER","TO OSC 1"},{"LFO RATE","TO OSC 2","TO AMP","OSC 2 PWM"}},
    {{"TUNE","GLIDE","KEY MODE","OUTPUT"},{"KEYTRACK","VELOCITY","VCA HOLD","AUTO-OSC"}}
};
int choices(int p) {
    if(p==PVX_OSC1_OCT || p==PVX_OSC2_OCT || p==PVX_OSC1_WAVE || p==PVX_OSC2_WAVE) return 5;
    if(p==PVX_LFO_WAVE) return 4;
    if(p==PVX_FILT_MODE || p==PVX_FILT_AUTOOSC || p==PVX_FENV_REPEAT || p==PVX_AENV_REPEAT || p==PVX_DUOPHONY || p==PVX_VCA_HOLD) return 2;
    return 0;
}
void value(int p,int word,char* out,int size) {
    float k=word/32767.f,b=(word-16384)/16384.f;
    int n=choices(p),index=n ? word*n/32768 : 0;
    if(p==PVX_OSC1_WAVE || p==PVX_OSC2_WAVE) { polivoksWaveName(word,out,size); return; }
    if(p==PVX_OSC1_OCT || p==PVX_OSC2_OCT) { const char* names[]={"32'","16'","8'","4'","2'"}; std::snprintf(out,size,"%s",names[index]); return; }
    if(p==PVX_LFO_WAVE) { const char* names[]={"TRI","SQR","S&H","NOISE"}; std::snprintf(out,size,"%s",names[index]); return; }
    if(p==PVX_FILT_MODE) { std::snprintf(out,size,"%s",index?"BP6":"LP12"); return; }
    if(p==PVX_DUOPHONY) { std::snprintf(out,size,"%s",index?"DUO":"MONO"); return; }
    if(p==PVX_FENV_REPEAT || p==PVX_AENV_REPEAT) { std::snprintf(out,size,"%s",index?"REPEAT":"SINGLE"); return; }
    if(n==2) { std::snprintf(out,size,"%s",index?"ON":"OFF"); return; }
    if(p==PVX_FILT_CUTOFF) { float hz=polivoksCutoffHz(word); if(hz>=1000) std::snprintf(out,size,"%.2f KHZ",hz/1000); else std::snprintf(out,size,"%.0f HZ",hz); return; }
    if(p==PVX_LFO_RATE) { std::snprintf(out,size,"%.2f HZ",.1f*std::pow(350.f,k)); return; }
    if(p==PVX_OSC2_DETUNE || p==PVX_MASTER_TUNE) { std::snprintf(out,size,"%+.2f ST",b*(p==PVX_OSC2_DETUNE?7:2)); return; }
    if(p==PVX_OSC2_FINE) { std::snprintf(out,size,"%+.1f C",50*b); return; }
    if(p==PVX_FILT_ENV_AMT) { std::snprintf(out,size,"%+.0f%%",100*b); return; }
    float seconds=-1;
    if(p==PVX_GLIDE) seconds=word<33 ? 0 : .001f+1.5f*k*k;
    if(p==PVX_FENV_ATTACK || p==PVX_AENV_ATTACK) seconds=.001f+2.999f*k*k*k;
    if(p==PVX_FENV_DECAY || p==PVX_FENV_RELEASE || p==PVX_AENV_DECAY || p==PVX_AENV_RELEASE) seconds=.002f+11.998f*k*k*k;
    if(seconds>=0) { if(seconds==0) std::snprintf(out,size,"OFF"); else if(seconds<1) std::snprintf(out,size,"%.0f MS",seconds*1000); else std::snprintf(out,size,"%.2f S",seconds); return; }
    std::snprintf(out,size,"%.0f%%",k*100);
}
void envelope(Canvas& c,const int16_t* p,int base,bool repeat) {
    const float x=455,y=354,w=158,h=39,s=p[base+2]/32767.f;
    float pts[]={x,y+h,x+w*.16f,y,x+w*.48f,y+h*(1-s),x+w*.74f,y+h*(1-s),x+w,y+h};
    if(repeat && s==0) {
        float loop[]={x,y+h,x+w*.14f,y,x+w*.49f,y+h,x+w*.64f,y,x+w,y+h};
        c.polyline(loop,5,1.25f,pal::silver);
    } else c.polyline(pts,5,1.25f,pal::silver);
    drawText(c,x+w,y+h+6,repeat?"REPEAT A-D":"ADSR",7,1,pal::dim,ALIGN_RIGHT);
}
}
void SynthView::updatePolivoks(const Buttons& b,const InputState& in,float dt,Track& t) {
    polivoksEnsure(t);
    AudioEngine* e=app_->engine();
    if(b.pressed(B_START)) app_->togglePlaySong();
    if(b.pressed(B_L3)) alt_=!alt_;
    if(b.pressed(B_L1) && t.octave>-3) { --t.octave; app_->markDirty(); }
    if(b.pressed(B_R1) && t.octave<3) { ++t.octave; app_->markDirty(); }
    for(int i=0;i<8;++i) {
        if(b.pressed(keys[i]) && e) {
            int pitch=12*(octave_+1)+scale[i];
            if(held_[i]>=0) e->send(EC_NOTE_OFF,track_,held_[i]);
            e->send(EC_NOTE_ON,track_,pitch,0,.85f); held_[i]=pitch;
        }
        if(b.released(keys[i]) && held_[i]>=0) { if(e) e->send(EC_NOTE_OFF,track_,held_[i]); held_[i]=-1; }
    }
    if(page_<0 || page_>6) return;
    bool changed=false;
    for(int i=0;i<4;++i) {
        int param=params[page_][alt_?1:0][i],count=choices(param);
        float axis=in.axis[axes[i]]*signs[i];
        int next=t.polivoks[param];
        if(count) {
            int d=stepPv_[i].steps(axis,dt,5);
            if(d) {
                int index=std::clamp(next*count/32768+d,0,count-1);
                next=count==2 ? index*32767 : polivoksChoice(index,count);
            }
        } else next=std::clamp(next+(int)std::lround(RelEncoder::delta(axis,dt,.7f)*32767),0,32767);
        if(next!=t.polivoks[param]) {
            t.polivoks[param]=(int16_t)next; changed=true;
            if(next>=16384 && param==PVX_FENV_REPEAT) t.polivoks[PVX_FENV_SUSTAIN]=0;
            if(next>=16384 && param==PVX_AENV_REPEAT) t.polivoks[PVX_AENV_SUSTAIN]=0;
        }
    }
    if(changed) app_->markDirty();
}
void SynthView::renderPolivoks(Canvas& c,const Track& t) {
    Project& p=app_->project(); AudioEngine* e=app_->engine();
    int16_t defaults[POLIVOKS_NUM_PARAMS];
    if(!t.polivoksSet) polivoksDefaultParams(defaults);
    const int16_t* words=t.polivoksSet?t.polivoks:defaults;
    if(e) e->polivoksView(track_,pvView_);
    pv_.draw(c,words,pvView_,dt_);
    bool playing=e && e->isPlaying(); int tick=playing?e->playTick():0;
    c.fillRect(0,0,640,44,pal::bg);
    screenHeader(c,ICON_KEYS,pal::green,"POLIVOKS",t.name,pal::track[t.color%pal::TRACK_COLORS],tick/TICKS_PER_BAR+1,(tick%TICKS_PER_BAR)/PPQ+1,playing,p.bpm);
    c.fillRect(0,334,640,146,pal::bg);
    int page=std::clamp(page_,0,6),layer=alt_?1:0;
    drawText(c,20,342,pages[page],13,1.5f,pal::white);
    for(int i=0;i<8;++i) { if(i==page) c.fillCircle(24+i*12,366,2.6f,pal::white); else c.circle(24+i*12,366,2.2f,1,pal::dim); }
    drawText(c,125,362,"SELECT = NEXT PAGE",7,1,pal::dim);
    drawText(c,20,389,hints[page][layer],7,1,layer?pal::blue:pal::dim);
    if(t.preset[0]) drawText(c,400,342,t.preset,8,1,pal::tan,ALIGN_RIGHT);
    int base=page==4?PVX_AENV_ATTACK:PVX_FENV_ATTACK;
    envelope(c,words,base,words[page==4?PVX_AENV_REPEAT:PVX_FENV_REPEAT]>=16384);
    if(t.octave) { char oct[20]; std::snprintf(oct,sizeof(oct),"OCT %+d",t.octave); drawText(c,400,368,oct,8,1,pal::white,ALIGN_RIGHT); }
    char text[4][24]; const char* values[4]; float positions[4];
    for(int i=0;i<4;++i) { int param=params[page][layer][i]; value(param,words[param],text[i],sizeof(text[i])); values[i]=text[i]; positions[i]=words[param]/32767.f; }
    encoderBar(c,labels[page][layer],values,positions);
}
} // namespace dp
