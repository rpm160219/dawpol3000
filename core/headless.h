// Tryb bez okna (testy, makiety dla Patryka): zrzut ekranu do PNG, dzwiek do WAV.
#pragma once
namespace dp {
// screen: taffy | taffypresets | taffyengine | newengine | engine | project | synthmenv | presets | voice | synth | synthenv | synthfenv | synthlfo | synthext | synthfilt | select | transform | tools | rolltools | arrange | track | trackedit | pattern | roll | test
int headlessPng(const char* path, const char* screen);
int headlessWav(const char* path, float seconds);  // demo projekt, cala aranzacja
}
