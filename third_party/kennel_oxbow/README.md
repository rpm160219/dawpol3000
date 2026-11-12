# kennel OXBOW (Hundekraft, wg OberVave zvensona) — port do DAW-POL 3000

Źródło: paczka źródeł Hundekraft 3.0.1 (`hundekraft-3.0.1-source.tar.gz`, link w README
https://github.com/Hundekraft/op1-workshop, SHA-256 `4e2de6bc3de05abe2c072e86d394ae4b2afccbfb738d296b4aab20e02e06da4a`),
katalog `ext/kennel/synth_sdk/plugins/oxbow`. OXBOW nie ma go w repo op1r (pcppcp) — tylko w tej paczce.
Autorzy: zvenson (syntezator OberVave, https://github.com/zvenson/obervave, `a2fb3d0`), Hundekraft (port do kennel).

**Licencja:** OberVave jest na **GPL-3.0-only**. `PROVENANCE_kennel.md` (z paczki) podaje, że opiekun projektu kennel
zgłosił zgodę zvensona (2026-10-15) na użycie kodu na warunkach GPL-3.0-or-later + pozwolenie OP-1; pisemnej kopii
w paczce nie ma. Do prywatnego użytku bez znaczenia; przy dystrybucji DAW-POL 3000 przyjąć **GPL-3.0-only** dla całości
(DAW-POL jest GPL-3.0-or-later, więc może być rozpowszechniany na GPL-3.0-only). `LICENSE`, `NOTICE` z kennel.

Bez zmian: `oxbow.c` (DSP + kod ekranu), `obv_dsp.h`, `obv_params.h`, `obv_tables.h`, `oxbow_geom.h`, `oxbow.svg`
(przepisany `tools/gen/svg_to_art2.py` do `oxbow_art.h`), `plugin.json`, `presets.py` (16 presetów).
Zaślepki: `plugins/oxbow/op1_plugin.h`, `seq/seq_text.h`. `core/synth/oxbow_dsp.c` kompiluje `oxbow.c` jako C; stan
ścieżki (zmienne pliku) zamieniany przy przełączaniu ścieżek. Ekran: `core/ui/oxbow_screen.cpp` (tło w cache).
Odrzucone: `gen_svg.py`, `gen_tables.py`, `presets/*.aif` (format OP-1).

W DAW-POL: CUTOFF / RESONANCE / ENV AMOUNT / DETUNE, strona 2 pod L3 (WAVE / FILTER / DRIVE = `push[0..2]`, słowa 5..7),
AMP ENV, MOD ENV, 2 LFO. Presety: gałki i strona 2 1:1, obwiednia OP-1 w przybliżeniu (8 s × (v/32767)^1,5), efekty
pominięte. Trzy presety z filtrem BP12 (BELL XMOD, BP QUACK, CLAV PLUCK) są bardzo ciche — cecha ich ustawień.

**OXBOW+ (rozszerzenie DAW-POL):** `plugins/oxbow/oxbow_plus.c` to **zmodyfikowana kopia** `oxbow.c` (oznaczona
w nagłówku), `oxbow_plus.h` — parametry. Stałe neutralnego głosu zastąpione parametrami ścieżki: fala, interwał,
poziom i szerokość pulsu każdego oscylatora, strojenie OSC 1, szum, SYNC, X-MOD, obwiednia filtra → wysokość OSC 2,
keytrack, velocity, VINTAGE, czasy obwiedni filtra. Z ustawieniami domyślnymi gra bit w bit jak oryginał z WAVE = SAW
(`tools/test_oxbowplus.sh`). Oryginał `oxbow.c` pozostaje bez zmian (silnik OXBOW).

Częstotliwość: wewnątrz OXBOW liczy w 22,05 kHz (połowa 44,1). Przy naszych 96 kHz głosu — 24 kHz wewnątrz: filtry
i czasy ~9% wyżej/szybciej, a rozpoznanie nuty z przyrostu fazy (śledzenie klawiatury filtra) ~1,5 półtonu niżej.
Test bit w bit przy 88,2 kHz: `tools/test_oxbow.sh` (oryginał z hostowym harnessem op1r).
