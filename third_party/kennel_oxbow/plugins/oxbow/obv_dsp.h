/* OberVave DSP building blocks, all fixed point.
 * Oscillators: naive saw / pulse with polyBLEP corrections applied as discontinuity events, one sample late:
 * a jump of height H at fraction t before the current sample (t = 0..32767 ~ 0..1 sample) adds
 * H/2 * t^2 to the previous sample and takes H/2 * (1-t)^2 from the current one. A natural wrap and a
 * hard-sync reset are the same event, so one piece of code serves both.
 * From OberVave (github.com/zvenson/obervave a2fb3d0, firmware/src/obv_dsp.h). Changed for kennel: the event
 * position t comes from a reciprocal of the phase step (rcp_t, made once per control block) instead of a division
 * at every wrap; pitch_inc is replaced by pitch_ratio (offsets on top of the OP-1's phase increment); 64-bit products
 * and divisions per control block are replaced by 32-bit parts and tables (mulq24, OCT_SEMI, inc_scale's cap, rcp30 and
 * div25 for the reciprocal and the filter's a1).
 * Copyright (C) 2026 zvenson, 2026 Hundekraft (the kennel changes). Used in kennel with zvenson's permission
 * (2026-10-15). GPL-3.0-or-later with the OP-1 firmware permission (see LICENSE and NOTICE).
 */
#include "obv_tables.h"

static inline int32_t clampi(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : v > hi ? hi : v; }

/* tanh(x / 32768 * 2) * 32767, interpolated: x = +-32768 -> +-0.76 full scale, +-65536 -> +-0.96 */
static inline __attribute__((always_inline)) int32_t softclip(int32_t x)
{
    int32_t a = x < 0 ? -x : x, i = a >> 8, y;
    if (i >= 256)
        y = TANH_Q15[256];
    else
        y = TANH_Q15[i] + (((TANH_Q15[i + 1] - TANH_Q15[i]) * (a & 255)) >> 8);
    return x < 0 ? -y : y;
}

static inline uint32_t xorshift(uint32_t *st)
{
    uint32_t s = *st;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    *st = s;
    return s;
}

/* Frequency modulation must saturate at the internal Nyquist limit, never wrap
 * into a negative phase step when a high note and positive modulation coincide. */
static inline int32_t obv_add_sat(int32_t a, int32_t b)
{
#ifdef __bfin__
    return __builtin_bfin_add_fr1x32(a, b);
#else
    int64_t sum = (int64_t)a + b;
    return sum > INT32_MAX ? INT32_MAX : sum < INT32_MIN ? INT32_MIN : (int32_t)sum;
#endif
}

/* 32 x 32 -> the product >> 24 without a 64-bit multiply (|a| < 2^30, 0 <= b < 2^24): envelopes and amps */
static inline __attribute__((always_inline)) int32_t mulq24(int32_t a, int32_t b)
{
    int32_t ah = a >> 12;                       /* a = ah * 2^12 + al, al 0..4095 */
    uint32_t al = (uint32_t)a & 4095u;
    int32_t bh = b >> 12;                       /* b = bh * 2^12 + bl */
    uint32_t bl = (uint32_t)b & 4095u;
    return ah * bh + ((ah * (int32_t)bl + (int32_t)(al * (uint32_t)bh)) >> 12) + (int32_t)((al * bl) >> 24);
}

/* the bit length of x (0 for 0), x < 2^31 */
static inline __attribute__((always_inline)) int32_t bitlen(uint32_t x)
{
#ifdef __bfin__
    return 31 - (int32_t)__builtin_bfin_norm_fr1x32((int32_t)x);
#else
    return x ? 32 - __builtin_clz(x) : 0;
#endif
}

/* floor(2^30 / d) for 2^15 <= d < 2^16 without a division: the tangent at the middle of d's 128-wide interval
 * (RCP_Q, RCP_S), then one correction step (exact for every d: oxbow_check.py) */
#define OBV_RCP30                               /* (oxbow_test.c checks rcp30 and div25) */
static inline __attribute__((always_inline)) uint32_t rcp30(uint32_t d)
{
    const uint32_t j = (d >> 7) - 256u;
    uint32_t q = (uint32_t)((int32_t)RCP_Q[j] - (((int32_t)RCP_S[j] * ((int32_t)(d & 127u) - 64)) >> 12));
    if (q * d > (1u << 30)) q--;
    else if ((q + 1u) * d <= (1u << 30)) q++;
    return q;
}

/* floor(2^25 / d) for 64 <= d < 2^18 (the filter's a1), exact; a division only below 2^12 (never for a played
 * filter: d >= 4093 there) */
static inline uint32_t div25(uint32_t d)
{
    int32_t s;
    uint32_t q;
    if (d < 4096u) return (1u << 25) / d;
    s = bitlen(d) - 16;
    if (s <= 0) return rcp30(d << -s) >> (5 + s);   /* d << -s exact: floor(floor(2^30 / dn) / 2^(5 - |s|)) */
    q = rcp30(d >> s) >> (5 + s);
    if (q * d > (1u << 25)) q--;
    else if ((q + 1u) * d <= (1u << 25)) q++;
    return q;
}

/* (n + 48) -> octave << 4 | semitone, for n = -48..+48: no division per tick */
static const uint8_t OCT_SEMI[97] = {
#define R(o) (o) << 4, (o) << 4 | 1, (o) << 4 | 2, (o) << 4 | 3, (o) << 4 | 4, (o) << 4 | 5, \
             (o) << 4 | 6, (o) << 4 | 7, (o) << 4 | 8, (o) << 4 | 9, (o) << 4 | 10, (o) << 4 | 11
    R(0), R(1), R(2), R(3), R(4), R(5), R(6), R(7), 8 << 4
#undef R
};

/* Q8 semitones (-48..+48, clamped) -> frequency ratio, Q16. SEMI * FINE / 2 < 2^32 */
static inline uint32_t pitch_ratio(int32_t p8)
{
    uint32_t os, r;
    int32_t o;
    p8 = clampi(p8, -48 * 256, 48 * 256);
    os = OCT_SEMI[(p8 >> 8) + 48];
    o = (int32_t)(os >> 4);
    r = (SEMI_Q16[os & 15u] * (FINE_Q16[p8 & 255] >> 1)) >> 15;   /* < 2^17 */
    return o >= 4 ? r << (o - 4) : r >> (4 - o);
}

/* phase step times a Q16 ratio (ratio < 2^21), capped at 2^31 - 1, in 32-bit parts. The cap needs no division:
 * inc * rh <= 2^31 - 1 (the table) leaves inc * rh + lo < 2^32, so its top bit tells */
#define L4(n) 0x7FFFFFFFu / (n), 0x7FFFFFFFu / (n + 1), 0x7FFFFFFFu / (n + 2), 0x7FFFFFFFu / (n + 3)
static const uint32_t INC_LIM[32] = { 0xFFFFFFFFu, 0x7FFFFFFFu, 0x7FFFFFFFu / 2, 0x7FFFFFFFu / 3, L4(4), L4(8), L4(12),
                                      L4(16), L4(20), L4(24), L4(28) };
#undef L4
static inline uint32_t inc_scale(uint32_t inc, uint32_t ratio)
{
    uint32_t rh = ratio >> 16, rl = ratio & 0xFFFFu, r;
    uint32_t lo = (inc >> 16) * rl + (((inc & 0xFFFFu) * rl) >> 16);   /* (inc * rl) >> 16, < 2^31 */
    if (inc > INC_LIM[rh]) return 0x7FFFFFFFu;    /* (only past 2^31: never for a played note) */
    r = inc * rh + lo;
    return (int32_t)r < 0 ? 0x7FFFFFFFu : r;
}

/* ---- the event position: t = x / inc in 0..32767 for 0 <= x < inc, by a reciprocal made per control block */
typedef struct { uint32_t inc, sh, r; } rcp_t;
static inline void rcp_make(rcp_t *c, uint32_t inc)
{
    int32_t sh = (int32_t)inc < 0 ? 16 : bitlen(inc) - 16;   /* the least shift that leaves inc below 2^16 */
    uint32_t d;
    if (sh < 0) sh = 0;
    c->inc = inc;
    c->sh = sh;
    d = inc >> sh;                              /* D in [2^15, 2^16): r in (2^14, 2^15] */
    c->r = d >= 32768u ? rcp30(d) : d ? (1u << 30) / d : 0;   /* (a division only for a step below 2^15) */
}
/* keep the reciprocal while the step moves less than 1/256 (t then errs < 0.4 %); the oscillators' steps drift a
 * little every tick (vintage), X-MOD moves them every sample */
static inline __attribute__((always_inline)) void rcp_track(rcp_t *c, uint32_t inc)
{
    uint32_t d = inc > c->inc ? inc - c->inc : c->inc - inc;
    if (d > (c->inc >> 8)) rcp_make(c, inc);
}
static inline __attribute__((always_inline)) int32_t rcp_frac(rcp_t *c, uint32_t inc, uint32_t x)
{
    uint32_t t;
    rcp_track(c, inc);
    t = ((x >> c->sh) * c->r) >> 15;
    return t > 32767u ? 32767 : (int32_t)t;
}
/* the same with the reciprocal already tracked to this step (a step that stays the same through the tick) */
static inline __attribute__((always_inline)) int32_t rcp_at(uint32_t sh, uint32_t r, uint32_t x)
{
    uint32_t t = ((x >> sh) * r) >> 15;
    return t > 32767u ? 32767 : (int32_t)t;
}

/* ---- oscillators */
typedef struct {
    uint32_t ph;                                /* phase, wraps */
    int32_t prev;                               /* the previous sample, still open to a correction */
} osc_t;

#define OSC_SAW 1u
#define OSC_PLS 2u

static inline __attribute__((always_inline)) int32_t osc_naive(uint32_t ph, uint32_t pw, uint32_t wave)   /* -98304 .. 98303 */
{
    int32_t v = 0;
    if (wave & OSC_SAW)
        v = (int32_t)(ph >> 16) - 32768;
    if (wave & OSC_PLS)                         /* saw(ph) - saw(ph + pw): a pulse with mean 0 */
        v += (int32_t)(ph >> 16) - (int32_t)((ph + pw) >> 16);
    return v;
}

/* the event: a jump of H (any size up to +-200000) at t (0..32767) before the current sample */
static inline __attribute__((always_inline)) void osc_event(int32_t *prev, int32_t *cur, int32_t h, int32_t t)
{
    int32_t u = 32767 - t, h16 = h >> 4;
    *prev += (h16 * ((t * t) >> 15)) >> 12;   /* H/2 * t^2: h16 = H/16, t^2 in Q15, so >> 12 */
    *cur -= (h16 * ((u * u) >> 15)) >> 12;
}

/* one sample: advance by inc (d = inc >> 15, hw = the height of a wrap of ph: -65536 per saw component,
 * both precomputed per block), return the delayed output; o->prev then holds this sample's naive value (the
 * X-MOD source). reset_t >= 0: a hard-sync reset to phase 0 happened reset_t before the current sample
 * (0..32767 = 0..1 sample). rc: the reciprocal of inc for the event positions. */
static inline __attribute__((always_inline)) int32_t osc_step(osc_t *o, uint32_t inc, uint32_t d, uint32_t pw,
                                                              uint32_t wave, int32_t hw, int32_t reset_t, rcp_t *rc)
{
    uint32_t old = o->ph, ph = old + inc;
    int32_t cur, out = o->prev;
    if (reset_t < 0 || !d) {
        cur = osc_naive(ph, pw, wave);
        if (ph < inc && d)                      /* wrapped: the saw(s) drop */
            osc_event(&out, &cur, hw, rcp_frac(rc, inc, ph));
        if ((wave & OSC_PLS) && ph + pw < inc && d)   /* the subtracted saw wraps: the pulse rises */
            osc_event(&out, &cur, 65536, rcp_frac(rc, inc, ph + pw));
    } else {
        uint32_t at = ph - d * (uint32_t)reset_t;   /* the phase at the reset instant */
        int32_t t;
        ph = d * (uint32_t)reset_t;             /* reset, then reset_t on */
        cur = osc_naive(ph, pw, wave);
        if (at < old) {                         /* ph wrapped on the way to the reset */
            t = rcp_frac(rc, inc, at) + reset_t;
            osc_event(&out, &cur, hw, t > 32767 ? 32767 : t);
        }
        if ((wave & OSC_PLS) && at + pw < old + pw) {   /* ph + pw wrapped on the way to the reset */
            t = rcp_frac(rc, inc, at + pw) + reset_t;
            osc_event(&out, &cur, 65536, t > 32767 ? 32767 : t);
        }
        osc_event(&out, &cur, osc_naive(0, pw, wave) - osc_naive(at, pw, wave), reset_t);
    }
    o->ph = ph;
    o->prev = cur;
    return out;
}

/* Preserve the full Q13 dot product during large resonant excursions. Normal
 * signals take the original fast path; split products keep the wider path in
 * 32-bit arithmetic without wrapping or calling a 64-bit helper. */
static inline __attribute__((always_inline)) int32_t svf_dot(int32_t a,int32_t x,int32_t b,int32_t y)
{
    if ((uint32_t)x+65536u<131072u && (uint32_t)y+65536u<131072u)
        return (a*x+b*y)>>13;
    return a*(x>>13)+b*(y>>13)+
        ((a*(int32_t)((uint32_t)x&8191u)+b*(int32_t)((uint32_t)y&8191u))>>13);
}
static inline __attribute__((always_inline)) int32_t svf_damp(int32_t k,int32_t x)
{
    if ((uint32_t)x+131072u<262144u) return (k*x)>>12;
    return k*(x>>12)+((k*(int32_t)((uint32_t)x&4095u))>>12);
}
#define SVF_STATE_MAX 131071

/* ---- trapezoidal state-variable filter (A. Simper), coefficients per block in Q13 */
typedef struct { int32_t a1, a2, a3, k; } svf_coef_t;
typedef struct { int32_t ic1, ic2; } svf_state_t;

/* cut: Q8 semitones (0..136 << 8); k: damping Q12 (2.0 = 8192; ~0 self-oscillates) */
static inline void svf_coef(svf_coef_t *c, int32_t cut, int32_t k)
{
    int32_t i, g, den;
    cut = clampi(cut, 0, 136 << 8);
    i = cut >> 8;
    g = SVF_G[i];
    if (i < 136)
        g += ((SVF_G[i + 1] - g) * (cut & 255)) >> 8;
    den = 4096 + ((g * (g + k)) >> 12);
    if (den < 64) den = 64;
    c->a1 = (int32_t)div25((uint32_t)den);
    c->a2 = (c->a1 * g) >> 12;
    c->a3 = (c->a2 * g) >> 12;
    c->k = k;
}

/* one sample; *lp, *bp, *hp out. The band-pass state passes a soft saturation: resonance stays warm and bounded
 * (self-oscillation settles instead of running away). Signals within +-150000 keep the products in 32 bits. */
static inline __attribute__((always_inline)) void svf_step(const svf_coef_t *c, svf_state_t *s, int32_t in, int32_t *lp, int32_t *bp, int32_t *hp)
{
    int32_t v3 = in - s->ic2;
    int32_t v1 = svf_dot(c->a1,s->ic1,c->a2,v3);
    int32_t v2 = s->ic2 + svf_dot(c->a2,s->ic1,c->a3,v3);
    int32_t n1 = 2 * v1 - s->ic1;
    s->ic1 = (uint32_t)(n1 + 24576) < 49152u ? n1 : softclip(n1 >> 1) * 2;   /* linear below +-0.75, then soft */
    s->ic2 = clampi(2*v2-s->ic2,-SVF_STATE_MAX,SVF_STATE_MAX);
    *lp = v2;
    *bp = v1;
    *hp = in - svf_damp(c->k,v1) - v2;
}

static inline __attribute__((always_inline)) int32_t svf_lp(const svf_coef_t *c, svf_state_t *s, int32_t in)   /* the plain stage (24 dB's second) */
{
    int32_t v3 = in - s->ic2;
    int32_t v1 = svf_dot(c->a1,s->ic1,c->a2,v3);
    int32_t v2 = s->ic2 + svf_dot(c->a2,s->ic1,c->a3,v3);
    s->ic1 = clampi(2*v1-s->ic1,-SVF_STATE_MAX,SVF_STATE_MAX);
    s->ic2 = clampi(2*v2-s->ic2,-SVF_STATE_MAX,SVF_STATE_MAX);
    return v2;
}
