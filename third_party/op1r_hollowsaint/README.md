# op1r HOLLOW SAINT (kennel) — port do DAW-POL 3000

Źródło: https://github.com/pcppcp/op1r-src — `synth_sdk/plugins/hollowsaint` (wersja 0.7.0, build kennel), commit `fc14074`.
Autorzy: „the hollow_saint authors” (SammiBabe666; poprawki częstotliwości i wspólny etap: kennel / Hundekraft); op1r:
pcppcp. Licencja: **GPL-3.0-or-later** z dodatkowym pozwoleniem dot. firmware OP-1 (`LICENSE`, `NOTICE` — pliki z katalogu
głównego op1r-src). Oryginalny opis: `README_original.md`.

Bez zmian: `hollow_saint.c` (DSP + ekran), `hollow_saint_geom.h`, `hollow_saint.svg` (→ `hollow_saint_art.h`, 43 grupy),
`plugin.json`, `presets.py` (8 presetów), `gen_screen.py`. Zaślepka: `op1_plugin.h`. Odrzucone: `hs_*.S` (jądra Blackfin;
pętle C dają te same próbki), pliki `.aif` presetów (dane z `presets.py`).

W DAW-POL: `core/synth/hollowsaint_dsp.c` kompiluje `hollow_saint.c` jako C z flagami kennel z `plugin.json`
(HS_SHARED_ROOM, HS_SHARED_RELIC, HS_CAT11K, HS_RELIC_LITE, HS_BOW_CHEAP, HS_HIGH_HALF). Cały stan oryginału (głosy, wspólny
pogłos i taśma, burdon, licznik bloków) jest w zmiennych pliku — każda ścieżka ma własną kopię, podmienianą leniwie przy
zmianie instancji (lista HS_STATE; test izolacji dwóch ścieżek: 0 różnic). Silnik czyta obwiednię głośności głosu (u nas
AMP ENV) i zegar taktu OP-1 (u nas tempo i pozycja transportu; zatrzymany transport = koło w wolnym biegu w tempie).
Sam silnik 1:1: gałki DRONE, STRINGS, WHEEL BUZZ, CATHEDRAL; L3 na SOUND = strona 2 oryginału: DRONE KEY (FOLLOW, C..B),
RELIC AGE, STROKES (STEADY, 1 BAR … 1/16) i RELIC wł./wył. (w OP-1 taśma gra, gdy otwarta jest strona 2 zielonej gałki —
u nas osobny przełącznik na czerwonej gałce, zapisywany w projekcie). Strony SOUND / AMP ENV / PRESETS. Ekran: wskaźniki
gałek z dołu oryginału pominięte (dublowałyby gałki DAW-POL). Presety: 8 z oryginału + 6 własnych (z taśmą RELIC).
Test bit w bit (wrapper z zastępczą obwiednią harnessa): `tools/test_hollowsaint.sh`. Częstotliwość: przy 96 kHz filtry
i zaniki liczone na próbkę ~9% szybciej względem 88,2 kHz oryginału.
