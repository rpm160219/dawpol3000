#include "headless.h"
#include "audio/engine.h"
#include "gfx/canvas.h"
#include "io/image_io.h"
#include "ui/piano_roll.h"
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

int headlessPng(const char* path, const char* screen) {
    if (screen && !strcmp(screen, "test")) return testScreenPng(path);
    static AudioEngine eng;
    eng.init(48000.f);
    PianoRoll roll;
    roll.init(&eng);
    roll.loadDemo();
    roll.setCursor(8, 72);
    InputState in;
    roll.update(in, 0.016f);
    in.btn[B_START] = true;  // play, i przewin troche, zeby byla widoczna glowica
    roll.update(in, 0.016f);
    float buf[2 * 256];
    for (int i = 0; i < 1300; i++) eng.process(buf, 256);  // ~6.9 s -> srodek taktu
    Canvas c;
    roll.render(c);
    return savePng(c, path);
}

int headlessWav(const char* path, float seconds) {
    const int sr = 48000, block = 256;
    int frames = (int)(seconds * sr);
    float* out = (float*)calloc((size_t)frames * 2 + block * 2, sizeof(float));
    static AudioEngine eng;
    eng.init((float)sr);
    PianoRoll roll;
    roll.init(&eng);
    roll.loadDemo();
    eng.send(EC_PLAY);
    for (int pos = 0; pos < frames; pos += block) eng.process(out + (size_t)pos * 2, block);
    bool ok = writeWav16(path, out, frames, sr);
    free(out);
    if (!ok) { fprintf(stderr, "Cannot write %s\n", path); return 1; }
    printf("WAV: %s (%.1f s)\n", path, seconds);
    return 0;
}

}  // namespace dp
