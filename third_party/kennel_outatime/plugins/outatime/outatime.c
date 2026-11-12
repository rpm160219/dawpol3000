/* Copyright (C) 2026 Hundekraft. GPL-3.0-or-later with the OP-1 firmware permission (see LICENSE and NOTICE).
 * The sound (ota_dsp.c, ota_tables.h) is opus's outatime by LizardMan (GPL-3.0, PROVENANCE.md). */
/* "outatime": opus's time-travel engine (LizardMan245/opus f0d2216, examples/opus.outatime) as a kennel synth. This
 * file is the glue in place of upstream's outatime.c; the sound is upstream's ota_dsp.c.
 *
 *   blue    year, 1955-2015          page 2: spread, the arms' detune      (preset word 5)
 *   green   speed, 0-88 mph          page 2: tone, darker or brighter       (preset word 6)
 *   white   power, 0-1.21 gigawatts  page 2: flux, the ensemble            (preset word 7)
 *   red     trail, the ignition      (no page 2)
 *
 * What the glue changes against upstream (README.md has the list):
 *   - The voices render as upstream's: 128 samples a block with upstream's tables, made for 88.2 kHz. On the unit the
 *     OS's voices run at 88.2 kHz (128 samples per 64-sample output block: OP1-Emulator
 *     docs/emu-audio-clock-2026-10-21.md, and opus's api.md); kennel's host harness and the emulator's default link
 *     write them out at 44.1 kHz, where drift and ignition run half speed and the filter sits an octave low.
 *   - op1_engine_t has no hook on the voices' sum, so upstream's block() (the overdrive with its level match, the DC
 *     blocker, the fizz lowpass, +4 dB and the soft limiter) runs per voice at the end of render, after the velocity:
 *     a chord is driven note by note (no intermodulation), and before the OS's envelope instead of after the mix.
 *     ota_dsp.c's OTA_FX_N block takes 128 samples at 88.2 kHz with the same times. A note_on resets its voice's
 *     followers, as upstream's sum rests after silence (the drive starts soft and the match comes up in 4 blocks).
 *   - Shift + turn is kennel's page 2 (encoder press): blue spread, green tone, white flux, in the preset words 5, 6, 7
 *     (upstream's knobs 5 spread, 6 flux, 7 tone: par[] is rebuilt in that order). Kennel starts a synth's page-2
 *     words at 0 (page2_defaults is FX-only), so a fresh sound has spread 0, flux 0 and tone 0 (upstream: 40, 30, 50);
 *     the "outatime" preset (presets.py) carries upstream's new sound.
 *   - Upstream's settings "always driving" and "overdrive type" have no synth-plugin equivalent: fixed at their
 *     defaults, the car drives only with a note and the drive's shape is warm (tanh).
 *   - The engine has no voice-build callback: a voice is initialised (ota_voice_init) at its first note. */
#include "op1_plugin.h"
#include "ota_dsp.h"

typedef struct {
    struct ota_voice v;
    struct ota_fx fx;
    int32_t ready;
} kv_voice;                              /* 64 B */

/* L1 state lease (op1_l1_claim_state, OP1_L1_API 2): the voices are persistent state at the partition's start, the
 * tables and the arms' sum after them (ota_place). The core copies the voices in from ota_home and back when another
 * engine takes the partition or the pool goes off. render and note_on claim first and use what ota_claim returns for
 * that call only; note_off (held) and the screen (the mirrors below) never touch ota_home. */
typedef struct { kv_voice vc[OP1_VOICES]; } ota_state;
#define OTA_STATE_BYTES ((uint32_t)sizeof(ota_state))
_Static_assert(sizeof(kv_voice) % 32 == 0 && sizeof(ota_state) % 32 == 0
               && OP1_L1_ALIGN(sizeof(ota_state)) + OTA_L1_BYTES <= 8192, "outatime's lease fits OP1_L1_SYNTH");
static ota_state ota_home __attribute__((aligned(32)));
static ota_state *ota_claim(void)
{
    ota_state *h = &ota_home;
#if defined(__bfin__) && !defined(ENGINE_HOST_TEST)
    int fresh;                           /* the tables after the state: copied in when fresh (ota_place) */
    uint8_t *l = op1_l1_claim_state(OP1_L1_SYNTH, h, OTA_STATE_BYTES, OP1_L1_ALIGN(OTA_STATE_BYTES) + OTA_L1_BYTES, &fresh);
    ota_place(l ? (short *)(void *)(l + OP1_L1_ALIGN(OTA_STATE_BYTES)) : 0, fresh);
    if (l)
        h = (ota_state *)(void *)l;
#endif
    return h;
}

/* for the screen (outatime_screen.c): render calls, the keys down, the newest note's voice, and each voice's drift and
 * ignition as render and note_on last left them (mirrors: the screen never reads the leased voices) */
volatile uint32_t outatime_blocks;
volatile int32_t outatime_held, outatime_newest = -1;
static volatile int32_t held[OP1_VOICES];   /* note_on sets, note_off clears: outside the lease */
static struct ota_voice mirror[OP1_VOICES];
const struct ota_voice *outatime_voice(int i) { return i >= 0 && i < OP1_VOICES ? &mirror[i] : 0; }
static void show(int i, const struct ota_voice *v)
{
    mirror[i].drift = v->drift;
    mirror[i].env = v->env;
}

static void note_on(void *self, const int16_t *params)
{
    const int i = op1_voice_index(self);
    kv_voice *const w = &ota_claim()->vc[i];
    (void)params;
    if (!w->ready) {
        ota_voice_init(&w->v);
        w->ready = 1;
    }
    ota_note_on(&w->v);
    ota_fx_init(&w->fx);                 /* shape stays 0: OTA_WARM */
    show(i, &w->v);
    if (!held[i]) {
        held[i] = 1;
        outatime_held++;
    }
    outatime_newest = i;
}

static void note_off(void *self)
{
    const int i = op1_voice_index(self);
    if (held[i]) {
        held[i] = 0;
        outatime_held--;
    }
}

static void render(void *self, int16_t *out, const int16_t *params)
{
    const int vi = op1_voice_index(self);
    kv_voice *const w = &ota_claim()->vc[vi];   /* the tables placed too: L1 or the SDRAM copies */
    if (!w->ready) {
        ota_voice_init(&w->v);
        ota_fx_init(&w->fx);
        w->ready = 1;
    }
    /* upstream's knob order: 0-3 the four, 5 spread, 6 flux, 7 tone */
    const short par[8] __attribute__((aligned(4))) =   /* word-aligned: ota_voice_block compares it as words */
        { params[0], params[1], params[2], params[3], params[4], params[5], params[7], params[6] };
    /* the voice, the OS's velocity before the sum (upstream's order), the sum's drive: ota_dsp.c */
    ota_voice_block(&w->v, &w->fx, out, par, (unsigned)OP1_PITCH_INC[0], op1_voice_vel_gain(self));
    show(vi, &w->v);
    outatime_blocks++;
}

const op1_engine_t ENGINE_outatime = { note_on, render, note_off,
                                       { OP1_PRESS_PAGE, OP1_PRESS_PAGE, OP1_PRESS_PAGE, OP1_PRESS_NONE } };
