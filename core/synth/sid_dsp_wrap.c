/* Opakowanie sid.c (C, bez zmian; sid_dsp.c kompilowany osobno). Stan: tablica glosow sid_state voices[6] w zmiennej
 * pliku; kazda instancja ma wlasna, przed wywolaniem wstawiamy stan tego glosu (render i note_on dotykaja tylko
 * swojego), po wywolaniu zabieramy. Wywolania z jednego watku audio. */
#include "synth/sid_dsp_wrap.h"
#include <stdlib.h>
#include <string.h>

const int32_t* sidw_pitch_ptr = 0;
int sidw_vel = 100;
int16_t sidw_sine[8192];

#include "../../third_party/op1r_sid/sid.c"

struct SidCore { sid_state v[OP1_VOICES]; };

struct SidCore* sidCreate(void) { return (struct SidCore*)calloc(1, sizeof(struct SidCore)); }

void sidNoteOn(struct SidCore* p, int voice, const int16_t* knobs) {
    voices[voice] = p->v[voice];
    ENGINE_sid.note_on((void*)(uintptr_t)voice, knobs);
    p->v[voice] = voices[voice];
}

void sidRender(struct SidCore* p, int voice, int16_t* out, const int16_t* knobs, const uint32_t* pitch, int vel) {
    voices[voice] = p->v[voice];
    sidw_pitch_ptr = (const int32_t*)pitch;
    sidw_vel = vel;
    ENGINE_sid.render((void*)(uintptr_t)voice, out, knobs);
    p->v[voice] = voices[voice];
}

unsigned sidWaveMode(int16_t knob0) { return sid_wave_mode(knob0); }
unsigned sidIntervalSemi(int knob1) { return sid_interval_semi(knob1); }
