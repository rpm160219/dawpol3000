/* Ekrany efektow op1r (kod C oryginalu) — interfejs dla core/ui/fx_screen.cpp. */
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*FxLineFn)(void* user, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, const uint8_t* rgba);  /* Q10 */
typedef void (*FxArtFn)(void* user);
typedef void (*FxTextFn)(void* user, int32_t xq10, int32_t y, int32_t size, const char* s, const uint8_t* rgba);
/* wezly SVG (elementy z id) zmieniane przez kod ekranu */
#define FX_MAX_NODES 64
typedef struct { char id[40]; int32_t m[9]; int hasM, visible, colorSet; uint8_t rgba[4]; } FxNode;
typedef void (*FxNodeColorFn)(void* user, const char* id, uint8_t* rgba);  /* kolor poczatkowy z rysunku */
void fxNodesReset(void);
FxNode* fxNodeFind(const char* id);
void fxScreenSetColorFn(FxNodeColorFn fn);
void fxScreenSetup(void* user, FxLineFn line, FxArtFn art, FxTextFn text, const int16_t w[8], const uint8_t page[4]);
void fxScreenJuniChorus(const void* tel, int reset, int dt);  /* tel: 5 x uint32 (op1r_fx.h) */
void fxScreenSister(const void* tel, int reset, int dt);      /* tel: si_pos_t */
/* tel: struktury Tel wrapperow (core/fx/fx_*.c) */
void fxScreenTape(const void* tel, int reset, int dt);
void fxScreenStutter(const void* tel, int reset, int dt);
void fxScreenEnte(const void* tel, int reset, int dt);
void fxScreenFather(const void* tel, int reset, int dt);
void fxScreenRackverb(const void* tel, int reset, int dt);
void fxScreenCrush(const void* tel, int reset, int dt);
void fxScreenSnowglobe(const void* tel, int reset, int dt);
#ifdef __cplusplus
}
#endif
