# Gin for kennel

Optional native drum synth by SquareWave Studio, derived from libpo32 by Eric Lewis. Four synthesis voices, 24 sounds per kit, per-sound A/B morph/endpoints, full Advanced editor and eight factory kits adapted from CNTR RNDM.

`vendor/` contains the corresponding native source pinned to `joseph-holland/op1r-src` `ec5d6f722df05b77f80706db239352992937f4d7`, linked by SquareWave Studio’s Gin 1.0 release. Its notices, component licences, original integration documents and preset permission records are retained. Files adjusted for kennel no longer match upstream’s SOURCE_SHA256SUMS; that receipt records upstream provenance. The vendored standalone Makefile/README describe the original module, not this port.

The live build is `plugin.json` plus `gin_feature.c` and `prepare.py`. Kennel owns native drum constructors/voices, shared screen/input/preset hooks and guarded callback stacks. Gin owns synthesis, rate conversion, UI, kit state and transactional saves. Old standalone patch assembly and fixed memory ranges are not linked into the plugin. The validated `op1_drum_synth_t` lives in the feature descriptor; registration only runs in the enabled-feature registration phase. Ordinary edits and named snapshots remain distinct.

Preparation generates font/prefix inputs locally from the verified R246 and builds matched bootstrap/factory coefficient data from source. Generated files live only under `synth_sdk/build/gin/`; do not commit them. Node with Playwright and a local Chrome installation is needed for the font geometry step. No firmware assets or fonts are downloaded. Source includes are preserved so changing preparation, factory data, DSP or geometry rebuilds the corresponding plugin.

Keep `GIN_FAST_FIXED=1`, `GIN_FW_OUTPUT_SHIFT=16`, `GIN_RATE_2X=1`, `GIN_FACTORY_PACKED_INDEX=1` and `GIN_SYNTH_VOICE_LIMIT=4` for this port. The audio interface is 64 samples at 44.1 kHz reconstructed to 128 native DBox samples with the 33-tap 2x FIR. DSP/preparation use O2; large screen/snapshot code uses Os. GUI preparation may use software floating point; audio remains fixed-point.
