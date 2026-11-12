# op1r SID (cRSID / Hermit, adaptacja Hundekraft) — port do DAW-POL 3000

Źródło: https://github.com/pcppcp/op1r-src — `synth_sdk/plugins/sid`, commit `042716b49e373af9e2d8cc7579ff060a29602726`.
Autorzy: Hermit (Mihaly Horvath) — prymitywy oscylatorów/filtra i tablice 8580 z cRSID 1.58
(https://hermit.cucumalac.hu/cRSID-1.58-with-source-code.tar.gz); adaptacja kennel: Hundekraft.
Licencje: kod cRSID — licencja „WTF” Hermita (zachowana w nagłówkach `sid_dsp.c` i tablic: rób co chcesz, mile
widziane podanie autora); integracja op1r/kennel — GPL-3.0-or-later + pozwolenie OP-1 (`LICENSE`, `NOTICE`).

Bez zmian: `sid.c` (6 głosów, wzmocnienie i trym), `sid_dsp.c` / `sid_dsp.h` (3 oscylatory na głos, filtr 8580),
`sid_filter_tables.h`, `sid_noise_tables.h`, `sid_screen.c` (przeniesiony do `core/ui/sid_screen.cpp`), `sid.svg`
(→ `sid_art.h`), `plugin.json`, `presets.py` (8 presetów). Zaślepki: `op1_plugin.h`, `op1_l1.h` (pusta).
`core/synth/sid_dsp_wrap.c` kompiluje `sid.c` jako C (stan głosu podmieniany per ścieżka), `sid_dsp.c` osobno.
Odrzucone: `sid_kernels.S` (Blackfin), skrypty kontrolne Pythona, testy hosta, `presets/*.aif`.

W DAW-POL: 4 gałki (WAVE, VOICES/WIDTH/INTERVAL, CUTOFF, RESONANCE), trym presetu = słowo 7 (`push[2]`), obwiednia
głośności, MOD ENV i 2 LFO. Presety: 8 z oryginału (obwiednia w przybliżeniu, bez efektów i oktawy) + 12 z modulacją.
Test bit w bit przy 88,2 kHz: `tools/test_sid.sh`. Przy 96 kHz filtr (liczony na próbkę) ~9% niżej względem dźwięku.
