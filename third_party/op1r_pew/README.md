# op1r PEW (Hundekraft) — port do DAW-POL 3000

Źródło: https://github.com/pcppcp/op1r-src — `synth_sdk/plugins/pew`, commit `70e23c3ec5194e0f1c877a8aff5445061296034c`.
Autor: Hundekraft (kennel), op1r: pcppcp. Licencja: **GPL-3.0-or-later** z dodatkowym pozwoleniem dot. firmware OP-1
(`LICENSE`, `NOTICE`). Nagłówki z copyright zachowane. Oryginalny opis: `README_original.md`.

Bez zmian: `pew.c` (DSP: organy z 9 suwakami przez głośnik rotacyjny — ścieżka C, nie asembler Blackfin),
`pew_tables.h`, `pew_macro.h`, `pew_geom.h`, `screen.c` (ekran — przeniesiony do `core/ui/pew_screen.cpp`),
`pew.svg` (scena „jazz club” — przepisana skryptem `tools/gen/svg_to_art2.py` do `pew_art.h`), `plugin.json`.

Zaślepka zamiast firmware: `hk/engine.h`. `pew.c` ma wspólny stan wszystkich głosów (tablice organów, wirniki
Leslie, perkusja z pojedynczym wyzwalaniem), więc `core/synth/pew_dsp.cpp` wkleja go do klasy (`#define static`
na czas wklejenia) — jedna instancja = jedna ścieżka.
Odrzucone: `pew_dsp.S` (Blackfin), `kern_guest.c`, `kern_check.py`, adaptery firmware (`pew_plugin.c`, `pew_src_*.c`),
`gen_svg.py` / `gen_tables.py` (generatory — wyniki są w repo).

Różnica wynikająca z częstotliwości: na OP-1 głos liczony jest przy 88,2 kHz (689 bloków/s), u nas przy 96 kHz
(750 bloków/s). Dźwięk jest identyczny (test bit w bit przy 88,2 kHz: `tools/test_pew.sh`), ale to, co liczone jest
„na blok” (wirniki Leslie, skaner, opadanie perkusji, klik), biegnie przy 96 kHz ~9% szybciej.
