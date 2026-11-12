# op1r TAFFY (Hundekraft) — port do DAW-POL 3000

Źródło: https://github.com/pcppcp/op1r-src — `synth_sdk/plugins/taffy`, commit `70e23c3ec5194e0f1c877a8aff5445061296034c`.
Autor: Hundekraft (kennel), op1r: pcppcp. Licencja: **GPL-3.0-or-later** z dodatkowym pozwoleniem dot. firmware OP-1
(`LICENSE`, `NOTICE`). Nagłówki z copyright zachowane.

Bez zmian: `taffy.c` (DSP: synteza z zakrzywianiem fazy w stylu Casio CZ — ścieżka C, nie asembler Blackfin),
`taffy_tables.h`, `taffy_geom.h`, `screen.c` (ekran — przeniesiony do `core/ui/taffy_screen.cpp`), `taffy.svg`
(statyczna grafika — przepisana skryptem `tools/gen/svg_to_art.py` do `taffy_art.h`).

Zaślepki zamiast firmware: `hk/engine.h` (głosy, wysokość, velocity podawane przez wrapper), `op1_l1.h` (pusta).
Odrzucone: `taffy_dsp.S`, `taffy_pairs.h` (Blackfin), adaptery firmware (`taffy_plugin.c`).

Różnica wynikająca z częstotliwości: na OP-1 głos liczony jest przy 88,2 kHz, u nas przy 96 kHz. Kształt dźwięku jest
identyczny (test bit w bit przy 88,2 kHz: `tools/test_taffy.sh`), ale wewnętrzna obwiednia barwy i filtr DC są
liczone „na blok”, więc przy 96 kHz biegną ~9% szybciej (obwiednia ~0,17 s zamiast ~0,19 s).
