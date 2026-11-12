# op1r ELG (Hundekraft) — port do DAW-POL 3000

Źródło: https://github.com/pcppcp/op1r-src — `synth_sdk/plugins/elg`, commit `70e23c3ec5194e0f1c877a8aff5445061296034c`.
Autor: Hundekraft (kennel), op1r: pcppcp. Licencja: **GPL-3.0-or-later** z dodatkowym pozwoleniem dot. firmware OP-1
(`LICENSE`, `NOTICE`). Nagłówki z copyright zachowane. Oryginalny opis: `README_original.md`.

Bez zmian: `elg.c` (DSP: dron — 3 piły na głos, przester „growl”, filtr Chamberlina, ruch — ścieżka C, nie asembler
Blackfin), `elg_level.h`, `elg_geom.h`, `screen.c` (ekran — przeniesiony do `core/ui/elg_screen.cpp`), `elg.svg`
(scena — przepisana skryptem `tools/gen/svg_to_art2.py` do `elg_art.h`; pominięte ukryte paski iter `base0..3`),
`plugin.json`.

Zaślepka zamiast firmware: `hk/engine.h`. `elg.c` trzyma głosy i wartości dla ekranu w zmiennych pliku, więc
`core/synth/elg_dsp.cpp` wkleja go do klasy (instancja na ścieżkę). Odrzucone: `elg_dsp.S` (Blackfin), adaptery
firmware (`elg_plugin.c`, `elg_src_*.c`), `gen_svg.py`.

Port „sam silnik” (decyzja Patryka): tylko 4 gałki oryginału i obwiednia głośności (jak stockowa obwiednia OP-1);
bez drugiej obwiedni, MOD ENV, LFO, trybów głosu i fabrycznych presetów (tylko INIT).

Różnica wynikająca z częstotliwości: na OP-1 głos liczony jest przy 88,2 kHz, u nas przy 96 kHz. Dźwięk jest
identyczny (test bit w bit przy 88,2 kHz: `tools/test_elg.sh`), ale LFO ruchu i dryf liczone „na blok” biegną przy
96 kHz ~9% szybciej.
