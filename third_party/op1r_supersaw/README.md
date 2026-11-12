# op1r SUPERSAW (pcppcp) — port do DAW-POL 3000

Źródło: https://github.com/pcppcp/op1r-src — `synth_sdk/plugins/supersaw`, commit `46dfde8`.
Autor: pcppcp (op1r). Licencja: **GPL-3.0-or-later** z dodatkowym pozwoleniem dot. firmware OP-1 (`LICENSE`, `NOTICE`
— pliki z katalogu głównego op1r-src).

Bez zmian: `supersaw.c` (DSP: 2 rozstrojone piły → kwadrat, sub sinus −1 okt., przester z miękkim obcinaniem, filtr
jednobiegunowy; + kod ekranu piły łańcuchowej), `supersaw_geom.h`, `supersaw.svg` (→ `supersaw_art.h` przez
`tools/gen/svg_to_art2.py`, grupy saw, sprk, bar0..3), `plugin.json`, `gen_svg.py`. Zaślepka: `op1_plugin.h`.
`core/synth/supersaw_dsp.c` kompiluje `supersaw.c` jako C; stan głosu (`voice_t`, „self” jak pamięć głosu OP-1) w instancji
ścieżki. Ekran = kod C oryginału, rysunek i linie przez funkcje zwrotne (`core/ui/supersaw_screen.cpp`, 2×).

W DAW-POL: gałki DETUNE / MORPH / SUB / DRIVE; strony SOUND, VOICE (POLY/MONO i GLIDE; UNISON i AGE usunięte — decyzja Patryka), AMP ENV, MOD ENV,
LFO 1, LFO 2, PRESETS oraz filtr DAW-POL za silnikiem (L3: CUTOFF / RESONANCE / TYPE, strona FILTER ENV; w pełni otwarty = oryginał). Wyjście ścieżki −3 dB po decymacji
(gęste piły z przesterem, jak MONOLIT). Oryginał nie ma presetów — 24 własne. Test bit w bit: `tools/test_supersaw.sh`.
Częstotliwość: przy 96 kHz filtr 4,9 kHz oryginału (współczynnik na próbkę) wypada ~9% wyżej.
