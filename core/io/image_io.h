// Zapis PNG (bez zlib — bloki "stored") i WAV 16-bit. Do trybu headless.
#pragma once
#include <stdint.h>

namespace dp {
bool writePng(const char* path, const uint8_t* rgba, int w, int h);
bool writeWav16(const char* path, const float* stereo, int frames, int sampleRate);
}
