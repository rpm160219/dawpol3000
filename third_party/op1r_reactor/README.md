# op1r REACTOR (Hundekraft) — port do DAW-POL 3000

Źródło: https://github.com/pcppcp/op1r-src — `synth_sdk/plugins/reactor`, commit `042716b49e373af9e2d8cc7579ff060a29602726` (op1r 63).
Autor: Hundekraft (kennel), op1r: pcppcp. Licencja: **GPL-3.0-or-later** z dodatkowym pozwoleniem dot. firmware OP-1
(`LICENSE`, `NOTICE`). Nagłówki z copyright zachowane. Oryginalny opis: `README_original.md`.

Układ katalogów jak w op1r (`plugins/reactor/`, bo `reactor.c` dołącza `../../seq/seq_text.h`).
Bez zmian: `reactor.c` (DSP: 4 sinusowe pręty w pętli PM ze sprzężeniem, temperatura rdzenia, stan krytyczny,
„ucieczka”; + kod ekranu), `reactor_dsp.h` (pętla próbek w C), `reactor_sin.h`, `reactor_geom.h`, `reactor.svg`
(przepisany skryptem `tools/gen/svg_to_art2.py` do `reactor_art.h`), `plugin.json`, `presets.py` (11 presetów).

Zaślepki: `plugins/reactor/op1_plugin.h` (wysokość, velocity, trym presetu, tablica sinusa jak na OP-1, puste funkcje
ekranu), `seq/seq_text.h` (tekst ekranu oryginału — nasz ekran rysuje odczyty własną czcionką).
`core/synth/reactor_dsp.c` kompiluje `reactor.c` jako C; stan głosu (`rx_voice`) i licznik bloków podmieniane przed
każdym wywołaniem (jedna instancja = jedna ścieżka). Ekran przeniesiony do `core/ui/reactor_screen.cpp` (z drżeniem).
Odrzucone: `reactor_kernel.S` (Blackfin), `reactor_demo.c` (render na Macu), `gen_svg.py`, `gen_tables.py`,
`reactor-art.webp`.

W DAW-POL: 4 gałki (COOLING, RADIATION, RODS, HEAT), trym presetu = słowo 7 (`Track::push[2]`), obwiednia
głośności, MOD ENV i 2 LFO. Presety oryginału: gałki i trym 1:1, obwiednia OP-1 w przybliżeniu; efekty i oktawa
(-1 w trzech basach) pominięte. Oryginał jest głośniejszy od pozostałych silników (presety ~ -6,6 dBFS).

Różnica wynikająca z częstotliwości: na OP-1 głos liczony jest przy 88,2 kHz, u nas przy 96 kHz. Dźwięk identyczny
w teście bit w bit przy 88,2 kHz (`tools/test_reactor.sh`); przy 96 kHz to, co liczone „na blok” (temperatura,
oddech rdzenia, trzaski), biegnie ~9% szybciej.
