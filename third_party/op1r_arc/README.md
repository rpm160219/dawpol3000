# op1r ARC (ultralazr) — port do DAW-POL 3000

Źródło: https://github.com/pcppcp/op1r-src — `synth_sdk/plugins/arc`, commit `06cc18d`.
Autor: ultralazr; op1r: pcppcp. Licencja: **GPL-3.0-or-later** z dodatkowym pozwoleniem dot. firmware OP-1 (`LICENSE`,
`NOTICE` — pliki z katalogu głównego op1r-src). Oryginalny opis: `README_original.md`.

Bez zmian: `arc.c` (DSP: oscylator Van der Pola „śpiewającego łuku” Duddella + ekran), `arc_tables.h`, `arc_calib.c`,
`arc_geom.h`, `duddell.svg` (→ `duddell_art.h`), `gen_svg.py`, `gen_tables.py`, `plugin.json`. Zaślepki: `op1_plugin.h`,
`seq/seq_text.h`. Odrzucone: `arc_dsp.S` (jądra Blackfin; pętle C dają te same próbki), podglądy PNG/WAV.

W DAW-POL: `core/synth/arc_dsp.c` kompiluje `arc.c` jako C; stan głosu = slot 0x24 B w instancji (self); globalne dane
ekranu (portret fazowy, poziomy) przekierowane makrami do instancji, głos śledzony podmieniany. Ekran:
`core/ui/arc_screen_c.c` kompiluje `arc.c` drugi raz (przedrostek ars_). Sam silnik 1:1: gałki CURRENT, CARBON, HISS,
STRIKE; L3 na SOUND = strona 2 (RATIO z zapadkami, COUPLING, BRIGHTNESS); strony SOUND / AMP ENV / PRESETS. Tarcze z dołu
ekranu i napis proporcji pominięte (dublowałyby gałki). Wyjście ścieżki −3 dB. Oryginał nie ma presetów — 12 własnych.
Test bit w bit: `tools/test_arc.sh` (szum startuje z ziarna zależnego od adresu pamięci głosu: harness bez PIE, adres z
tablicy symboli, ziarno ustawiane w wrapperze).
