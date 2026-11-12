// Bufor 640x480 RGBA + rasteryzer z antyaliasingiem (linie, luki, kola).
#pragma once
#include <stdint.h>

namespace dp {

struct Color { uint8_t r, g, b, a; };
constexpr Color rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) { return {r, g, b, a}; }

namespace pal {
// 4 kolory = 4 enkodery
constexpr Color bg    = rgb(0, 0, 0);
constexpr Color blue  = rgb(64, 128, 255);
constexpr Color green = rgb(40, 215, 110);
constexpr Color white = rgb(235, 235, 235);
constexpr Color red   = rgb(255, 62, 48);
constexpr Color enc[4] = {blue, green, white, red};
// Szarosci
constexpr Color dim   = rgb(95, 95, 95);
constexpr Color faint = rgb(38, 38, 38);
constexpr Color dark  = rgb(18, 18, 18);
// Elementy menu (belki, pigulki, listy)
constexpr Color steel  = rgb(58, 86, 126);   // niebieskoszary
constexpr Color tan    = rgb(170, 124, 72);  // brazowo-zloty
constexpr Color silver = rgb(170, 170, 170); // jasnoszara belka
constexpr Color pill   = rgb(120, 120, 120); // szara pigulka
// Kolory sciezek
constexpr Color purple = rgb(150, 95, 255);
constexpr Color cyan   = rgb(40, 205, 230);
constexpr Color yellow = rgb(240, 196, 60);
constexpr Color pink   = rgb(255, 95, 170);
// 32 kolory sciezek; pierwsze 8 bez zmian (zgodnosc ze starymi projektami)
constexpr int TRACK_COLORS = 32;
constexpr Color track[TRACK_COLORS] = {
    blue, green, red, tan, purple, cyan, yellow, pink,
    rgb(255, 140, 40),  rgb(170, 230, 50),  rgb(0, 180, 160),   rgb(100, 100, 235),  // pomarancz, limonka, morski, indygo
    rgb(230, 60, 200),  rgb(120, 240, 190), rgb(250, 128, 114), rgb(180, 160, 255),  // magenta, mieta, losos, lawenda
    rgb(110, 190, 255), rgb(230, 180, 30),  rgb(150, 160, 60),  rgb(240, 110, 140),  // niebo, zloto, oliwka, roz
    rgb(255, 110, 80),  rgb(60, 230, 230),  rgb(190, 80, 255),  rgb(255, 190, 140),  // koral, akwa, fiolet, brzoskwinia
    rgb(50, 190, 110),  rgb(100, 140, 190), rgb(220, 30, 70),   rgb(255, 170, 0),    // zielen morska, stal, karmazyn, bursztyn
    rgb(210, 255, 60),  rgb(30, 210, 200),  rgb(220, 120, 220), rgb(210, 190, 140),  // seledyn, turkus, orchidea, piasek
};
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
    void copyRows(const Canvas& src, int y0, int y1);  // kopia pasa wierszy (cache statycznej grafiki)
    void fillRect(float x, float y, float w, float h, Color c);
    void fillCircle(float cx, float cy, float r, Color c);
    void fillPolygon(const float* xy, int n, Color c);  // regula even-odd, antyaliasing
    void rect(float x, float y, float w, float h, float lw, Color c);  // obrys
    // Zaokraglony prostokat (r = promien naroznika; r = h/2 daje "pigulke").
    void fillRoundRect(float x, float y, float w, float h, float r, Color c) { roundRect(x, y, w, h, r, 0, c); }
    void strokeRoundRect(float x, float y, float w, float h, float r, float lw, Color c) { roundRect(x, y, w, h, r, lw, c); }

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
    void roundRect(float x, float y, float w, float h, float r, float lw, Color c);  // lw <= 0: wypelnienie
};

}  // namespace dp
