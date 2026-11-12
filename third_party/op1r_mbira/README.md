# op1r MBIRA (Hundekraft) — port do DAW-POL 3000

Źródło: https://github.com/pcppcp/op1r-src — `synth_sdk/plugins/mbira`, commit `70e23c3ec5194e0f1c877a8aff5445061296034c`.
Autor: Hundekraft (kennel), op1r: pcppcp. Licencja: **GPL-3.0-or-later** z dodatkowym pozwoleniem dot. firmware OP-1
(`LICENSE`, `NOTICE`). Nagłówki z copyright zachowane. Oryginalny opis: `README_original.md`.

Bez zmian: `mbira.c` (DSP pianina kciukowego — synteza modalna, + kod ekranu; ścieżka C, nie asembler Blackfin),
`mbira_noise.h`, `mbira_geom.h`, `mbira.svg` (przepisany skryptem `tools/gen/svg_to_art2.py` do `mbira_art.h`),
`plugin.json`, `presets.py` (8 presetów oryginału).

Zaślepka zamiast firmware/op1r: `op1_plugin.h` (wysokość, velocity, tablica sinusa jak na OP-1, puste funkcje
ekranu). `core/synth/mbira_dsp.c` kompiluje `mbira.c` jako C; stan ścieżki: `hot_t` pod wskaźnikiem `mb_home_at`
i struktura `mb_g` podmieniane przed każdym wywołaniem (jedna instancja = jedna ścieżka). Pamięć podręczna
współczynników (`coeff_cache`) wspólna — wartości zależą tylko od kluczy. Ekran z `mbira.c` przeniesiony do
`core/ui/mbira_screen.cpp`. Odrzucone: `mbira_dsp.S` (Blackfin), `presets/*.aif` (format OP-1).

W DAW-POL: 4 gałki + strona 2 z oryginału (L3 na SOUND: RING, DOUBLE, WAH = `Track::push[0..2]`, słowa 5..7),
obwiednia głośności, MOD ENV i 2 LFO (jak EVA). Presety oryginału: gałki i strona 2 1:1, obwiednia OP-1
w przybliżeniu; efekty i oktawa pominięte.

Różnica wynikająca z częstotliwości: na OP-1 głos liczony jest przy 88,2 kHz, u nas przy 96 kHz. Dźwięk identyczny
w teście bit w bit przy 88,2 kHz (`tools/test_mbira.sh`); przy 96 kHz zaniki i pudło ~9% szybciej/wyżej.
