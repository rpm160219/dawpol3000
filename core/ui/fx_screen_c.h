/* Ekrany efektow op1r (kod C oryginalu) — interfejs dla core/ui/fx_screen.cpp. */
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*FxLineFn)(void* user, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, const uint8_t* rgba);  /* Q10 */
typedef void (*FxArtFn)(void* user);
typedef void (*FxTextFn)(void* user, int32_t xq10, int32_t y, int32_t size, const char* s, const uint8_t* rgba);
void fxScreenSetup(void* user, FxLineFn line, FxArtFn art, FxTextFn text, const int16_t w[8], const uint8_t page[4]);
void fxScreenJuniChorus(const void* tel, int reset, int dt);  /* tel: 5 x uint32 (op1r_fx.h) */
void fxScreenSister(const void* tel, int reset, int dt);      /* tel: si_pos_t */
#ifdef __cplusplus
}
#endif
