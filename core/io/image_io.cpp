#include "io/image_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

namespace dp {

static uint32_t crcTable[256];
static void crcInit() {
    for (uint32_t n = 0; n < 256; n++) {
        uint32_t c = n;
        for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        crcTable[n] = c;
    }
}
static uint32_t crc(uint32_t c, const uint8_t* b, size_t n) {
    c ^= 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++) c = crcTable[(c ^ b[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}
static void be32(uint8_t* p, uint32_t v) { p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }

static void chunk(FILE* f, const char* type, const uint8_t* data, uint32_t len) {
    uint8_t h[8];
    be32(h, len);
    memcpy(h + 4, type, 4);
    fwrite(h, 1, 8, f);
    if (len) fwrite(data, 1, len, f);
    uint32_t c = crc(0, (const uint8_t*)type, 4);
    if (len) c = crc(c, data, len);
    uint8_t cb[4];
    be32(cb, c);
    fwrite(cb, 1, 4, f);
}

bool writePng(const char* path, const uint8_t* rgba, int w, int h) {
    crcInit();
    FILE* f = fopen(path, "wb");
    if (!f) return false;
    static const uint8_t sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    fwrite(sig, 1, 8, f);
    uint8_t ihdr[13];
    be32(ihdr, w); be32(ihdr + 4, h);
    ihdr[8] = 8; ihdr[9] = 2; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;  // RGB 8-bit
    chunk(f, "IHDR", ihdr, 13);

    size_t rawLen = (size_t)h * (w * 3 + 1);
    uint8_t* raw = (uint8_t*)malloc(rawLen);
    for (int y = 0; y < h; y++) {
        uint8_t* r = raw + y * (w * 3 + 1);
        r[0] = 0;
        for (int x = 0; x < w; x++) memcpy(r + 1 + x * 3, rgba + (y * w + x) * 4, 3);
    }
    size_t nblk = (rawLen + 65534) / 65535;
    size_t zLen = 2 + rawLen + nblk * 5 + 4;
    uint8_t* z = (uint8_t*)malloc(zLen);
    size_t o = 0;
    z[o++] = 0x78; z[o++] = 0x01;
    uint32_t a = 1, b = 0;
    for (size_t i = 0; i < rawLen; i++) { a = (a + raw[i]) % 65521; b = (b + a) % 65521; }
    for (size_t pos = 0; pos < rawLen;) {
        size_t n = rawLen - pos > 65535 ? 65535 : rawLen - pos;
        z[o++] = (pos + n == rawLen) ? 1 : 0;
        z[o++] = n & 0xFF; z[o++] = n >> 8;
        z[o++] = ~n & 0xFF; z[o++] = (~n >> 8) & 0xFF;
        memcpy(z + o, raw + pos, n);
        o += n; pos += n;
    }
    be32(z + o, (b << 16) | a);
    o += 4;
    chunk(f, "IDAT", z, (uint32_t)o);
    chunk(f, "IEND", nullptr, 0);
    free(raw); free(z);
    fclose(f);
    return true;
}

static void le16(FILE* f, uint16_t v) { fputc(v & 0xFF, f); fputc(v >> 8, f); }
static void le32(FILE* f, uint32_t v) { le16(f, v & 0xFFFF); le16(f, v >> 16); }

bool writeWav16(const char* path, const float* s, int frames, int sr) {
    FILE* f = fopen(path, "wb");
    if (!f) return false;
    uint32_t data = frames * 4;
    fwrite("RIFF", 1, 4, f); le32(f, 36 + data); fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f); le32(f, 16); le16(f, 1); le16(f, 2); le32(f, sr); le32(f, sr * 4);
    le16(f, 4); le16(f, 16);
    fwrite("data", 1, 4, f); le32(f, data);
    for (int i = 0; i < frames * 2; i++) {
        float v = s[i];
        if (v > 1.f) v = 1.f;
        if (v < -1.f) v = -1.f;
        le16(f, (uint16_t)(int16_t)lrintf(v * 32767.f));
    }
    fclose(f);
    return true;
}

}  // namespace dp
