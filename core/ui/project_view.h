// Ekran PROJECT (pierwsza zakladka, przed ARRANGE): zapis, wczytywanie, nowy projekt, usuwanie.
//
// Sterowanie:
//   gora/dol   wybor z listy (pierwsza pozycja "+ NEW" = nowy pusty projekt)
//   X          zmiana nazwy biezacego projektu
//   Y          zapisz biezacy projekt (pod jego nazwa)
//   A          wczytaj wybrany / utworz nowy (przy niezapisanych zmianach — potwierdzenie)
//   B          usun wybrany projekt z karty (dwa razy)
//   Start      play / stop
//   R2         ARRANGE
#pragma once
#include "ui/view.h"
#include "ui/widgets.h"

namespace dp {

class ProjectView : public View {
public:
    void onShow();  // odswieza liste projektow
    void update(const Buttons& b, const InputState& in, float dt) override;
    void render(Canvas& c) override;
    void message(const char* m, bool error = false);

private:
    int sel_ = 0, first_ = 0;
    int confirm_ = 0;  // 0 brak, 1 = wczytanie/nowy, 2 = usuniecie, 3 = nadpisanie
    char msg_[64] = "";
    bool msgErr_ = false;
    float msgT_ = 0;
    NameEdit ed_;
};

}  // namespace dp
