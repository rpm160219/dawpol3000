/* OXBOW+ (DAW-POL 3000): wersja rozszerzona oxbow.c — ZMODYFIKOWANA KOPIA, nie oryginal (oryginal 1:1: oxbow.c obok).
 * Zmiany DAW-POL (2026-11-13): parametry, ktore w oryginale sa stalymi neutralnego glosu, sa tu ustawiane przez
 * sciezke (struktura obx_params, wypelniana przez core/synth/oxbow_plus_dsp.c przed kazdym wywolaniem): fala,
 * interwal i poziom kazdego oscylatora, strojenie OSC 1, szerokosci pulsu, szum, SYNC, X-MOD, obwiednia filtra na
 * wysokosc OSC 2, sledzenie klawiatury, velocity -> filtr, VINTAGE, czasy i podtrzymanie obwiedni filtra.
 * Tryby fal z galki WAVE (slowo 5) zastapione niezaleznymi oscylatorami; pamiec podreczna voice_block wylaczona
 * (parametry zmieniaja sie z modulacja). Reszta kodu bez zmian. Licencja jak oryginal (zob. ponizej i README). */
#define ENGINE_oxbow ENGINE_oxbow_plus
#define SCREEN_oxbow SCREEN_oxbow_plus
/* Oxbow: zvenson's OberVave, an 8-voice American poly of 1979-84 in the OB-X / OB-8 manner from his FM-1 firmware, as a
 * kennel synth plugin (loaded from oxbow.knl). Per voice: two oscillators (saw, pulse, saw+pulse; OSC 2 with hard
 * sync, X-MOD into OSC 1, the filter envelope on its pitch), noise, drive, a state-variable filter (LP 12, HP 12,
 * BP 12, notch, LP 24, SEM morph), a filter ADSR with RC curves and the stock T2 amp envelope, one LFO, and each voice card's own
 * small tune, cutoff and envelope offsets plus a slow pitch drift ("vintage").
 *
 * blue   (knob 0) CUTOFF
 * green  (knob 1) RESONANCE
 * white  (knob 2) ENV AMOUNT: the filter envelope, bipolar (middle = none)
 * orange (knob 3) DETUNE: OSC 2 against OSC 1, 0 .. +1/2 semitone
 * Pressed (page 2, saved with the sound):
 * blue   WAVE: saw, pulse, both, sync, cross-mod or fifth (word 5)
 * green  FILTER TYPE: LP 12, HP 12, BP 12, NOTCH, LP 24, SEM (word 6)
 * white  DRIVE: drive before the filter (word 7)
 *
 * The OP-1 allocates six voices and provides pitch, glide and the sole amplitude
 * envelope (T2). No internal programs or second amp envelope remain. The original
 * filter envelope and per-card drift remain; normal OP-1 presets hold sound choices.
 * Instrument state (the LFO, the cutoff and resonance slew) advances once per block, from the first
 * voice rendered in it (voices render in ascending order, as EVA does).
 * Synthesis runs at 22.05 kHz with control steps every eight internal samples;
 * pitch/control timing matches the native 44.1 kHz interface. Filter coefficients
 * use the internal sample rate; four-point interpolation reconstructs each output
 * block. Neutral voice constants replace the former program-bank setup. T2 is
 * still the sole amplitude envelope.
 *
 * From OberVave (github.com/zvenson/obervave a2fb3d0, firmware/src/obv_engine.h), the voice code nearly unchanged.
 * Copyright (C) 2026 zvenson (the synth), 2026 Hundekraft (the kennel port). Used in kennel with zvenson's
 * permission (2026-10-15). GPL-3.0-or-later with the OP-1 firmware permission (see LICENSE and NOTICE).
 */
#include "op1_plugin.h"
#include "obv_dsp.h"
#include "obv_params.h"
#include "oxbow_geom.h"
#include "../../seq/seq_text.h"

/* Match the 22.05 kHz output bus. Control ticks keep their original duration;
 * native pitch increments remain expressed at 44.1 kHz. */
#define OBV_NATIVE_HALF_RATE 1
#define OBV_SAMPLES (OP1_BLOCK / 2)
_Static_assert(CTL == 8, "Oxbow ramp and table control rate must agree");
#define ENV_ONE (1 << 24)
#define LEVEL_FLOOR 2048                        /* -78 dB: a release below this ends the voice */
#define TICK (2 * (int)CTL)                     /* internal samples per control tick: 16, matching 32 native samples */
#define NT (OBV_SAMPLES / TICK)                   /* control ticks per block: 4 */
#define NA ((int)(OBV_SAMPLES / CTL))             /* envelope and amp steps per block (every CTL = 8 internal samples): 8 */
#define OUT_GAIN 20000                          /* Q15 output scaling */

enum { E_IDLE, E_ATT, E_DEC, E_SUS, E_REL };
typedef struct { uint32_t stage; int32_t level; } env_t;   /* level Q24 */

typedef struct {
    osc_t o1, o2;
    rcp_t r1, r2;
    uint32_t noise;
    env_t fe;
    svf_state_t f1, f2;
    svf_coef_t c1, c2;
    uint32_t inc1, inc2, pw1, pw2;              /* this tick's oscillator settings */
    int32_t up3,up2,up1;
    int32_t amp0, amp1;                         /* Q15: amp1 the amp at the block's end (the next one starts there) */
    int32_t drift;                              /* the vintage random walk */
    uint32_t lfo_blk;                           /* ticks since the note on (LFO delay / fade) */
    int32_t lfo_fade;                           /* Q15 */
    int32_t c1_cut, c1_k, c2_cut;               /* the inputs c1 / c2 were made from (made once they differ) */
    uint8_t note, vel, gate, fresh;             /* fresh: note and velocity still to be read in render */
    uint8_t c1_ok, c2_ok;
} voice_t;

static voice_t vc[OP1_VOICES];
static int last_voice = OP1_VOICES;

/* One neutral voice; normal OP-1 patches store every exposed sound choice. */
#define pv OBV_DEF
static int osc_mode;


/* vintage: each voice card has its own small offsets, scaled by the neutral voice VINTAGE */
static const int16_t VINT_TUNE8[OP1_VOICES] = {-20, 12, 7, -15, 18, -9};        /* +-8 cents */
static const int16_t VINT_CUT8[OP1_VOICES] = {-300, 250, 120, -380, 60, 330};   /* +-1.5 semitones */
static const int8_t VINT_ENV[OP1_VOICES] = {10, -6, 14, -12, 3, -16};           /* /128: +-12 % on the times */

/* ---- instrument state, once per block */
static uint32_t lfo_ph, lfo_rnd_a, lfo_rnd_b, rnd_state = 0x9E3779B9u;
static uint32_t t_lfo_ph[NT], t_rnd_a[NT], t_rnd_b[NT];   /* the LFO at each control tick of this block */
static int32_t t_cut[NA];                       /* the slewed CUTOFF at each step, Q8 semitones */
static int32_t cut_sm = -1, reso_sm;
static int32_t fenv_amt, det_fine8;
#include "oxbow_plus.h"
obx_params obx = OBX_DEFAULTS;                   /* DAW-POL: parametry OXBOW+ biezacego glosu (wrapper) */
#define keytrk (obx.keytrk)
#define velflt (obx.velflt)
#define env_o2 (obx.envo2)
static const int32_t lfo_pd=0,lfo_fd=0,lfo_pwd=0,lfo_ad=0,uni_det8=0;
static int32_t drive_q12, reso_k;
#define lvl1 (obx.lvl1)
#define lvl2 (obx.lvl2)
#define lvln (obx.lvln)
#define vint (obx.vint)
#define sus_f (obx.sus)
static const int32_t morph_q=0;
static uint32_t w1,w2,ftype,sync_on,xmod_q;
#define pw1_base (obx.pw1)
#define pw2_base (obx.pw2)
static const uint32_t lfo_dl=0;                         /* LFO delay in ticks */
static const int32_t lfo_fstep=32768;                       /* the LFO fade-in per tick after the delay, Q15 */

static volatile uint32_t obv_blocks;            /* for the screen: rendered blocks */
static volatile uint32_t obv_vblk[OP1_VOICES];  /* for the screen: obv_blocks when each voice last sounded */
static volatile int32_t obv_venv[OP1_VOICES];   /* for the screen: each voice's filter envelope, Q15 */

/* Filter selector: compact cyclic steps. Wave uses six stable full-range bands. */
static unsigned ox_select(int32_t q, unsigned count) { return ((unsigned)(q < 0 ? 0 : q) >> 9) % count; }
static inline int32_t knob(const int16_t *k, int i) { return k[i] < 0 ? 0 : k[i]; }

/* Saturate before doubling, including pitches above the output Nyquist limit. */
static inline uint32_t phase_2x(uint32_t x)
{
    return x > 0x3fffffffu ? 0x7fffffffu : x << 1;
}

static int32_t lfo_shape(uint32_t ph, uint32_t shape, uint32_t ra, uint32_t rb)
{
    int32_t p;
    switch (shape) {
    case L_TRI:
        p = (int32_t)((ph + 0x40000000u) >> 15);
        return p < 65536 ? p - 32768 : 98303 - p;
    case L_SAW: return 32767 - (int32_t)(ph >> 16);
    case L_RAMP: return (int32_t)(ph >> 16) - 32768;
    case L_SQR: return ph < 0x80000000u ? 32767 : -32767;
    case L_SH: return (int32_t)(rb >> 16) - 32768;
    default: {                                  /* smooth random: a line from one random value to the next */
        int32_t a = (int32_t)(ra >> 16) - 32768, b = (int32_t)(rb >> 16) - 32768;
        return a + mulq24(b - a, (int32_t)(ph >> 8));
    }
    }
}

static void block_shared(const int16_t *k)
{
    int32_t d, i;
    osc_mode = 0; /* DAW-POL: niezalezne oscylatory (obx) zamiast trybow WAVE */
    /* the knobs */
    int32_t cut_t = (knob(k, 0) * 127) >> 7;    /* Q8 semitones of OberVave's CUTOFF (0..127 << 8) */
    int32_t reso_t = (knob(k, 1) * 127) >> 7;
    fenv_amt = ((knob(k, 2) * 127) >> 15) - 64;
    det_fine8 = (knob(k, 3) * 128) >> 15;
    i = (int)ox_select(knob(k, 6), NFTYPE);
    ftype = (uint32_t)i;
    d = pv[P_DRIVE] + ((knob(k, 7) * 127) >> 15);
    d = d > 127 ? 127 : d;
    drive_q12 = d ? 4096 + (d * d * 7 * 4096 / 16129) : 0;
    /* CUTOFF and RESO slew, tau 64 steps of CTL = 23 ms (a knob step glides) */
    if (cut_sm < 0) { cut_sm = cut_t; reso_sm = reso_t; }
    for (i = 0; i < NA; i++) {
        cut_sm += (cut_t - cut_sm) >> 6;
        reso_sm += (reso_t - reso_sm) >> 6;
        t_cut[i] = cut_sm;
    }
    reso_k = 8192 - reso_sm * 8400 / (127 * 256);
    /* the LFO at each tick: OberVave's steps of CTL, a tick sees the first of its two half a step on */
    for (i = 0; i < NA; i++) {
        uint32_t old = lfo_ph;
        lfo_ph += LFO_INC[pv[P_LFO_RATE]];
        if (lfo_ph < old) { lfo_rnd_a = lfo_rnd_b; lfo_rnd_b = xorshift(&rnd_state); }
        if (i & 1) continue;
        t_lfo_ph[i >> 1] = lfo_ph + (LFO_INC[pv[P_LFO_RATE]] >> 1);
        t_rnd_a[i >> 1] = lfo_rnd_a;
        t_rnd_b[i >> 1] = lfo_rnd_b;
    }
    w1=obx.w1; w2=obx.w2;                       /* DAW-POL: fale, SYNC i X-MOD sciezki */
    sync_on=obx.sync; xmod_q=obx.xmod;
    obv_blocks++;
}

/* ---- envelopes: analog RC curves per tick (CTL samples), level Q24. ka, kd, kr: the stages' coefficients with the
 * voice card's offset (k + (k * vt >> 7), made once per block) */
static inline __attribute__((always_inline)) void env_tick(env_t *e, int32_t ka, int32_t kd, int32_t sus, int32_t kr, int32_t gate)
{
    if (gate && e->stage >= E_REL) e->stage = E_ATT;
    if (!gate && e->stage && e->stage < E_REL) e->stage = E_REL;
    switch (e->stage) {
    case E_ATT:                                 /* aims at 1.3, stops at 1.0 */
        e->level += mulq24(ENV_ONE + (ENV_ONE * 3 >> 3) - e->level, ka);
        if (e->level >= ENV_ONE) { e->level = ENV_ONE; e->stage = E_DEC; }
        break;
    case E_DEC:
        e->level += mulq24(sus - e->level, kd) - 16;
        if (e->level <= sus + 64) { e->level = sus; e->stage = E_SUS; }
        break;
    case E_SUS:
        e->level = sus;
        break;
    case E_REL:
        e->level += mulq24(-e->level, kr) - 32;
        if (e->level < LEVEL_FLOOR) { e->level = 0; e->stage = E_IDLE; }
        break;
    default:
        e->level = 0;
    }
}
static inline int32_t env_k(uint32_t k, int32_t vt) { return (int32_t)k + (((int32_t)k * vt) >> 7); }

/* base + mod * 256 clamped to 2^26 .. 2^32 - 1 - 2^26 (|mod * 256| < 2^31: lfo * lfo_pwd < 2^22) */
static inline uint32_t pw_mod(uint32_t base, int32_t mod)
{
    const uint32_t lo = 1u << 26, hi = 0xFFFFFFFFu - (1u << 26);
    uint32_t m = (uint32_t)(mod < 0 ? -mod : mod) << 8;
    if (mod < 0) return base < lo + m ? lo : (base - m > hi ? hi : base - m);
    return base > hi - m ? hi : (base + m < lo ? lo : base + m);
}

/* the MIDI note nearest a phase increment (key tracking follows the key): once per note */
static uint8_t note_of(uint32_t inc)
{
    /* Same monotone thresholds as the descending scan, at most seven probes. */
    uint32_t lo = 0, hi = 127;
    while (lo < hi) {
        uint32_t n = (lo + hi + 1u) >> 1;
        uint32_t i = INC_C9[n % 12u] >> (10u - n / 12u);
        if (inc >= i - (i >> 5)) lo = n;
        else hi = n - 1u;
    }
    return (uint8_t)lo;
}

/* ---- a voice's numbers that hold through the block (the voice card, the note) */
typedef struct {
    int32_t fka, fkd, fkr;       /* envelope coefficients */
    int32_t p8, p2, cut, velg, p1o;             /* pitch offsets, the cutoff's fixed part, the velocity gain; p1o: DAW-POL OSC 1 */
    uint32_t spread, lfo_on, to1, to2, pw_to1, pw_to2;
} vblk_t;

/* These values depend on oscillator mode, card, note/velocity and DETUNE, not envelope time.
 * Preserve them across blocks and restrikes when their exact inputs are unchanged. */
static vblk_t block_cache[OP1_VOICES];
static struct { int mode, detune; uint8_t note, vel, valid; } block_key[OP1_VOICES];
static const vblk_t *voice_block(const voice_t *v, int card)
{
    vblk_t *c = &block_cache[card];
    if (0 && block_key[card].valid && block_key[card].mode == osc_mode &&   /* DAW-POL: zawsze od nowa */
        block_key[card].detune == det_fine8 && block_key[card].note == v->note && block_key[card].vel == v->vel)
        return c;
    block_key[card].mode = osc_mode; block_key[card].detune = det_fine8;
    block_key[card].note = v->note; block_key[card].vel = v->vel; block_key[card].valid = 1;
    const int32_t vt = (vint * VINT_ENV[card]) >> 7;
    c->fka = env_k(ENV_K_ATT[obx.fa], vt); c->fkd = env_k(ENV_K_DEC[obx.fd], vt); c->fkr = env_k(ENV_K_DEC[obx.fr], vt);
    c->p8 = (VINT_TUNE8[card] * vint) >> 7;
    c->p2 = ((int32_t)pv[P_O2_SEMI] - 24) * 256 + det_fine8 + uni_det8 + obx.semi2 * 256 + obx.cent8_2;   /* DAW-POL: interwal OSC 2 (+ modulacja) */
    c->p1o = obx.semi1 * 256 + obx.cent8_1;     /* DAW-POL: interwal i strojenie OSC 1 */
    c->cut = 8 * 256 + ((int32_t)v->note - 60) * keytrk * 2 + (((int32_t)v->vel - 64) * velflt >> 1)
           + ((VINT_CUT8[card] * vint) >> 7);
    c->velg = 32767 - ((((127 - (int32_t)v->vel) * (int32_t)pv[P_VEL_AMP]) >> 7) * 258);
    c->spread = pv[P_LFO_PHASE] == LP_SPREAD ? (uint32_t)card << 29 : 0;
    c->lfo_on = lfo_pd | lfo_pwd | lfo_fd | lfo_ad;   /* no depth: the LFO's value is never used */
    c->to1 = pv[P_LFO_PITCH_TO] != TO_O2; c->to2 = pv[P_LFO_PITCH_TO] != TO_O1;
    c->pw_to1 = pv[P_LFO_PWM_TO] != TO_O2; c->pw_to2 = pv[P_LFO_PWM_TO] != TO_O1;
    return c;
}

/* ---- the filter coefficients for one step (made only when the cutoff or the resonance moved) */
static inline void voice_coef(voice_t *v, int32_t cut, svf_coef_t *c1, svf_coef_t *c2)
{
    if (!v->c1_ok || cut != v->c1_cut || reso_k != v->c1_k) {
        svf_coef(&v->c1, cut, reso_k);
        v->c1_cut = cut; v->c1_k = reso_k; v->c1_ok = 1;
    }
    *c1 = v->c1;
    if (ftype == F_LP24) {
        if (!v->c2_ok || cut != v->c2_cut) {
            svf_coef(&v->c2, cut, 5793);
            v->c2_cut = cut; v->c2_ok = 1;
        }
        *c2 = v->c2;
    }
}

/* ---- one control tick (TICK samples) for a voice: pitch and pulse widths, and two steps (CTL samples each, as
 * OberVave) of the envelopes, the drift, the cutoff and the amp; amp: the amp after each step; c1, c2: the
 * filter coefficients of each step; noise: the noise samples of the tick (each step's after its drift draw, one
 * generator), when the voice has noise */
static void voice_tick(voice_t *v, const vblk_t *c, int t, uint32_t base, int32_t *amp, svf_coef_t *c1, svf_coef_t *c2,
                       int32_t *noise)
{
    int32_t lfo = 0, p8, p1, p2, cut, fenv, tmp, trem, lfo_to, f1, a1;
    /* the LFO as this voice sees it */
    if (c->lfo_on) lfo = lfo_shape(t_lfo_ph[t] + c->spread, pv[P_LFO_SHAPE], t_rnd_a[t], t_rnd_b[t]);
    for (int s = 0; s < 2; s++)
        if (v->lfo_fade < 32767) {
            if (v->lfo_blk >= lfo_dl) { v->lfo_fade += lfo_fstep; if (v->lfo_fade > 32767) v->lfo_fade = 32767; }
            v->lfo_blk++;
        }
    lfo = (lfo * v->lfo_fade) >> 15;
    /* envelopes, two steps */
    env_tick(&v->fe, c->fka, c->fkd, sus_f, c->fkr, v->gate);
    f1 = v->fe.level;
    env_tick(&v->fe, c->fka, c->fkd, sus_f, c->fkr, v->gate);
    a1=ENV_ONE; /* T2 applies the sole amplitude envelope after engine render. */
    fenv = ((f1 >> 1) + (v->fe.level >> 1)) >> 9;   /* Q15: OSC 2's pitch holds the two steps' mean */
    /* vintage: a slow random walk on the pitch, a step per CTL */
    for (int s = 0; s < 2; s++) {
        v->drift += (int32_t)(xorshift(&v->noise) & 31u) - 16;
        v->drift -= v->drift >> 11;
        if (noise) {
            uint32_t ns = v->noise;
            for (int i = 0; i < (int)CTL; i++) *noise++ = ((int32_t)(xorshift(&ns) >> 16) - 32768) * lvln;
            v->noise = ns;
        }
    }
    /* pitch offsets, Q8 semitones, on top of the OP-1's phase increment */
    p8 = c->p8 + ((v->drift * vint) >> 10);
    lfo_to = (lfo * lfo_pd * 24) >> 15;
    p1 = p8 + c->p1o + (c->to1 ? lfo_to : 0);
    p2 = p8 + (c->to2 ? lfo_to : 0) + c->p2 + ((fenv * env_o2 * 97) >> 15);
    v->inc1 = inc_scale(base, pitch_ratio(p1));
    v->inc2 = inc_scale(base, pitch_ratio(p2));
    tmp = lfo * lfo_pwd;
    v->pw1 = pw_mod(pw1_base, c->pw_to1 ? tmp : 0);
    v->pw2 = pw_mod(pw2_base, c->pw_to2 ? tmp : 0);
    /* cutoff, Q8 semitones, per step */
    cut = c->cut + ((lfo * lfo_fd * 97) >> 15);
    voice_coef(v, cut + t_cut[2 * t] + (((f1 >> 9) * fenv_amt * 390) >> 15), c1, c2);
    voice_coef(v, cut + t_cut[2 * t + 1] + (((v->fe.level >> 9) * fenv_amt * 390) >> 15), c1 + 1, c2 + 1);
    rcp_track(&v->r1, v->inc1);
    rcp_track(&v->r2, v->inc2);
    /* amp, after each step */
    trem = 32767 - (((32767 - lfo) * lfo_ad) >> 8);
    amp[0] = (((a1 >> 9) * c->velg >> 15) * trem) >> 15;
    amp[1] = (((a1 >> 9) * c->velg >> 15) * trem) >> 15;
}

/* ---- the samples: oscillators -> mix -> drive -> filter -> amp, in passes over the whole block (each pass keeps its
 * state in registers; the arithmetic is the one-sample-at-a-time code's). The ticks run first and leave their
 * settings per tick here; the coupled oscillators (sync, X-MOD) still run tick by tick, between the ticks. */
typedef struct { uint32_t inc, pw, sh, r; } otick_t;
static otick_t tk1[NT], tk2[NT];                /* each oscillator's step, pulse width and reciprocal per tick */
static svf_coef_t tc1[NA], tc2[NA];             /* the filter coefficients per step */
static int32_t tamp[NA + 1];                    /* the amp at each step's start (and the block's end) */
static int32_t bmix[OBV_SAMPLES], bnoise[OBV_SAMPLES];

/* one free-running oscillator (no sync, no X-MOD) through the block, out * lvl added to mix (or stored: first) */
static inline __attribute__((always_inline)) void osc_pass(osc_t *o, const otick_t *tk, uint32_t wave, int32_t hw,
                                                           int32_t lvl, int32_t *mix, int first)
{
    uint32_t ph = o->ph;
    int32_t prev = o->prev;
    for (int t = 0; t < NT; t++, tk++, mix += TICK) {
        const uint32_t inc = tk->inc, pw = tk->pw, sh = tk->sh, r = tk->r;
        const int ev = (inc >> 15) != 0;        /* a step below 2^15: no corrections (as osc_step) */
        for (int i = 0; i < TICK; i++) {
            int32_t cur, out = prev;
            ph += inc;
            cur = osc_naive(ph, pw, wave);
            if (ev) {
                if (ph < inc)                   /* wrapped: the saw(s) drop */
                    osc_event(&out, &cur, hw, rcp_at(sh, r, ph));
                if ((wave & OSC_PLS) && ph + pw < inc)   /* the subtracted saw wraps: the pulse rises */
                    osc_event(&out, &cur, 65536, rcp_at(sh, r, ph + pw));
            }
            prev = cur;
            mix[i] = first ? out * lvl : mix[i] + out * lvl;
        }
    }
    o->ph = ph;
    o->prev = prev;
}

static void osc_pass_w(osc_t *o, const otick_t *tk, uint32_t wave, int32_t hw, int32_t lvl, int32_t *mix, int first)
{
    switch (wave) {                             /* one loop per wave */
    case W_SAW: if (first) osc_pass(o, tk, W_SAW, hw, lvl, mix, 1); else osc_pass(o, tk, W_SAW, hw, lvl, mix, 0); break;
    case W_PLS: if (first) osc_pass(o, tk, W_PLS, hw, lvl, mix, 1); else osc_pass(o, tk, W_PLS, hw, lvl, mix, 0); break;
    case W_BOTH: if (first) osc_pass(o, tk, W_BOTH, hw, lvl, mix, 1); else osc_pass(o, tk, W_BOTH, hw, lvl, mix, 0); break;
    default: if (first) osc_pass(o, tk, W_OFF, hw, lvl, mix, 1); else osc_pass(o, tk, W_OFF, hw, lvl, mix, 0); break;
    }
}

/* the oscillators with hard sync or X-MOD through one tick: one sample at a time (OSC 2 follows OSC 1 within the
 * sample) */
static void osc_coupled(voice_t *v, int32_t *mix, int32_t hw1, int32_t hw2)
{
    uint32_t inc1 = v->inc1, inc2 = v->inc2, pw1 = v->pw1, pw2 = v->pw2, d1 = inc1 >> 15, d2 = inc2 >> 15;
    uint32_t xq = xmod_q, sy = sync_on, ww1 = w1, ww2 = w2;
    int32_t o2n = v->o2.prev, l1 = lvl1, l2 = lvl2;
    for (int i = 0; i < TICK; i++) {
        int32_t s1, s2, rt = -1;
        uint32_t ie = inc1;
        if (xq) {                               /* OSC2 -> OSC1 linear FM, the step never below inc/16 */
            int32_t m = (int32_t)(inc1 >> 8 > 0x200000u ? 0x200000u : inc1 >> 8) * (((o2n >> 8) * (int32_t)xq) >> 6);
            m = obv_add_sat(m, (int32_t)inc1);
            ie = m < (int32_t)(inc1 >> 4) ? inc1 >> 4 : (uint32_t)m;
            d1 = ie >> 15;
        }
        s1 = osc_step(&v->o1, ie, d1, pw1, ww1, hw1, -1, &v->r1);
        if (sy && v->o1.ph < ie && d1)
            rt = rcp_frac(&v->r1, ie, v->o1.ph);
        s2 = osc_step(&v->o2, inc2, d2, pw2, ww2, hw2, rt, &v->r2);
        o2n = v->o2.prev;
        mix[i] = s1 * l1 + s2 * l2;
    }
}

/* X-MOD without sync through one tick: OSC 2 runs free (its naive values kept: OSC 1's modulator, one sample
 * late), then OSC 1 with the step it modulates, sample by sample (the reciprocal tracked at each event) */
static inline __attribute__((always_inline)) void xmod_osc1(voice_t *v, int32_t *mix, const int32_t *o2c, int32_t o2n,
                                                            uint32_t wave, int32_t hw)
{
    const uint32_t inc1 = v->inc1, pw1 = v->pw1, xq = xmod_q, lim = inc1 >> 8 > 0x200000u ? 0x200000u : inc1 >> 8;
    const int32_t l1 = lvl1;
    for (int i = 0; i < TICK; i++) {
        int32_t m = (int32_t)lim * (((o2n >> 8) * (int32_t)xq) >> 6), s1;
        uint32_t ie;
        m = obv_add_sat(m, (int32_t)inc1);
        ie = m < (int32_t)(inc1 >> 4) ? inc1 >> 4 : (uint32_t)m;
        s1 = osc_step(&v->o1, ie, ie >> 15, pw1, wave, hw, -1, &v->r1);
        mix[i] += s1 * l1;
        o2n = o2c[i];
    }
}

static void osc_xmod(voice_t *v, int32_t *mix, int32_t hw1, int32_t hw2)
{
    int32_t o2c[TICK];
    const int32_t o2n = v->o2.prev;
    uint32_t ph = v->o2.ph;                     /* OSC 2: osc_pass's loop, one tick, the naive values kept */
    int32_t prev = v->o2.prev;
    const uint32_t inc = v->inc2, pw = v->pw2, sh = v->r2.sh, r = v->r2.r, wave = w2;
    const int32_t l2 = lvl2, ev = (inc >> 15) != 0;
    for (int i = 0; i < TICK; i++) {
        int32_t cur, out = prev;
        ph += inc;
        cur = osc_naive(ph, pw, wave);
        if (ev) {
            if (ph < inc)
                osc_event(&out, &cur, hw2, rcp_at(sh, r, ph));
            if ((wave & OSC_PLS) && ph + pw < inc)
                osc_event(&out, &cur, 65536, rcp_at(sh, r, ph + pw));
        }
        prev = cur;
        o2c[i] = cur;
        mix[i] = out * l2;
    }
    v->o2.ph = ph;
    v->o2.prev = prev;
    switch (w1) {
    case W_SAW: xmod_osc1(v, mix, o2c, o2n, W_SAW, hw1); break;
    case W_PLS: xmod_osc1(v, mix, o2c, o2n, W_PLS, hw1); break;
    case W_BOTH: xmod_osc1(v, mix, o2c, o2n, W_BOTH, hw1); break;
    default: xmod_osc1(v, mix, o2c, o2n, W_OFF, hw1); break;
    }
}

/* mix >> 7 -> drive (or the clamp) in place */
static void drive_pass(int32_t *mix)
{
    const int32_t drv = drive_q12;
    if (drv)
        for (int i = 0; i < OBV_SAMPLES; i++) mix[i] = softclip((((mix[i] >> 7) >> 3) * drv) >> 9);
    else
        for (int i = 0; i < OBV_SAMPLES; i++) mix[i] = clampi(mix[i] >> 7, -65535, 65535);
}

/* the filter's first stage (svf_step) and the type's output, in place, for one filter type */
static inline __attribute__((always_inline)) void filter_pass(svf_state_t *f, int32_t *mix, uint32_t ft)
{
    int32_t ic1 = f->ic1, ic2 = f->ic2;
    const int32_t mq = morph_q;
    const svf_coef_t *c = tc1;
    for (int t = 0; t < NA; t++, c++, mix += CTL) {
        const int32_t a1 = c->a1, a2 = c->a2, a3 = c->a3, k = c->k;
        for (int i = 0; i < (int)CTL; i++) {
            int32_t x = mix[i], v1, v2, v3, n1, y;
            v3 = x - ic2;
            v1 = svf_dot(a1,ic1,a2,v3);
            v2 = ic2 + svf_dot(a2,ic1,a3,v3);
            n1 = 2 * v1 - ic1;
            ic1 = (uint32_t)(n1 + 24576) < 49152u ? n1 : softclip(n1 >> 1) * 2;   /* linear below +-0.75, then soft */
            ic2 = clampi(2*v2-ic2,-SVF_STATE_MAX,SVF_STATE_MAX);
            switch (ft) {                       /* (a constant here: one loop per type) */
            case F_HP12: y = x - svf_damp(k,v1) - v2; break;
            case F_BP12: y = v1; break;
            case F_NOTCH: y = x - svf_damp(k,v1); break;   /* hp + lp */
            case F_SEM: y = v2 + (((x - svf_damp(k,v1) - v2 - v2) * mq) >> 7); break;   /* LP .. notch .. HP pot */
            default: y = v2; break;             /* LP 12, and LP 24's first stage */
            }
            mix[i] = y;
        }
    }
    f->ic1 = ic1; f->ic2 = ic2;
}

/* LP 24's second stage (svf_lp) in place */
static void lp2_pass(svf_state_t *f, int32_t *mix)
{
    int32_t jc1 = f->ic1, jc2 = f->ic2;
    const svf_coef_t *c = tc2;
    for (int t = 0; t < NA; t++, c++, mix += CTL) {
        const int32_t b1 = c->a1, b2 = c->a2, b3 = c->a3;
        for (int i = 0; i < (int)CTL; i++) {
            int32_t v3 = mix[i] - jc2, n1, y;
            n1 = svf_dot(b1,jc1,b2,v3);
            y = jc2 + svf_dot(b2,jc1,b3,v3);
            jc1 = clampi(2*n1-jc1,-SVF_STATE_MAX,SVF_STATE_MAX);
            jc2 = clampi(2*y-jc2,-SVF_STATE_MAX,SVF_STATE_MAX);
            mix[i] = y;
        }
    }
    f->ic1 = jc1; f->ic2 = jc2;
}

/* clamp -> amp (ramped through each tick) -> out */
static void amp_pass(const int32_t *mix, int16_t *out)
{
    for (int t = 0; t < NA; t++, mix += CTL, out += CTL) {
        const int32_t amp0 = tamp[t], da = tamp[t + 1] - tamp[t];
        for (int i = 0; i < (int)CTL; i++) {
            int32_t y = clampi(mix[i], -65000, 65000), amp = amp0 + ((da * i) >> 3);
            y = (y * amp) >> 16;
            out[i] = op1_sat16((y * OUT_GAIN) >> 15);
        }
    }
}

static void voice_block_samples(voice_t *v, int vi, int16_t *out)
{
    const uint32_t ww1 = w1, ww2 = w2, coupled = xmod_q || sync_on;
    const int32_t hw1 = (ww1 & OSC_SAW ? -65536 : 0) + (ww1 & OSC_PLS ? -65536 : 0);
    const int32_t hw2 = (ww2 & OSC_SAW ? -65536 : 0) + (ww2 & OSC_PLS ? -65536 : 0);
    const vblk_t *c = voice_block(v, vi);
    tamp[0] = v->amp1;
    for (int t = 0; t < NT; t++) {
        voice_tick(v, c, t, phase_2x((uint32_t)OP1_PITCH_INC[t * TICK * 2]), tamp + 2 * t + 1, tc1 + 2 * t, tc2 + 2 * t,
                   lvln ? bnoise + t * TICK : 0);
        tk1[t].inc = v->inc1; tk1[t].pw = v->pw1; tk1[t].sh = v->r1.sh; tk1[t].r = v->r1.r;
        tk2[t].inc = v->inc2; tk2[t].pw = v->pw2; tk2[t].sh = v->r2.sh; tk2[t].r = v->r2.r;
        if (sync_on) osc_coupled(v, bmix + t * TICK, hw1, hw2);
        else if (coupled) osc_xmod(v, bmix + t * TICK, hw1, hw2);
    }
    v->amp1 = tamp[NA];
    if (!coupled) {
        osc_pass_w(&v->o1, tk1, ww1, hw1, lvl1, bmix, 1);
        osc_pass_w(&v->o2, tk2, ww2, hw2, lvl2, bmix, 0);
    }
    if (lvln)
        for (int i = 0; i < OBV_SAMPLES; i++) bmix[i] += bnoise[i];
    drive_pass(bmix);
    switch (ftype) {
    case F_HP12: filter_pass(&v->f1, bmix, F_HP12); break;
    case F_BP12: filter_pass(&v->f1, bmix, F_BP12); break;
    case F_NOTCH: filter_pass(&v->f1, bmix, F_NOTCH); break;
    case F_SEM: filter_pass(&v->f1, bmix, F_SEM); break;
    default: filter_pass(&v->f1, bmix, F_LP12); break;
    }
    if (ftype == F_LP24) lp2_pass(&v->f2, bmix);
    amp_pass(bmix, out);
}

static void obv_note_on(void *self, const int16_t *knobs)
{
    const int vi = op1_voice_index(self);
    voice_t *v = &vc[vi];
    int i, alone = 1;
    (void)knobs;
    for (i = 0; i < OP1_VOICES; i++) if (i != vi && vc[i].gate) alone = 0;
    if (!v->noise) v->noise = 0x9E3779B9u ^ ((uint32_t)(vi + 1) * 0x85EBCA6Bu);
    v->gate = 1;
    v->fresh = 1;
    v->fe.stage = E_ATT;
    v->lfo_blk = 0;
    v->lfo_fade = pv[P_LFO_DELAY] ? 0 : 32767;
    if (alone && pv[P_LFO_PHASE] == LP_SYNC) lfo_ph = 0;   /* the first key of a chord restarts the LFO */
}

static void obv_note_off(void *self)
{
    vc[op1_voice_index(self)].gate = 0;
}

static void obv_render(void *self, int16_t *out, const int16_t *knobs)
{
    const int vi = op1_voice_index(self);
    voice_t *v = &vc[vi];
    if (vi <= last_voice) block_shared(knobs);
    last_voice = vi;
    const uint32_t base = (uint32_t)OP1_PITCH_INC[0];
    if (v->fresh) {                             /* the firmware writes pitch and velocity around note_on */
        v->fresh = 0;
        v->note = note_of(base);
        v->vel = (uint8_t)(op1_voice_vel_gain(self) / 258);
    }
    /* Four-point interpolation preserves the shared 128-sample voice interface.
     * History belongs to each voice and persists through ordinary restrikes. */
    int16_t half[OBV_SAMPLES];
    voice_block_samples(v,vi,half);
    int32_t h3=v->up3,h2=v->up2,h1=v->up1;
    for(int i=0;i<OBV_SAMPLES;i++){
        const int32_t y=half[i];
        out[2*i]=(int16_t)h2;
        out[2*i+1]=op1_sat16((-h3+9*h2+9*h1-y)>>4);
        h3=h2;h2=h1;h1=y;
    }
    v->up3=h3;v->up2=h2;v->up1=h1;
    obv_vblk[vi] = obv_blocks;
    obv_venv[vi] = v->fe.level >> 9;
}

const op1_engine_t ENGINE_oxbow = { obv_note_on, obv_render, obv_note_off,
                                       { OP1_PRESS_PAGE, OP1_PRESS_PAGE, OP1_PRESS_PAGE, OP1_PRESS_NONE } };

/* ======================= screen (GUI thread) =======================
 * oxbow.svg (gen_svg.py, traced from the owner's design oxbow-reference.png; numbers in oxbow_geom.h). Every part
 * sits where the reference has it when its knob is at plugin.json's default, except the birds, which reach the
 * reference spot at DETUNE max. The OLED layout fills the width while mapping the full scene to y6..138.
 *   hornl, hornr  CUTOFF: the horns widen (stretched out from their roots) as the cutoff opens, droop below the default
 *   birdl, birdr  DETUNE: the birds walk out along their horn's top edge to the reference spot (and ride the horn)
 *   bell          RESONANCE: the bell swings about its ring, wider with the resonance (none at 0); a little wider
 *                 while notes play
 *   breath        ENV AMOUNT: the breath streaks and clouds grow from the nostril with the envelope amount (either
 *                 side of the middle); while notes play they puff with the filter envelope
 *   s0..s5        one star per voice, lit while that voice sounds (a dim copy shows under each) */
#define ONE 1024
#define DEG 23                                   /* sine table steps per degree (8192 per turn), rounded */
static const int32_t KDEF[4] = { 11869, 5677, 23738, 2560 };   /* plugin.json's defaults: the reference pose */
static void *n_horn[2], *n_bird[2], *n_bell, *n_breath, *n_star[OP1_VOICES];
static uint32_t s_blocks, s_frame, s_swing;
static int32_t s_act, s_puff;

static inline int32_t sin15(int32_t a) { return OP1_SINE_TABLE[a & 8191]; }
static inline int32_t cos15(int32_t a) { return OP1_SINE_TABLE[(a + 2048) & 8191]; }

/* point (x, y) on horn s with stretch sx (Q10) and turn (c, sn: Q10 cos, sin) about the horn's root; Q10 */
static void horn_map(int s, int32_t sx, int32_t c, int32_t sn, int32_t x, int32_t y, int32_t *ox, int32_t *oy)
{
    const int32_t px = s ? OX_HORN_R_X : OX_HORN_L_X, py = s ? OX_HORN_R_Y : OX_HORN_L_Y;
    const int32_t dx = ((x - px) * sx) >> 10, dy = y - py;
    *ox = px + ((c * dx - sn * dy) >> 10);
    *oy = py + ((sn * dx + c * dy) >> 10);
}

/* x' = a x + c y + tx, y' = b x + d y + ty (Q10) */
static void set_m(void *n, int32_t a, int32_t b, int32_t c, int32_t d, int32_t tx, int32_t ty)
{
    if (!n) return;
    const int32_t m[9] = { a, b, 0, c, d, 0, tx, ty, ONE };
    op1_node_set_transform(n, m);
}

/* knob k (Q15) -> lo .. 0 (at the default d) .. hi */
static inline int32_t around(int32_t k, int32_t d, int32_t lo, int32_t hi)
{
    return k < d ? lo - (lo * k) / d : (hi * (k - d)) / (32767 - d);
}

static void ox_screen_init(void *scr)
{
    static const char *const stars[OP1_VOICES] = { "s0", "s1", "s2", "s3", "s4", "s5" };
    n_horn[0] = op1_find_node(scr, "hornl");
    n_horn[1] = op1_find_node(scr, "hornr");
    n_bird[0] = op1_find_node(scr, "birdl");
    n_bird[1] = op1_find_node(scr, "birdr");
    n_bell = op1_find_node(scr, "bell");
    n_breath = op1_find_node(scr, "breath");
    for (int i = 0; i < OP1_VOICES; i++) n_star[i] = op1_find_node(scr, stars[i]);
}

/* Show the active control, including independently pushed secondary pages. */
static void ox_controls(void *r,const int32_t k[4])
{
    static const uint8_t colors[4][4]={{105,142,255,255},{0,237,149,255},{245,245,245,255},{255,58,93,255}};
    static const char *const waves[6]={"SAW","PULSE","BOTH","SYNC","X-MOD","FIFTH"};
    static const char *const filters[NFTYPE]={"LP12","HP12","BP12","NOTCH","LP24","SEM"};
    static const char *const primary[4]={"CUTOFF ","RES ","ENV ","DETUNE "};
    const int16_t *w=op1_synth_knobs(OP1_SCREEN_SLOT&7);
    for(int i=0;i<4;i++){
        char b[24],*o=b;const char *name=primary[i],*value=0;
        int v=(k[i]*100+16383)/32767;
        if(i<3&&op1_eng_page[i]){
            if(i==0){name="WAVE ";value=waves[(knob(w,5)*6)>>15];}
            else if(i==1){name="FILTER ";value=filters[ox_select(knob(w,6),NFTYPE)];}
            else{name="DRIVE ";v=(knob(w,7)*100+16383)/32767;}
        }else if(i==2)v=((k[i]-16384)*100)/16384;
        while(*name)*o++=*name++;
        if(value){while(*value)*o++=*value++;*o=0;}
        else{if(i==2&&!op1_eng_page[i]){*o++=v<0?'-':'+';if(v<0)v=-v;}o=sq_num(o,(unsigned)v,1);*o++='%';*o=0;}
        sq_text(r,(14+80*i)*1024,142*1024,896,b,750,colors[i]);
    }
}

static void ox_screen_update(void *scr, int dt)
{
    int16_t kn[4];
    op1_synth_knobs_live(OP1_SCREEN_SLOT & 7, kn);
    int32_t k[4];
    for (int i = 0; i < 4; i++) k[i] = kn[i] < 0 ? 0 : kn[i];
    const uint32_t blocks = obv_blocks;
    const int on = blocks != s_blocks;
    s_blocks = blocks;
    s_frame++;

    /* stars: a voice that rendered in the last few blocks is sounding */
    int32_t env = 0, nv = 0;
    for (int i = 0; i < OP1_VOICES; i++) {
        const int lit = on && blocks - obv_vblk[i] < 4u;
        if (lit) {
            nv++;
            if (obv_venv[i] > env) env = obv_venv[i];
        }
        if (n_star[i]) op1_node_set_visible(n_star[i], lit);
    }
    s_act += ((nv ? ONE : 0) - s_act) >> 3;

    /* horns (CUTOFF): wider (stretched out from the root, 0.88 .. 1.12) as the cutoff opens; below the default
     * they also droop (tips down, up to 4 degrees). The birds (DETUNE) ride on them: each walks along its horn's top
     * edge from 60 reference px in towards the head (DETUNE 0) out to where the reference has it (DETUNE max);
     * the separate-axis OLED layout retains top/bottom clearance without shrinking the width. */
    const int32_t sx = ONE + around(k[0], KDEF[0], -123, 123);
    const int32_t droop = k[0] < KDEF[0] ? -(4 * DEG * (KDEF[0] - k[0])) / KDEF[0] : 0;
    const int32_t bd = (k[3] * ((OX_BIRD_N - 1) * 1024)) >> 15;
    const int32_t b0 = bd >> 10 < OX_BIRD_N - 1 ? bd >> 10 : OX_BIRD_N - 2, bt = bd - (b0 << 10);
    for (int s = 0; s < 2; s++) {
        const int32_t a = s ? -droop : droop;                     /* tips down: left anticlockwise, right clockwise */
        const int32_t c = (cos15(a) + 16) >> 5, sn = (sin15(a) + 16) >> 5;
        const int32_t px = s ? OX_HORN_R_X : OX_HORN_L_X, py = s ? OX_HORN_R_Y : OX_HORN_L_Y;
        set_m(n_horn[s], (c * sx) >> 10, (sn * sx) >> 10, -sn, c,
              px - ((((c * sx) >> 10) * px - sn * py) >> 10), py - ((((sn * sx) >> 10) * px + c * py) >> 10));
        const int32_t (*path)[2] = s ? OX_BIRD_R : OX_BIRD_L;
        const int32_t fx = s ? OX_FEET_R_X : OX_FEET_L_X, fy = s ? OX_FEET_R_Y : OX_FEET_L_Y;
        int32_t qx = fx + path[b0][0] + (((path[b0 + 1][0] - path[b0][0]) * bt) >> 10);
        int32_t qy = fy + path[b0][1] + (((path[b0 + 1][1] - path[b0][1]) * bt) >> 10);
        horn_map(s, sx, c, sn, qx, qy, &qx, &qy);
        set_m(n_bird[s], c, sn, -sn, c, qx - ((c * fx - sn * fy) >> 10), qy - ((sn * fx + c * fy) >> 10));
    }

    /* bell (RESONANCE): swings about its ring, 0 .. 26 degrees, 1.5x while notes play; ~0.8 Hz at 60 frames/s */
    s_swing += 440;                                          /* 110 sine steps a frame: 0.8 Hz at 60 frames/s */
    const int32_t amp = ((k[1] * 26 * DEG) >> 15) * (ONE + (s_act >> 1)) >> 10;
    const int32_t ba = (sin15((int32_t)(s_swing >> 2)) * amp) >> 15;
    const int32_t bc = (cos15(ba) + 16) >> 5, bs = (sin15(ba) + 16) >> 5;
    set_m(n_bell, bc, bs, -bs, bc, OX_BELL_X - ((bc * OX_BELL_X - bs * OX_BELL_Y) >> 10),
          OX_BELL_Y - ((bs * OX_BELL_X + bc * OX_BELL_Y) >> 10));

    /* breath (ENV AMOUNT, filter envelope): 0.5 .. 1 (the reference, at the default) .. 1.12 of its size; the big
     * cloud's right edge reaches the screen's at 1.16 */
    const int32_t amt = k[2] > 16384 ? k[2] - 16384 : 16384 - k[2], ad = KDEF[2] - 16384;
    int32_t sc = amt < ad ? 512 + (512 * amt) / ad : ONE + (123 * (amt - ad)) / (16383 - ad);
    const int32_t puff = 780 + ((env * 260) >> 15);                 /* 0.76 .. 1.01 with the envelope */
    s_puff += ((on && nv ? puff : ONE) - s_puff) >> 2;
    if (s_frame < 2) s_puff = ONE;
    sc = (sc * s_puff) >> 10;
    if (n_breath) {
        const int32_t m[9] = { sc, 0, 0, 0, sc, 0, OX_BREATH_X - ((sc * OX_BREATH_X) >> 10),
                               OX_BREATH_Y - ((sc * OX_BREATH_Y) >> 10), ONE };
        op1_node_set_transform(n_breath, m);
    }
    (void)dt;
    op1_screen_draw(scr, dt);
    ox_controls(op1_raster(),k);
}

const op1_screen_t SCREEN_oxbow = { ox_screen_init, ox_screen_update };
