#include "headless.h"
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

int headlessWav(const char* path, float seconds) {
    const int sr = 48000, block = 256;
    int frames = (int)(seconds * sr);
    float* out = (float*)calloc((size_t)frames * 2 + block * 2, sizeof(float));
    static AudioEngine eng;
    eng.init((float)sr);
    static App app;
    app.init(&eng);
    app.loadDemo();
    eng.send(EC_PLAY_SONG);
    for (int pos = 0; pos < frames; pos += block) eng.process(out + (size_t)pos * 2, block);
    bool ok = writeWav16(path, out, frames, sr);
    free(out);
    if (!ok) { fprintf(stderr, "Cannot write %s\n", path); return 1; }
    printf("WAV: %s (%.1f s)\n", path, seconds);
    return 0;
}

}  // namespace dp
