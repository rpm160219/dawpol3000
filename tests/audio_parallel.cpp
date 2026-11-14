#include "audio/engine.h"
#include "io/preset_store.h"
#include "synth/arc_dsp.h"
#include "fx/fx_registry.h"
#include "synth/polivoks_presets.h"
#include <algorithm>
#include <atomic>
#include <cfenv>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <thread>
#include <vector>
using namespace dp;
static void check(bool ok, const char *text) {
    if (!ok) {
        std::fprintf(stderr, "FAIL %s\n", text);
        std::exit(1);
    }
}

static void setup(Track &t, int engine) {
    t = Track();
    t.engine = (uint8_t)engine;
    PresetStore::initInstrument(t);
    t.volume = .32f;
    t.env[3] = .003f; // retire legacy host voices before the next engine case
    t.pan = 0;
    t.age = 0;
    t.unison = 0;
    if (engine == ENG_POLIVOKS) {
        polivoksLoadPreset(1, t.polivoks);
        t.polivoksSet = true;
    }
    if (engine == ENG_PROPHET) {
        prophetPreset(0, t.prophet);
        t.prophetSet = true;
    }
    if (engine == ENG_GIN)
        ginEnsure(t);
}
static void publish(AudioEngine &e, const Project &p) {
    e.projectBack() = p;
    e.publishProject();
}
static void output(FILE *f, const float *x, int n) {
    for (int i = 0; i < n * 2; i++)
        check(std::isfinite(x[i]), "finite audio");
    check(std::fwrite(x, sizeof(float), n * 2, f) == (size_t)n * 2, "write comparison stream");
}
static void renderCases(int threads, const char *path) {
    auto engine = std::make_unique<AudioEngine>();
    auto project = std::make_unique<Project>();
    auto &e = *engine;
    auto &p = *project;
    e.init(48000, threads);
    FILE *f = std::fopen(path, "wb");
    check(f, "open stream");
    float out[1026];
    // All engines, including multiple copies of legacy global-context ports,
    // interleaved with native parallel tracks. Exercise exact partial blocks.
    const int frames[] = {1, 17, 64, 127, 256, 513, 31};
    p.nTracks = 6;
    for (int kind = 0; kind < ENG_COUNT; kind++) {
        std::fprintf(stderr, "CASE %d %s\n", kind, engineName(kind));
        e.send(EC_STOP);
        for (int i = 0; i < 6; i++)
            setup(p.tracks[i], i % 2 ? ENG_WAVETABLE : kind);
        if (kind % 2 == 0)
            setup(p.tracks[5], ENG_PROPHET);
        else
            setup(p.tracks[5], ENG_POLIVOKS);
        if (kind == ENG_WAVETABLE || kind == ENG_OXBOW)
            p.tracks[0].alt = true; // cover original and + DSP in the same batch
        p.bpm = 87 + kind * 7;
        publish(e, p);
        // ARC's original noise seed contains its heap address. Fix it only
        // in the comparison fixture so ASLR cannot change the reference.
        if (kind == ENG_ARC)
            for (int t = 0; t < 6; t++)
                if (t % 2 == 0)
                    for (int v = 0; v < 6; v++)
                        arcSetSeed(const_cast<ArcCore *>(e.arcCore(t)), v, 0x9e3779b9u * (1 + t * 6 + v));
        for (int t = 0; t < 6; t++) {
            for (int v = 0; v < 3; v++)
                e.send(EC_NOTE_ON, t, (kind == ENG_GIN ? 36 : 48) + v * 4 + t, 0, .7f);
        }
        for (int k = 0; k < 14; k++) {
            int n = frames[k % 7];
            e.process(out, n);
            output(f, out, n);
        }
        for (int t = 0; t < 6; t++)
            for (int v = 0; v < 3; v++)
                e.send(EC_NOTE_OFF, t, (kind == ENG_GIN ? 36 : 48) + v * 4 + t);
        for (int k = 0; k < 4; k++) {
            e.process(out, 127);
            output(f, out, 127);
        }
    }
    // Sample-accurate sequencing, loops, mutes, engine changes, BPM, structural
    // deletion while playing and live commands between callbacks.
    p.nTracks = 6;
    p.nPatterns = 6;
    p.nClips = 6;
    p.loopOn = true;
    p.loopIn = 0;
    p.loopOut = 1;
    p.bpm = 287;
    int types[] = {ENG_PROPHET, ENG_POLIVOKS, ENG_PARALLAX, ENG_LOUNGE, ENG_FELT, ENG_GRAND};
    for (int t = 0; t < 6; t++) {
        setup(p.tracks[t], types[t]);
        p.patterns[t] = Pattern();
        for (int i = 0; i < 20; i++)
            p.patterns[t].add({(uint16_t)(i * 17), (uint16_t)(11 + i % 3), (uint8_t)(44 + i % 12 + t),
                               (uint8_t)(70 + i % 50)});
        p.clips[t] = {(uint8_t)t, (uint8_t)t, 0};
    }
    fxDefaults(p.tracks[0].ins[0], FX_JUNI_CHORUS);
    fxDefaults(p.aux[0].fx, FX_SISTER);
    p.tracks[0].send[0] = .23f;
    p.tracks[2].send[0] = .17f;
    p.tracks[1].comp.on = true;
    p.tracks[1].comp.type = COMP_VCA;
    p.tracks[3].eq.gain[0] = 2.0f;
    p.bus.on = true;
    publish(e, p);
    e.send(EC_PLAY_SONG);
    for (int k = 0; k < 100; k++) {
        if (k == 17) {
            p.tracks[1].mute = true;
            p.bpm = 113;
            publish(e, p);
        }
        if (k == 28) {
            setup(p.tracks[2], ENG_OXBOW);
            publish(e, p);
        }
        if (k == 40) {
            p.tracks[0].prophet[P5_REVISION] = 32767;
            publish(e, p);
            e.send(EC_PROPHET_TUNE, 0);
        }
        if (k == 60) {
            p.nTracks = 4;
            publish(e, p);
        }
        if (k == 70)
            e.send(EC_PLAY_PATTERN, 0, 0, 0);
        if (k == 80)
            e.send(EC_STOP);
        int n = frames[k % 7];
        e.process(out, n);
        output(f, out, n);
    }
    std::fclose(f);
    std::printf("PASS render: threads=%d, all engines, MIDI/events, tails, loops, edits/deletion\n",
                e.renderThreads());
}
static void benchmark(int threads, int tracks = 3, bool paced = false) {
    auto engine = std::make_unique<AudioEngine>();
    auto project = std::make_unique<Project>();
    auto &e = *engine;
    auto &p = *project;
    e.init(48000, threads);
    p.nTracks = tracks;
    p.nPatterns = 0;
    p.nClips = 0;
    for (int t = 0; t < tracks; t++)
        setup(p.tracks[t], ENG_PROPHET);
    publish(e, p);
    float out[512];
    e.process(out, 256);
    for (int t = 0; t < tracks; t++)
        for (int v = 0; v < 5; v++)
            e.send(EC_NOTE_ON, t, 48 + v * 3 + t, 0, .8f);
    for (int i = 0; i < 16; i++)
        e.process(out, 256);
    std::vector<double> costs;
    const int batches = paced ? 600 : 240;
    costs.reserve(batches);
    int late = 0;
    double energy = 0;
    auto start = std::chrono::steady_clock::now();
    for (int k = 0; k < batches; k++) {
        auto a = std::chrono::steady_clock::now();
        e.process(out, 256);
        costs.push_back(std::chrono::duration<double>(std::chrono::steady_clock::now() - a).count());
        for (float x : out)
            energy += x * x;
        if (costs.back() > 256 / 48000.)
            ++late;
        if (paced)
            std::this_thread::sleep_until(start + std::chrono::duration<double>((k + 1) * 256 / 48000.));
    }
    double wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::sort(costs.begin(), costs.end());
    check(energy > .001, "audible benchmark");
    std::printf("BENCH threads=%d: %d Prophets x5 voices, paced=%d, wall/audio=%.3f, median=%.3fms "
                "p95=%.3fms max=%.3fms (buffer 5.333ms), late=%d/%d, energy=%.9f\n",
                e.renderThreads(), tracks, paced, wall / (batches * 256 / 48000.), costs[batches / 2] * 1000,
                costs[batches * 95 / 100] * 1000, costs.back() * 1000, late, batches, energy);
}
int main(int argc, char **argv) {
    check(argc >= 2, "mode required");

    check(argc >= 3, "thread count required");
    int n = std::atoi(argv[2]);
    if (!std::strcmp(argv[1], "pace")) {
        benchmark(n, 4, true);
        return 0;
    }
    if (!std::strcmp(argv[1], "bench")) {
        benchmark(n);
        return 0;
    }
    check(argc == 4, "output path required");
    renderCases(n, argv[3]);
    return 0;
}
