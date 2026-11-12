# op1r BLOODMOON (Nola Drift) — port do DAW-POL 3000

Źródło: https://github.com/pcppcp/op1r-src — `synth_sdk/plugins/bloodmoon` (build kennel), commit `fc14074`.
Autorka: Nola Drift (publikowane jako SammiBabe666, https://github.com/SammiBabe666/Project_Bloodmoon); kennel; op1r:
pcppcp. Licencja: **GPL-3.0-or-later** z dodatkowym pozwoleniem dot. firmware OP-1 (`LICENSE`, `NOTICE` — pliki z katalogu
głównego op1r-src). Oryginalny opis: `README_original.md`.

Bez zmian: `bloodmoon.c` (DSP: organy z harmonicznymi 1–4 i nawiedzonym dzwonem, wibrato po ataku), `moon.c` (ekran),
`bloodmoon_geom.h`, `bloodmoon.svg` (→ `bloodmoon_art.h`), `bloodmoon-art.svg`, `gen_svg.py`, `plugin.json`. Zaślepki:
`op1_plugin.h`, `op1_synth.h`. Odrzucone: `bloodmoon_dsp.S` (jądro Blackfin; pętla C daje te same próbki), pliki `.aif`
presetów (dane przepisane do `core/io/factory_presets.cpp`).

W DAW-POL: `core/synth/bloodmoon_dsp.c` kompiluje oba pliki jako C; stan głosu = slot 0x24 B w instancji (self), ziarno
wibrata `bm_seed` per instancja. Sam silnik 1:1 + AMP ENV (decyzja Patryka): gałki BLOOD, ECLIPSE, HAUNTING, MOONRISE;
strony SOUND / AMP ENV / PRESETS. Ekran: napisy oryginału z nazwami i wartościami gałek pominięte (dublowałyby gałki).
Presety: 8 autorki (gałki i oktawa 1:1, ADSR w przybliżeniu, bez efektów OP-1). Test bit w bit: `tools/test_bloodmoon.sh`.
Częstotliwość: przy 96 kHz wibrato i zaniki (na blok) ~9% szybciej względem 88,2 kHz oryginału.
