// JEDNA tabela mapowania sterowania. Zmieniac tu po testach na konsoli.
#pragma once
#include <SDL.h>
#include "../core/input.h"

namespace dp {

// Klawiatura (desktop).
struct KeyMap { SDL_Keycode key; int btn; };
static const KeyMap kKeyMap[] = {
    {SDLK_UP, B_UP}, {SDLK_DOWN, B_DOWN}, {SDLK_LEFT, B_LEFT}, {SDLK_RIGHT, B_RIGHT},
    {SDLK_x, B_A}, {SDLK_z, B_B}, {SDLK_s, B_X}, {SDLK_a, B_Y},
    {SDLK_q, B_L1}, {SDLK_w, B_R1}, {SDLK_1, B_L2}, {SDLK_2, B_R2},
    {SDLK_RSHIFT, B_SELECT}, {SDLK_RETURN, B_START}, {SDLK_TAB, B_MENU},
};

// Klawiatura udajaca analogi: {klawisz, os, kierunek}.
struct KeyAxis { SDL_Keycode key; int axis; float dir; };
static const KeyAxis kKeyAxis[] = {
    {SDLK_f, AX_LX, -1}, {SDLK_h, AX_LX, 1}, {SDLK_t, AX_LY, -1}, {SDLK_g, AX_LY, 1},
    {SDLK_j, AX_RX, -1}, {SDLK_l, AX_RX, 1}, {SDLK_i, AX_RY, -1}, {SDLK_k, AX_RY, 1},
};

// Pad rozpoznany przez SDL jako GameController (uklad Nintendo: A po prawej, B na dole).
// SDL nazywa przyciski pozycjami Xboksa: SOUTH=A(SDL), EAST=B(SDL).
struct PadMap { SDL_GameControllerButton b; int btn; };
static const PadMap kPadMap[] = {
    {SDL_CONTROLLER_BUTTON_DPAD_UP, B_UP}, {SDL_CONTROLLER_BUTTON_DPAD_DOWN, B_DOWN},
    {SDL_CONTROLLER_BUTTON_DPAD_LEFT, B_LEFT}, {SDL_CONTROLLER_BUTTON_DPAD_RIGHT, B_RIGHT},
    {SDL_CONTROLLER_BUTTON_B, B_A}, {SDL_CONTROLLER_BUTTON_A, B_B},
    {SDL_CONTROLLER_BUTTON_Y, B_X}, {SDL_CONTROLLER_BUTTON_X, B_Y},
    {SDL_CONTROLLER_BUTTON_LEFTSHOULDER, B_L1}, {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, B_R1},
    {SDL_CONTROLLER_BUTTON_BACK, B_SELECT}, {SDL_CONTROLLER_BUTTON_START, B_START},
    {SDL_CONTROLLER_BUTTON_GUIDE, B_MENU},
};
// L2/R2 jako osie triggerow (prog 0.5).
static const SDL_GameControllerAxis kPadL2 = SDL_CONTROLLER_AXIS_TRIGGERLEFT;
static const SDL_GameControllerAxis kPadR2 = SDL_CONTROLLER_AXIS_TRIGGERRIGHT;

// Surowy joystick RG35XX Pro (gdy SDL nie rozpozna go jako GameController).
// DO UZUPELNIENIA po tescie na konsoli — numery odczytamy z ekranu testowego (RAW).
struct RawBtnMap { int joyButton; int btn; };
static const RawBtnMap kRawBtnMap[] = {
    {-1, -1},
};
struct RawAxisMap { int joyAxis; int axis; float scale; };
static const RawAxisMap kRawAxisMap[] = {
    {0, AX_LX, 1.f}, {1, AX_LY, 1.f}, {3, AX_RX, 1.f}, {4, AX_RY, 1.f},
};

}  // namespace dp
