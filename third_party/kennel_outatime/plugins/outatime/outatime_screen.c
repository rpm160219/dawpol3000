/* Copyright (C) 2026 Hundekraft. GPL-3.0-or-later with the OP-1 firmware permission (see LICENSE and NOTICE).
 * Ported from opus's ota_page.c (LizardMan245/opus f0d2216, examples/opus.outatime, GPL-3.0; PROVENANCE.md): its
 * instruments, animation and drawing code, moved onto kennel's screen services. */
/* OUTATIME's screen (GUI thread): the owner's time-machine page (opus outatime's T1, v3). outatime.svg (gen_svg.py)
 * holds the still art and is drawn through the static-art cache, whole; everything below is drawn on it each frame,
 * as upstream's page draws it, in upstream's units (x 256 on the 320 x 160 page) mapped through gen_svg.py's fit:
 *
 *   year  (blue)    the time circuits: DEST the set year, PRES the first arm's year as the time drift moves it
 *                   (ota_arm_year, the newest note's voice), LAST the destination before the last change
 *   speed (green)   the speedometer: its arc and digits the set speed, the needle the car's speed; the road, the
 *                   speed streaks, the wheels' spokes
 *   power (white)   the flux capacitor's rods (the three arms, glowing with power, breathing with the drift while a
 *                   note plays), the readout in gigawatts, the current along the three time cables
 *   trail (red)     the fire behind the car once the needle reaches the set speed: as high as trail while a key is
 *                   down, burning down after the last one over the ignition's time
 *   page 2          a paged knob's tab shows its page-2 name and value (SPREAD, TONE, FLUX), as upstream's shift did
 *
 * What changed against upstream (README.md):
 *   - no kept-pixel zones (opus_page_keep/put): the still art is the static-prefix cache, every live part is drawn
 *     each frame with op1_raster_line (flat ends: upstream's dots are two crossing lines)
 *   - the time circuits show the year only: upstream filled each row with the OP-1's month, day and time
 *     (opus_clock), which kennel has no service for, so the date and time fields stay dark
 *   - letters (the tabs' names) in kennel's stroke font (upstream drew the OP-1's own font, firmware data), sized to
 *     the tab's cap height or narrower to fit the tab
 *   - fixed encoder colours (blue #698EFF, green #00ED95, white, red #FF3A5D) for upstream's theme accents
 *   - page 2 is per knob (kennel's encoder press) instead of shift; the trail tab stays lit (red has no page 2)
 *   - no "always driving" setting: the car drives while a note plays
 *   - the clock is the frame's dt (ms) instead of the cycle counter */
#include "op1_plugin.h"
#include "ota_dsp.h"
#include "outatime_geom.h"
#include "screens/stroke_font.h"
#pragma GCC push_options
#pragma GCC optimize("Os")                             /* GUI code: size, not speed */

typedef uint32_t u32;

extern volatile uint32_t outatime_blocks;
extern volatile int32_t outatime_held, outatime_newest;
const struct ota_voice *outatime_voice(int i);

#define U(v)            ((int)((v) * 256 + 0.5))   /* units x 256 (constants only: folded by the compiler) */
#define FLAMES          40
#define FLAME_GAP       U(6.5)
#define FLAME_LIFE      2250
#define THIRD           0x55555555u

#define DKBLUE          0x383572u
#define OFFBLUE         0x1D2040u
#define BLUE            0x698EFFu
#define LIGHTBLUE       0xBFD8FFu
#define LAVENDER        0xADADFFu
#define GREY            0x7C7990u
#define WHITE           0xFFFFFFu
static const unsigned ACCENT[4] = { 0x698EFFu, 0x00ED95u, 0xFFFFFFu, 0xFF3A5Du };   /* kennel's encoder colours */

/* the animation (upstream's) */
static int act;
static int road, wheel, streak, emit;
static int cable_d[CABLES];
static struct flame { int x, lv, L; unsigned id, born; } flame[FLAMES];
static int flames;
static unsigned flame_id, now_ms;
static int dest_year = -1, last_year = -1, moving, moved_ms;
static const void *lit_voice;
static int lit_env, fire_lv;
static int cable_at[CABLE_N], cable_len[CABLES];
static int at_speed;
static int rod_u[3][2];
static u32 rs;
static uint32_t seen_blocks;
static int quiet = 99;

/* ---------------------------------------------------------------- helpers */

static unsigned isqrt(unsigned v)
{
    unsigned r = 0, b = 1u << 30;
    while (b > v)
        b >>= 2;
    while (b) {
        if (v >= r + b) {
            v -= r + b;
            r = (r >> 1) + b;
        } else
            r >>= 1;
        b >>= 2;
    }
    return r;
}

static int clampk(int v) { return v < 0 ? 0 : v > 32767 ? 32767 : v; }
static int rnd(void)
{
    rs ^= rs << 13;
    rs ^= rs >> 17;
    rs ^= rs << 5;
    return (int)(rs >> 17);
}
static void seed(u32 s) { rs = s ? s : 0x9E3779B9u; }

static unsigned shade(unsigned rgb, int f256)
{
    return ((((rgb >> 16) & 0xFF) * f256 >> 8) << 16) | ((((rgb >> 8) & 0xFF) * f256 >> 8) << 8) | ((rgb & 0xFF) * f256 >> 8);
}

static unsigned mix(unsigned a, unsigned b, int t256)
{
    unsigned out = 0;
    for (int sh = 0; sh <= 16; sh += 8) {
        const int ca = (int)((a >> sh) & 0xFF), cb = (int)((b >> sh) & 0xFF);
        out |= (unsigned)(ca + ((cb - ca) * t256 >> 8)) << sh;
    }
    return out;
}

static unsigned ramp(const unsigned *col, const int *at, int n, int t)
{
    for (int i = 1; i < n; ++i)
        if (t <= at[i])
            return mix(col[i - 1], col[i], (t - at[i - 1]) * 256 / (at[i] - at[i - 1]));
    return col[n - 1];
}
static unsigned rod_col(int c)
{
    static const unsigned col[4] = { DKBLUE, BLUE, LIGHTBLUE, WHITE };
    static const int at[4] = { 0, 13107, 26214, 32767 };
    return ramp(col, at, 4, c);
}
static unsigned rod_halo(int c)
{
    static const unsigned col[3] = { OFFBLUE, DKBLUE, BLUE };
    static const int at[3] = { 0, 16384, 32767 };
    return ramp(col, at, 3, c);
}

/* the fit (gen_svg.py): units x 256 -> screen Q10 */
static int32_t fx(int x) { return (OTA_FIT_CX + ((x - OTA_FIT_CX) * OTA_FIT_S12 >> 12)) * 4; }
static int32_t fy(int y) { return (OTA_FIT_CY + ((y - OTA_FIT_CY) * OTA_FIT_S12 >> 12)) * 4; }
static int32_t fl(int d) { return (d * OTA_FIT_S12 >> 12) * 4; }   /* a length */

static void *ras;

static void rgba(uint8_t *c, unsigned rgb)
{
    c[0] = (uint8_t)(rgb >> 16);
    c[1] = (uint8_t)(rgb >> 8);
    c[2] = (uint8_t)rgb;
    c[3] = 0xFF;
}

/* a line in upstream's units; its width is kept (gen_svg.py keeps stroke widths) */
static void ln(int x0, int y0, int x1, int y1, int w, unsigned rgb)
{
    if (x0 == x1 && y0 == y1)
        return;
    uint8_t c[4];
    rgba(c, rgb);
    const op1_pt a = { fx(x0), fy(y0) }, b = { fx(x1), fy(y1) };
    op1_raster_line(ras, &a, &b, w * 4, c);
}
/* upstream's dot (a zero-length round-capped line, radius r): R246 draws no zero-length line and has flat ends, so
 * two crossing lines, 2r long and 1.4 r wide */
static void dot(int x, int y, int r, unsigned rgb)
{
    uint8_t c[4];
    rgba(c, rgb);
    const int32_t X = fx(x), Y = fy(y), R = fl(r), w = r * 4 * 14 / 10;
    const op1_pt a = { X - R, Y }, b = { X + R, Y }, p = { X, Y - R }, q = { X, Y + R };
    op1_raster_line(ras, &a, &b, w, c);
    op1_raster_line(ras, &p, &q, w, c);
}

static void sincos(int turn, int *s, int *c)   /* 1/4096 of a turn, any; x 4096 */
{
    *s = OP1_SINE_TABLE[(turn * 2) & 8191] >> 3;
    *c = OP1_SINE_TABLE[(turn * 2 + 2048) & 8191] >> 3;
}

static int pow07(int x)
{
    static const unsigned short T[17] = { 0, 4706, 7645, 10152, 12416, 14516, 16492, 18370, 20172, 21906, 23583,
                                          25208, 26791, 28334, 29845, 31320, 32768 };
    x = clampk(x);
    const int i = x >> 11, f = x & 2047;
    return T[i] + ((T[i + 1] - T[i]) * f >> 11);
}

static void two(char *p, int v) { p[0] = (char)('0' + v / 10 % 10); p[1] = (char)('0' + v % 10); }

/* ---------------------------------------------------------------- letters and segments */

static int text_units(const char *s)                  /* stroke_font.h's advances, the last gap left out */
{
    int n = 0;
    for (; *s; ++s)
        n += *s == '1' || *s == '.' ? 4 : 6;
    return n > 2 ? n - 2 : n;
}

/* a tab's name in kennel's stroke font: the tab's cap height, narrower when the name would leave the tab */
static void tab_name(int i, const char *s, unsigned rgb)
{
    const int32_t avail = fl(TAB[i][2] - 2 * TAB_PAD), cap = fl(TAB_CAP);
    int32_t u = cap / 6;
    const int n = text_units(s);
    if (u * n > avail)
        u = avail / n;
    uint8_t c[4];
    rgba(c, rgb);
    sf_text(ras, fx(TAB[i][0] + TAB_PAD), fy(TAB[i][1]) - 3 * u, u, s, OP1_Q10(1.1), c);
}

struct segs { int w, h, sw, slant, glow_w; unsigned lit, glow; };

static void seg_char(int ch, int x, int y, const struct segs *o)   /* a seven-segment digit (op1draw.js's segChar) */
{
    unsigned m = ch >= '0' && ch <= '9' ? MAP7[ch - '0'] : 0;
    for (int pass = o->glow_w ? 0 : 1; pass < 2; ++pass)
        for (int k = 0; k < 7; ++k) {
            if (!(m >> k & 1))
                continue;
            const int x1 = x + SEG7[k][0] * o->w / 2 + (((2 - SEG7[k][1]) * o->h / 2) * o->slant >> 12), y1 = y + SEG7[k][1] * o->h / 2;
            const int x2 = x + SEG7[k][2] * o->w / 2 + (((2 - SEG7[k][3]) * o->h / 2) * o->slant >> 12), y2 = y + SEG7[k][3] * o->h / 2;
            const int dx = x2 - x1, dy = y2 - y1, L = (int)isqrt((unsigned)(dx * dx + dy * dy));
            int g = o->sw * 95 / 100;
            if (g > L * 3 / 10)
                g = L * 3 / 10;
            /* upstream's ends are round: R246's are flat, so each line runs on by half its width to the same ink */
            const int gp = g - (pass ? o->sw : o->glow_w) / 2;
            const int ox = L ? dx * gp / L : 0, oy = L ? dy * gp / L : 0;
            if (pass)
                ln(x1 + ox, y1 + oy, x2 - ox, y2 - oy, o->sw, o->lit);
            else
                ln(x1 + ox, y1 + oy, x2 - ox, y2 - oy, o->glow_w, o->glow);
        }
}

/* ---------------------------------------------------------------- the instruments */

/* a time circuits row: the year (its four digits at upstream's places) */
static void row(int r, int year, unsigned lit, unsigned glow)
{
    const int y = r == 0 ? TC_ROW0 : r == 1 ? TC_ROW1 : TC_ROW2;
    const struct segs o7 = { TC_W7, TC_H, TC_SW, 205, glow ? TC_SW * 23 / 10 : 0, lit, glow };
    char s[4];
    s[0] = (char)('0' + year / 1000 % 10);
    s[1] = (char)('0' + year / 100 % 10);
    two(s + 2, year % 100);
    for (int i = 0; i < 4; ++i) {
        const int at = TC_YEAR + i * TC_PITCH;
        seg_char(s[i], s[i] == '1' ? at - TC_W7 * 45 / 100 : at, y, &o7);
    }
}

static void arc(int a0, int a1, int r, int w, unsigned rgb)
{
    int n = ((a1 > a0 ? a1 - a0 : a0 - a1) + 56) / 57, lx = 0, ly = 0;
    n = n < 1 ? 1 : n;
    for (int i = 0; i <= n; ++i) {
        int s, c;
        sincos(a0 + (a1 - a0) * i / n, &s, &c);
        const int x = GA_CX + (c * r >> 12), y = GA_CY - (s * r >> 12);
        if (i)
            ln(lx, ly, x, y, w, rgb);
        lx = x;
        ly = y;
    }
}

static int meter_angle(int v) { return GA_A0 + ((GA_A1 - GA_A0) * clampk(v) >> 15); }

static void meter(int mph, unsigned lit, unsigned glow)
{
    const int drv = mph * 32767 / 88;
    if (drv > 164) {
        arc(GA_A0, meter_angle(drv), GA_R, U(3.4), glow);
        arc(GA_A0, meter_angle(drv), GA_R, U(1.5), lit);
    }
    const struct segs o = { MPH_W, MPH_H, U(1.5), 328, U(1.5) * 24 / 10, lit, glow };
    seg_char('0' + mph / 10 % 10, MPH_X, MPH_Y, &o);
    seg_char('0' + mph % 10, MPH_X + MPH_PITCH, MPH_Y, &o);
}

static void readout(int gw100, unsigned lit, unsigned glow)
{
    const struct segs o = { READ_W, READ_H, U(1.4), 328, U(1.4) * 23 / 10, lit, glow };
    const int r = U(1.4) * 62 / 100;
    int cx = READ_X;
    seg_char('0' + gw100 / 100 % 10, cx, READ_Y, &o);
    cx += READ_PITCH;
    dot(cx - READ_PITCH * 18 / 100, READ_Y + READ_H, 2 * r, glow);
    dot(cx - READ_PITCH * 18 / 100, READ_Y + READ_H, r, lit);
    cx += READ_PITCH * 36 / 100;
    seg_char('0' + gw100 / 10 % 10, cx, READ_Y, &o);
    seg_char('0' + gw100 % 10, cx + READ_PITCH, READ_Y, &o);
}

/* ---------------------------------------------------------------- what moves */

static void cubic(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, int w, unsigned rgb)
{
    int lx = x0, ly = y0;
    for (int i = 1; i <= 4; ++i) {
        const int u = 4 - i, a = u * u * u, b = 3 * u * u * i, c = 3 * u * i * i, d = i * i * i;
        const int x = (a * x0 + b * x1 + c * x2 + d * x3) / 64, y = (a * y0 + b * y1 + c * y2 + d * y3) / 64;
        ln(lx, ly, x, y, w, rgb);
        lx = x;
        ly = y;
    }
}

static void tongue(int x, int gy, int h, int w, int lean, int lw, unsigned rgb)
{
    const int xl = x - w / 2, xr = x + w / 2, tx = x + lean, ty = gy - h;
    cubic(xl, gy, xl, gy - h * 55 / 100, tx - w * 18 / 100, gy - h * 82 / 100, tx, ty, lw, rgb);
    cubic(tx, ty, tx + w * 12 / 100, gy - h * 72 / 100, xr, gy - h / 2, xr, gy, lw, rgb);
}

static int cable_point(int c, int d, int *x, int *y)
{
    int i = CABLE_AT[c] + 1;
    while (i < CABLE_AT[c + 1] - 1 && cable_at[i] < d)
        ++i;
    const int a = cable_at[i - 1], b = cable_at[i], u = b > a ? (d - a) * 4096 / (b - a) : 0;
    *x = CABLE[i - 1][0] + ((CABLE[i][0] - CABLE[i - 1][0]) * u >> 12);
    *y = CABLE[i - 1][1] + ((CABLE[i][1] - CABLE[i - 1][1]) * u >> 12);
    if (!CABLE[i][2])
        return -1;
    while (i < CABLE_AT[c + 1] - 1 && CABLE[i + 1][2])
        ++i;
    return cable_at[i];
}

static int step(int held, int speed01, int lv, int dt)
{
    const int target = held ? 32767 : 0, tau = held ? 250 : 700;
    const int x = dt * 32768 / tau, f = x >= 32768 ? 32767 : x - (x * x >> 16) + ((x * x >> 15) * x >> 15) / 6;
    act += (target - act) * (f > 32767 ? 32767 : f) >> 15;
    if (!held && act < 66)
        act = 0;
    const int vel = U(26) + ((U(270) >> 4) * speed01 >> 11);
    const int dx = act ? vel * dt / 1000 : 0;
    road = (road + dx) % U(26);
    wheel = (wheel - (dx * 869 >> 12)) & 4095;
    streak = (streak + dx) % U(1700);
    if (act) {
        const int adv = (cable_len[0] * (410 + (1229 * speed01 >> 15)) >> 10) * dt / 1000;
        for (int c = 0; c < CABLES; ++c)
            cable_d[c] = (cable_d[c] + adv) % cable_len[c];
    }
    int n = 0;
    for (int i = 0; i < flames; ++i) {
        struct flame f1 = flame[i];
        f1.x += dx;
        const int age = (f1.x - EMIT_X) * 128 / (f1.L >> 8) + (int)((now_ms - f1.born) * 32768 / FLAME_LIFE);
        if (age < 32768 && f1.x < U(322))
            flame[n++] = f1;
    }
    flames = n;
    emit += dx;
    while (emit >= FLAME_GAP) {
        emit -= FLAME_GAP;
        if (lv > 655 && flames < FLAMES) {
            struct flame *const f1 = &flame[flames++];
            f1->x = EMIT_X + emit;
            f1->lv = lv;
            f1->L = U(30) + ((U(225) >> 4) * lv >> 11);
            f1->id = flame_id++;
            f1->born = now_ms;
        }
    }
    return dx;
}

static void flicker(const struct flame *f, int *size, int *lean)
{
    const unsigned t = now_ms * 18, n = t / 1000, u = (t % 1000) * 32 / 1000;
    int a[2], b[2];
    for (int k = 0; k < 2; ++k) {
        seed(f->id * 7919u + n + (unsigned)k);
        a[k] = rnd();
        b[k] = rnd();
    }
    const int r1 = a[0] + ((a[1] - a[0]) * (int)u >> 5), r2 = b[0] + ((b[1] - b[0]) * (int)u >> 5);
    *size = 23593 + (9175 * r1 >> 15);
    *lean = ((r2 - 16384) * U(2.2) >> 15) + U(1.2);
}

static void fire(unsigned a3)
{
    static const int at[4] = { 0, 9830, 21299, 32767 };
    const unsigned col[4] = { a3, shade(a3, 200), shade(a3, 77), 0 };
    static const int hot_at[3] = { 0, 6554, 13107 };
    const unsigned halo = shade(a3, 77), hot[3] = { mix(a3, WHITE, 154), a3, 0 };
    for (int i = 0; i < flames; ++i) {
        const struct flame *const f = &flame[i];
        const int age = clampk((f->x - EMIT_X) * 128 / (f->L >> 8) + (int)((now_ms - f->born) * 32768 / FLAME_LIFE));
        const int life = 32767 - age;
        int size, lean;
        flicker(f, &size, &lean);
        const unsigned grown = now_ms - f->born;
        const int base = U(4) + (U(13) * f->lv >> 15);
        int h = ((base * pow07(life) >> 15) * size) >> 15;
        if (grown < 60)
            h = h * (int)grown / 60;
        const int w = U(3.6) + (U(2.6) * life >> 15);
        if (h < U(0.4))
            continue;
        const int gy = GROUND - U(0.6);
        if (age < 19661)
            tongue(f->x, gy, h, w, lean, U(3), mix(halo, 0, age * 256 / 19661));
        tongue(f->x, gy, h, w, lean, U(1.15), ramp(col, at, 4, age));
        if (age < 13107 && h > U(2.5)) {
            const int small = h < U(4) ? (U(4) - h) * 256 / (U(4) - U(2.5)) : 0;
            tongue(f->x + U(0.2), gy, h / 2, w * 45 / 100, lean * 6 / 10, U(1), mix(ramp(hot, hot_at, 3, age), 0, small));
        }
    }
}

/* ---------------------------------------------------------------- the screen */

static void ot_init(void *scr)
{
    (void)scr;
    for (int c = 0; c < CABLES; ++c) {
        cable_at[CABLE_AT[c]] = 0;
        for (int i = CABLE_AT[c] + 1; i < CABLE_AT[c + 1]; ++i) {
            const int dx = CABLE[i][0] - CABLE[i - 1][0], dy = CABLE[i][1] - CABLE[i - 1][1];
            cable_at[i] = cable_at[i - 1] + (int)isqrt((unsigned)(dx * dx + dy * dy));
        }
        cable_len[c] = cable_at[CABLE_AT[c + 1] - 1];
    }
    for (int k = 0; k < 3; ++k) {
        const int dx = FX_JX - FX_EL[k][0], dy = FX_JY - FX_EL[k][1], L = (int)isqrt((unsigned)(dx * dx + dy * dy));
        rod_u[k][0] = dx * 4096 / L;
        rod_u[k][1] = dy * 4096 / L;
    }
}

static void ot_update(void *scr, int dt)
{
    const int slot = OP1_SCREEN_SLOT & 7;
    const int16_t *words = op1_synth_knobs(slot);
    int16_t live[4];
    op1_synth_knobs_live(slot, live);
    /* upstream's knob order: 0-3 the four, 5 spread, 6 flux, 7 tone (kennel's page-2 words 5, 7, 6) */
    const short set[8] = { words[0], words[1], words[2], words[3], 0, words[5], words[7], words[6] };
    const short k[8] = { live[0], live[1], live[2], live[3], 0, words[5], words[7], words[6] };

    if (dt <= 0 || dt > 250)
        dt = dt <= 0 ? 33 : 250;
    now_ms += (unsigned)dt;
    const uint32_t b = outatime_blocks;
    quiet = b != seen_blocks ? 0 : quiet < 99 ? quiet + 1 : quiet;
    seen_blocks = b;
    const int held = outatime_held > 0 && quiet < 9;          /* a key down while the voices render (upstream's) */
    const struct ota_voice *const v = outatime_voice(outatime_newest);

    op1_screen_draw_cached(scr, dt, 0);                       /* outatime.svg is all still: cached whole */
    ras = op1_raster();
    const unsigned a0 = ACCENT[0], a1 = ACCENT[1], a2 = ACCENT[2], a3 = ACCENT[3];

    const int spd = clampk(k[OTA_SPEED]), power = clampk(k[OTA_POWER]), trail = clampk(k[OTA_TRAIL]);
    const int speed01 = act * (4915 + (spd * 27853 >> 15)) >> 15;
    const int env = v ? clampk(v->env) : 0;
    const int mph = ota_mph(set[OTA_SPEED]), drv = mph * 32767 / 88;
    const int there = act > 655 && speed01 * 50 > drv * 49;
    if ((v && (v != lit_voice || env > lit_env)) || (there && !at_speed))
        emit = FLAME_GAP;
    at_speed = there;
    lit_voice = v;
    lit_env = env;
    if (held)
        fire_lv = trail;
    else if (fire_lv) {
        const int tau = 40 + (1360 * trail >> 15), x = dt * 32768 / tau;
        const int f = x >= 32768 ? 32767 : x - (x * x >> 16) + ((x * x >> 15) * x >> 15) / 6;   /* 1 - e^-x (x < 1) */
        fire_lv -= fire_lv * (f > 32767 ? 32767 : f) >> 15;
        fire_lv = fire_lv < 164 ? 0 : fire_lv;
    }
    step(held, speed01, there ? fire_lv : 0, dt);
    const int year = ota_year(set[OTA_YEAR]);
    if (dest_year < 0)
        dest_year = year;
    if (year != dest_year) {
        if (!moving)
            last_year = dest_year;
        moving = 1;
        dest_year = year;
        moved_ms = (int)now_ms;
    } else if (moving && (int)now_ms - moved_ms > 1000)
        moving = 0;

    /* the tabs: trail; speed, year, power, or their page-2 knob (kennel's per-knob page) */
    char key[16];
    key[0] = 'T', key[1] = 'R', key[2] = 'A', key[3] = 'I', key[4] = 'L', key[5] = ' ';
    two(key + 6, (clampk(set[OTA_TRAIL]) * 99 + 16383) / 32767);
    key[8] = 0;
    tab_name(0, key, a3);
    static const char *const NAME[3] = { "SPEED", "YEAR", "POWER" }, *const PAGED[3] = { "TONE ", "SPREAD ", "FLUX " };
    static const signed char KNOB[3] = { OTA_TONE, OTA_SPREAD, OTA_FLUX }, ENC[3] = { 1, 0, 2 };
    for (int i = 0; i < 3; ++i) {
        int n = 0;
        const char *p = op1_eng_page[ENC[i]] ? PAGED[i] : NAME[i];
        for (; *p; ++p)
            key[n++] = *p;
        if (op1_eng_page[ENC[i]]) {
            two(key + n, (clampk(set[KNOB[i]]) * 99 + 16383) / 32767);
            n += 2;
        }
        key[n] = 0;
        tab_name(1 + i, key, ACCENT[ENC[i]]);
    }
    /* the time circuits (the year only), the meter, the readout */
    row(0, dest_year, a0, shade(a0, 77));
    row(1, v ? ota_arm_year(k, v->drift, 0) : year, shade(a1, 154), shade(a1, 46));
    row(2, last_year < 0 ? dest_year : last_year, GREY, 0);
    meter(mph, a1, shade(a1, 77));
    readout(ota_gw100(set[OTA_POWER]), a2, shade(a2, 77));

    /* the road under the car, the speed streaks */
    for (int x = road - U(26); x < U(330); x += U(26)) {
        const int x0 = x < U(2) ? U(2) : x, x1 = x + U(11) > U(318) ? U(318) : x + U(11);
        if (x1 > x0 + 128)
            ln(x0, GROUND + U(3.2), x1, GROUND + U(3.2), U(1), OFFBLUE);
    }
    if (speed01 > 14746 && act > 1638) {
        const int n = (5 * (speed01 - 14746) + 9011) / 18022;
        seed(7);
        for (int i = 0; i < n; ++i) {
            const int y = U(14) + (U(40) >> 4) * rnd() / 2048, len = U(10) + ((U(20) >> 4) * rnd() >> 11) * speed01 / 32768;
            const int kk = 6554 + (rnd() >> 3), x = U(160) + (((streak >> 4) * kk >> 8) + (U(400) >> 4) * rnd() / 2048) % U(170);
            if (x < U(312))
                ln(x, y, x + len > U(318) ? U(318) : x + len, y, U(1), DKBLUE);
        }
    }
    fire(a3);

    /* the cables' current while a note plays */
    if (act > 655 && power > 655) {
        const int n = 1 + (2 * power + 16384) / 32768, len = cable_len[0] * 36 >> 10;
        const unsigned pc = mix(a0, WHITE, 64 + (90 * act >> 15)), halo = shade(a0, 77);
        for (int c = 0; c < CABLES; ++c) {
            const int m = (n * cable_len[c] + cable_len[0] / 2) / cable_len[0], nc = m < 1 ? 1 : m;
            for (int i = 0; i < nc; ++i) {
                const int d0 = (cable_d[c] + i * cable_len[c] / nc) % cable_len[c];
                int x0, y0, x1, y1;
                const int e = cable_point(c, d0, &x0, &y0);
                if (e < 0)
                    continue;
                cable_point(c, d0 + len < e ? d0 + len : e, &x1, &y1);
                ln(x0, y0, x1, y1, U(3), halo);
                ln(x0, y0, x1, y1, U(1.4), pc);
            }
        }
    }

    /* the wheels */
    for (int w = 0; w < 2; ++w)
        for (int j = 0; j < SPOKES; ++j) {
            int s, c2;
            sincos(SPOKE[w][j][0] + wheel, &s, &c2);
            ln(WHEEL[w][0] + (c2 * SPOKE[w][j][1] >> 12), WHEEL[w][1] + (s * SPOKE[w][j][1] >> 12),
               WHEEL[w][0] + (c2 * SPOKE[w][j][2] >> 12), WHEEL[w][1] + (s * SPOKE[w][j][2] >> 12), U(1), a0);
        }

    /* the needle: the car's speed, up to the knob's; a tremble flat out */
    {
        const int shown = speed01 < drv ? speed01 : drv;
        int a = meter_angle(shown), s, c2;
        if (there && drv) {
            sincos((int)((now_ms * 46725u) >> 10), &s, &c2);
            a += s * 15 >> 12;
        }
        sincos(a, &s, &c2);
        ln(GA_CX, GA_CY, GA_CX + (c2 * (GA_R - U(3.5)) >> 12), GA_CY - (s * (GA_R - U(3.5)) >> 12), U(1.3), a1);
        dot(GA_CX, GA_CY, U(1.8), LAVENDER);
    }

    /* the flux capacitor: each rod an arm, glowing with power; while a note plays it breathes at the arm's drift */
    {
        int glow[3], sum = 0;
        for (int r = 0; r < 3; ++r) {
            int s = 0, c2;
            if (v)
                sincos((int)((v->drift + (u32)r * THIRD) >> 20), &s, &c2);
            const int half = (2048 + s / 2) * 8;
            glow[r] = power <= 164 ? 0 : power * (18022 + (14746 * (act * half >> 15) >> 15)) >> 15;
            sum += glow[r];
        }
        for (int r = 0; r < 3; ++r) {
            const int ex = FX_EL[r][0], ey = FX_EL[r][1];
            const int x0 = ex + (rod_u[r][0] * ROD_E >> 12), y0 = ey + (rod_u[r][1] * ROD_E >> 12);
            const int x1 = FX_JX - (rod_u[r][0] * ROD_J >> 12), y1 = FX_JY - (rod_u[r][1] * ROD_J >> 12);
            if (glow[r] > 3932)
                ln(x0, y0, x1, y1, U(4.8), rod_halo(glow[r]));
            ln(x0, y0, x1, y1, U(2.1), rod_col(glow[r]));
        }
        const int g = sum / 3;
        if (g > 3932)
            dot(FX_JX, FX_JY, U(3.2) + (U(1.6) * g >> 15), rod_halo(g));
        dot(FX_JX, FX_JY, U(1.7) + (U(0.9) * g >> 15), g > 19661 ? WHITE : rod_col(g));
    }
}
#pragma GCC pop_options

const op1_screen_t SCREEN_outatime = { ot_init, ot_update };
