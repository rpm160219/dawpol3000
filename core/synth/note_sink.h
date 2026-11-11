// Wspolny interfejs "cos, co gra nuty" (silnik/instrument). Wywolywany w watku audio.
#pragma once

namespace dp {

class NoteSink {
public:
    virtual ~NoteSink() {}
    virtual void noteOn(int pitch, float vel) = 0;  // vel 0..1
    virtual void noteOff(int pitch) = 0;
    virtual void allOff() = 0;
};

}  // namespace dp
