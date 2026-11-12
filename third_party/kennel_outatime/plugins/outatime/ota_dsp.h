/* Imported from opus (github.com/LizardMan245/opus, commit f0d2216, src/plugin-kit/examples/opus.outatime/ota_dsp.h).
 * Copyright (C) 2026 LizardMan. GPL-3.0 (the opus repository's LICENSE); unchanged but for this header. */
/* outatime's sound (ota_dsp.c): plain C, for the engine and the host renders. */
#ifndef OTA_DSP_H
#define OTA_DSP_H

/* The knobs (0-32767): 0 YEAR, 1 SPEED, 2 POWER, 3 TRAIL (blue, green, white, orange); under shift 5 SPREAD (blue),
 * 6 FLUX (white), 7 TONE (green). Knob 4 is the core's. */
enum { OTA_YEAR, OTA_SPEED, OTA_POWER, OTA_TRAIL, OTA_MARK, OTA_SPREAD, OTA_FLUX, OTA_TONE };

/* A voice: three oscillators (the flux capacitor's arms), at 88.2 kHz. */
struct ota_voice {
    unsigned ph[3];                        /* the arms' phases (2^32 a cycle) */
    unsigned drift;                        /* the time drift's phase; arm k reads it k thirds of a cycle on */
    unsigned flux;                         /* the ensemble's phase, the same way */
    int env;                               /* the ignition: 1 at the note, burning down, Q15 */
    int low, band;                         /* its filter (a state-variable filter) */
};

/* The voices' sum: the overdrive (64 samples at 44.1 kHz). Its shapes (the setting "overdrive type"). */
enum { OTA_WARM, OTA_HARD, OTA_TUBE, OTA_FOLD, OTA_SHAPES };
struct ota_fx {
    int shape;                             /* OTA_WARM .. OTA_FOLD, set from the plugin's setting */
    int in, out;                           /* the level followers: the sum before the drive, the drive's (Q23) */
    int gain;                              /* the level match now (Q12) */
    int hp_x, hp_y;                        /* the DC blocker after the drive (the tube's asymmetry) */
    int lp;                                /* the fizz's lowpass after the drive */
};

void ota_voice_init(struct ota_voice *v);
void ota_note_on(struct ota_voice *v);
void ota_render(struct ota_voice *v, short *out, const short *par, unsigned pitch);
void ota_fx_init(struct ota_fx *f);
void ota_block(struct ota_fx *f, short *buf, const short *par);
/* The L1 pool (OP1_L1_SYNTH, 8,192 B; 7,296 B here, after the voices' state lease in outatime.c): the era waves, the
 * tanh pairs, the arms' sum and the kernel's parameter block at l1 (copied in first when fresh), or l1 = 0: all in
 * SDRAM. Call it every render and note_on call (outatime.c's ota_claim), before ota_render. */
#define OTA_L1_BYTES 7296
void ota_place(short *l1, int fresh);
/* kennel: a voice's block as outatime.c runs it: ota_render, the OS's velocity gain vg (32767: none), ota_block. On
 * the Blackfin the kernel (ota_kern.S) does it, the C loops with OTA_C_LOOP and in host builds. par: word-aligned. */
void ota_voice_block(struct ota_voice *v, struct ota_fx *fx, short *out, const short *par, unsigned pitch, int vg);

/* For the page: the knobs as the page shows them. */
int ota_year(int knob);                    /* 1955-2015 */
int ota_mph(int knob);                     /* 0-88 */
int ota_gw100(int knob);                   /* gigawatts x 100: 0-121 */
int ota_arm_year(const short *par, unsigned drift, int k);   /* arm k (0-2) now, by its voice's drift */

#endif
