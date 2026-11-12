# op1r MIXTUR (ultralazr) — port do DAW-POL 3000

Źródło: https://github.com/pcppcp/op1r-src — `synth_sdk/plugins/mixtur`, commit `46dfde8`.
Autor: ultralazr (współautor `mixtur_dsp.h`: Hundekraft); op1r: pcppcp. Licencja: **GPL-3.0-or-later** z dodatkowym
pozwoleniem dot. firmware OP-1 (`LICENSE`, `NOTICE` — pliki z katalogu głównego op1r-src). Oryginalny opis: `README_original.md`.

Bez zmian: `mixtur.c` (DSP + ekran drabiny undertonów), `mixtur_dsp.h` (pętle C — te same próbki co jądra Blackfin),
`mixtur_tables.h`, `mixtur_geom.h`, `mixtur.svg` (→ `mixtur_art.h`), `plugin.json`, `gen_svg.py`, `gen_tables.py`.
Zaślepki: `op1_plugin.h`, `seq/seq_text.h` (napisy z dołu ekranu oryginału pominięte — dublowałyby gałki DAW-POL).
Odrzucone: `mixtur_dsp.S`, `mixtur_dsp_l1.S` (asembler Blackfin), podglądy PNG/WAV.

W DAW-POL: `core/synth/mixtur_dsp.c` kompiluje `mixtur.c` jako C; pamięć głosu = 6 slotów po 0x24 bajty w instancji
ścieżki (jak w firmware), tablica sygnatur `own[]` podmieniana per instancja. Sam silnik 1:1 (decyzja Patryka, jak ELG):
gałki UNDERTONE A..D (krokowo: OFF, 1/1 … 1/24), L3 na SOUND = strona 2 oryginału (FORMANT 1, FORMANT 2, RESONANCE),
tylko obwiednia głośności (AMP ENV), bez MOD ENV / LFO / VOICE. Oryginał nie ma presetów — 13 własnych z samych gałek.
Test bit w bit: `tools/test_mixtur.sh`. Częstotliwość: przy 96 kHz formanty (współczynniki na próbkę) ~9% wyżej, progi
wyciszania undertonów (25/40 Hz) ~9% wyżej względem 88,2 kHz oryginału.
