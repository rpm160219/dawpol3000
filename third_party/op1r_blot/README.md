# op1r BLOT (pcppcp) — port do DAW-POL 3000

Źródło: https://github.com/pcppcp/op1r-src — `synth_sdk/plugins/blot`, commit `fc14074`.
Autor: pcppcp; filtr wg trybu TB-303 z TeeBeeFilter projektu Open303 (Robin Schmidt, licencja MIT, (c) 2009 rs-met.com).
Licencja: **GPL-3.0-or-later** z dodatkowym pozwoleniem dot. firmware OP-1 (`LICENSE`, `NOTICE` — pliki z katalogu głównego
op1r-src).

Bez zmian: `blot.c` (DSP w stylu TB-303 + ekran kwasowego smileya), `blot_tables.h`, `blot.svg` (pusty), `gen_tables.py`,
`plugin.json`. Zaślepki: `op1_plugin.h`, `op1_synth.h`. Odrzucone: `blot_dsp.S` (oscylator na Blackfin; pętla C daje te
same próbki).

W DAW-POL: `core/synth/blot_dsp.c` kompiluje `blot.c` jako C; stan głosu = slot 0x24 B w instancji (self). Silnik 1:1:
gałki CUTOFF, RESONANCE, ENV MOD, DECAY; L3 na SOUND = strona Shift oryginału (WAVE, ACCENT, DRIVE); akcent przy
velocity ≥ 100. Slide w oryginale to portamento OP-1 (tryb mono/legato), więc strona VOICE ma MODE i GLIDE; poza tym
tylko AMP ENV (bez MOD ENV / LFO). Wyjście ścieżki +6 dB po decymacji (oryginał ×0,6, filtr szybko się zamyka). Ekran:
napisy i kreski dolnego rzędu oryginału pominięte (dublowałyby gałki). Oryginał nie ma presetów — 14 własnych.
Test bit w bit: `tools/test_blot.sh`. Częstotliwość: przy 96 kHz filtr (współczynniki z tablic dla 88,2 kHz) ~9% wyżej,
obwiednia ~9% szybciej.
