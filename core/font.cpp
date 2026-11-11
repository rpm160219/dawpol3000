// Glify na siatce 8 x 12 (x: 0..8, y: 0..12, y w dol). Kazda kreska to ciag punktow
// zapisanych jako pary znakow (0-9, a-f = 10-15), kreski oddzielone spacja.
// Prefiks '~' = kreska wygladzana (Chaikin), domknieta gdy pierwszy punkt == ostatni.
#include "font.h"
#include <ctype.h>
#include <string.h>

namespace dp {

static const char* glyph(char ch) {
    switch (ch) {
    case '0': return "~206073796c2c190320";
    case '1': return "22404c";
    case '2': return "~03113050718376 760c8c";
    case '3': return "~02206082846636 ~66888a6c2c0a";
    case '4': return "6c600888";
    case '5': return "801005 ~053464868a6c2c0a";
    case '6': return "~7030040a2c6c8a87652507";
    case '7': return "00803c";
    case '8': return "~206082846626040220 ~2666888a6c2c0a0826";
    case '9': return "~1c5c888260200205276785";
    case 'A': return "0c408c 2868";
    case 'B': return "000c ~005071745606 ~0656888a6c0c";
    case 'C': return "~82602003092c6c8a";
    case 'D': return "000c ~004073794c0c";
    case 'E': return "80000c8c 0656";
    case 'F': return "80000c 0656";
    case 'G': return "~82602003092c6c8a87 8747";
    case 'H': return "000c 808c 0686";
    case 'I': return "404c 2060 2c6c";
    case 'J': return "8089 ~898b6c2c0a";
    case 'K': return "000c 8007 358c";
    case 'L': return "000c8c";
    case 'M': return "0c0046808c";
    case 'N': return "0c008c80";
    case 'O': return "~407184887b4c1b08040140";
    case 'P': return "0c00 ~005082845606";
    case 'Q': return "~206083896c2c090320 5a8c";
    case 'R': return "0c00 ~005082845606 468c";
    case 'S': return "~82602002042666888a6c2c0a";
    case 'T': return "0080 404c";
    case 'U': return "~00092c6c8980";
    case 'V': return "004c80";
    case 'W': return "002c466c80";
    case 'X': return "008c 800c";
    case 'Y': return "004680 464c";
    case 'Z': return "00800c8c";
    case '-': return "1676";
    case '+': return "2666 4448";
    case '.': return "4b4c";
    case ',': return "4b3d";
    case ':': return "4344 4a4b";
    case '/': return "0c80";
    case '(': return "~50222a5c";
    case ')': return "~30626a3c";
    case '[': return "60202c6c";
    case ']': return "20606c2c";
    case '<': return "70067c";
    case '>': return "10861c";
    case '=': return "1474 1878";
    case '_': return "0c8c";
    case '\'': return "4043";
    case '!': return "4048 4b4c";
    case '?': return "~02206082846447 4b4c";
    case '%': return "0c80 ~1021120110 ~7a8b7c6b7a";
    case '*': return "4248 2165 2561";
    default: return nullptr;
    }
}

static inline int hexv(char c) { return c <= '9' ? c - '0' : c - 'a' + 10; }

static const int MAXP = 256;

// Wygladzanie Chaikina (2 iteracje).
static int chaikin(const float* in, int n, float* out, bool closed) {
    float a[MAXP * 2], b[MAXP * 2];
    memcpy(a, in, sizeof(float) * 2 * n);
    int m = n;
    for (int it = 0; it < 2; it++) {
        int k = 0;
        int segs = closed ? m - 1 : m - 1;  // przy domknietej ostatni punkt == pierwszy
        if (!closed) { b[k * 2] = a[0]; b[k * 2 + 1] = a[1]; k++; }
        for (int i = 0; i < segs && k + 2 < MAXP; i++) {
            float x0 = a[i * 2], y0 = a[i * 2 + 1], x1 = a[i * 2 + 2], y1 = a[i * 2 + 3];
            bool first = !closed && i == 0, last = !closed && i == segs - 1;
            if (!first) { b[k * 2] = 0.75f * x0 + 0.25f * x1; b[k * 2 + 1] = 0.75f * y0 + 0.25f * y1; k++; }
            if (!last) { b[k * 2] = 0.25f * x0 + 0.75f * x1; b[k * 2 + 1] = 0.25f * y0 + 0.75f * y1; k++; }
        }
        if (closed) { b[k * 2] = b[0]; b[k * 2 + 1] = b[1]; k++; }
        else { b[k * 2] = a[(m - 1) * 2]; b[k * 2 + 1] = a[(m - 1) * 2 + 1]; k++; }
        memcpy(a, b, sizeof(float) * 2 * k);
        m = k;
    }
    memcpy(out, a, sizeof(float) * 2 * m);
    return m;
}

static const float ADV = 12.f;  // szerokosc znaku 8 + odstep 4

// Znaki waskie (kropka, przecinek...): mniejszy krok, glif przesuniety w lewo.
static bool narrow(char c) { return c == '.' || c == ',' || c == ':' || c == '\'' || c == '!'; }
static float advOf(char c) { return narrow(c) ? 6.f : ADV; }

float textWidth(const char* s, float size) {
    float w = 0;
    for (const char* p = s; *p; p++) w += advOf(*p);
    if (w > 0) w -= 4.f;
    return w * size / 12.f;
}

float drawText(Canvas& c, float x, float y, const char* s, float size, float width, Color col, int align) {
    float w = textWidth(s, size);
    if (align == ALIGN_CENTER) x -= w * 0.5f;
    else if (align == ALIGN_RIGHT) x -= w;
    float u = size / 12.f;
    c.beginStroke();
    for (const char* p = s; *p; x += advOf(*p) * u, p++) {
        const char* g = glyph((char)toupper((unsigned char)*p));
        if (!g) continue;
        float gx = narrow(*p) ? x - 3.f * u : x;
        while (*g) {
            while (*g == ' ') g++;
            if (!*g) break;
            bool smooth = false;
            if (*g == '~') { smooth = true; g++; }
            float pts[MAXP * 2];
            int n = 0;
            while (g[0] && g[0] != ' ' && g[1] && g[1] != ' ' && n < 64) {
                pts[n * 2] = (float)hexv(g[0]);
                pts[n * 2 + 1] = (float)hexv(g[1]);
                n++;
                g += 2;
            }
            while (*g && *g != ' ') g++;  // smieci w opisie glifu
            if (n == 0) continue;
            if (smooth && n > 2) {
                bool closed = pts[0] == pts[n * 2 - 2] && pts[1] == pts[n * 2 - 1];
                float sm[MAXP * 2];
                n = chaikin(pts, n, sm, closed);
                memcpy(pts, sm, sizeof(float) * 2 * n);
            }
            if (n == 1) { c.strokeSeg(gx + pts[0] * u, y + pts[1] * u, gx + pts[0] * u, y + pts[1] * u, width); continue; }
            for (int i = 0; i + 1 < n; i++)
                c.strokeSeg(gx + pts[i * 2] * u, y + pts[i * 2 + 1] * u, gx + pts[i * 2 + 2] * u,
                            y + pts[i * 2 + 3] * u, width);
        }
    }
    c.endStroke(col);
    return w;
}

}  // namespace dp
