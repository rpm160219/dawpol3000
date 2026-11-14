#include "synth/polivoks_synth.h"
#include "synth/polivoks_presets.h"
#include "io/project_file.h"
#include "io/preset_store.h"
#include "audio/engine.h"
#include "ui/app.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
using namespace dp;
#include "polivoks_approved.inc"
static void check(bool x,const char* text) { if(!x) { std::fprintf(stderr,"FAIL %s\n",text); std::exit(1); } }
// Check visible behavior with an identical audio snapshot: resonance changes
// the optical circuits alone, and cutoff moves the actual reference jaw.
static void checkBearControls() {
    int16_t p[POLIVOKS_NUM_PARAMS]; polivoksLoadPreset(0,p);
    p[PVX_FILT_CUTOFF]=32767; p[PVX_FILT_RES]=0;
    PolivoksView telemetry;
    PolivoksScreen bear;
    Canvas white,red,closed,modulated;
    for(int i=0;i<12;++i) bear.draw(white,p,telemetry,.1f);
    p[PVX_FILT_RES]=32767; bear.draw(red,p,telemetry,0);
    int changed=0;
    for(int y=44;y<334;++y) for(int x=0;x<640;++x) {
        const uint8_t* a=white.pixels()+4*(y*640+x);
        const uint8_t* b=red.pixels()+4*(y*640+x);
        if(std::memcmp(a,b,3)) {
            ++changed;
            check(x>=214 && x<426 && y>=157 && y<224,"resonance changes only the eyes and their optical glow");
        }
    }
    check(changed>300,"resonance visibly changes eye color");
    for(int x:{290,350}) {
        const uint8_t* a=white.pixels()+4*(186*640+x);
        const uint8_t* b=red.pixels()+4*(186*640+x);
        check(a[0]>200 && a[0]==a[1] && a[1]==a[2],"minimum resonance has white eyes");
        check(b[0]>200 && b[0]>b[1]+120 && b[0]>b[2]+120,"maximum resonance has red eyes");
    }
    p[PVX_FILT_CUTOFF]=0;
    for(int i=0;i<12;++i) bear.draw(closed,p,telemetry,.1f);
    auto lowerTeethHeight=[](const Canvas& canvas) {
        double sum=0; int count=0;
        for(int y=245;y<305;++y) for(int x=305;x<335;++x) {
            const uint8_t* pixel=canvas.pixels()+4*(y*640+x);
            int low=std::min({pixel[0],pixel[1],pixel[2]}),high=std::max({pixel[0],pixel[1],pixel[2]});
            if(low>140 && high-low<45) {sum+=y; ++count;}
        }
        check(count>50,"lower jaw retains the reference teeth and fur");
        return sum/count;
    };
    check(lowerTeethHeight(red)>lowerTeethHeight(closed)+12,"cutoff opens the reference jaw");
    for(int y=100;y<225;++y) for(int x=250;x<390;++x)
        check(!std::memcmp(red.pixels()+4*(y*640+x),closed.pixels()+4*(y*640+x),3),"cutoff preserves hat, eyes, nose and upper face");
    telemetry.state.filter_env=1; p[PVX_FILT_ENV_AMT]=32767;
    bear.draw(modulated,p,telemetry,0);
    check(!std::memcmp(closed.pixels()+44*640*4,modulated.pixels()+44*640*4,290*640*4),"jaw follows cutoff alone, independent of the envelope");
    puts("PASS reference bear: cutoff jaw, white-to-red eyes, unchanged upper face and envelope independence");
    p[PVX_MIX_OSC1]=p[PVX_MIX_OSC2]=0;bear.draw(white,p,telemetry,.1f);
    p[PVX_MIX_OSC1]=p[PVX_MIX_OSC2]=32767;bear.draw(red,p,telemetry,0);
    for(int osc=0;osc<2;++osc) {
        int colored=0;
        for(int y=150;y<280;++y) for(int x=osc?526:68;x<(osc?574:118);++x) {
            const uint8_t* a=white.pixels()+4*(y*640+x);const uint8_t* b=red.pixels()+4*(y*640+x);
            bool ink=osc ? b[1]>b[0]+20 && b[1]>b[2]*1.05f : b[2]>b[0]+20 && b[2]>b[1]*1.05f;
            if(ink) {
                check(a[0]==a[1] && a[1]==a[2],"zero mixer level makes the spring white");
                ++colored;
            }
        }
        check(colored>300,"full mixer level restores the oscillator spring color");
    }
    p[PVX_FILT_MODE]=32767;bear.draw(closed,p,telemetry,.1f);
    for(int y=225;y<305;++y) for(int x=260;x<515;++x) {
        const uint8_t* a=closed.pixels()+4*(y*640+x);
        bool cool=(a[1]>a[0]+25 && a[1]>a[2]*1.08f) || (a[2]>a[0]+25 && a[1]>a[0]+8);
        if(cool) check(x>=316 && x<=502 && std::fabs((y+.5f)-238.5f)<=4.6f*(x-318)/182.f+2,"no old scope branches outside the narrow live triangle");
    }
    Canvas saw;saw.clear(pal::bg);drawPolivoksWave(saw,PVX_SAW,20,20,100,40,pal::white);
    auto lit=[&](int x,int y) { return saw.pixels()[4*(y*640+x)]>100; };
    check(lit(32,49)||lit(32,50),"SAW diagram has the correct straight ramp");
    check(!lit(32,20),"SAW diagram is not a sine wave");
    puts("PASS oscillator wave diagrams, level-sensitive springs and isolated scope triangle");

}
int main(int argc,char** argv) {
    check(polivoksPresetCount()==24,"complete 24-preset bank");
    check(argc==3,"golden.raw and writable directory arguments");
    FILE* ref=std::fopen(argv[1],"rb"); check(ref,"open reference");
    const int boundaries[]={0,5120,10240,15360,25600,51200,76800};
    float maxDifference=0;
    for(int preset=0;preset<polivoksPresetCount();++preset) {
        int16_t params[POLIVOKS_NUM_PARAMS]; polivoksLoadPreset(preset,params);
        if(preset<12) check(!std::memcmp(params,approvedPolivoks[preset],POLIVOKS_V2_PARAMS*sizeof(int16_t)),"approved original preset parameters are unchanged");
        PolivoksSynth synth; synth.init(48000); synth.configure(params,0);
        int frame=0,event=0,chunk=0;
        while(frame<76800) {
            if(frame==boundaries[event]) {
                if(event==0) synth.noteOn(45,110/127.f);
                if(event==1) synth.noteOn(57,110/127.f);
                if(event==2) synth.noteOff(57);
                if(event==3) synth.noteOff(45);
                if(event==4) synth.noteOn(48,110/127.f);
                if(event==5) synth.noteOff(48);
                ++event;
            }
            const int sizes[]={1,37,113,128,61};
            int n=std::min(sizes[chunk++%5],boundaries[event]-frame);
            float stereo[256]={},gold[128]; synth.render(stereo,n);
            check(std::fread(gold,sizeof(float),n,ref)==(size_t)n,"read golden samples");
            for(int i=0;i<n;++i) {
                check(stereo[i*2]==stereo[i*2+1],"native mono duplication");
                maxDifference=std::fmax(maxDifference,std::fabs(stereo[i*2]-gold[i]));
                check(std::fabs(stereo[i*2]-gold[i])<=1e-7f,"standalone and host produce the same samples");
            }
            frame+=n;
        }
        PolivoksView view; check(synth.view(view),"safe telemetry snapshot");
        synth.noteOn(60,1); synth.configure(params,1); synth.noteOff(60);
        float release[256]={}; synth.render(release,128); check(synth.view(view),"telemetry after octave");
        check(view.state.held_keys==0,"octave change preserves the original note-off mapping");
        synth.reset(); std::memset(release,0,sizeof release); synth.render(release,128);
        for(float x:release) check(x==0,"hard reset silences retired track");
    }
    std::fclose(ref);
    std::printf("PASS 24 presets (12 approved unchanged), independent DSP, variable segment lengths; max difference %.9g\n",maxDifference);
    static Project p,loaded;
    p.addTrack(TRACK_MIDI,"POLIVOKS");
    Track& t=p.tracks[0]; t.engine=ENG_POLIVOKS; t.polivoksSet=true; polivoksLoadPreset(7,t.polivoks);
    t.octave=-1; t.polivoks[PVX_MASTER_TUNE]=17000;
    std::string path=std::string(argv[2])+"/polivoks_roundtrip.dpp"; char err[128];
    check(saveProjectFile(p,path.c_str()),"save project");
    check(loadProjectFile(loaded,path.c_str(),err,sizeof err),"load project");
    check(loaded.tracks[0].engine==ENG_POLIVOKS && loaded.tracks[0].polivoksSet,"project engine type");
    check(!std::memcmp(t.polivoks,loaded.tracks[0].polivoks,sizeof(t.polivoks)),"all 41 project parameters round-trip");
    static PresetStore store; store.init(argv[2],"polivoks");
    check(store.count()==24,"factory bank installed exactly once");
    for(int i=0;i<polivoksPresetCount();++i) {
        Track preset; preset.engine=ENG_POLIVOKS;
        auto info=polivoksPresetInfo(i); int at=store.find(info->name);
        check(at>=0 && store.load(at,preset),"load native factory preset");
        int16_t expected[POLIVOKS_NUM_PARAMS]; polivoksLoadPreset(i,expected);
        check(!std::memcmp(expected,preset.polivoks,sizeof expected),"factory bank equals approved standalone bank");
    }
    check(store.save(t,"PVX USER",CAT_LEAD),"save user preset");
    Track user; user.engine=ENG_POLIVOKS;
    check(store.load(store.find("PVX USER"),user),"load user preset");
    check(!std::memcmp(t.polivoks,user.polivoks,sizeof t.polivoks),"all 41 user preset parameters round-trip");
    puts("PASS project, user preset and 24 factory presets round-trip");
    for(int i=12;i<24;++i) check(store.remove(store.find(polivoksPresetInfo(i)->name)),"simulate old 12-preset installation");
    check(std::remove((std::string(argv[2])+"/polivoks/_factory_v2").c_str())==0,"remove new marker for upgrade test");
    FILE* marker=std::fopen((std::string(argv[2])+"/polivoks/_factory_v1").c_str(),"w");check(marker,"old factory marker");std::fclose(marker);
    Track edited=t;edited.polivoks[PVX_FILT_CUTOFF]=12345;
    check(store.save(edited,"FOUNDATION",CAT_BASS),"save edited original factory preset");
    edited.polivoks[PVX_FILT_CUTOFF]=22222;
    check(store.save(edited,"RED HAMMER",CAT_BASS),"existing same-name user patch before bank upgrade");
    store.init(argv[2],"polivoks");
    check(store.count()==25,"upgrade adds twelve new presets and preserves user preset");
    Track preserved;preserved.engine=ENG_POLIVOKS;
    check(store.load(store.find("FOUNDATION"),preserved) && preserved.polivoks[PVX_FILT_CUTOFF]==12345,"upgrade preserves edited original preset");
    check(store.load(store.find("RED HAMMER"),preserved) && preserved.polivoks[PVX_FILT_CUTOFF]==22222,"new bank preserves same-name user patch");
    for(int i=12;i<24;++i) check(store.find(polivoksPresetInfo(i)->name)>=0,"new factory preset installed during upgrade");
    puts("PASS factory v1 -> v2 upgrade preserves original edits and user presets");

    static AudioEngine engine; engine.init(48000);
    engine.projectBack()=p; engine.publishProject();
    float audio[512]={}; engine.process(audio,256);
    engine.send(EC_NOTE_ON,0,60,0,1); engine.process(audio,256);
    PolivoksView view; check(engine.polivoksView(0,view) && view.state.held_keys==1,"engine routes notes to independent Polivoks");
    static Project empty; engine.projectBack()=empty; engine.publishProject(); engine.process(audio,256);
    check(engine.polivoksView(0,view) && view.state.held_keys==0,"track deletion clears Polivoks keys");
    engine.projectBack()=p; engine.publishProject(); engine.process(audio,256);
    check(engine.polivoksView(0,view) && view.state.held_keys==0,"reused track does not inherit held keys");
    // Host-wide synthesis settings must have no effect on the independent path.
    p.tracks[0].env[0]=9; p.tracks[0].fenvAmt=-1; p.tracks[0].unison=50; p.tracks[0].age=1;
    engine.projectBack()=p; engine.publishProject(); engine.send(EC_NOTE_ON,0,60,0,1); engine.process(audio,256);
    check(engine.polivoksView(0,view) && view.state.amp_env>.1f,"host ADSR does not wrap Polivoks");
    engine.send(EC_STOP); engine.process(audio,256);
    check(engine.polivoksView(0,view) && view.state.held_keys==0,"STOP releases audition notes");
    puts("PASS event routing, deletion/reuse, independent envelopes and STOP");
    static App app; app.init(&engine); app.project()=p; app.openPresets(argv[2]);
    app.markDirty(); app.openSynth(0); app.update(InputState(),.016f);
    Canvas screen;
    app.project().tracks[0].engine=ENG_WAVETABLE;app.openSynth(0);app.synth.setPage(5);
    app.project().tracks[0].engine=ENG_POLIVOKS;app.openSynth(0);app.update(InputState(),.016f);
    int priorWave=app.project().tracks[0].polivoks[PVX_OSC1_WAVE],priorCut=app.project().tracks[0].polivoks[PVX_FILT_CUTOFF];
    InputState firstControl;firstControl.axis[AX_LX]=1;app.update(firstControl,.1f);app.update(InputState(),.016f);
    check(app.project().tracks[0].polivoks[PVX_OSC1_WAVE]!=priorWave && app.project().tracks[0].polivoks[PVX_FILT_CUTOFF]==priorCut,"entering Polivoks starts on OSC 1");
    for(int page=0;page<PV_PAGE_PRESETS;++page) for(int layer=0;layer<2;++layer) {
        app.synth.setPage(page); app.synth.setAlt(layer!=0);
        app.render(screen);
        InputState input; input.axis[AX_LX]=1; input.axis[AX_LY]=-1; input.axis[AX_RX]=1; input.axis[AX_RY]=-1;
        app.update(input,.016f); app.update(InputState(),.016f);
        for(int16_t x:app.project().tracks[0].polivoks) check(x>=0,"UI parameter remains normalized");
    }
    bool reached[POLIVOKS_NUM_PARAMS]={};
    for(int page=0;page<PV_PAGE_PRESETS;++page) for(int layer=0;layer<2;++layer) for(int axis=0;axis<4;++axis) for(int direction:{-1,1}) {
        app.synth.setPage(page); app.synth.setAlt(layer!=0);
        for(int16_t& word:app.project().tracks[0].polivoks) word=16000;
        int16_t before[POLIVOKS_NUM_PARAMS];std::memcpy(before,app.project().tracks[0].polivoks,sizeof before);
        app.update(InputState(),.016f);
        InputState input; const int axes[]={AX_LX,AX_LY,AX_RX,AX_RY};
        input.axis[axes[axis]]=(float)direction;
        app.update(input,.1f);app.update(InputState(),.016f);
        for(int j=0;j<POLIVOKS_NUM_PARAMS;++j) if(before[j]!=app.project().tracks[0].polivoks[j]) reached[j]=true;
    }
    app.synth.setPage(PV_PAGE_LFO);app.synth.setAlt(false);
    InputState sync;sync.btn[B_L3]=true;app.update(sync,.016f);app.update(InputState(),.016f);
    reached[PVX_LFO_SYNC]=app.project().tracks[0].polivoks[PVX_LFO_SYNC]>=16384;
    int oldDivision=app.project().tracks[0].polivoks[PVX_LFO_DIV];
    InputState division;division.axis[AX_LX]=1;app.update(division,.1f);app.update(InputState(),.016f);
    reached[PVX_LFO_DIV]=app.project().tracks[0].polivoks[PVX_LFO_DIV]!=oldDivision;
    app.update(sync,.016f);app.update(InputState(),.016f);
    // Move the editor destination to OSC 1, then test the four independent routes.
    InputState select;select.axis[AX_RY]=1;
    for(int i=0;i<4;++i) {app.update(select,.1f);app.update(InputState(),.016f);}
    const int depths[]={PVX_LFO_TO_PITCH,PVX_LFO_TO_OSC2,PVX_LFO_TO_FILT,PVX_LFO_TO_AMP};
    for(int destination=0;destination<4;++destination) {
        for(int id:depths) app.project().tracks[0].polivoks[id]=10000;
        int16_t before[POLIVOKS_NUM_PARAMS];std::memcpy(before,app.project().tracks[0].polivoks,sizeof before);
        InputState depth;depth.axis[AX_LY]=-1;app.update(depth,.1f);app.update(InputState(),.016f);
        for(int j=0;j<POLIVOKS_NUM_PARAMS;++j) {
            bool changed=before[j]!=app.project().tracks[0].polivoks[j];
            check(changed==(j==depths[destination]),"LFO depth edits only the selected native destination");
            if(changed) reached[j]=true;
        }
        std::memcpy(before,app.project().tracks[0].polivoks,sizeof before);
        select.axis[AX_RY]=-1;app.update(select,.1f);app.update(InputState(),.016f);
        check(!std::memcmp(before,app.project().tracks[0].polivoks,sizeof before),"LFO destination selection preserves all synthesis settings");
    }
    for(int j=0;j<POLIVOKS_NUM_PARAMS;++j)
        check(reached[j]==(j!=PVX_MIX_EXTERNAL && j!=PVX_MASTER_LEVEL),"all 42 useful controls reachable; external/output are hidden");
    puts("PASS 42 visible parameters, hidden legacy levels and independent parallel LFO editing");
    uint64_t diagrams[4]={};
    for(int wave=0;wave<4;++wave) {
        app.project().tracks[0].polivoks[PVX_LFO_WAVE]=polivoksChoice(wave,4);app.render(screen);
        uint64_t hash=1469598103934665603ULL;
        for(int y=350;y<389;++y) for(int x=455;x<614;++x) for(int channel=0;channel<3;++channel)
            hash=(hash^screen.pixels()[4*(y*640+x)+channel])*1099511628211ULL;
        diagrams[wave]=hash;
        for(int other=0;other<wave;++other) check(diagrams[wave]!=diagrams[other],"four native LFO shapes have distinct waveform diagrams");
    }
    app.synth.setPage(PV_PAGE_FENV);app.synth.setAlt(true);
    app.project().tracks[0].polivoks[PVX_FENV_REPEAT]=0;app.project().tracks[0].polivoks[PVX_FENV_SUSTAIN]=20000;
    InputState repeat;repeat.axis[AX_LX]=1;app.update(repeat,.1f);app.update(InputState(),.016f);
    check(app.project().tracks[0].polivoks[PVX_FENV_REPEAT]==32767 && app.project().tracks[0].polivoks[PVX_FENV_SUSTAIN]==0,"Repeat selects zero sustain");
    app.synth.setAlt(false);repeat=InputState();repeat.axis[AX_RX]=1;app.update(repeat,.1f);app.update(InputState(),.016f);
    check(app.project().tracks[0].polivoks[PVX_FENV_REPEAT]==0 && app.project().tracks[0].polivoks[PVX_FENV_SUSTAIN]>0,"editing sustain selects Single mode");
    puts("PASS distinct native LFO diagrams and consistent Repeat/Sustain editing");

    app.synth.setPage(PV_PAGE_PRESETS); app.render(screen);
    InputState load; load.btn[B_A]=true; app.update(load,.016f); app.update(InputState(),.016f);
    check(app.project().tracks[0].engine==ENG_POLIVOKS,"preset browser preserves independent engine type");
    puts("PASS all eight control pages, L3 layers, rendering and preset browser");
    checkBearControls();
    return 0;
}
