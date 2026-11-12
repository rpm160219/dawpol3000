// Port screen.c z op1r/taffy (Hundekraft). Arytmetyka i liczby jak w oryginale; rysowanie przez nasz Canvas.
#include "ui/taffy_screen.h"
#include "synth/taffy_dsp.h"
#include <math.h>
#include <string.h>

#include "../../third_party/op1r_taffy/taffy_art.h"
#include "../../third_party/op1r_taffy/taffy_geom.h"

namespace dp {

static const float S = 2.f, OX = 0.f, OY = 44.f;  // ekran OP-1 -> nasz (jak WAVETABLE)
static inline float PX(float u) { return OX + u * S; }
static inline float PY(float u) { return OY + u * S; }
static inline float LW(float units) { return units * S * 0.8f; }

static const Color BLUE = rgb(0x69, 0x8E, 0xFF), GREEN = rgb(0x00, 0xED, 0x95);
static const Color RED = rgb(0xFF, 0x3A, 0x5D), WHITE = rgb(0xFF, 0xFF, 0xFF);

#define R_SIL TF_RSIL
#define R_LOUD TF_RLOUD
#define R_MAX TF_RMAX
#define DV0 19661
#define DV1 26214
static const uint8_t lobes[4] = {3, 6, 12, 6};
static const int16_t shape_k0[4] = {0, 10923, 21846, 32767};

static int16_t SIN_T[8192];
static void sinInit() {
    if (SIN_T[2048]) return;
    for (int i = 0; i < 8192; i++) SIN_T[i] = (int16_t)lrint(32767.0 * sin(2 * M_PI * i / 8192.0));
}
static int32_t sn(uint32_t a) { return SIN_T[a & 8191]; }
static int32_t cs(uint32_t a) { return SIN_T[(a + 2048) & 8191]; }

static int32_t log2q8(uint32_t x) {
    const int b = 31 - __builtin_clz(x);
    const uint32_t m = b >= 8 ? x >> (b - 8) : x << (8 - b);
    return b * 256 + (int32_t)(m & 255);
}
static int32_t clamp15(int32_t v) { return v < 0 ? 0 : v > 32767 ? 32767 : v; }
static int32_t ease(int32_t cur, int32_t to) {
    if (cur < 0) return to;
    const int32_t d = to - cur;
    return d > -64 && d < 64 ? to : cur + (d >> 2);
}

// linia w Q10, przycieta do wierszy 3.7..137 (jak line() w oryginale)
static void line(Canvas& c, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, Color col) {
    enum { TOP = 3 * 1024 + 717, BOT = 137 * 1024 };
    if ((y0 < TOP && y1 < TOP) || (y0 > BOT && y1 > BOT)) return;
    if (y0 < TOP || y0 > BOT) { const int32_t e = y0 < TOP ? TOP : BOT; x0 += (int32_t)((int64_t)(x1 - x0) * (e - y0) / (y1 - y0)); y0 = e; }
    if (y1 < TOP || y1 > BOT) { const int32_t e = y1 < TOP ? TOP : BOT; x1 += (int32_t)((int64_t)(x0 - x1) * (e - y1) / (y0 - y1)); y1 = e; }
    c.line(PX(x0 / 1024.f), PY(y0 / 1024.f), PX(x1 / 1024.f), PY(y1 / 1024.f), LW(w / 1024.f), col);
}

void TaffyScreen::buildCache() {
    sinInit();
    cache_ = new Canvas();
    Canvas& c = *cache_;
    c.clear(pal::bg);
    for (const TFLine& l : TF_LINES) {
        Color col = rgb((l.rgb >> 16) & 255, (l.rgb >> 8) & 255, l.rgb & 255);
        c.beginStroke();
        for (int i = 0; i + 1 < l.count; i++) {
            const float* p = TF_PTS + (l.first + i) * 2;
            c.strokeSeg(PX(p[0]), PY(p[1]), PX(p[2]), PY(p[3]), LW(l.width));
        }
        c.endStroke(col);
    }
    for (const TFCircle& d : TF_CIRCLES)
        c.fillCircle(PX(d.x), PY(d.y), d.r * S + 0.6f, rgb((d.rgb >> 16) & 255, (d.rgb >> 8) & 255, d.rgb & 255));
}

void TaffyScreen::animate(const int16_t kin[4], bool playing, int32_t env, int32_t lev) {
    playing_ = playing;
    if (!playing) env = lev = 0;
    for (int i = 0; i < 4; i++) k_[i] = kin[i] < 0 ? 0 : kin[i];
    kw_ = ease(kw_, k_[0]);
    const int32_t t = lev > 0 ? clamp15((log2q8((uint32_t)lev) - 9 * 256) * 32767 / (6 * 256)) : 0;
    eLev_ += ((t - eLev_) * (t > eLev_ ? 16384 : 1092)) >> 15;
    if (t < eLev_ && eLev_ - t < 32) eLev_ = t;
    dcw_ = playing ? k_[1] + ((k_[2] * env) >> 15) : k_[1];
    if (dcw_ > 32767) dcw_ = 32767;
    kd_ = ease(kd_, dcw_);
    flash_ = playing ? (env * (9830 + ((k_[2] * 22937) >> 15))) >> 15 : 0;
    size_ = R_SIL + (int32_t)(((int64_t)(R_LOUD - R_SIL) * eLev_) >> 15) +
            (int32_t)(((int64_t)(TF_KICK0 + ((k_[2] * TF_KICK_ADD) >> 15)) * env) >> 15);
    bx_ = TF_BX + (((size_ - TF_BR) * 5) >> 2);
    if (playing) { rot_ += 2 + ((eLev_ * 10) >> 15); ripple_ += 160; }
    const int32_t dv = DV0 + (((DV1 - DV0) * kd_) >> 15);
    const int32_t pos = kw_ * 3, frac = pos & 0x7FFF;
    const int sa = pos >> 15, sb = sa + (sa < 3);
    if (sa != waveSa_ || dv != waveDv_) {
        waveSa_ = sa;
        waveDv_ = dv;
        for (int s = 0; s < 2; s++) {
            taffyWaveCycle(wave_, NW, shape_k0[s ? sb : sa], dv);
            int32_t sum = 0, lo = 32767, hi = -32768;
            for (int i = 0; i < NW; i++) {
                sum += wave_[i];
                if (wave_[i] < lo) lo = wave_[i];
                if (wave_[i] > hi) hi = wave_[i];
            }
            const int32_t mean = sum / NW, pp = hi - lo > 256 ? hi - lo : 256;
            for (int i = 0; i < NW; i++) wn_[s][i] = (int32_t)(((int64_t)(wave_[i] - mean) << 16) / pp);
        }
    }
    int32_t hi = 0;
    for (int i = 0; i < N; i++) {
        int32_t w2[2];
        for (int s = 0; s < 2; s++) {
            const int32_t m = (i * lobes[s ? sb : sa]) % N;
            const int32_t u = (m * 4067203) >> 16;
            const int j = u >> 8, j1 = j + 1 == NW ? 0 : j + 1;
            w2[s] = wn_[s][j] + (((wn_[s][j1] - wn_[s][j]) * (u & 255)) >> 8);
        }
        rad_[i] = w2[0] + (int32_t)(((int64_t)(w2[1] - w2[0]) * frac) >> 15);
        if (rad_[i] > hi) hi = rad_[i];
    }
    int32_t depth = (int32_t)(((int64_t)size_ * (41 + ((kd_ * 133) >> 15))) >> 10);
    if (hi > 0 && size_ + (int32_t)(((int64_t)depth * hi) >> 15) > R_MAX) depth = (int32_t)(((int64_t)(R_MAX - size_) << 15) / hi);
    if (depth < 0) depth = 0;
    int32_t lo = R_MAX;
    for (int i = 0; i < N; i++) {
        rad_[i] = size_ + (int32_t)(((int64_t)depth * rad_[i]) >> 15);
        if (rad_[i] < lo) lo = rad_[i];
    }
    amp_ = k_[3] > 8192 ? (int32_t)(((int64_t)(k_[3] - 8192) * 2 * TF_RIPPLE) / 24575) : 0;
    hlR_ = (int32_t)(((int64_t)size_ * TF_HL_INSET) >> 10);
    if (hlR_ > lo - 4096 - amp_) hlR_ = lo - 4096 - amp_;
}

int32_t TaffyScreen::radiusAt(uint32_t a) const {
    const uint32_t u = ((a - (uint32_t)TF_LOBE0 - (uint32_t)rot_) & 8191) * N;
    const int i = (int)(u >> 13), j = i + 1 == N ? 0 : i + 1;
    const int32_t f = (int32_t)(u & 8191);
    return rad_[i] + (int32_t)(((int64_t)(rad_[j] - rad_[i]) * f) >> 13);
}

void TaffyScreen::polar(uint32_t a, int32_t r, int32_t* x, int32_t* y) const {
    *x = bx_ + (int32_t)(((int64_t)r * cs(a)) >> 15);
    *y = TF_BY + (int32_t)(((int64_t)r * sn(a)) >> 15);
}

void TaffyScreen::strand(Canvas& c, int32_t x0, int32_t y0, int32_t x2, int32_t y2, int32_t bw, int32_t tq, int32_t* tx,
                         int32_t* ty) {
    const int32_t x1 = (x0 + x2) >> 1, y1 = ((y0 + y2) >> 1) - (int32_t)(((int64_t)(x2 - x0) * bw) >> 10);
    int32_t px = x0, py = y0;
    for (int i = 1; i <= 8; i++) {
        const int32_t u = i * 128, v = 1024 - u;
        const int32_t x = (int32_t)(((int64_t)v * v * x0 + 2LL * u * v * x1 + (int64_t)u * u * x2) >> 20);
        const int32_t y = (int32_t)(((int64_t)v * v * y0 + 2LL * u * v * y1 + (int64_t)u * u * y2) >> 20);
        line(c, px, py, x, y, 1331, GREEN);
        px = x;
        py = y;
    }
    const int32_t u = tq, v = 1024 - tq;
    *tx = (int32_t)(((int64_t)v * v * x0 + 2LL * u * v * x1 + (int64_t)u * u * x2) >> 20);
    *ty = (int32_t)(((int64_t)v * v * y0 + 2LL * u * v * y1 + (int64_t)u * u * y2) >> 20);
}

int32_t TaffyScreen::bow(int32_t b) const {
    const int32_t g = k_[1];
    if (g < 8192) return b - (((8192 - g) * 410) >> 13);
    return (int32_t)(((int64_t)b * (32767 - g)) / 24575);
}

void TaffyScreen::dash(Canvas& c, uint32_t a, int32_t r0, int32_t r1) {
    const int32_t push = (flash_ * TF_PUSH) >> 15;
    const int32_t i = size_ + r0 + push;
    int32_t o = i + (int32_t)(((int64_t)(r1 - r0) * (32768 + k_[2] + ((flash_ * 3) >> 2))) >> 15);
    const int32_t sv = sn(a), room = sv > 0 ? 136 * 1024 - TF_BY : sv < 0 ? TF_BY - 4608 : 0;
    if (sv) {
        const int32_t lim = (int32_t)(((int64_t)room << 15) / (sv > 0 ? sv : -sv));
        if (o > lim) o = lim;
        if (i >= o) return;
    }
    int32_t x0, y0, x1, y1;
    polar(a, i, &x0, &y0);
    polar(a, o, &x1, &y1);
    line(c, x0, y0, x1, y1, 1331, WHITE);
}

void TaffyScreen::drawLive(Canvas& c) {
    int32_t sx[3] = {}, sy[3] = {};
    for (int s = 0; s < 5; s++) {  // nici gumy
        const int32_t* d = tf_strand[s];
        const uint32_t a = (uint32_t)d[2];
        int32_t ex, ey, x0, y0, tx, ty;
        polar(a, radiusAt(a), &ex, &ey);
        if (d[0] < 0) {
            x0 = TF_MX + (int32_t)(((int64_t)(ex - TF_MX) * d[3]) >> 10);
            y0 = TF_MY + (int32_t)(((int64_t)(ey - TF_MY) * d[3]) >> 10);
        } else { x0 = sx[d[0]]; y0 = sy[d[0]]; }
        const int32_t tq = s == 0 ? tf_strand[3][1] : s == 2 ? tf_strand[4][1] : 0;
        strand(c, x0, y0, ex, ey, bow(d[4]), tq, &tx, &ty);
        if (s < 3) { sx[s] = tx; sy[s] = ty; }
    }
    {  // balon
        int32_t px = 0, py = 0;
        c.beginStroke();
        for (int i = 0; i <= N; i++) {
            const int j = i == N ? 0 : i;
            const uint32_t a = (uint32_t)TF_LOBE0 + (uint32_t)rot_ + (uint32_t)((j * 8192 + N / 2) / N);
            int32_t x, y;
            polar(a, rad_[j], &x, &y);
            if (i) c.strokeSeg(PX(px / 1024.f), PY(py / 1024.f), PX(x / 1024.f), PY(y / 1024.f), LW(1536 / 1024.f));
            px = x;
            py = y;
        }
        c.endStroke(BLUE);
    }
    {  // odblaski (RESONANCE)
        const int32_t k3 = k_[3];
        const int32_t len = k3 < 8192 ? 410 + ((k3 * 614) >> 13) : 1024 + (((k3 - 8192) * 1024) / 24575);
        const int32_t freq = 18 + ((((256 + ((dcw_ * 3840) >> 15)) >> 4) * 26) >> 8);
        for (unsigned h = 0; h < sizeof tf_hl / sizeof tf_hl[0]; h++) {
            const int32_t g = k3 - tf_hl[h][3];
            if (g <= 0) continue;
            const int32_t grow = g >= 3277 ? 1024 : (g * 1024) / 3277;
            const int32_t half = (int32_t)(((int64_t)tf_hl[h][1] * len >> 10) * grow >> 10);
            const int n = 1 + half / 23;
            const Color col = tf_hl[h][2] ? WHITE : RED;
            int32_t px = 0, py = 0;
            for (int i = 0; i <= n; i++) {
                const uint32_t a = (uint32_t)(tf_hl[h][0] - half + (2 * half * i) / n);
                int32_t r = hlR_;
                if (amp_) r += (int32_t)(((int64_t)amp_ * sn(a * (uint32_t)freq + (uint32_t)ripple_)) >> 15);
                int32_t x, y;
                polar(a, r, &x, &y);
                if (i) line(c, px, py, x, y, 1485, col);
                px = x;
                py = y;
            }
            if (!half) line(c, px, py, px + 300, py, 1485, col);
        }
    }
    // iskry (ENV)
    for (unsigned s = 0; s < sizeof tf_spark / sizeof tf_spark[0]; s++) dash(c, (uint32_t)tf_spark[s][0], tf_spark[s][1], tf_spark[s][2]);
    const int32_t pop = flash_ > ((k_[2] * 26214) >> 15) ? flash_ : (k_[2] * 26214) >> 15;
    if (pop > 3277)
        for (unsigned s = 0; s < sizeof tf_pop / sizeof tf_pop[0]; s++) {
            const int32_t r1 = tf_pop[s][1] + (int32_t)(((int64_t)(tf_pop[s][2] - tf_pop[s][1]) * pop) >> 15);
            dash(c, (uint32_t)tf_pop[s][0], tf_pop[s][1], r1);
        }
    for (unsigned s = 0; s < sizeof tf_lipspark / sizeof tf_lipspark[0]; s++) {
        const int32_t* d = tf_lipspark[s];
        const int32_t dx = d[2] - d[0], dy = d[3] - d[1];
        const int32_t a = (flash_ * 410) >> 15, b = 1024 + a + ((flash_ * 666) >> 15) + ((k_[2] * 410) >> 15);
        line(c, d[0] + ((dx * a) >> 10), d[1] + ((dy * a) >> 10), d[0] + ((dx * b) >> 10), d[1] + ((dy * b) >> 10), 1331, WHITE);
    }
}

void TaffyScreen::draw(Canvas& c, const int16_t k[4], bool playing, int16_t env, int16_t lev) {
    if (!cache_) buildCache();
    c.copyRows(*cache_, (int)OY, (int)(OY + 160 * S));
    animate(k, playing, env, lev);
    drawLive(c);
}

}  // namespace dp
