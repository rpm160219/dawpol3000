/* Opakowanie vapora.c (C, bez zmian; vapora_dsp.c kompilowany osobno). Stan oryginalu w zmiennych pliku: voices[6],
 * licznik nut serial (ziarno fazy), ticks[6], skrzynki ekranu ps_mail[6], ps_held_serial[6]. Kazda instancja
 * (sciezka) ma wlasne kopie; przed wywolaniem wstawiamy stan tego glosu, po wywolaniu zabieramy. Skrzynki i numery
 * trzymanych nut po kazdym wywolaniu kopiowane do instancji — czyta je ekran (watek UI; wyscig dotyczy tylko obrazu). */
#include "synth/vapora_dsp.h"
#include <stdlib.h>
#include <string.h>

const int32_t* vpw_pitch_ptr = 0;
int vpw_vel = 100;
const int16_t vpw_env[128] = {0};  /* obwiednia OP-1 tylko do migawek ekranu (poziom/slad — ekran ich nie uzywa) */

#define ps_mail vpw_mail_tmp
#define ps_held_serial vpw_held_tmp
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "../../third_party/op1r_vapora/vapora.c"
#pragma GCC diagnostic pop

struct VaporaCore {
    ps_voice v[PS_VOICES];
    uint32_t serial, ticks[PS_VOICES];
    ps_mailbox mail[PS_VOICES];
    volatile uint32_t held[PS_VOICES];
};

static void enter(struct VaporaCore* p, int vi) { voices[vi] = p->v[vi]; serial = p->serial; ticks[vi] = p->ticks[vi]; }
static void leave(struct VaporaCore* p, int vi) {
    p->v[vi] = voices[vi]; p->serial = serial; p->ticks[vi] = ticks[vi];
    memcpy((void*)&p->mail[vi], (const void*)&vpw_mail_tmp[vi], sizeof(ps_mailbox));
    p->held[vi] = vpw_held_tmp[vi];
}

struct VaporaCore* vaporaCreate(void) {
    struct VaporaCore* p;
    if (posix_memalign((void**)&p, 32, sizeof *p)) return 0;
    memset(p, 0, sizeof *p);  /* jak statyczne zmienne oryginalu */
    return p;
}

void vaporaNoteOn(struct VaporaCore* p, int voice, const int16_t* knobs) {
    enter(p, voice);
    memcpy((void*)&vpw_mail_tmp[voice], (const void*)&p->mail[voice], sizeof(ps_mailbox));
    ENGINE_vapora.note_on((void*)(uintptr_t)voice, knobs);
    leave(p, voice);
}

void vaporaNoteOff(struct VaporaCore* p, int voice) {
    if (!p->held[voice]) return;
    enter(p, voice);
    vpw_held_tmp[voice] = p->held[voice];
    ENGINE_vapora.note_off((void*)(uintptr_t)voice);
    p->held[voice] = vpw_held_tmp[voice];
}

void vaporaRender(struct VaporaCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel) {
    enter(p, voice);
    memcpy((void*)&vpw_mail_tmp[voice], (const void*)&p->mail[voice], sizeof(ps_mailbox));
    vpw_held_tmp[voice] = p->held[voice];
    vpw_pitch_ptr = (const int32_t*)pitch;
    vpw_vel = vel;
    ENGINE_vapora.render((void*)(uintptr_t)voice, out, knobs);
    leave(p, voice);
}

const void* vaporaMail(const struct VaporaCore* p) { return p->mail; }
const volatile uint32_t* vaporaHeld(const struct VaporaCore* p) { return p->held; }
