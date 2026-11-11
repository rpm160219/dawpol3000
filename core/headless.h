// Tryb bez okna (testy, makiety dla Patryka): zrzut ekranu do PNG, dzwiek do WAV.
#pragma once
namespace dp {
int headlessPng(const char* path, const char* screen);  // screen: "roll" | "test"
int headlessWav(const char* path, float seconds);       // demo pattern z piano rolla
}
