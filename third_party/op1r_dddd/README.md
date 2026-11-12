# op1r DDDD (pcppcp) — port do DAW-POL 3000

Źródło: https://github.com/pcppcp/op1r-src — `synth_sdk/plugins/dddd` + `synth_sdk/dddd/dd_core.{c,h}`, commit `0da4b24`.
Autor: pcppcp. Licencja: **GPL-3.0-or-later** z dodatkowym pozwoleniem dot. firmware OP-1 (`LICENSE`, `NOTICE` — pliki z
katalogu głównego op1r-src).

Bez zmian: `plugins/dddd/dddd.c` (parafonia + ekran osy), `plugins/dddd/dddd.svg` (pusty), `plugins/dddd/plugin.json`,
`dddd/dd_core.c`, `dddd/dd_core.h` (DSP ściany szumu). Zaślepki: `plugins/dddd/op1_plugin.h`, `op1_synth.h`.

W DAW-POL: `core/synth/dddd_dsp.c` kompiluje `dddd.c` jako C (`dd_core.c` osobno, -O3 jak w plugin.json). Cały stan
(ściana `dd_t` z buforem pętli 16 KB, licznik wywołań, kolejność nut, trzymane klawisze) per ścieżka, podmieniany leniwie
(lista DD_STATE). Silnik czyta obwiednię głośności głosu (u nas AMP ENV: otwiera filtr). Ekran: `core/ui/dddd_screen_c.c`
kompiluje `dddd.c` drugi raz (przedrostek dds_); dolny rząd z napisami pominięty. Sam silnik 1:1: SOURCE, FEEDBACK,
DESTROY, HANDS; L3 na SOUND = FILTER, SPREAD, RATE; strony SOUND / AMP ENV / PRESETS. Pokrętło SOURCE w OP-1 ma 8
stopni na zakres („coarse”), u nas płynne (te same wartości). Oryginał nie ma presetów — 11 własnych.
Test bit w bit (wrapper z zastępczą obwiednią harnessa): `tools/test_dddd.sh`.
