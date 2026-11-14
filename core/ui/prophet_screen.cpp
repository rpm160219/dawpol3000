#include "ui/prophet_screen.h"
#include "gfx/font.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
namespace dp {
namespace {
#include "ui/prophet_art.inc"
struct Art {
    Canvas canvas;
    Art() {
        canvas.clear(pal::bg);
        float xy[2 * kProphetMaxPoints];
        for (const auto &a : kProphetContours) {
            for (unsigned i = 0; i < a.count; i++) {
                xy[2 * i] = 20 + .25f * kProphetPoints[a.start + i][0];
                xy[2 * i + 1] = 5 + .25f * kProphetPoints[a.start + i][1];
            }
            canvas.fillPolygon(xy, a.count, a.color);
        }
    }
};
const Canvas &reference() {
    static Art art;
    return art.canvas;
}
float waveValue(const int16_t *p, int osc, float t) {
    int bits = p[osc ? P5_BWAVE : P5_AWAVE] * (osc ? 8 : 4) / 32768;
    float pw = .01f + .98f * p[osc ? P5_BPW : P5_APW] / 32767.f, v = 0;
    t -= std::floor(t);
    if (bits & 1)
        v += 2 * t - 1;
    if (bits & (osc ? 4 : 2))
        v += t < pw ? 1 : -1;
    if (osc && (bits & 2))
        v += 1 - 4 * std::fabs(t - .5f);
    return v;
}
Color oscColor(const int16_t *p, int osc) {
    return p[P5_REVISION] >= 16384 ? (osc ? rgb(174, 119, 242) : rgb(240, 174, 72))
                                   : (osc ? pal::green : pal::blue);
}
void wavePlot(Canvas &c, const int16_t *p, int osc, float x, float y) {
    float xy[260];
    for (int j = 0; j < 130; j++) {
        xy[2 * j] = x + j * 100 / 129.f;
        xy[2 * j + 1] = y - waveValue(p, osc, 2 * j / 129.f) * 8;
    }
    c.line(x, y, x + 100, y, 1, pal::faint);
    c.polyline(xy, 130, 1.15f, oscColor(p, osc));
}
uint8_t byte(float v) { return (uint8_t)std::clamp(v, 0.f, 255.f); }
void palette(uint8_t *px) {
    int r = px[0], g = px[1], b = px[2], m = std::max({r, g, b});
    Color col;
    if (b > r + 12 && b > g * 1.15f)
        col = rgb(240, 174, 72);
    else if (g > r + 12 && g > b * 1.15f)
        col = rgb(174, 119, 242);
    else if (r > g * 1.35f && r > b * 1.25f)
        col = rgb(67, 192, 224);
    else if (r > g * 1.05f && g > b * 1.05f)
        col = rgb(192, 202, 229);
    else
        return;
    px[0] = byte(m * col.r / 255.f);
    px[1] = byte(m * col.g / 255.f);
    px[2] = byte(m * col.b / 255.f);
}
// Sample the original line work, never replace the eye with a different icon.
void eye(Canvas &c, const Canvas &art, float opening, float resonance) {
    // Keep both original lids, the triangle and its rays completely fixed.
    // Only the traced iris/spiral changes aperture inside its original disc.
    constexpr float cx = 320, cy = 101, radius = 8.5f;
    for (int y = 92; y <= 110; y++)
        for (int x = 311; x <= 329; x++) {
            float dx = x - cx, dy = y - cy, d2 = dx * dx + dy * dy;
            if (d2 > radius * radius)
                continue;
            uint8_t *dst = c.pixels() + 4 * (y * 640 + x);
            float sx = cx + dx / opening, sy = cy + dy / opening;
            float edge = std::clamp((radius - std::sqrt(d2) / opening) * opening + .5f, 0.f, 1.f);
            if (edge <= 0) {
                dst[0] = dst[1] = dst[2] = 0;
                continue;
            }
            int ix = (int)sx, iy = (int)sy;
            float fx = sx - ix, fy = sy - iy;
            const uint8_t *src = art.pixels() + 4 * (iy * 640 + ix);
            for (int k = 0; k < 3; k++) {
                float upper = src[k] + fx * (src[k + 4] - src[k]);
                float lower = src[k + 640 * 4] + fx * (src[k + 640 * 4 + 4] - src[k + 640 * 4]);
                float value = (upper + fy * (lower - upper)) * edge;
                dst[k] = byte(value * (k == 1 ? 1 - .82f * resonance : k == 2 ? 1 - .76f * resonance : 1));
            }
        }
}
// Sample a point ON a source lightning contour. Traveling white discharge
// stays inside its existing red light, independently of Poly-Mod routing.
void sparks(Canvas &c, const Canvas &art, float time) {
    for (int side = 0; side < 2; side++)
        for (int lane = 0; lane < 2; lane++) {
            float t = std::fmod(time * (.35f + .09f * lane) + .47f * lane + .29f * side, 1.f);
            int x = (int)(side ? 356 + t * 86 : 284 - t * 86), aim = lane ? 182 : 169, best = -1;
            float score = -1e6f;
            for (int y = 151; y < 204; y++) {
                const uint8_t *q = art.pixels() + 4 * (y * 640 + x);
                float red = q[0] - std::max(q[1], q[2]);
                float rank = red - 2.5f * std::abs(y - aim);
                if (red > 30 && rank > score) {
                    score = rank;
                    best = y;
                }
            }
            if (best < 0)
                continue;
            for (int yy = best - 3; yy <= best + 3; yy++)
                for (int xx = x - 4; xx <= x + 4; xx++) {
                    const uint8_t *q = art.pixels() + 4 * (yy * 640 + xx);
                    if (q[0] < q[1] * 1.4f || q[0] < q[2] * 1.3f)
                        continue;
                    float d = (xx - x) * (xx - x) / 16.f + (yy - best) * (yy - best) / 9.f;
                    if (d >= 1)
                        continue;
                    uint8_t *dst = c.pixels() + 4 * (yy * 640 + xx);
                    float strength = (1 - d) * .85f;
                    for (int k = 0; k < 3; k++)
                        dst[k] = byte(dst[k] + strength * (255 - dst[k]));
                }
        }
}
} // namespace
void drawProphetWheel(Canvas &c, const int16_t *p, float x, float y, float w, float h) {
    float depth = p[P5_WHEEL] / 32767.f, mix = p[P5_WMIX] / 32767.f;
    float cx = x + w * .5f, top = y + 2, bottom = y + h - 5, radius = std::min(h * .42f, w * .1f);
    c.strokeRoundRect(cx - radius, top, 2 * radius, bottom - top, radius * .7f, 1.2f, pal::silver);
    c.fillRoundRect(cx - radius + 3, top + 3, 2 * radius - 6, bottom - top - 6, radius * .5f,
                    rgb(23, 23, 27));
    for (int i = 0; i < 7; i++) {
        float sy = top + 4 + std::fmod(i * 5.f + depth * 21, bottom - top - 8);
        c.line(cx - radius + 4, sy, cx + radius - 4, sy, .7f, pal::dim);
    }
    float mark = bottom - 5 - depth * (bottom - top - 10);
    c.line(cx - radius + 2, mark, cx + radius - 2, mark, 2, pal::red);
    c.fillRect(cx - radius - 5, bottom - depth * (bottom - top), 2, depth * (bottom - top), pal::red);
    drawText(c, x, top, "LFO", 7, 1, mix < .99f ? pal::blue : pal::dim);
    drawText(c, x, top + 12, "PINK", 7, 1, mix > .01f ? pal::green : pal::dim);
    c.line(x + 26, top + 4, cx - radius - 7, top + 12, 1, rgb(64, 128, 255, byte(255 * (1 - mix))));
    c.line(x + 26, top + 16, cx - radius - 7, top + 12, 1, rgb(40, 215, 110, byte(255 * mix)));
    c.line(cx + radius + 2, y + h * .5f, x + w - 21, y + h * .5f, 1, depth ? pal::red : pal::dim);
    int ids[] = {P5_WFREQA, P5_WFREQB, P5_WPWA, P5_WPWB, P5_WFILTER};
    const char *names[] = {"A", "B", "PWA", "PWB", "FLT"};
    for (int i = 0; i < 5 && w > 110; i++)
        drawText(c, x + w, y + i * (h - 2) / 5, names[i], 6, 1, p[ids[i]] >= 16384 ? pal::white : pal::dim,
                 ALIGN_RIGHT);
    char value[16];
    std::snprintf(value, sizeof(value), "%d%%", (int)std::lround(depth * 100));
    drawText(c, cx, bottom + 4, value, 7, 1, pal::red, ALIGN_CENTER);
}
float ProphetScreen::random() {
    rng_ ^= rng_ << 13;
    rng_ ^= rng_ >> 17;
    rng_ ^= rng_ << 5;
    return (rng_ >> 8) / 16777216.f;
}
void ProphetScreen::draw(Canvas &c, const int16_t *p, const ProphetView &v, float dt) {
    dt = std::clamp(dt, 0.f, .1f);
    time_ += dt;
    float amp = 0, env = 0;
    for (int i = 0; i < 5; i++) {
        amp = std::max(amp, v.state.amp[i]);
        env = std::max(env, v.state.env[i]);
    }
    flash_ = std::max(flash_ * std::exp(-dt * 10), std::min(1.f, std::max(0.f, amp - lastAmp_) * 5));
    lastAmp_ = amp;
    eye_ += (.32f + .68f * p[P5_CUTOFF] / 32767.f - eye_) * (1 - std::exp(-dt * 18));
    shimmerClock_ -= dt;
    if (shimmerClock_ <= 0) {
        shimmerClock_ = .075f;
        for (float &t : target_)
            t = .38f + 1.05f * random();
    }
    for (int i = 0; i < 6; i++)
        shimmer_[i] += (target_[i] - shimmer_[i]) * (1 - std::exp(-dt * 8));
    const Canvas &art = reference();
    std::memcpy(c.pixels() + 44 * 640 * 4, art.pixels() + 44 * 640 * 4, 290 * 640 * 4);
    const float stars[6][2] = {{263.5f, 73}, {376.5f, 73},  {226, 97},
                               {414, 97},    {256, 124.5f}, {384, 124.5f}};
    for (int i = 0; i < 6; i++)
        for (int y = (int)stars[i][1] - 11; y < stars[i][1] + 11; y++)
            for (int x = (int)stars[i][0] - 11; x < stars[i][0] + 11; x++) {
                uint8_t *q = c.pixels() + 4 * (y * 640 + x);
                for (int k = 0; k < 3; k++)
                    q[k] = byte(q[k] * shimmer_[i]);
            }
    float pulse = env * (p[P5_ENVAMT] / 32767.f) * .15f;
    if (p[P5_WFILTER] >= 16384)
        pulse += .08f * (p[P5_WHEEL] / 32767.f) * v.state.lfo;
    for (int y = 59; y < 127; y++)
        for (int x = 276; x < 365; x++) {
            if ((x - 320.f) * (x - 320.f) / 625 + (y - 101.f) * (y - 101.f) / 196 < 1)
                continue;
            uint8_t *q = c.pixels() + 4 * (y * 640 + x);
            if (std::max({q[0], q[1], q[2]}) - std::min({q[0], q[1], q[2]}) > 18)
                continue;
            for (int k = 0; k < 3; k++)
                q[k] = byte(q[k] * (1 + pulse));
        }
    eye(c, art, eye_, p[P5_RESONANCE] / 32767.f);
    for (int y = 129; y < 305; y++)
        for (int x = 178; x < 462; x++) {
            uint8_t *q = c.pixels() + 4 * (y * 640 + x);
            int osc = x < 220 ? 0 : x > 425 ? 1 : -1;
            bool ink = osc >= 0 && (osc ? q[1] > q[0] + 12 && q[1] > q[2] * 1.2f
                                        : q[2] > q[0] + 12 && q[2] > q[1] * 1.2f);
            if (ink) {
                float level = p[osc ? P5_MIXB : P5_MIXA] / 32767.f;
                int high = std::max({q[0], q[1], q[2]});
                for (int k = 0; k < 3; k++)
                    q[k] = byte((high + level * (q[k] - high)) * (1 + .12f * flash_));
            } else if (y > 232 && x > 210 && x < 432 && q[0] > q[1] * 1.05f && q[1] > q[2] * 1.05f) {
                for (int k = 0; k < 3; k++)
                    q[k] = byte(q[k] * (1 + .09f * amp + .09f * flash_));
            }
        }
    sparks(c, art, time_);
    // Palette is applied after source-ink animations; original geometry survives.
    if (p[P5_REVISION] >= 16384)
        for (int y = 44; y < 334; y++)
            for (int x = 170; x < 470; x++)
                if ((x - 320.f) * (x - 320.f) + (y - 101.f) * (y - 101.f) >= 81)
                    palette(c.pixels() + 4 * (y * 640 + x));
    for (int osc = 0; osc < 2; osc++) {
        float level = p[osc ? P5_MIXB : P5_MIXA] / 32767.f;
        Color color = oscColor(p, osc);
        color.a = byte(180 * level);
        for (int part = 0; part < 2; part++) {
            float yy = part ? 268 : 139;
            int count = part ? 32 : 88;
            float points[176];
            for (int j = 0; j < count; j++) {
                float y = yy + j, cx = osc ? 453 - (y - 130) * .063f : 188 + (y - 130) * .077f;
                points[2 * j] =
                    cx +
                    std::clamp(waveValue(p, osc, time_ * (.35f + .4f * amp) + (y - 130) / 31), -2.f, 2.f) *
                        1.2f;
                points[2 * j + 1] = y;
            }
            c.polyline(points, count, .7f, color);
        }
    }
    c.line(20, 305, 620, 305, 1.5f, p[P5_REVISION] >= 16384 ? rgb(120, 82, 65) : rgb(85, 69, 115));
    for (int osc = 0; osc < 2; osc++) {
        float x = osc ? 500 : 40;
        drawText(c, x + 50, 170, osc ? "OSC B" : "OSC A", 10, 1.2f, oscColor(p, osc), ALIGN_CENTER);
        wavePlot(c, p, osc, x, 201);
        char value[24];
        prophetValue(osc ? P5_BFREQ : P5_AFREQ, p[osc ? P5_BFREQ : P5_AFREQ], value, sizeof(value));
        drawText(c, x + 50, 222, osc && p[P5_BLOW] >= 16384 ? "LO FREQ" : value, 8, 1, pal::silver,
                 ALIGN_CENTER);
    }
    float xy[256];
    for (int j = 0; j < 128; j++) {
        xy[2 * j] = 40 + j * 100 / 127.f;
        xy[2 * j + 1] = 264 - std::clamp(v.scope[j] * 60, -17.f, 17.f);
    }
    c.polyline(xy, 128, 1, pal::tan);
    for (int i = 0; i < 5; i++)
        c.fillCircle(62 + i * 14, 288, 2.5f,
                     rgb(byte(55 + 180 * v.state.amp[i]), byte(48 + 155 * v.state.amp[i]),
                         byte(38 + 135 * v.state.amp[i])));
    drawText(c, 550, 249, "MOD WHEEL", 7, 1, pal::dim, ALIGN_CENTER);
    drawProphetWheel(c, p, 504, 261, 92, 33);
    drawText(c, 619, 316, p[P5_REVISION] >= 16384 ? "REV 2 / SSM" : "REV 3 / CEM", 7, 1,
             p[P5_REVISION] >= 16384 ? rgb(240, 174, 72) : pal::dim, ALIGN_RIGHT);
}
void drawProphetIllustration(Canvas &c, float x, float y, float w, float h) {
    const auto &a = reference();
    float scale = std::min(w / 330, h / 250);
    int dw = (int)(330 * scale), dh = (int)(250 * scale);
    x += (w - dw) * .5f;
    for (int j = 0; j < dh; j++)
        for (int i = 0; i < dw; i++) {
            int sx = std::clamp((int)(155 + i / scale), 0, 639),
                sy = std::clamp((int)(57 + j / scale), 0, 479);
            const uint8_t *p = a.pixels() + 4 * (sy * 640 + sx);
            c.fillRect(x + i, y + j, 1, 1, rgb(p[0], p[1], p[2]));
        }
}
} // namespace dp
