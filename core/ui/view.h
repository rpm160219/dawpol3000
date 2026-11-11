// Bazowa klasa ekranu. Ekran dostaje wspolny stan przyciskow (Buttons) od App,
// dzieki temu przelaczenie ekranu nie "przenosi" wcisniecia na nowy ekran.
#pragma once
#include "gfx/canvas.h"
#include "ui/buttons.h"
#include "ui/input.h"

namespace dp {

class App;

class View {
public:
    virtual ~View() {}
    virtual void update(const Buttons& b, const InputState& in, float dt) = 0;
    virtual void render(Canvas& c) = 0;
    void attach(App* a) { app_ = a; }

protected:
    App* app_ = nullptr;
};

}  // namespace dp
