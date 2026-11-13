#include "headless.h"
#include "fx/fx_registry.h"
#include "audio/engine.h"
#include "gfx/canvas.h"
#include "io/image_io.h"
#include "ui/app.h"
#include "ui/test_screen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace dp {

static int savePng(const Canvas& c, const char* path) {
    if (!writePng(path, c.pixels(), Canvas::W, Canvas::H)) { fprintf(stderr, "Cannot write %s\n", path); return 1; }
    printf("PNG: %s\n", path);
    return 0;
}

static int testScreenPng(const char* path) {
    static ToneSynth syn;
    syn.init(48000.f);
    TestScreen app;
    app.init(&syn);
    app.setEncoder(1, 0.72f); app.setEncoder(2, 0.38f); app.setEncoder(3, 0.45f);
    InputState in;
    strcpy(in.pad, "PAD: DESKTOP (HEADLESS)");
    app.update(in, 0.016f);
    in.btn[B_A] = true;
    app.update(in, 0.f);
    float buf[2 * 2048];
    for (int i = 0; i < 4; i++) syn.process(buf, 2048);
    AudioInfo ai;
    ai.ok = true; ai.rate = 48000; ai.frames = 256; ai.channels = 2;
    strcpy(ai.driver, "ALSA");
    Canvas c;
    app.render(c, in, ai, 60.f);
    return savePng(c, path);
}

// Pusta klatka (zatwierdza przelaczenie ekranu).
static void idle(App& app) {
    InputState in;
    app.update(in, 0.016f);
}

// Symuluje wcisniecie przycisku (jedna klatka wcisniety, jedna puszczony).
static void tap(App& app, int btn) {
    InputState in;
    in.btn[btn] = true;
    app.update(in, 0.016f);
    in.btn[btn] = false;
    app.update(in, 0.016f);
}

int headlessPng(const char* path, const char* screen) {
    if (screen && !strcmp(screen, "test")) return testScreenPng(path);
    static AudioEngine eng;
    eng.init(48000.f);
    static App app;
    app.init(&eng);
    app.loadDemo();
    float buf[2 * 256];
    if (!strcmp(screen, "project")) {
        // przykladowe projekty w out/projects_demo (makieta listy)
        app.openStore("out/projects_demo");
        app.loadDemo();
        const char* names[] = {"SKETCH 01", "NIGHT BUS", "DEMO", "PROJECT 01"};
        for (const char* n : names) { snprintf(app.project().name, 16, "%s", n); app.saveProject(); }
        app.project().bpm = 128;
        app.markDirty();
        app.arrange.setCursor(0, 0);
        tap(app, B_L2);  // ARRANGE -> PROJECT
    } else if (!strcmp(screen, "roll")) {
        app.openPattern(1, 1);  // HOOK na sciezce LEAD
        idle(app);
        app.roll.setCursor(14, 70);
        app.roll.setView(75);
        tap(app, B_START);
        for (int i = 0; i < 160; i++) eng.process(buf, 256);
    } else if (!strcmp(screen, "select")) {
        app.openPattern(1, 1);
        idle(app);
        app.roll.setView(75);
        tap(app, B_R3);                    // tryb zaznaczania
        app.roll.selectRect(4, 63, 9, 72);  // zaznacz 3 nuty
        app.roll.setCursor(9, 72);
    } else if (!strcmp(screen, "transform")) {
        app.openPattern(1, 1);
        idle(app);
        app.roll.setView(75);
        tap(app, B_R3);
        app.roll.setSelectLayer(1);
        app.roll.selectRect(4, 63, 9, 72);
        app.roll.setCursor(9, 72);
    } else if (!strcmp(screen, "presets") || !strcmp(screen, "voice") || !strcmp(screen, "pluspresets")) {
        app.openPresets("out/presets_demo");
        Track& tr = app.project().tracks[1];
        if (!strcmp(screen, "pluspresets")) tr.alt = true;
        if (!strcmp(screen, "voice")) { tr.mono = true; tr.unison = 22; tr.glideSync = true; tr.glideDiv = 7; tr.age = 0.4f; }
        app.markDirty();
        app.openSynth(1);
        idle(app);
        app.synth.setPageKind(!strcmp(screen, "voice") ? SynthView::K_VOICE : SynthView::K_PRESETS);
        if (strcmp(screen, "voice")) { tap(app, B_LEFT); tap(app, B_DOWN); tap(app, B_DOWN); tap(app, B_DOWN); tap(app, B_RIGHT); tap(app, B_A); tap(app, B_DOWN); }
        idle(app);
    } else if (!strcmp(screen, "synth") || !strcmp(screen, "synthenv") || !strcmp(screen, "synthfenv") ||
               !strcmp(screen, "synthlfo") || !strcmp(screen, "synthext") || !strcmp(screen, "synthfilt") ||
               !strcmp(screen, "synthmenv")) {
        Track& tr = app.project().tracks[1];
        if (!strcmp(screen, "synthmenv")) { tr.menv[1] = 0.25f; tr.menvAmt = -0.4f; tr.menvDest = DEST_PITCH; }
        if (!strcmp(screen, "synthext") || !strcmp(screen, "synthfilt")) {
            tr.alt = true; tr.knob[0] = 9400; tr.ext.osc[1].wave = 26000; tr.ext.osc[1].semi = 7; tr.ext.osc[1].level = 0.7f;
            tr.ext.sub = 0.4f; tr.ext.fmode = !strcmp(screen, "synthfilt") ? FLT_BP : FLT_LP24; tr.knob[3] = 18000;
        }
        if (!strcmp(screen, "synthfenv")) { tr.fenv[1] = 0.6f; tr.fenv[2] = 0.3f; tr.fenv[3] = 0.8f; tr.fenvAmt = 0.5f; }
        if (!strcmp(screen, "synthlfo")) {
            tr.lfo[0].depth = 0.35f; tr.lfo[0].dest = DEST_WAVE; tr.lfo[0].sync = true; tr.lfo[0].div = 8;  // 1/16T
        }
        app.markDirty();
        app.openSynth(1);
        idle(app);
        if (!strcmp(screen, "synthenv")) app.synth.setPageKind(SynthView::K_AMP);
        if (!strcmp(screen, "synthfenv")) { app.synth.setPageKind(SynthView::K_FENV); app.synth.setAlt(true); }
        if (!strcmp(screen, "synthlfo")) app.synth.setPageKind(SynthView::K_LFO1);
        if (!strcmp(screen, "synthmenv")) { app.synth.setPageKind(SynthView::K_MENV); app.synth.setAlt(true); }
        if (!strcmp(screen, "synthext")) app.synth.setPageKind(SynthView::K_OSC2);
        if (!strcmp(screen, "synthfilt")) app.synth.setPageKind(SynthView::K_FILTER);
        InputState in;
        in.btn[B_A] = true;  // trzymana nuta
        app.update(in, 0.016f);
        for (int i = 0; i < 60; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
    } else if (!strcmp(screen, "taffy") || !strcmp(screen, "taffypresets")) {
        Track& tr = app.project().tracks[1];
        tr.engine = ENG_TAFFY;
        engineDefaultKnobs(ENG_TAFFY, tr.knob);
        tr.knob[3] = 14000; tr.knob[2] = 16000;
        app.openPresets("out/presets_demo");
        app.markDirty();
        app.openSynth(1);
        idle(app);
        if (!strcmp(screen, "taffypresets")) app.synth.setPageKind(SynthView::K_PRESETS);
        InputState in;
        in.btn[B_A] = true;
        app.update(in, 0.016f);
        for (int i = 0; i < 20; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
    } else if (!strcmp(screen, "pew") || !strcmp(screen, "pewbars") || !strcmp(screen, "pewpresets")) {
        Track& tr = app.project().tracks[1];
        tr.engine = ENG_PEW;
        engineDefaultKnobs(ENG_PEW, tr.knob);
        tr.knob[1] = 20000; tr.knob[2] = 20000; tr.knob[3] = 28000;  // perkusja, drive, Leslie FAST
        tr.push[2] = 16000;
        app.openPresets("out/presets_demo");
        app.markDirty();
        app.openSynth(1);
        idle(app);
        if (!strcmp(screen, "pewbars")) app.synth.setPageKind(SynthView::K_BARS);
        if (!strcmp(screen, "pewpresets")) app.synth.setPageKind(SynthView::K_PRESETS);
        InputState in;
        in.btn[B_A] = true;   // trzymany akord: A, B, X
        app.update(in, 0.016f);
        for (int i = 0; i < 6; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
        in.btn[B_B] = true;
        app.update(in, 0.016f);
        for (int i = 0; i < 6; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
        in.btn[B_X] = true;
        app.update(in, 0.016f);
        for (int i = 0; i < 40; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
    } else if (!strcmp(screen, "elg") || !strcmp(screen, "elgenv") || !strcmp(screen, "elgsync")) {
        Track& tr = app.project().tracks[1];
        if (!strcmp(screen, "elgsync")) { tr.movSync = true; tr.movDiv = 3; }
        tr.engine = ENG_ELG;
        engineDefaultKnobs(ENG_ELG, tr.knob);
        tr.knob[2] = 26000; tr.knob[3] = 20000;
        app.openPresets("out/presets_demo");
        app.markDirty();
        app.openSynth(1);
        idle(app);
        if (!strcmp(screen, "elgenv")) app.synth.setPageKind(SynthView::K_AMP);
        if (!strcmp(screen, "elgsync")) app.synth.setAlt(true);
        InputState in;
        in.btn[B_A] = true;
        app.update(in, 0.016f);
        for (int i = 0; i < 60; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
    } else if (!strcmp(screen, "eva") || !strcmp(screen, "evaoct") || !strcmp(screen, "evalfo") || !strcmp(screen, "evamenv")) {
        Track& tr = app.project().tracks[1];
        if (!strcmp(screen, "evalfo")) { tr.lfo[0].depth = 0.4f; tr.lfo[0].dest = DEST_DETUNE; tr.lfo[0].rate = 0.5f; }
        if (!strcmp(screen, "evamenv")) { tr.menv[1] = 0.8f; tr.menvAmt = 0.5f; tr.menvDest = DEST_WAVE; }
        tr.engine = ENG_EVA;
        engineDefaultKnobs(ENG_EVA, tr.knob);
        tr.knob[3] = 17000;  // radio
        app.openPresets("out/presets_demo");
        app.markDirty();
        app.openSynth(1);
        idle(app);
        if (!strcmp(screen, "evaoct")) app.synth.setAlt(true);
        if (!strcmp(screen, "evalfo")) app.synth.setPageKind(SynthView::K_LFO1);
        if (!strcmp(screen, "evamenv")) { app.synth.setPageKind(SynthView::K_MENV); app.synth.setAlt(true); }
        InputState in;
        in.btn[B_A] = true;
        app.update(in, 0.016f);
        for (int i = 0; i < 60; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
    } else if (!strcmp(screen, "mbira") || !strcmp(screen, "mbirap2")) {
        Track& tr = app.project().tracks[1];
        tr.engine = ENG_MBIRA;
        engineDefaultKnobs(ENG_MBIRA, tr.knob);
        tr.knob[3] = 16000;  // kapsle
        tr.push[0] = 12000; tr.push[2] = 20000;
        app.openPresets("out/presets_demo");
        app.markDirty();
        app.openSynth(1);
        idle(app);
        if (!strcmp(screen, "mbirap2")) app.synth.setAlt(true);
        InputState in;
        in.btn[B_A] = true;
        app.update(in, 0.016f);
        for (int i = 0; i < 20; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
    } else if (!strcmp(screen, "reactor") || !strcmp(screen, "reactorhot")) {
        Track& tr = app.project().tracks[1];
        tr.engine = ENG_REACTOR;
        engineDefaultKnobs(ENG_REACTOR, tr.knob);
        if (!strcmp(screen, "reactorhot")) { tr.knob[0] = 0; tr.knob[1] = 20000; tr.knob[2] = 32767; tr.knob[3] = 32767; }
        app.openPresets("out/presets_demo");
        app.markDirty();
        app.openSynth(1);
        idle(app);
        InputState in;
        in.btn[B_A] = true;
        app.update(in, 0.016f);
        for (int i = 0; i < 120; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
    } else if (!strcmp(screen, "oxbow") || !strcmp(screen, "oxbowp2")) {
        Track& tr = app.project().tracks[1];
        tr.engine = ENG_OXBOW;
        engineDefaultKnobs(ENG_OXBOW, tr.knob);
        tr.knob[1] = 16000; tr.knob[3] = 20000;  // dzwonek i ptaki w ruchu
        app.openPresets("out/presets_demo");
        app.markDirty();
        app.openSynth(1);
        idle(app);
        if (!strcmp(screen, "oxbowp2")) app.synth.setAlt(true);
        InputState in;
        in.btn[B_A] = true; in.btn[B_B] = true; in.btn[B_X] = true;  // akord: trzy gwiazdy
        app.update(in, 0.016f);
        for (int i = 0; i < 40; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
    } else if (!strcmp(screen, "obposc1") || !strcmp(screen, "obposc2") || !strcmp(screen, "obpfilt") || !strcmp(screen, "obpfenv")) {
        Track& tr = app.project().tracks[1];
        tr.engine = ENG_OXBOW;
        tr.alt = true;
        engineDefaultKnobs(ENG_OXBOW, tr.knob);
        tr.obx.wave[1] = 2; tr.obx.semi[1] = 7; tr.obx.noise = 30; tr.obx.sync = !strcmp(screen, "obposc2");
        app.openPresets("out/presets_demo");
        app.markDirty();
        app.openSynth(1);
        idle(app);
        app.synth.setPageKind(!strcmp(screen, "obposc1") ? SynthView::K_OSC1 : !strcmp(screen, "obposc2") ? SynthView::K_OSC2
                              : !strcmp(screen, "obpfilt") ? SynthView::K_FILTER : SynthView::K_FENV);
        if (!strcmp(screen, "obposc2")) app.synth.setAlt(true);
        InputState in;
        in.btn[B_A] = true;
        app.update(in, 0.016f);
        for (int i = 0; i < 30; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
    } else if (!strcmp(screen, "oxbowplusengine")) {
        app.arrange.setCursor(1, 2);
        tap(app, B_SELECT);
        tap(app, B_R2);
        for (int i = 0; i < 9; i++) tap(app, B_DOWN);  // OXBOW+
    } else if (!strcmp(screen, "sid") || !strcmp(screen, "sidsync") || !strcmp(screen, "sidfenv")) {
        Track& tr = app.project().tracks[1];
        if (!strcmp(screen, "sidfenv")) { tr.fenv[1] = 0.5f; tr.fenv[2] = 0.2f; tr.fenvAmt = -0.4f; }
        tr.engine = ENG_SID;
        engineDefaultKnobs(ENG_SID, tr.knob);
        if (!strcmp(screen, "sidsync")) { tr.knob[0] = 4 * 512 + 256; tr.knob[1] = 12000; }
        app.openPresets("out/presets_demo");
        app.markDirty();
        app.openSynth(1);
        idle(app);
        if (!strcmp(screen, "sidfenv")) { app.synth.setPageKind(SynthView::K_FENV); app.synth.setAlt(true); }
        InputState in;
        in.btn[B_A] = true;
        app.update(in, 0.016f);
        for (int i = 0; i < 20; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
    } else if (!strcmp(screen, "monolit") || !strcmp(screen, "monolitp2") || !strcmp(screen, "monolitvoice") || !strcmp(screen, "monolitfenv")) {
        Track& tr = app.project().tracks[1];
        tr.engine = ENG_MONOLIT;
        engineDefaultKnobs(ENG_MONOLIT, tr.knob);
        tr.knob[0] = 24576; tr.knob[3] = 26000; tr.push[0] = 22938; tr.push[2] = 9000;
        if (!strcmp(screen, "monolitvoice")) { tr.mono = true; tr.glide = 0.08f; }
        app.openPresets("out/presets_demo");
        app.markDirty();
        app.openSynth(1);
        idle(app);
        if (!strcmp(screen, "monolitp2")) app.synth.setAlt(true);
        if (!strcmp(screen, "monolitvoice")) app.synth.setPageKind(SynthView::K_VOICE);
        if (!strcmp(screen, "monolitfenv")) {
            tr.fenv[1] = 0.6f; tr.fenvAmt = 0.4f; app.markDirty(); idle(app);
            app.synth.setPageKind(SynthView::K_FENV); app.synth.setAlt(true);
        }
        InputState in;
        in.btn[B_A] = true;
        app.update(in, 0.016f);
        for (int i = 0; i < 12; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
    } else if (!strcmp(screen, "vapora") || !strcmp(screen, "vaporap2") || !strcmp(screen, "vaporafenv")) {
        Track& tr = app.project().tracks[1];
        tr.engine = ENG_VAPORA;
        engineDefaultKnobs(ENG_VAPORA, tr.knob);
        tr.push[0] = 3328; tr.push[1] = 18023; tr.push[2] = 12000; tr.octave = -1;
        app.openPresets("out/presets_demo");
        app.markDirty();
        app.openSynth(1);
        idle(app);
        if (!strcmp(screen, "vaporap2")) app.synth.setAlt(true);
        if (!strcmp(screen, "vaporafenv")) {
            tr.fenv[1] = 0.6f; tr.fenvAmt = 0.4f; app.markDirty(); idle(app);
            app.synth.setPageKind(SynthView::K_FENV);
        }
        InputState in;
        in.btn[B_A] = true;
        app.update(in, 0.016f);
        for (int i = 0; i < 12; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
    } else if (!strcmp(screen, "supersaw") || !strcmp(screen, "supersawvoice") || !strcmp(screen, "supersawfilter")) {
        Track& tr = app.project().tracks[1];
        tr.engine = ENG_SUPERSAW;
        engineDefaultKnobs(ENG_SUPERSAW, tr.knob);
        tr.knob[1] = 12000; tr.knob[2] = 26000; tr.knob[3] = 20000;
        if (!strcmp(screen, "supersawvoice")) { tr.mono = true; tr.unison = 25.f; }
        app.openPresets("out/presets_demo");
        app.markDirty();
        app.openSynth(1);
        idle(app);
        if (!strcmp(screen, "supersawvoice")) app.synth.setPageKind(SynthView::K_VOICE);
        if (!strcmp(screen, "supersawfilter")) { tr.push[0] = 14000; tr.push[1] = 20000; tr.push[2] = 0; tr.fenvAmt = 0.5f; app.markDirty(); idle(app); app.synth.setPageKind(SynthView::K_FILTER); }
        InputState in;
        in.btn[B_A] = true;
        app.update(in, 0.016f);
        for (int i = 0; i < 12; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
    } else if (!strcmp(screen, "mixtur") || !strcmp(screen, "mixturp2") || !strcmp(screen, "mixturlfo")) {
        Track& tr = app.project().tracks[1];
        tr.engine = ENG_MIXTUR;
        engineDefaultKnobs(ENG_MIXTUR, tr.knob);
        app.openPresets("out/presets_demo");
        app.markDirty();
        app.openSynth(1);
        idle(app);
        if (!strcmp(screen, "mixturp2")) app.synth.setAlt(true);
        if (!strcmp(screen, "mixturlfo")) { tr.lfo[0].dest = DEST_W1; tr.lfo[0].depth = 0.3f; app.markDirty(); idle(app); app.synth.setPageKind(SynthView::K_LFO1); }
        InputState in;
        in.btn[B_A] = true;
        app.update(in, 0.016f);
        for (int i = 0; i < 30; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
    } else if (!strcmp(screen, "brass")) {
        Track& tr = app.project().tracks[1];
        tr.engine = ENG_BRASS;
        engineDefaultKnobs(ENG_BRASS, tr.knob);
        tr.knob[2] = 20000; tr.knob[3] = 16000;
        app.openPresets("out/presets_demo");
        app.markDirty();
        app.openSynth(1);
        idle(app);
        InputState in;
        in.btn[B_A] = true;
        app.update(in, 0.016f);
        for (int i = 0; i < 30; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
    } else if (!strcmp(screen, "hollow") || !strcmp(screen, "hollowp2")) {
        Track& tr = app.project().tracks[1];
        tr.engine = ENG_HOLLOW;
        engineDefaultKnobs(ENG_HOLLOW, tr.knob);
        engineDefaultPush(ENG_HOLLOW, tr.push);
        tr.knob[3] = 26000;
        if (!strcmp(screen, "hollowp2")) tr.relic = true;
        app.openPresets("out/presets_demo");
        app.markDirty();
        app.openSynth(1);
        idle(app);
        if (!strcmp(screen, "hollowp2")) app.synth.setAlt(true);
        InputState in;
        in.btn[B_A] = true;
        app.update(in, 0.016f);
        for (int i = 0; i < 30; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
    } else if (!strcmp(screen, "blood")) {
        Track& tr = app.project().tracks[1];
        tr.engine = ENG_BLOOD;
        engineDefaultKnobs(ENG_BLOOD, tr.knob);
        tr.knob[2] = 20000; tr.knob[3] = 24000;
        app.openPresets("out/presets_demo");
        app.markDirty();
        app.openSynth(1);
        idle(app);
        InputState in;
        in.btn[B_A] = true;
        app.update(in, 0.016f);
        for (int i = 0; i < 30; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
    } else if (!strcmp(screen, "blot") || !strcmp(screen, "blotp2")) {
        Track& tr = app.project().tracks[1];
        tr.engine = ENG_BLOT;
        engineDefaultKnobs(ENG_BLOT, tr.knob);
        tr.knob[1] = 26000; tr.knob[3] = 20000;
        tr.mono = true; tr.glide = 0.06f;
        app.openPresets("out/presets_demo");
        app.markDirty();
        app.openSynth(1);
        idle(app);
        if (!strcmp(screen, "blotp2")) app.synth.setAlt(true);
        InputState in;
        in.btn[B_A] = true;
        app.update(in, 0.016f);
        for (int i = 0; i < 12; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
    } else if (!strcmp(screen, "aether") || !strcmp(screen, "aethertilt")) {
        Track& tr = app.project().tracks[1];
        tr.engine = ENG_AETHER;
        engineDefaultKnobs(ENG_AETHER, tr.knob);
        tr.tilt[0] = 14000; tr.tilt[1] = 6000; tr.tilt[2] = 10000;
        app.openPresets("out/presets_demo");
        app.markDirty();
        app.openSynth(1);
        idle(app);
        if (!strcmp(screen, "aethertilt")) app.synth.setPageKind(SynthView::K_TILT);
        InputState in;
        in.btn[B_A] = true;
        app.update(in, 0.016f);
        for (int i = 0; i < 30; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
    } else if (!strcmp(screen, "gin") || !strcmp(screen, "ginedit") || !strcmp(screen, "ginenv") || !strcmp(screen, "ginabout")) {
        Track& tr = app.project().tracks[1];
        tr.engine = ENG_GIN;
        ginLoadKit(tr, 0);
        tr.gin[0].morph = 350;
        app.openPresets("out/presets_demo");
        app.markDirty();
        app.openSynth(1);
        idle(app);
        if (!strcmp(screen, "ginedit")) app.synth.setPage(4);  // NOISE FILTER
        if (!strcmp(screen, "ginenv")) app.synth.setPage(2);   // TONE ENV
        if (!strcmp(screen, "ginabout")) app.synth.setPage(9);
        InputState in;
        in.btn[B_A] = true;
        app.update(in, 0.016f);
        for (int i = 0; i < 40; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
    } else if (!strcmp(screen, "ginengine")) {
        app.arrange.setCursor(1, 2);
        tap(app, B_SELECT);
        tap(app, B_R2);
        for (int i = 0; i < 30; i++) tap(app, B_UP);
        for (int i = 0; i < 8; i++) tap(app, B_DOWN);  // GIN (alfabetycznie 9.)
    } else if (!strcmp(screen, "grand") || !strcmp(screen, "grandsoft") || !strcmp(screen, "grandengine")) {
        if (!strcmp(screen, "grandengine")) {
            app.arrange.setCursor(1, 2);
            tap(app, B_SELECT);
            tap(app, B_R2);
            for (int i = 0; i < 30; i++) tap(app, B_UP);
            for (int i = 0; i < 10; i++) tap(app, B_DOWN);  // GRAND
        } else {
            Track& tr = app.project().tracks[1];
            tr.engine = ENG_GRAND;
            engineDefaultKnobs(ENG_GRAND, tr.knob);
            engineDefaultPush(ENG_GRAND, tr.push);
            if (!strcmp(screen, "grandsoft")) { tr.knob[0] = 2000; tr.knob[1] = 30000; tr.knob[3] = 4000; tr.push[0] = 26000; tr.push[1] = 30000; }
            app.openPresets("out/presets_demo");
            app.markDirty();
            app.openSynth(1);
            idle(app);
            InputState in;
            in.btn[B_A] = true;
            app.update(in, 0.016f);
            for (int i = 0; i < 6; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
        }
    } else if (!strcmp(screen, "felt") || !strcmp(screen, "feltuna") || !strcmp(screen, "feltworn") || !strcmp(screen, "feltshut") || !strcmp(screen, "feltengine")) {
        if (!strcmp(screen, "feltengine")) {
            app.arrange.setCursor(1, 2);
            tap(app, B_SELECT);
            tap(app, B_R2);
            for (int i = 0; i < 30; i++) tap(app, B_UP);
            for (int i = 0; i < 8; i++) tap(app, B_DOWN);  // FELT
        } else {
            Track& tr = app.project().tracks[1];
            tr.engine = ENG_FELT;
            engineDefaultKnobs(ENG_FELT, tr.knob);
            if (!strcmp(screen, "feltuna")) { tr.knob[1] = 2000; tr.knob[0] = 32000; tr.knob[3] = 30000; }
            else if (!strcmp(screen, "feltshut")) { tr.knob[2] = 1000; }
            else if (!strcmp(screen, "feltworn")) { tr.knob[1] = 30000; tr.push[0] = 22000; tr.push[1] = 32767; app.synth.setAlt(true); }
            else tr.knob[1] = 30000;
            app.openPresets("out/presets_demo");
            app.markDirty();
            app.openSynth(1);
            idle(app);
            InputState in;
            in.btn[B_A] = true;
            app.update(in, 0.016f);
            for (int i = 0; i < 5; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
        }
    } else if (!strcmp(screen, "lounge") || !strcmp(screen, "loungereed") || !strcmp(screen, "loungeclav") || !strcmp(screen, "loungeengine")) {
        if (!strcmp(screen, "loungeengine")) {
            app.arrange.setCursor(1, 2);
            tap(app, B_SELECT);
            tap(app, B_R2);
            for (int i = 0; i < 30; i++) tap(app, B_UP);
            for (int i = 0; i < 11; i++) tap(app, B_DOWN);  // LOUNGE
        } else {
            Track& tr = app.project().tracks[1];
            tr.engine = ENG_LOUNGE;
            engineDefaultKnobs(ENG_LOUNGE, tr.knob);
            if (!strcmp(screen, "loungereed")) tr.knob[3] = 16384;
            if (!strcmp(screen, "loungeclav")) { tr.knob[3] = 32767; tr.knob[1] = 9000; }
            app.openPresets("out/presets_demo");
            app.markDirty();
            app.openSynth(1);
            idle(app);
            InputState in;
            in.btn[B_A] = true;
            app.update(in, 0.016f);
            for (int i = 0; i < 12; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
        }
    } else if (!strcmp(screen, "parallax") || !strcmp(screen, "parallaxengine")) {
        if (!strcmp(screen, "parallaxengine")) {
            app.arrange.setCursor(1, 2);
            tap(app, B_SELECT);
            tap(app, B_R2);
            for (int i = 0; i < 30; i++) tap(app, B_UP);
            for (int i = 0; i < 16; i++) tap(app, B_DOWN);  // PARALLAX
        } else {
            Track& tr = app.project().tracks[1];
            tr.engine = ENG_PARALLAX;
            engineDefaultKnobs(ENG_PARALLAX, tr.knob);
            tr.knob[0] = 13000; tr.knob[1] = 14000; tr.knob[2] = 20500; tr.knob[3] = 15000;
            app.openPresets("out/presets_demo");
            app.markDirty();
            app.openSynth(1);
            idle(app);
            InputState in;
            in.btn[B_A] = true;
            app.update(in, 0.016f);
            for (int i = 0; i < 30; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
        }
    } else if (!strcmp(screen, "outatime")) {
        Track& tr = app.project().tracks[1];
        tr.engine = ENG_OUTATIME;
        engineDefaultKnobs(ENG_OUTATIME, tr.knob);
        tr.knob[1] = 20000;
        app.openPresets("out/presets_demo");
        app.markDirty();
        app.openSynth(1);
        idle(app);
        InputState in;
        in.btn[B_A] = true;
        app.update(in, 0.016f);
        for (int i = 0; i < 60; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
    } else if (!strcmp(screen, "juni")) {
        Track& tr = app.project().tracks[1];
        tr.engine = ENG_JUNI;
        engineDefaultKnobs(ENG_JUNI, tr.knob);
        tr.knob[3] = 20000;
        app.openPresets("out/presets_demo");
        app.markDirty();
        app.openSynth(1);
        idle(app);
        InputState in;
        in.btn[B_A] = true;
        app.update(in, 0.016f);
        for (int i = 0; i < 30; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
    } else if (!strcmp(screen, "dddd")) {
        Track& tr = app.project().tracks[1];
        tr.engine = ENG_DDDD;
        engineDefaultKnobs(ENG_DDDD, tr.knob);
        tr.knob[2] = 22000; tr.knob[3] = 20000;
        app.openPresets("out/presets_demo");
        app.markDirty();
        app.openSynth(1);
        idle(app);
        InputState in;
        in.btn[B_A] = true;
        app.update(in, 0.016f);
        for (int i = 0; i < 30; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
    } else if (!strcmp(screen, "arc") || !strcmp(screen, "arcp2") || !strcmp(screen, "arclfo")) {
        Track& tr = app.project().tracks[1];
        tr.engine = ENG_ARC;
        engineDefaultKnobs(ENG_ARC, tr.knob);
        tr.knob[0] = 20000; tr.push[1] = 9000;
        app.openPresets("out/presets_demo");
        app.markDirty();
        app.openSynth(1);
        idle(app);
        if (!strcmp(screen, "arcp2")) app.synth.setAlt(true);
        if (!strcmp(screen, "arclfo")) { tr.lfo[0].dest = DEST_W2; tr.lfo[0].depth = 0.4f; app.markDirty(); idle(app); app.synth.setPageKind(SynthView::K_LFO1); }
        InputState in;
        in.btn[B_A] = true;
        app.update(in, 0.016f);
        for (int i = 0; i < 30; i++) { for (int k = 0; k < 4; k++) eng.process(buf, 256); app.update(in, 0.016f); }
    } else if (!strcmp(screen, "aetherengine")) {
        app.arrange.setCursor(1, 2);
        tap(app, B_SELECT);
        tap(app, B_R2);
        for (int i = 0; i < 19; i++) tap(app, B_DOWN);  // AETHER
    } else if (!strcmp(screen, "blotengine")) {
        app.arrange.setCursor(1, 2);
        tap(app, B_SELECT);
        tap(app, B_R2);
        for (int i = 0; i < 18; i++) tap(app, B_DOWN);  // BLOT
    } else if (!strcmp(screen, "bloodengine")) {
        app.arrange.setCursor(1, 2);
        tap(app, B_SELECT);
        tap(app, B_R2);
        for (int i = 0; i < 17; i++) tap(app, B_DOWN);  // BLOODMOON
    } else if (!strcmp(screen, "hollowengine")) {
        app.arrange.setCursor(1, 2);
        tap(app, B_SELECT);
        tap(app, B_R2);
        for (int i = 0; i < 16; i++) tap(app, B_DOWN);  // HOLLOW SAINT
    } else if (!strcmp(screen, "brassengine")) {
        app.arrange.setCursor(1, 2);
        tap(app, B_SELECT);
        tap(app, B_R2);
        for (int i = 0; i < 15; i++) tap(app, B_DOWN);  // BRASSOPHONE
    } else if (!strcmp(screen, "mixturengine")) {
        app.arrange.setCursor(1, 2);
        tap(app, B_SELECT);
        tap(app, B_R2);
        for (int i = 0; i < 14; i++) tap(app, B_DOWN);  // MIXTUR
    } else if (!strcmp(screen, "supersawengine")) {
        app.arrange.setCursor(1, 2);
        tap(app, B_SELECT);
        tap(app, B_R2);
        for (int i = 0; i < 13; i++) tap(app, B_DOWN);  // SUPERSAW
    } else if (!strcmp(screen, "vaporaengine")) {
        app.arrange.setCursor(1, 2);
        tap(app, B_SELECT);
        tap(app, B_R2);
        for (int i = 0; i < 12; i++) tap(app, B_DOWN);  // VAPORA
    } else if (!strcmp(screen, "monolitengine")) {
        app.arrange.setCursor(1, 2);
        tap(app, B_SELECT);
        tap(app, B_R2);
        for (int i = 0; i < 11; i++) tap(app, B_DOWN);  // MONOLIT
    } else if (!strcmp(screen, "sidengine")) {
        app.arrange.setCursor(1, 2);
        tap(app, B_SELECT);
        tap(app, B_R2);
        for (int i = 0; i < 10; i++) tap(app, B_DOWN);  // SID
    } else if (!strcmp(screen, "oxbowengine")) {
        app.arrange.setCursor(1, 2);
        tap(app, B_SELECT);
        tap(app, B_R2);
        for (int i = 0; i < 8; i++) tap(app, B_DOWN);  // OXBOW
    } else if (!strcmp(screen, "reactorengine")) {
        app.arrange.setCursor(1, 2);
        tap(app, B_SELECT);
        tap(app, B_R2);
        for (int i = 0; i < 7; i++) tap(app, B_DOWN);  // REACTOR
    } else if (!strcmp(screen, "mbiraengine")) {
        app.arrange.setCursor(1, 2);
        tap(app, B_SELECT);
        tap(app, B_R2);
        for (int i = 0; i < 6; i++) tap(app, B_DOWN);  // MBIRA
    } else if (!strcmp(screen, "evaengine")) {
        app.arrange.setCursor(1, 2);
        tap(app, B_SELECT);
        tap(app, B_R2);
        for (int i = 0; i < 5; i++) tap(app, B_DOWN);  // EVA
    } else if (!strcmp(screen, "elgengine")) {
        app.arrange.setCursor(1, 2);
        tap(app, B_SELECT);
        tap(app, B_R2);
        for (int i = 0; i < 4; i++) tap(app, B_DOWN);  // ELG
    } else if (!strcmp(screen, "splash")) {
        app.startSplash(1.9f);
    } else if (!strcmp(screen, "pewengine")) {
        app.arrange.setCursor(1, 2);
        tap(app, B_SELECT);
        tap(app, B_R2);
        for (int i = 0; i < 3; i++) tap(app, B_DOWN);  // PEW
    } else if (!strcmp(screen, "taffyengine")) {
        app.arrange.setCursor(1, 2);
        tap(app, B_SELECT);
        tap(app, B_R2);
        tap(app, B_DOWN);
        tap(app, B_DOWN);  // TAFFY
    } else if (!strcmp(screen, "newengine")) {
        app.arrange.setCursor(4, 0);
        tap(app, B_A);      // ADD TRACK
        tap(app, B_R2);     // karta ENGINE
        tap(app, B_DOWN);   // WAVETABLE+
    } else if (!strcmp(screen, "engine")) {
        app.arrange.setCursor(1, 2);
        tap(app, B_SELECT);  // okno TRACK
        tap(app, B_R2);      // karta ENGINE
        tap(app, B_DOWN);    // WAVETABLE+
    } else if (!strcmp(screen, "allengines")) {  // utwor demo ALL ENGINES w ARRANGE, w trakcie grania
        app.loadDemoAllEngines();
        app.arrange.setCursor(0, 0);
        idle(app);
        eng.send(EC_PLAY_SONG);
        for (int i = 0; i < 1900; i++) eng.process(buf, 256);  // ~10 s: takt 5
        InputState in;
        app.update(in, 0.016f);
    } else if (!strcmp(screen, "tools")) {        // ARRANGE + pasek narzedzi, petla takty 3-6
        app.arrange.setCursor(1, 2);
        tap(app, B_L3);
        tap(app, B_Y);
        app.arrange.setCursor(1, 5);
        tap(app, B_Y);
    } else if (!strcmp(screen, "rolltools")) {    // PIANO ROLL + pasek narzedzi, petla kroki 5-12
        app.openPattern(1, 1);
        idle(app);
        app.roll.setView(75);
        app.roll.setCursor(4, 70);
        tap(app, B_L3);
        tap(app, B_Y);
        app.roll.setCursor(11, 70);
        tap(app, B_Y);
    } else if (!strncmp(screen, "mixer", 5)) {
        // MIXER na utworze SYNTHWAVE w trakcie grania; przykladowe ustawienia EQ / kompresora / wysylek
        app.loadDemoSynthwave(true);
        Project& p = app.project();
        fxDefaults(p.tracks[3].ins[1], FX_SISTER);
        p.tracks[1].eq.gain[0] = 4.f; p.tracks[1].eq.gain[2] = -3.f;
        p.tracks[2].eq.gain[1] = 5.f; p.tracks[2].eq.freq[1] = 2500.f; p.tracks[2].eq.q[1] = 2.f;
        p.tracks[1].comp.on = true; p.tracks[1].comp.type = COMP_FET;
        p.tracks[3].comp.on = true; p.tracks[3].comp.type = COMP_OPTO;
        p.tracks[2].send[0] = .4f; p.tracks[3].send[0] = .6f; p.tracks[3].send[1] = .3f;
        p.tracks[2].pan = -.4f; p.tracks[4].pan = .5f;
        p.bus.on = true;
        app.markDirty();
        app.openMixer(1);
        idle(app);
        tap(app, B_START);
        for (int i = 0; i < 1500; i++) { eng.process(buf, 256); if (i % 8 == 0) idle(app); }
        if (!strcmp(screen, "mixereq")) { app.mixer.setCursor(2, MixerView::R_EQ); app.mixer.openWindow(MixerView::W_EQ); }
        if (!strcmp(screen, "mixeropto")) { app.mixer.setCursor(3, MixerView::R_COMP); app.mixer.openWindow(MixerView::W_COMP); }
        if (!strcmp(screen, "mixervca")) { p.tracks[3].comp.type = COMP_VCA; app.markDirty(); app.mixer.setCursor(3, MixerView::R_COMP); app.mixer.openWindow(MixerView::W_COMP); }
        if (!strcmp(screen, "mixerfet")) { app.mixer.setCursor(1, MixerView::R_COMP); app.mixer.openWindow(MixerView::W_COMP); }
        if (!strcmp(screen, "mixerbus")) { app.mixer.setCursor(-1, MixerView::R_COMP); app.mixer.openWindow(MixerView::W_BUS); }
        if (!strcmp(screen, "mixerfx")) { app.mixer.setCursor(1, MixerView::R_INS0); app.mixer.openWindow(MixerView::W_FX, 0); }
        if (!strcmp(screen, "mixerchorus")) { app.mixer.setCursor(2, MixerView::R_INS0); app.mixer.openWindow(MixerView::W_FX, 0); }
        if (!strcmp(screen, "mixersister")) { app.mixer.setCursor(p.nTracks, MixerView::R_INS0); app.mixer.openWindow(MixerView::W_FX, 0); }
        if (!strcmp(screen, "mixeraux")) app.mixer.setCursor(p.nTracks, MixerView::R_VOL);
        for (int i = 0; i < 60; i++) { eng.process(buf, 256); idle(app); Canvas tmp; app.render(tmp); }
    } else if (!strcmp(screen, "track")) {
        app.arrange.setCursor(4, 0);
        tap(app, B_A);  // ADD TRACK
    } else if (!strcmp(screen, "pattern")) {
        app.arrange.setCursor(2, 8);
        tap(app, B_Y);  // nowy pattern na KEYS
        tap(app, B_RIGHT);
        tap(app, B_RIGHT);
        tap(app, B_RIGHT);  // 4 takty
    } else if (!strcmp(screen, "trackedit")) {
        app.arrange.setCursor(1, 0);
        tap(app, B_SELECT);  // ustawienia sciezki LEAD
        tap(app, B_DOWN);    // kolor
    } else {  // arrange
        app.project().tracks[2].mute = true;   // KEYS wyciszone
        app.project().tracks[1].solo = true;   // LEAD solo
        app.project().tracks[0].solo = true;   // BASS solo
        app.markDirty();
        app.arrange.setCursor(1, 6);
        tap(app, B_START);
        for (int i = 0; i < 700; i++) eng.process(buf, 256);  // ~3.7 s
    }
    Canvas c;
    app.render(c);
    return savePng(c, path);
}

int headlessWav(const char* path, float seconds, const char* song) {
    const int sr = 48000, block = 256;
    int frames = (int)(seconds * sr);
    float* out = (float*)calloc((size_t)frames * 2 + block * 2, sizeof(float));
    static AudioEngine eng;
    eng.init((float)sr);
    static App app;
    app.init(&eng);
    if (song && !strcmp(song, "allengines")) app.loadDemoAllEngines();
    else if (song && !strcmp(song, "synthwave")) app.loadDemoSynthwave();
    else if (song && !strcmp(song, "synthwavemix")) app.loadDemoSynthwave(true);
    else if (song && !strcmp(song, "etude")) app.loadDemoEtude();
    else app.loadDemo();
    eng.send(EC_PLAY_SONG);
    for (int pos = 0; pos < frames; pos += block) eng.process(out + (size_t)pos * 2, block);
    bool ok = writeWav16(path, out, frames, sr);
    free(out);
    if (!ok) { fprintf(stderr, "Cannot write %s\n", path); return 1; }
    printf("WAV: %s (%.1f s)\n", path, seconds);
    return 0;
}

}  // namespace dp
