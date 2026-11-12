#include "ui/widgets.h"
#include "gfx/font.h"
#include "gfx/icons.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

namespace dp {

static const Color kDarkText = rgb(22, 22, 22);

void headerBar(Canvas& c, float x, float y, float w, float h, const char* text, Color fill) {
    c.fillRoundRect(x, y, w, h, 8, fill);
    float sz = h * 0.5f;
    drawText(c, x + w * 0.5f, y + (h - sz) * 0.5f, text, sz, 2.f, kDarkText, ALIGN_CENTER);
}

// Tekst w pigulce zawsze miesci sie w obrysie: rozmiar zmniejszany do dostepnej szerokosci.
void pillButton(Canvas& c, float x, float y, float w, float h, const char* label, Color fill, char key) {
    c.fillRoundRect(x, y, w, h, h * 0.5f, fill);
    float left = x + h * 0.5f, right = x + w - h * 0.5f;  // obszar bez zaokraglen
    if (key) {
        // srodek kola i litery wyrownany do srodka piksela — symetryczny antyaliasing
        float kr = floorf(h * 0.32f) + 0.5f;
        float kx = floorf(x + h * 0.5f + 1) + 0.5f, ky = floorf(y + h * 0.5f) + 0.5f;
        c.fillCircle(kx, ky, kr, rgb(0, 0, 0, 110));
        char k[2] = {key, 0};
        float ks = roundf(kr * 0.95f);
        drawText(c, kx, ky - ks * 0.5f, k, ks, 1.2f, pal::white, ALIGN_CENTER);
        left = kx + kr + 4;
    }
    float sz = h * 0.42f;
    float tw = textWidth(label, sz), avail = right - left;
    if (tw > avail) { sz *= avail / tw; tw = avail; }
    drawText(c, (left + right) * 0.5f, y + (h - sz) * 0.5f, label, sz, sz > 10 ? 1.6f : 1.2f, pal::white, ALIGN_CENTER);
}

void actionBar(Canvas& c, const char* const labels[4], const Color cols[4], const char keys[4], const char* status) {
    c.fillRect(0, 416, 640, 64, pal::bg);
    c.line(20, 418, 620, 418, 1.f, pal::faint);
    for (int i = 0; i < 4; i++) pillButton(c, 20 + i * 152, 427, 140, 26, labels[i], cols[i], keys[i]);
    if (status && *status) drawText(c, 320, 461, status, 7, 1.f, pal::dim, ALIGN_CENTER);
}

void valueBox(Canvas& c, float x, float y, float w, float h, const char* label, const char* value, const char* unit,
              Color col, bool active) {
    drawText(c, x, y - 20, label, 11, 1.4f, active ? pal::white : pal::dim);
    c.strokeRoundRect(x, y, w, h, 8, active ? 2.f : 1.4f, col);
    float sz = h * 0.55f;
    drawText(c, x + 14, y + (h - sz) * 0.5f, value, sz, 1.8f, active ? pal::white : col);
    if (unit && *unit) drawText(c, x + w + 8, y + 2, unit, 11, 1.4f, pal::dim);
}

void listBox(Canvas& c, float x, float y, float w, float h, const char* const* items, int n, int sel, int first,
             Color col, const bool* disabled) {
    c.line(x, y, x, y + h, 2.f, pal::steel);
    c.line(x + w, y, x + w, y + h, 2.f, rgb(70, 70, 70));
    const float rowH = 30, sz = 18;
    int rows = (int)(h / rowH);
    for (int r = 0; r < rows && first + r < n; r++) {
        int i = first + r;
        float ry = y + 6 + r * rowH;
        bool dis = disabled && disabled[i];
        Color tc = dis ? rgb(70, 60, 50) : col;
        if (i == sel) {
            c.fillRoundRect(x + 8, ry - 3, w - 16, sz + 6, 6, rgb(col.r, col.g, col.b, 40));
            tc = dis ? rgb(110, 90, 70) : rgb(245, 210, 160);
        }
        drawText(c, x + 18, ry, items[i], sz, 1.8f, tc);
    }
    // pasek przewijania
    if (n > rows) {
        float th = h * rows / n, ty = y + (h - th) * first / (float)(n - rows);
        c.line(x + w, ty, x + w, ty + th, 3.f, col);
    }
}

void screenHeader(Canvas& c, int icon, Color iconCol, const char* title, const char* sub, Color subCol, int bar,
                  int beat, bool playing, float bpm) {
    char b[32];
    drawIcon(c, (IconId)icon, 18, 12, 18, iconCol);
    drawText(c, 44, 15, title, 13, 1.5f, pal::white);
    if (sub && *sub) {  // podtytul przyciety tak, by nie wchodzil na BAR
        float sx = 44 + textWidth(title, 13) + 12;
        drawTextFit(c, sx, 17, sub, 10, 1.3f, subCol, 262 - sx);
    }
    snprintf(b, sizeof b, "BAR %d.%d", bar, beat);
    drawText(c, 320, 17, b, 11, 1.3f, playing ? pal::white : pal::dim, ALIGN_CENTER);
    drawIcon(c, playing ? ICON_PLAY : ICON_STOP, 418, 13, 16, playing ? pal::green : pal::dim);
    snprintf(b, sizeof b, "%.0f", bpm);
    drawText(c, 572, 9, b, 24, 1.9f, pal::red, ALIGN_RIGHT);
    drawText(c, 580, 22, "BPM", 10, 1.3f, pal::red);
    c.line(20, 44, 620, 44, 1.f, pal::faint);
}

bool drawTextFit(Canvas& c, float x, float y, const char* s, float size, float width, Color col, float maxW) {
    char buf[64];
    int n = 0;
    for (; s[n] && n < 63; n++) {
        buf[n] = s[n];
        buf[n + 1] = 0;
        if (textWidth(buf, size) > maxW) break;
    }
    buf[n] = 0;
    if (n == 0) return false;
    drawText(c, x, y, buf, size, width, col);
    return true;
}

void miniKnob(Canvas& c, float x, float y, float v, Color col, const char* label, const char* val) {
    const float a0 = 0.75f * 3.14159265f, a1 = 2.25f * 3.14159265f, r = 14;
    float cx = x + 16, cy = y + 18;
    v = v < 0 ? 0 : (v > 1 ? 1 : v);
    c.arc(cx, cy, r, a0, a1, 1.2f, pal::faint);
    float av = a0 + (a1 - a0) * v;
    if (v > 0.002f) c.arc(cx, cy, r, a0, av, 2.f, col);
    c.fillCircle(cx + cosf(av) * r, cy + sinf(av) * r, 3.f, col);
    drawText(c, x + 42, y + 2, label, 9, 1.1f, col);
    drawText(c, x + 42, y + 18, val, 16, 1.5f, pal::white);
}

void encoderBar(Canvas& c, const char* const labels[4], const char* const values[4], const float pos[4]) {
    c.line(20, 418, 620, 418, 1.f, pal::faint);
    for (int i = 0; i < 4; i++) miniKnob(c, 20 + i * 152, 428, pos[i], pal::enc[i], labels[i], values[i]);
}

static const char kChars[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-#";

void NameEdit::update(const Buttons& b) {
    if (!active_ || !buf_) return;
    int len = (int)strlen(buf_);
    if (pos_ >= len) {  // dopisz spacje az do kursora
        for (int i = len; i <= pos_ && i < MAX; i++) buf_[i] = ' ';
        buf_[pos_ + 1 <= MAX ? pos_ + 1 : MAX] = 0;
    }
    int ci = 0;
    const char* f = strchr(kChars, buf_[pos_]);
    if (f && buf_[pos_]) ci = (int)(f - kChars);
    int nc = (int)sizeof(kChars) - 1;
    if (b.repeat(B_UP)) buf_[pos_] = kChars[(ci + 1) % nc];
    if (b.repeat(B_DOWN)) buf_[pos_] = kChars[(ci + nc - 1) % nc];
    if (b.repeat(B_RIGHT) && pos_ < MAX - 1) pos_++;
    if (b.repeat(B_LEFT) && pos_ > 0) pos_--;
    if (b.pressed(B_A) || b.pressed(B_B) || b.pressed(B_X)) {
        active_ = false;
        int l = (int)strlen(buf_);  // usun koncowe spacje
        while (l > 0 && buf_[l - 1] == ' ') buf_[--l] = 0;
        if (l == 0) { buf_[0] = '-'; buf_[1] = 0; }
    }
}

void nameBox(Canvas& c, float x, float y, float w, float h, const char* label, const char* name, const NameEdit* ed,
             Color col) {
    bool act = ed && ed->active();
    drawText(c, x, y - 20, label, 11, 1.4f, act ? pal::white : pal::dim);
    c.strokeRoundRect(x, y, w, h, 8, act ? 2.f : 1.4f, col);
    // rozmiar dopasowany do pola (przy edycji liczymy pelna dlugosc MAX znakow)
    int n = act ? NameEdit::MAX : (int)strlen(name);
    if (n < 6) n = 6;
    float sz = (w - 26) * 12.f / (n * 12.f - 4.f);
    if (sz > h * 0.5f) sz = h * 0.5f;
    float tx = x + 13, ty = y + (h - sz) * 0.5f;
    if (act) {
        float cw = sz;  // krok znaku w foncie = 12 jednostek = sz
        float cx = tx + ed->pos() * cw;
        c.fillRoundRect(cx - 2, ty - 4, cw * 0.67f + 4, sz + 8, 3, rgb(col.r, col.g, col.b, 90));
    }
    drawText(c, tx, ty, name, sz, 1.8f, pal::white);
}

}  // namespace dp
