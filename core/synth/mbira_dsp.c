/* Opakowanie mbira.c (C, bez zmian). Stan silnika: hot_t pod wskaznikiem mb_home_at (oryginal wylicza go raz dla
 * swojej puli) i struktura mb_g (tablica sinusa + wartosci dla ekranu). Kazda instancja ma wlasne; przed wywolaniem
 * wstawiamy je, po wywolaniu zabieramy. Pamiec podrecznych wspolczynnikow (coeff_cache) jest wspolna — jak miedzy
 * glosami w oryginale: wartosci zaleza tylko od kluczy. Wywolania z jednego watku audio. */
#include "synth/mbira_dsp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

const int32_t* mbw_pitch_ptr = 0;
int16_t mbw_vel[6];
int16_t mbw_sine[8192];
uint8_t mbw_eng_page[4];

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"  /* noise_in: tylko sciezka Blackfin */
#include "../../third_party/op1r_mbira/mbira.c"
#pragma GCC diagnostic pop

struct MbiraCore {
    hot_t hot;  /* wyrownane do 32 (jak w oryginale) */
    __typeof__(mb_g) g;
};

static void enter(struct MbiraCore* p) {
    mb_home_at = &p->hot;
    memcpy((void*)&mb_g, (const void*)&p->g, sizeof mb_g);
}
static void leave(struct MbiraCore* p) {
    memcpy((void*)&p->g, (const void*)&mb_g, sizeof mb_g);
    mb_home_at = 0;
}

struct MbiraCore* mbiraCreate(void) {
    if (!mbw_sine[2048])
        for (int i = 0; i < 8192; i++) mbw_sine[i] = (int16_t)lrint(32767.0 * sin(2 * M_PI * i / 8192.0));
    const size_t sz = (sizeof(struct MbiraCore) + 31) & ~(size_t)31;
    struct MbiraCore* p = (struct MbiraCore*)aligned_alloc(32, sz);
    memset(p, 0, sz);  /* jak statyczna pula oryginalu */
    return p;
}

void mbiraNoteOn(struct MbiraCore* p, int voice, const int16_t* knobs, int vel) {
    enter(p);
    mbw_vel[voice] = (int16_t)(vel << 8);
    ENGINE_mbira.note_on((void*)(uintptr_t)voice, knobs);
    leave(p);
}

void mbiraRender(struct MbiraCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel) {
    enter(p);
    mbw_pitch_ptr = (const int32_t*)pitch;
    mbw_vel[voice] = (int16_t)(vel << 8);
    ENGINE_mbira.render((void*)(uintptr_t)voice, out, knobs);
    leave(p);
}

void mbiraView(const struct MbiraCore* p, struct MbiraView* v) {
    v->blocks = p->g.blocks;
    for (int i = 0; i < 6; i++) v->level[i] = p->g.level[i];
}
