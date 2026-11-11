// Wspolne elementy UI w stylu menu: belki naglowkow, pigulki akcji, pola wartosci, listy, galki.
#pragma once
#include "gfx/canvas.h"
#include "ui/buttons.h"

namespace dp {

// Pelna zaokraglona belka z ciemnym napisem (naglowek okna/panelu).
void headerBar(Canvas& c, float x, float y, float w, float h, const char* text, Color fill);
// Pigulka akcji na dole ekranu; key = litera przycisku (np. 'A') albo 0.
void pillButton(Canvas& c, float x, float y, float w, float h, const char* label, Color fill, char key);
// Obrysowane pole z duza wartoscia, podpisem nad i jednostka obok.
void valueBox(Canvas& c, float x, float y, float w, float h, const char* label, const char* value, const char* unit,
              Color col, bool active);
// Lista z duzym tekstem; sel = zaznaczony, first = pierwszy widoczny, rows = ile miesci sie w h.
void listBox(Canvas& c, float x, float y, float w, float h, const char* const* items, int n, int sel, int first,
             Color col, const bool* disabled = nullptr);
// Wspolny naglowek ekranow ARRANGE / PIANO ROLL (ten sam uklad i pozycje):
// ikona + tytul (+ podtytul w kolorze), pozycja "BAR t.m" na srodku, play/stop, BPM po prawej.
void screenHeader(Canvas& c, int icon, Color iconCol, const char* title, const char* sub, Color subCol, int bar,
                  int beat, bool playing, float bpm);
// Tekst przyciety do szerokosci maxW (zwraca false, gdy nie miesci sie nawet 1 znak).
bool drawTextFit(Canvas& c, float x, float y, const char* s, float size, float width, Color col, float maxW);
// Mala galka enkodera z podpisem i wartoscia (pasek na dole ekranu).
void miniKnob(Canvas& c, float x, float y, float v, Color col, const char* label, const char* val);
// Pasek 4 enkoderow na dole ekranu.
void encoderBar(Canvas& c, const char* const labels[4], const char* const values[4], const float pos[4]);

// Edycja nazwy przyciskami: gora/dol = znak, lewo/prawo = pozycja, A/B/X = koniec.
class NameEdit {
public:
    static const int MAX = 11;
    void begin(char* target) { buf_ = target; pos_ = 0; active_ = true; }
    bool active() const { return active_; }
    int pos() const { return pos_; }
    void update(const Buttons& b);

private:
    char* buf_ = nullptr;
    int pos_ = 0;
    bool active_ = false;
};
// Rysuje nazwe w polu; przy edycji podswietla biezacy znak.
void nameBox(Canvas& c, float x, float y, float w, float h, const char* label, const char* name, const NameEdit* ed,
             Color col);

}  // namespace dp
