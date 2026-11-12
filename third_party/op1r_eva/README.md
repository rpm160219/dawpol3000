# op1r EVA (Hundekraft) — port do DAW-POL 3000

Źródło: https://github.com/pcppcp/op1r-src — `synth_sdk/plugins/eva`, commit `70e23c3ec5194e0f1c877a8aff5445061296034c`.
Autor: Hundekraft (kennel), op1r: pcppcp. Licencja: **GPL-3.0-or-later** z dodatkowym pozwoleniem dot. firmware OP-1
(`LICENSE`, `NOTICE`). Nagłówki z copyright zachowane.

Bez zmian: `eva.c` (DSP „kosmiczny chór” + kod ekranu — ścieżka C, nie asembler Blackfin), `eva_tables.h`,
`eva_geom.h`, `eva.svg` (ekran — przepisany skryptem `tools/gen/svg_to_art2.py` do `eva_art.h`, z kolejnością
elementów), `eva-art.svg` (rysunek źródłowy), `plugin.json`, `presets.py` (8 presetów oryginału).

Zaślepka zamiast firmware/op1r: `op1_plugin.h` (wysokość, velocity, puste funkcje ekranu). `eva.c` jest w C
(inicjalizatory tylko z C), więc `core/synth/eva_dsp.c` kompiluje go jako C; stan ścieżki: `hot_t` pod wskaźnikiem
`ev_h` (oryginał sam go przełącza dla puli L1) oraz małe `ev_q`/`ev_m` podmieniane przed każdym wywołaniem —
jedna instancja = jedna ścieżka. Ekran z `eva.c` przeniesiony do `core/ui/eva_screen.cpp`.
Odrzucone: `eva_dsp.S` (Blackfin), `gen_svg.py`, `gen_tables.py`, pliki `presets/*.aif` (format OP-1).

Port: 4 gałki + strona 2 COM z oryginału (oktawa sygnału Quindar, L3 na SOUND, `Track::push[2]` = słowo 7),
obwiednia głośności; na prośbę Patryka dodane MOD ENV i 2 LFO (modulują gałki, kod `eva.c` bez zmian). Presety oryginału: gałki 1:1, obwiednia OP-1 przeliczona
w przybliżeniu na sekundy; efekty (spring/delay), LFO i oktawa presetów pominięte.

Różnica wynikająca z częstotliwości: na OP-1 głos liczony jest przy 88,2 kHz (wewnętrznie 44,1 kHz), u nas przy
96 kHz (wewnętrznie 48 kHz). Dźwięk identyczny w teście bit w bit przy 88,2 kHz (`tools/test_eva.sh`); przy 96 kHz
filtry, wibrato, chorus i sygnały Quindar są ~9% wyżej/szybciej.
