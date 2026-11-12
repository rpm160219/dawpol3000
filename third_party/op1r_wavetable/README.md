# op1r WAVETABLE (Hundekraft) — port do DAW-POL 3000

Źródło: https://github.com/pcppcp/op1r-src — `synth_sdk/plugins/wavetable`, commit `70e23c3ec5194e0f1c877a8aff5445061296034c`.
Autor: Hundekraft (kennel 3.1.4), op1r: pcppcp. Licencja: **GPL-3.0-or-later** z dodatkowym pozwoleniem dot. firmware OP-1
(zob. `LICENSE`, `NOTICE`). Nagłówki z copyright zachowane bez zmian.

Pliki skopiowane bez zmian: `wavetable_dsp.h` (DSP — używamy ścieżki C, nie asemblera Blackfin; autorzy deklarują
identyczność bit w bit), `wavetable_level.h`, `wavetable_geom.h` (geometria ekranu), `orig_88k/wavetables.h` (oryginał).

`wavetables.h` — wygenerowany ich generatorem (`tools/gen/op1r_wavetable_gen_tables.py 96000`): fale identyczne
z oryginałem, tabela cutoff przeliczona dla 96 kHz (głos u nas = 2 × 48 kHz; na OP-1 R246 = 2 × 44,1 kHz),
dzięki czemu filtr ma te same częstotliwości w Hz. Generator z argumentem 88200 odtwarza oryginał bajt w bajt.

Nieużywane/odrzucone: `wavetable_dsp.S` (asembler Blackfin), `wavetable_pairs.h`, adaptery firmware
(`wavetable_plugin.c`, L1 pool), `wavetable.svg` (statyczna grafika odtworzona w kodzie wg `gen_svg.py`).
