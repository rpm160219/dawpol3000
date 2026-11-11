// Tryb bez okna: makieta ekranu do PNG i probka dzwieku do WAV.
#include "headless.h"
#include "canvas.h"
#include "hello_app.h"
#include "image_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace dp {

int headlessPng(const char* path) {
    static ToneSynth syn;
    syn.init(48000.f);
    HelloApp app;
    app.init(&syn);
    app.setEncoder(1, 0.72f);
    app.setEncoder(2, 0.38f);
    app.setEncoder(3, 0.45f);
    InputState in;
    strcpy(in.pad, "PAD: DESKTOP (HEADLESS)");
    in.logRaw("JOY0 BTN 3 DOWN");
    in.logRaw("JOY0 AXIS 1 -0.62");
    app.update(in, 0.016f);
    in.btn[B_A] = true;   // A = 7. stopien w trybie KEYS
    in.axis[AX_LX] = 0.0f; in.axis[AX_LY] = -0.62f; in.axis[AX_RX] = 0.4f; in.axis[AX_RY] = 0.1f;
    app.update(in, 0.0f);
    float buf[2 * 2048];
    for (int i = 0; i < 4; i++) syn.process(buf, 2048);
    AudioInfo ai;
    ai.ok = true; ai.rate = 48000; ai.frames = 256; ai.channels = 2;
    strcpy(ai.driver, "ALSA");
    Canvas c;
    app.render(c, in, ai, 60.f);
    if (!writePng(path, c.pixels(), Canvas::W, Canvas::H)) { fprintf(stderr, "Nie moge zapisac %s\n", path); return 1; }
    printf("PNG: %s\n", path);
    return 0;
}

int headlessWav(const char* path, float seconds) {
    const int sr = 48000, block = 256;
    int frames = (int)(seconds * sr);
    float* out = (float*)calloc((size_t)frames * 2 + block * 2, sizeof(float));
    static ToneSynth syn;
    syn.init((float)sr);
    HelloApp app;
    app.init(&syn);
    InputState in;
    static const int notes[8] = {B_LEFT, B_DOWN, B_RIGHT, B_UP, B_Y, B_B, B_A, B_X};
    float dt = (float)block / sr;
    for (int pos = 0, step = 0; pos < frames; pos += block, step++) {
        float t = (float)pos / sr;
        int n = (int)(t / 0.3f);
        memset(in.btn, 0, sizeof in.btn);
        float within = t - n * 0.3f;
        if (n < 16 && within < 0.22f) in.btn[notes[n % 8]] = true;
        // druga polowa: drive rosnie (prawy analog w gore)
        in.axis[AX_RY] = t > seconds * 0.5f ? -0.6f : 0.f;
        app.update(in, dt);
        syn.process(out + (size_t)pos * 2, block);
    }
    bool ok = writeWav16(path, out, frames, sr);
    free(out);
    if (!ok) { fprintf(stderr, "Nie moge zapisac %s\n", path); return 1; }
    printf("WAV: %s (%.1f s)\n", path, seconds);
    return 0;
}

}  // namespace dp
