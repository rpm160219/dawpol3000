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
const char* pages[]={"OSC 1","OSC 2","FILTER","MIXER","FILTER ENV","AMP ENV","LFO","VOICE","PRESETS"};
constexpr int NONE=-1,DEPTH=-2,DEST=-3;
const int lfoTargets[]={PVX_LFO_TO_PITCH,PVX_LFO_TO_OSC2,PVX_LFO_TO_FILT,PVX_LFO_TO_AMP};
const char* targetNames[]={"OSC 1","OSC 2","FILTER","AMP"};
const char* hints[8][2]={
    {"L3 = OSC 1 LFO DEPTH","L3 = WAVE / RANGE / LEVEL / CROSS-MOD"},
    {"L3 = LFO DEPTH / FINE / PWM","L3 = WAVE / RANGE / LEVEL / DETUNE"},
    {"L3 = KEYTRACK / LFO DEPTH","L3 = CUTOFF / RESONANCE / ENV / MODE"},
    {"LEVELS DRIVE THE FILTER   RING = EXTRA","LEVELS DRIVE THE FILTER   RING = EXTRA"},
    {"L3 = SINGLE / REPEAT","REPEAT SETS SUSTAIN TO ZERO"},
    {"L3 = SINGLE / REPEAT / VCA HOLD","REPEAT SETS SUSTAIN TO ZERO"},
    {"L3 = SYNC TO BPM","L3 = FREE RUN"},
    {"L3 = VELOCITY / EXCITE / VCA HOLD","L3 = TUNE / GLIDE / KEY MODE / VINTAGE"}
};
const int params[8][2][4]={
    {{PVX_OSC1_WAVE,PVX_OSC1_OCT,PVX_MIX_OSC1,PVX_XMOD_DEPTH},{PVX_LFO_TO_PITCH,NONE,NONE,NONE}},
    {{PVX_OSC2_WAVE,PVX_OSC2_OCT,PVX_MIX_OSC2,PVX_OSC2_DETUNE},{PVX_LFO_TO_OSC2,PVX_OSC2_FINE,PVX_OSC2_PWM,NONE}},
    {{PVX_FILT_CUTOFF,PVX_FILT_RES,PVX_FILT_ENV_AMT,PVX_FILT_MODE},{PVX_FILT_KEYTRACK,PVX_LFO_TO_FILT,NONE,NONE}},
    {{PVX_MIX_OSC1,PVX_MIX_OSC2,PVX_MIX_NOISE,PVX_MIX_RING},{NONE,NONE,NONE,NONE}},
    {{PVX_FENV_ATTACK,PVX_FENV_DECAY,PVX_FENV_SUSTAIN,PVX_FENV_RELEASE},{PVX_FENV_REPEAT,NONE,NONE,NONE}},
    {{PVX_AENV_ATTACK,PVX_AENV_DECAY,PVX_AENV_SUSTAIN,PVX_AENV_RELEASE},{PVX_AENV_REPEAT,PVX_VCA_HOLD,NONE,NONE}},
    {{PVX_LFO_RATE,DEPTH,PVX_LFO_WAVE,DEST},{NONE,NONE,NONE,NONE}},
    {{PVX_MASTER_TUNE,PVX_GLIDE,PVX_DUOPHONY,PVX_VINTAGE},{PVX_VELOCITY,PVX_FILT_AUTOOSC,PVX_VCA_HOLD,NONE}}
};
const char* labels[8][2][4]={
    {{"WAVE","RANGE","LEVEL","CROSS-MOD"},{"LFO DEPTH","","",""}},
    {{"WAVE","RANGE","LEVEL","DETUNE"},{"LFO DEPTH","FINE","PWM",""}},
    {{"CUTOFF","RESONANCE","ENV MOD","MODE"},{"KEYTRACK","LFO DEPTH","",""}},
    {{"OSC 1 LVL","OSC 2 LVL","NOISE","RING MOD"},{"","","",""}},
    {{"ATTACK","DECAY","SUSTAIN","RELEASE"},{"ENV MODE","","",""}},
    {{"ATTACK","DECAY","SUSTAIN","RELEASE"},{"ENV MODE","VCA HOLD","",""}},
    {{"RATE","DEPTH","SHAPE","DEST"},{"","","",""}},
    {{"TUNE","GLIDE","KEY MODE","VINTAGE"},{"VELOCITY","EXCITE","VCA HOLD",""}}
};
int layerFor(int page,bool alt) { return alt && page!=PV_PAGE_MIXER && page!=PV_PAGE_LFO ? 1 : 0; }
int choices(int p) {
    if(p==PVX_OSC1_OCT || p==PVX_OSC2_OCT || p==PVX_OSC1_WAVE || p==PVX_OSC2_WAVE) return 5;
    if(p==PVX_LFO_WAVE) return 4;
    if(p==PVX_LFO_DIV) return POLIVOKS_LFO_DIVISIONS;
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
    if(p==PVX_LFO_DIV) { std::snprintf(out,size,"%s",polivoksLfoDivisionName(word)); return; }
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
void pageDiagram(Canvas& c,const int16_t* p,int page,int dest,float liveLfo) {
    constexpr float x=455,y=350,w=158,h=38;
    if(page==PV_PAGE_FENV || page==PV_PAGE_AENV) {
        int base=page==PV_PAGE_AENV?PVX_AENV_ATTACK:PVX_FENV_ATTACK;
        envelope(c,p,base,p[page==PV_PAGE_AENV?PVX_AENV_REPEAT:PVX_FENV_REPEAT]>=16384);
    } else if(page==PV_PAGE_OSC1 || page==PV_PAGE_OSC2) {
        int osc=page==PV_PAGE_OSC2;
        drawPolivoksWave(c,p[osc?PVX_OSC2_WAVE:PVX_OSC1_WAVE]*5/32768,x,y,w,h,osc?pal::green:pal::blue);
        char text[24]; polivoksWaveName(p[osc?PVX_OSC2_WAVE:PVX_OSC1_WAVE],text,sizeof(text));
        drawText(c,x+w,y+h+5,text,7,1,pal::dim,ALIGN_RIGHT);
    } else if(page==PV_PAGE_LFO) {
        float pts[400]; int n=0; uint32_t seed=7; float random=0;
        int shape=p[PVX_LFO_WAVE]*4/32768;
        float depth=p[lfoTargets[dest]]/32767.f,amp=14*(.25f+.75f*depth);
        float previous=0;
        for(int i=0;i<100;++i) {
            float t=i/99.f,phase=std::fmod(t*2,1.f),v;
            if(shape==PVX_LFO_TRI) v=1-4*std::fabs(phase-.5f);
            else if(shape==PVX_LFO_SQUARE) v=phase<.5f?1:-1;
            else {
                if(shape==PVX_LFO_NOISE || i%12==0) {seed=seed*1664525u+1013904223u;random=((seed>>8)/8388608.f)-1;}
                v=random;
            }
            float px=x+t*w;
            if(i && shape!=PVX_LFO_TRI) {pts[2*n]=px;pts[2*n+1]=y+h*.5f-previous*amp;++n;}
            pts[2*n]=px;pts[2*n+1]=y+h*.5f-v*amp;++n; previous=v;
        }
        c.line(x,y+h*.5f,x+w,y+h*.5f,1,pal::faint);
        c.polyline(pts,n,1.35f,depth>0?pal::green:pal::silver);
        c.fillCircle(x+w+6,y+h*.5f-liveLfo*14,1.6f,pal::white);
        char routes[48];std::snprintf(routes,sizeof(routes),"OSC 1:%d OSC 2:%d FLT:%d AMP:%d",p[lfoTargets[0]]*100/32767,p[lfoTargets[1]]*100/32767,p[lfoTargets[2]]*100/32767,p[lfoTargets[3]]*100/32767);
        drawText(c,x+w,y+h+6,routes,6,1,pal::dim,ALIGN_RIGHT);
    } else if(page==PV_PAGE_MIXER) {
        const int ids[]={PVX_MIX_OSC1,PVX_MIX_OSC2,PVX_MIX_NOISE,PVX_MIX_RING};
        const char* names[]={"I","II","N","R"};
        for(int i=0;i<4;++i) {float level=p[ids[i]]/32767.f,px=x+16+i*38; c.fillRect(px,y+30-30*level,12,30*level,pal::enc[i]);drawText(c,px+6,y+38,names[i],7,1,pal::dim,ALIGN_CENTER);}
    } else if(page==PV_PAGE_FILTER) {
        float cut=p[PVX_FILT_CUTOFF]/32767.f,peak=8*p[PVX_FILT_RES]/32767.f;
        float knee=x+18+cut*(w-42),mid=y+18;
        float lp[]={x,mid,knee-5,mid,knee,mid-peak,knee+7,mid+4,x+w,y+h};
        float bp[]={x,y+h,knee-12,mid+3,knee,mid-peak,knee+12,mid+3,x+w,y+h};
        c.polyline(p[PVX_FILT_MODE]>=16384?bp:lp,5,1.3f,pal::silver);
        drawText(c,x+w,y+h+5,p[PVX_FILT_MODE]>=16384?"BAND-PASS":"LOW-PASS",7,1,pal::dim,ALIGN_RIGHT);
    } else {
        drawText(c,x+w,y+10,p[PVX_DUOPHONY]>=16384?"DUO":"MONO",8,1,pal::silver,ALIGN_RIGHT);
        char vintage[24];std::snprintf(vintage,sizeof(vintage),"VINTAGE %d%%",p[PVX_VINTAGE]*100/32767);
        drawText(c,x+w,y+30,vintage,7,1,pal::dim,ALIGN_RIGHT);
    }
}
}
void SynthView::syncPolivoksUi(const int16_t* words) {
    if(pvUiPage_==page_ && pvUiTrack_==track_) return;
    pvUiPage_=page_;pvUiTrack_=track_;
    for(auto& step:stepPv_) step=RelEncoder();
    if(page_==PV_PAGE_LFO) {
        pvLfoDest_=0;
        for(int i=1;i<4;++i) if(words[lfoTargets[i]]>words[lfoTargets[pvLfoDest_]]) pvLfoDest_=i;
    }
}
void SynthView::updatePolivoks(const Buttons& b,const InputState& in,float dt,Track& t) {
    polivoksEnsure(t);
    syncPolivoksUi(t.polivoks);
    AudioEngine* e=app_->engine();
    if(b.pressed(B_START)) app_->togglePlaySong();
    if(b.pressed(B_L3)) {
        if(page_==PV_PAGE_LFO) {t.polivoks[PVX_LFO_SYNC]=t.polivoks[PVX_LFO_SYNC]>=16384?0:32767;app_->markDirty();}
        else alt_=!alt_;
        for(auto& step:stepPv_) step=RelEncoder();
    }
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
    if(page_<0 || page_>=PV_PAGE_PRESETS) return;
    bool changed=false;
    for(int i=0;i<4;++i) {
        int param=params[page_][layerFor(page_,alt_)][i];
        if(page_==PV_PAGE_LFO && i==0 && t.polivoks[PVX_LFO_SYNC]>=16384) param=PVX_LFO_DIV;
        float axis=in.axis[axes[i]]*signs[i];
        if(param==NONE) continue;
        if(param==DEST) {
            int step=stepPv_[i].steps(axis,dt,5);
            if(step) pvLfoDest_=std::clamp(pvLfoDest_+step,0,3);
            continue; // editor selection only; all four native depths remain active
        }
        if(param==DEPTH) param=lfoTargets[pvLfoDest_];
        int count=choices(param),next=t.polivoks[param];
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
            if(next>0 && param==PVX_FENV_SUSTAIN) t.polivoks[PVX_FENV_REPEAT]=0;
            if(next>0 && param==PVX_AENV_SUSTAIN) t.polivoks[PVX_AENV_REPEAT]=0;
        }
    }
    if(changed) app_->markDirty();
}
void SynthView::renderPolivoks(Canvas& c,const Track& t) {
    Project& p=app_->project(); AudioEngine* e=app_->engine();
    int16_t defaults[POLIVOKS_NUM_PARAMS];
    if(!t.polivoksSet) polivoksDefaultParams(defaults);
    const int16_t* words=t.polivoksSet?t.polivoks:defaults;
    syncPolivoksUi(words);
    if(e) e->polivoksView(track_,pvView_);
    pv_.draw(c,words,pvView_,dt_);
    bool playing=e && e->isPlaying(); int tick=playing?e->playTick():0;
    c.fillRect(0,0,640,44,pal::bg);
    screenHeader(c,ICON_KEYS,pal::green,"POLIVOKS",t.name,pal::track[t.color%pal::TRACK_COLORS],tick/TICKS_PER_BAR+1,(tick%TICKS_PER_BAR)/PPQ+1,playing,p.bpm);
    c.fillRect(0,334,640,146,pal::bg);
    int page=std::clamp(page_,0,(int)PV_PAGE_PRESETS-1),layer=layerFor(page,alt_);
    drawText(c,20,334,t.preset[0]?t.preset:"INIT",8,1,pal::tan);
    drawText(c,20,350,pages[page],13,1.5f,pal::white);
    for(int i=0;i<PV_PAGE_COUNT;++i) { if(i==page) c.fillCircle(24+i*12,376,2.6f,pal::white); else c.circle(24+i*12,376,2.2f,1,pal::dim); }
    drawText(c,137,372,"SELECT = NEXT PAGE",7,1,pal::dim);
    const char* hint=page==PV_PAGE_LFO ? hints[page][words[PVX_LFO_SYNC]>=16384?1:0] : hints[page][layer];
    drawText(c,20,397,hint,7,1,layer?pal::blue:pal::dim);
    pageDiagram(c,words,page,pvLfoDest_,pvView_.state.lfo);
    if(t.octave) { char oct[20]; std::snprintf(oct,sizeof(oct),"OCT %+d",t.octave); drawText(c,400,368,oct,8,1,pal::white,ALIGN_RIGHT); }
    char text[4][24]; const char* values[4]; const char* controlLabels[4]; float positions[4];
    for(int i=0;i<4;++i) {
        int param=params[page][layer][i];
        controlLabels[i]=labels[page][layer][i];
        if(page==PV_PAGE_LFO && i==0 && words[PVX_LFO_SYNC]>=16384) {param=PVX_LFO_DIV;controlLabels[i]="SYNC";}
        if(param==DEST) { std::snprintf(text[i],sizeof(text[i]),"%s",targetNames[pvLfoDest_]); positions[i]=pvLfoDest_/3.f; }
        else if(param==NONE) { text[i][0]=0; positions[i]=-1; }
        else {
            if(param==DEPTH) param=lfoTargets[pvLfoDest_];
            value(param,words[param],text[i],sizeof(text[i]));
            int count=choices(param);
            positions[i]=count>1 ? (words[param]*count/32768)/(float)(count-1) : words[param]/32767.f;
        }
        values[i]=text[i];
    }
    encoderBar(c,controlLabels,values,positions);
}
} // namespace dp
