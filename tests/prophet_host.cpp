#include "audio/engine.h"
#include "io/project_file.h"
#include "io/preset_store.h"
#include "synth/prophet_synth.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
using namespace dp;
static void check(bool ok, const char *why) {
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", why);
        std::exit(1);
    }
}
int main(int argc, char **argv) {
    check(argc == 2, "temporary data directory argument");
    auto p = std::make_unique<Project>();
    p->nTracks = 1;
    p->tracks[0].engine = ENG_PROPHET;
    prophetEnsure(p->tracks[0]);
    char path[256], error[256];
    std::snprintf(path, sizeof(path), "%s/roundtrip.dpp", argv[1]);
    check(saveProjectFile(*p, path), "save project");
    auto restored = std::make_unique<Project>();
    check(loadProjectFile(*restored, path, error, sizeof(error)), "load project");
    check(restored->tracks[0].engine == ENG_PROPHET && restored->tracks[0].prophetSet,
          "native engine survives reload");
    check(!std::memcmp(p->tracks[0].prophet, restored->tracks[0].prophet, sizeof(p->tracks[0].prophet)),
          "all native controls survive reload");
    PresetStore store;
    store.init(argv[1], "prophet5");
    check(store.count() == 24, "complete factory bank installs");
    for (int i = 0; i < 24; i++) {
        Track t;
        check(store.load(i, t), "load factory preset");
        check(t.engine == ENG_PROPHET && t.prophetSet, "factory patch restores native engine");
        int16_t words[PROPHET_PARAMS];
        int original = -1;
        for (int j = 0; j < 24; j++)
            if (!std::strcmp(store.name(i), prophetPresetName(j)))
                original = j;
        check(original >= 0, "known factory patch name");
        prophetPreset(original, words);
        check(!std::memcmp(words, t.prophet, sizeof(words)),
              "factory editor and browser use identical parameters");
    }
    // Arbitrary sequencer segments cannot alter the waveform or LFO clock.
    ProphetSynth whole, split;
    whole.init(48000);
    split.init(48000);
    whole.configure(p->tracks[0].prophet, 0);
    split.configure(p->tracks[0].prophet, 0);
    whole.noteOn(60, .8f);
    split.noteOn(60, .8f);
    float a[256] = {}, b[256] = {};
    whole.render(a, 128);
    split.render(b, 37);
    split.render(b + 74, 91);
    for (int i = 0; i < 256; i++)
        check(std::fabs(a[i] - b[i]) < 1e-6f, "sequencer segmentation preserves audio");
    // The shared WT controls must have no influence on Prophet output.
    auto e1 = std::make_unique<AudioEngine>();
    auto e2 = std::make_unique<AudioEngine>();
    e1->init(48000);
    e2->init(48000);
    auto altered = std::make_unique<Project>(*p);
    Track &t = altered->tracks[0];
    t.env[0] = 10;
    t.env[2] = 0;
    t.fenvAmt = 1;
    t.mono = true;
    t.glide = 3;
    t.age = 1;
    t.unison = 1;
    t.lfo[0].depth = 1;
    t.lfo[0].dest = DEST_PITCH;
    e1->projectBack() = *p;
    e1->publishProject();
    e2->projectBack() = *altered;
    e2->publishProject();
    e1->send(EC_NOTE_ON, 0, 60, 0, .8f);
    e2->send(EC_NOTE_ON, 0, 60, 0, .8f);
    for (int k = 0; k < 10; k++) {
        e1->process(a, 128);
        e2->process(b, 128);
        for (int i = 0; i < 256; i++)
            check(std::fabs(a[i] - b[i]) < 1e-6f,
                  "no WavetableSynth ADSR/LFO/AGE/unison/glide in native path");
    }
    std::puts("PASS: project/preset roundtrip, 24 factory patches, event segments, independent host path");
    return 0;
}
