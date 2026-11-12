# op1r JUNI (Hundekraft) — port do DAW-POL 3000

Źródło: https://github.com/pcppcp/op1r-src — `synth_sdk/plugins/juni`, commit `0da4b24`.
Autor: Hundekraft; op1r: pcppcp. Licencja: **GPL-3.0-or-later** z dodatkowym pozwoleniem dot. firmware OP-1 (`LICENSE`,
`NOTICE` — pliki z katalogu głównego op1r-src). Oryginalny opis: `README_original.md`.

Bez zmian: `juni.c` (DSP 6-głosowego poli z jednym DCO, filtrem drabinkowym, HPF i chorusem + ekran), `juni_geom.h`,
`juni_tables.h`, `juni.svg` (→ `juni_art.h`), `juni_test.c`, `gen_svg.py`, `gen_tables.py`, `plugin.json`, `presets.py`
(12 presetów). Zaślepka: `op1_plugin.h`.

W DAW-POL: `core/synth/juni_dsp.c` kompiluje `juni.c` jako C; stan pliku (głosy, wspólne parametry bloku, LFO PWM i
chorusa, linie chorusa) per ścieżka, podmieniany leniwie (lista JN_STATE). Jedyna obwiednia to obwiednia głośności
(u nas AMP ENV) — prowadzi też filtr przez SWEEP, jak w oryginale. Ekran: `core/ui/juni_screen_c.c` kompiluje `juni.c`
drugi raz (przedrostek jns_). Sam silnik 1:1: TONE, SHAPE, SWEEP, CHORUS; L3 na SOUND = RESONANCE, HPF, PWM RATE; strony
SOUND / AMP ENV / PRESETS. Wyjście ścieżki −3 dB (głośny: akordy przesterowałyby). Presety: 12 z oryginału (ADSR w
przybliżeniu). Test bit w bit (wrapper z zastępczą obwiednią harnessa): `tools/test_juni.sh`.
