#include "gfx/canvas.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace dp {

static inline float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }

Canvas::Canvas() {
    px_ = (uint8_t*)calloc(W * H, 4);
    cov_ = (float*)calloc(W * H, sizeof(float));
    dx0_ = W; dy0_ = H; dx1_ = -1; dy1_ = -1;
}
Canvas::~Canvas() { free(px_); free(cov_); }

void Canvas::clear(Color c) {
    for (int i = 0; i < W * H; i++) {
        uint8_t* p = px_ + i * 4;
        p[0] = c.r; p[1] = c.g; p[2] = c.b; p[3] = 255;
    }
}

void Canvas::blend(int idx, Color c, float a) {
    a *= c.a * (1.f / 255.f);
    if (a <= 0.f) return;
    uint8_t* p = px_ + idx * 4;
    p[0] = (uint8_t)(p[0] + (c.r - p[0]) * a + 0.5f);
    p[1] = (uint8_t)(p[1] + (c.g - p[1]) * a + 0.5f);
    p[2] = (uint8_t)(p[2] + (c.b - p[2]) * a + 0.5f);
}

void Canvas::fillRect(float x, float y, float w, float h, Color c) {
    int x0 = (int)clampf(floorf(x), 0, W), x1 = (int)clampf(ceilf(x + w), 0, W);
    int y0 = (int)clampf(floorf(y), 0, H), y1 = (int)clampf(ceilf(y + h), 0, H);
    for (int j = y0; j < y1; j++) {
        float cy = clampf(fminf(j + 1.f, y + h) - fmaxf((float)j, y), 0, 1);
        for (int i = x0; i < x1; i++) {
            float cx = clampf(fminf(i + 1.f, x + w) - fmaxf((float)i, x), 0, 1);
            blend(j * W + i, c, cx * cy);
        }
    }
}

void Canvas::fillCircle(float cx, float cy, float r, Color c) {
    int x0 = (int)clampf(floorf(cx - r - 1), 0, W), x1 = (int)clampf(ceilf(cx + r + 1), 0, W);
    int y0 = (int)clampf(floorf(cy - r - 1), 0, H), y1 = (int)clampf(ceilf(cy + r + 1), 0, H);
    for (int j = y0; j < y1; j++)
        for (int i = x0; i < x1; i++) {
            float dx = i + 0.5f - cx, dy = j + 0.5f - cy;
            float a = clampf(r + 0.5f - sqrtf(dx * dx + dy * dy), 0, 1);
            if (a > 0) blend(j * W + i, c, a);
        }
}

void Canvas::rect(float x, float y, float w, float h, float lw, Color c) {
    float p[8] = {x, y, x + w, y, x + w, y + h, x, y + h};
    polyline(p, 4, lw, c, true);
}

// Odleglosc ze znakiem od zaokraglonego prostokata (ujemna w srodku).
static inline float sdRoundRect(float px, float py, float cx, float cy, float hw, float hh, float r) {
    float qx = fabsf(px - cx) - (hw - r), qy = fabsf(py - cy) - (hh - r);
    float ox = qx > 0 ? qx : 0, oy = qy > 0 ? qy : 0;
    float in = qx > qy ? qx : qy;
    return sqrtf(ox * ox + oy * oy) + (in < 0 ? in : 0) - r;
}

void Canvas::roundRect(float x, float y, float w, float h, float r, float lw, Color c) {
    if (w <= 0 || h <= 0) return;
    if (r > w * 0.5f) r = w * 0.5f;
    if (r > h * 0.5f) r = h * 0.5f;
    float cx = x + w * 0.5f, cy = y + h * 0.5f, ext = lw > 0 ? lw : 0;
    int x0 = (int)clampf(floorf(x - ext - 1), 0, W), x1 = (int)clampf(ceilf(x + w + ext + 1), 0, W);
    int y0 = (int)clampf(floorf(y - ext - 1), 0, H), y1 = (int)clampf(ceilf(y + h + ext + 1), 0, H);
    for (int j = y0; j < y1; j++)
        for (int i = x0; i < x1; i++) {
            float d = sdRoundRect(i + 0.5f, j + 0.5f, cx, cy, w * 0.5f, h * 0.5f, r);
            float a = lw > 0 ? clampf(lw * 0.5f + 0.5f - fabsf(d), 0, 1) : clampf(0.5f - d, 0, 1);
            if (a > 0) blend(j * W + i, c, a);
        }
}

void Canvas::beginStroke() { dx0_ = W; dy0_ = H; dx1_ = -1; dy1_ = -1; }

void Canvas::strokeSeg(float x0, float y0, float x1, float y1, float w) {
    float hw = w * 0.5f;
    float amul = w < 1.f ? w : 1.f;
    if (hw < 0.5f) hw = 0.5f;
    int xa = (int)clampf(floorf(fminf(x0, x1) - hw - 1), 0, W);
    int xb = (int)clampf(ceilf(fmaxf(x0, x1) + hw + 1), 0, W);
    int ya = (int)clampf(floorf(fminf(y0, y1) - hw - 1), 0, H);
    int yb = (int)clampf(ceilf(fmaxf(y0, y1) + hw + 1), 0, H);
    if (xa >= xb || ya >= yb) return;
    float dx = x1 - x0, dy = y1 - y0, l2 = dx * dx + dy * dy;
    float inv = l2 > 1e-9f ? 1.f / l2 : 0.f;
    for (int j = ya; j < yb; j++) {
        float py = j + 0.5f - y0;
        float* row = cov_ + j * W;
        for (int i = xa; i < xb; i++) {
            float px = i + 0.5f - x0;
            float t = clampf((px * dx + py * dy) * inv, 0, 1);
            float ex = px - t * dx, ey = py - t * dy;
            float a = hw + 0.5f - sqrtf(ex * ex + ey * ey);
            if (a <= 0.f) continue;
            if (a > 1.f) a = 1.f;
            a *= amul;
            if (a > row[i]) row[i] = a;
        }
    }
    if (xa < dx0_) dx0_ = xa;
    if (ya < dy0_) dy0_ = ya;
    if (xb > dx1_) dx1_ = xb;
    if (yb > dy1_) dy1_ = yb;
}

void Canvas::endStroke(Color c) {
    for (int j = dy0_; j < dy1_; j++) {
        float* row = cov_ + j * W;
        for (int i = dx0_; i < dx1_; i++) {
            if (row[i] > 0.f) { blend(j * W + i, c, row[i]); row[i] = 0.f; }
        }
    }
    beginStroke();
}

void Canvas::line(float x0, float y0, float x1, float y1, float w, Color c) {
    beginStroke();
    strokeSeg(x0, y0, x1, y1, w);
    endStroke(c);
}

void Canvas::polyline(const float* xy, int n, float w, Color c, bool closed) {
    beginStroke();
    for (int i = 0; i + 1 < n; i++) strokeSeg(xy[2 * i], xy[2 * i + 1], xy[2 * i + 2], xy[2 * i + 3], w);
    if (closed && n > 2) strokeSeg(xy[2 * n - 2], xy[2 * n - 1], xy[0], xy[1], w);
    endStroke(c);
}

void Canvas::arc(float cx, float cy, float r, float a0, float a1, float w, Color c) {
    float span = fabsf(a1 - a0);
    int n = (int)(r * span / 3.f);
    if (n < 6) n = 6;
    if (n > 256) n = 256;
    beginStroke();
    float px = cx + r * cosf(a0), py = cy + r * sinf(a0);
    for (int i = 1; i <= n; i++) {
        float a = a0 + (a1 - a0) * i / n;
        float x = cx + r * cosf(a), y = cy + r * sinf(a);
        strokeSeg(px, py, x, y, w);
        px = x; py = y;
    }
    endStroke(c);
}

}  // namespace dp
