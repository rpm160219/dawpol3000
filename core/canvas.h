// Bufor 640x480 RGBA + rasteryzer z antyaliasingiem (linie, luki, kola).
#pragma once
#include <stdint.h>

namespace dp {

struct Color { uint8_t r, g, b, a; };
constexpr Color rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) { return {r, g, b, a}; }

namespace pal {
constexpr Color bg    = rgb(0, 0, 0);
constexpr Color blue  = rgb(70, 140, 255);
constexpr Color green = rgb(60, 220, 120);
constexpr Color white = rgb(236, 236, 236);
constexpr Color red   = rgb(255, 72, 60);
constexpr Color dim   = rgb(90, 90, 90);
constexpr Color faint = rgb(38, 38, 38);
constexpr Color enc[4] = {blue, green, white, red};
}  // namespace pal

class Canvas {
public:
    static const int W = 640, H = 480;
    Canvas();
    ~Canvas();
    // Bajty w pamieci: R,G,B,A (SDL_PIXELFORMAT_ABGR8888 na little-endian).
    uint8_t* pixels() { return px_; }
    const uint8_t* pixels() const { return px_; }

    void clear(Color c);
    void fillRect(float x, float y, float w, float h, Color c);
    void fillCircle(float cx, float cy, float r, Color c);

    void line(float x0, float y0, float x1, float y1, float w, Color c);
    void polyline(const float* xy, int n, float w, Color c, bool closed = false);
    // Katy w radianach, 0 = w prawo, rosnaco zgodnie z ruchem wskazowek (os Y w dol).
    void arc(float cx, float cy, float r, float a0, float a1, float w, Color c);
    void circle(float cx, float cy, float r, float w, Color c) { arc(cx, cy, r, 0.f, 6.2831853f, w, c); }

    // Wsadowe rysowanie kreski: nakladajace sie segmenty nie mieszaja sie podwojnie.
    void beginStroke();
    void strokeSeg(float x0, float y0, float x1, float y1, float w);
    void endStroke(Color c);

private:
    uint8_t* px_;
    float* cov_;
    int dx0_, dy0_, dx1_, dy1_;
    void blend(int idx, Color c, float a);
};

}  // namespace dp
