// Logiczne przyciski i osie (niezalezne od platformy).
#pragma once
#include <string.h>

namespace dp {

enum Btn {
    B_UP, B_DOWN, B_LEFT, B_RIGHT, B_A, B_B, B_X, B_Y,
    B_L1, B_R1, B_L2, B_R2, B_SELECT, B_START, B_MENU,
    B_L3, B_R3,  // klikniecie lewej / prawej galki analogowej
    BTN_COUNT
};
enum Axis { AX_LX, AX_LY, AX_RX, AX_RY, AXIS_COUNT };

struct InputState {
    bool btn[BTN_COUNT] = {};
    float axis[AXIS_COUNT] = {};  // -1..1, Y w dol dodatnie
    static const int RAW_N = 4;
    char raw[RAW_N][48] = {};     // ostatnie surowe zdarzenia (do mapowania na konsoli)
    char pad[64] = "NO PAD";
    void logRaw(const char* s) {
        for (int i = RAW_N - 1; i > 0; i--) memcpy(raw[i], raw[i - 1], sizeof(raw[0]));
        strncpy(raw[0], s, sizeof(raw[0]) - 1);
        raw[0][sizeof(raw[0]) - 1] = 0;
    }
};

}  // namespace dp
