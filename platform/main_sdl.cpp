// Warstwa platformy SDL2: okno, wejscie, audio. Plus tryby headless.
#include <SDL.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "audio/engine.h"
#include "gfx/canvas.h"
#include "headless.h"
#include "synth/tone_synth.h"
#include "ui/app.h"
#include "ui/test_screen.h"
#include "controls_map.h"

using namespace dp;

static ToneSynth gSynth;     // ekran testowy
static AudioEngine gEngine;  // glowny silnik
static bool gTestMode = false;

static void audioCb(void*, Uint8* stream, int len) {
    int frames = len / (int)(sizeof(float) * 2);
    if (gTestMode) gSynth.process((float*)stream, frames);
    else gEngine.process((float*)stream, frames);
}

// Hotkey+Start w Knulli robi "killall" (SIGTERM): konczymy normalnie, zeby zadzialal autozapis.
static volatile sig_atomic_t gTerm = 0;
static void onTerm(int) { gTerm = 1; }

static void usage() {
    printf("dawpol3000 [--test] [--fullscreen] [--frames N] [--render-png file.png [arrange|track|pattern|roll|test]] [--render-wav file.wav [seconds]]\n");
}

int main(int argc, char** argv) {
    setvbuf(stdout, nullptr, _IOLBF, 0);  // logi od razu widoczne (np. przez ssh)
    int frames = 256;
#ifdef __aarch64__
    bool fullscreen = true;
#else
    bool fullscreen = false;
#endif
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--render-png") && i + 1 < argc) return headlessPng(argv[i + 1], i + 2 < argc ? argv[i + 2] : "arrange");
        if (!strcmp(argv[i], "--render-wav") && i + 1 < argc) {
            float secs = (i + 2 < argc) ? (float)atof(argv[i + 2]) : 5.f;
            return headlessWav(argv[i + 1], secs > 0 ? secs : 5.f);
        }
        if (!strcmp(argv[i], "--frames") && i + 1 < argc) frames = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--fullscreen")) fullscreen = true;
        else if (!strcmp(argv[i], "--window")) fullscreen = false;
        else if (!strcmp(argv[i], "--test")) gTestMode = true;
        else { usage(); return 1; }
    }

    signal(SIGTERM, onTerm);
    signal(SIGINT, onTerm);
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    printf("SDL video: %s, audio: %s\n", SDL_GetCurrentVideoDriver(), SDL_GetCurrentAudioDriver());

    SDL_Window* win = SDL_CreateWindow("DAW-POL 3000", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       Canvas::W, Canvas::H, fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
    if (!win) { fprintf(stderr, "Window: %s\n", SDL_GetError()); return 1; }
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!ren) { fprintf(stderr, "Renderer: %s\n", SDL_GetError()); return 1; }
    SDL_RenderSetLogicalSize(ren, Canvas::W, Canvas::H);
    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING,
                                         Canvas::W, Canvas::H);
    SDL_ShowCursor(SDL_DISABLE);

    // Audio: 48 kHz, float, stereo.
    AudioInfo ai;
    gSynth.init(48000.f);
    gEngine.init(48000.f);
    SDL_AudioSpec want = {}, have = {};
    want.freq = 48000;
    want.format = AUDIO_F32SYS;
    want.channels = 2;
    want.samples = (Uint16)frames;
    want.callback = audioCb;
    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
    if (dev) {
        ai.ok = true;
        ai.rate = have.freq;
        ai.frames = have.samples;
        ai.channels = have.channels;
        snprintf(ai.driver, sizeof ai.driver, "%s", SDL_GetCurrentAudioDriver());
        if (have.freq != 48000) { gSynth.init((float)have.freq); gEngine.init((float)have.freq); }
        printf("Audio: %d Hz, %d frames, %d ch\n", have.freq, have.samples, have.channels);
        SDL_PauseAudioDevice(dev, 0);
    } else {
        snprintf(ai.err, sizeof ai.err, "%s", SDL_GetError());
        fprintf(stderr, "Audio: %s\n", SDL_GetError());
    }

    Canvas canvas;
    TestScreen app;
    app.init(&gSynth);
    static App app2;  // duzy obiekt (projekt) — nie na stosie
    app2.init(&gEngine);
    app2.openStore("projects");
    app2.openPresets("presets");  // presety instrumentow (konsola: /userdata/dawpol3000/presets)  // katalog wzgledem biezacego (konsola: /userdata/dawpol3000/projects)
    InputState in;
    SDL_GameController* pad = nullptr;
    SDL_Joystick* joy = nullptr;
    bool keyAxis[sizeof(kKeyAxis) / sizeof(kKeyAxis[0])] = {};
    float joyAxis[AXIS_COUNT] = {}, padAxis[AXIS_COUNT] = {};
    char line[48];
    bool rawHeld[32] = {};

    Uint64 last = SDL_GetPerformanceCounter();
    float fps = 0;
    bool run = true;
    while (run) {
        if (gTerm) run = false;
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT: run = false; break;
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                bool down = e.type == SDL_KEYDOWN;
                if (down && e.key.keysym.sym == SDLK_ESCAPE) run = false;
                for (auto& m : kKeyMap) if (m.key == e.key.keysym.sym) in.btn[m.btn] = down;
                for (size_t i = 0; i < sizeof(kKeyAxis) / sizeof(kKeyAxis[0]); i++)
                    if (kKeyAxis[i].key == e.key.keysym.sym) keyAxis[i] = down;
                break;
            }
            case SDL_JOYDEVICEADDED: {
                int idx = e.jdevice.which;
                if (SDL_IsGameController(idx) && !pad) {
                    pad = SDL_GameControllerOpen(idx);
                    snprintf(in.pad, sizeof in.pad, "PAD: %s", SDL_GameControllerName(pad));
                } else if (!joy && !pad) {
                    joy = SDL_JoystickOpen(idx);
                    snprintf(in.pad, sizeof in.pad, "JOY: %s", SDL_JoystickName(joy));
                }
                printf("Device %d: %s (controller=%d)\n", idx, SDL_JoystickNameForIndex(idx),
                       SDL_IsGameController(idx));
                break;
            }
            // Surowe zdarzenia — zawsze logowane (do mapowania na konsoli).
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP: {
                bool down = e.type == SDL_JOYBUTTONDOWN;
                snprintf(line, sizeof line, "JOY%d BTN %d %s", e.jbutton.which, e.jbutton.button, down ? "DOWN" : "UP");
                in.logRaw(line);
                printf("%s\n", line);
                if (!pad)
                    for (auto& m : kRawBtnMap) if (m.joyButton == e.jbutton.button) in.btn[m.btn] = down;
                if (e.jbutton.button < 32) rawHeld[e.jbutton.button] = down;
                if (rawHeld[kRawQuitA] && rawHeld[kRawQuitB]) run = false;
                break;
            }
            case SDL_JOYHATMOTION:
                snprintf(line, sizeof line, "JOY%d HAT %d = %d", e.jhat.which, e.jhat.hat, e.jhat.value);
                in.logRaw(line);
                printf("%s\n", line);
                if (!pad) {
                    in.btn[B_UP] = e.jhat.value & SDL_HAT_UP;
                    in.btn[B_DOWN] = e.jhat.value & SDL_HAT_DOWN;
                    in.btn[B_LEFT] = e.jhat.value & SDL_HAT_LEFT;
                    in.btn[B_RIGHT] = e.jhat.value & SDL_HAT_RIGHT;
                }
                break;
            case SDL_JOYAXISMOTION: {
                float v = e.jaxis.value / 32767.f;
                static float lastLogged[16];
                int a = e.jaxis.axis;
                if (a < 16 && (v - lastLogged[a] > 0.25f || lastLogged[a] - v > 0.25f)) {
                    lastLogged[a] = v;
                    snprintf(line, sizeof line, "JOY%d AXIS %d %+.2f", e.jaxis.which, a, v);
                    in.logRaw(line);
                    printf("%s\n", line);
                }
                for (auto& m : kRawAxisMap) if (m.joyAxis == a) joyAxis[m.axis] = v * m.scale;
                break;
            }
            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP: {
                bool down = e.type == SDL_CONTROLLERBUTTONDOWN;
                for (auto& m : kPadMap) if (m.b == e.cbutton.button) in.btn[m.btn] = down;
                break;
            }
            case SDL_CONTROLLERAXISMOTION: {
                float v = e.caxis.value / 32767.f;
                switch (e.caxis.axis) {
                case SDL_CONTROLLER_AXIS_LEFTX: padAxis[AX_LX] = v; break;
                case SDL_CONTROLLER_AXIS_LEFTY: padAxis[AX_LY] = v; break;
                case SDL_CONTROLLER_AXIS_RIGHTX: padAxis[AX_RX] = v; break;
                case SDL_CONTROLLER_AXIS_RIGHTY: padAxis[AX_RY] = v; break;
                default: break;
                }
                if (e.caxis.axis == kPadL2) in.btn[B_L2] = v > 0.5f;
                if (e.caxis.axis == kPadR2) in.btn[B_R2] = v > 0.5f;
                break;
            }
            }
        }
        for (int a = 0; a < AXIS_COUNT; a++) {
            float v = pad ? padAxis[a] : joyAxis[a];
            for (size_t i = 0; i < sizeof(kKeyAxis) / sizeof(kKeyAxis[0]); i++)
                if (keyAxis[i] && kKeyAxis[i].axis == a) v = kKeyAxis[i].dir;
            in.axis[a] = v;
        }

        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - last) / (float)SDL_GetPerformanceFrequency();
        last = now;
        if (dt > 0.1f) dt = 0.1f;
        fps = fps * 0.95f + (dt > 0 ? 1.f / dt : 0) * 0.05f;

        if (gTestMode) {
            app.update(in, dt);
            if (app.quitRequested()) run = false;
            app.render(canvas, in, ai, fps);
        } else {
            app2.update(in, dt);
            if (app2.quitRequested()) run = false;
            app2.render(canvas);
        }
        SDL_UpdateTexture(tex, nullptr, canvas.pixels(), Canvas::W * 4);
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, nullptr, nullptr);
        SDL_RenderPresent(ren);
        if (dt < 0.012f) SDL_Delay(4);  // gdy brak vsync
    }

    if (!gTestMode) app2.shutdown();  // autozapis
    SDL_Delay(30);
    if (dev) SDL_CloseAudioDevice(dev);
    if (pad) SDL_GameControllerClose(pad);
    if (joy) SDL_JoystickClose(joy);
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
