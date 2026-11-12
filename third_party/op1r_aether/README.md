# op1r AETHER (pcppcp) — port do DAW-POL 3000

Źródło: https://github.com/pcppcp/op1r-src — `synth_sdk/plugins/aether`, commit `fc14074`.
Autor: pcppcp. Licencja: **GPL-3.0-or-later** z dodatkowym pozwoleniem dot. firmware OP-1 (`LICENSE`, `NOTICE` — pliki
z katalogu głównego op1r-src).

Bez zmian: `aether.c` (DSP „przechylanego teremina” + ekran anteny), `aether_geom.h`, `aether.svg` (→ `aether_art.h`),
`orbit-1.svg`, `gen_svg.py`, `plugin.json`. Zaślepki: `op1_plugin.h`, `op1_synth.h`.

Czujnik: na OP-1 silnik czyta akcelerometr (przechylenie przód/tył = wysokość, uniesienie prawego boku = ściszanie,
potrząsanie = wibrato; kąt przy nucie = „dom”). RG35XX nie ma czujnika ruchu, więc (decyzja Patryka) `aether.c` jest
kompilowany z OP1_HOST, a jego czujnik (`host_accel`) wypełnia wrapper `core/synth/aether_dsp.c` z wirtualnego
przechylenia: strona TILT (PITCH TILT −60..+60°, VOL TILT 0..30°, SHAKE = drganie osi Z skalibrowane do detektora
oryginału: próg ~650, pełne ~2050 jednostek przy 1 g = 16384) plus modulacja MOD ENV / LFO (cele PTLT, VTLT, SHAK).
Dom = poziomo. Stan głosów `st[6]` i `ae_cur` per instancja. Ekran: `core/ui/aether_screen_c.c` kompiluje `aether.c`
drugi raz (osobne zmienne ekranu i czujnik); tarcze, suwaki, łuki wartości i etykiety pominięte (dublowałyby gałki).
Strony: SOUND (L3 = DEAD ZONE, TILT RANGE, VIB RATE), TILT, VOICE (MODE, GLIDE — w oryginale tryby gry OP-1), AMP ENV,
MOD ENV, LFO 1, LFO 2, PRESETS. Wyjście ścieżki −3 dB. Oryginał nie ma presetów — 10 własnych.
Test bit w bit (bez przechylenia, jak harness): `tools/test_aether.sh`.
