# op1r MONOLIT (Hundekraft) — port do DAW-POL 3000

Źródło: https://github.com/pcppcp/op1r-src — `synth_sdk/plugins/monolit`, commit `042716b49e373af9e2d8cc7579ff060a29602726`.
Autor: Hundekraft (kennel), op1r: pcppcp; arytmetyka filtra drabinkowego: autorzy MOAI („moai contributors”).
Licencja: **GPL-3.0-or-later** z dodatkowym pozwoleniem dot. firmware OP-1 (`LICENSE`, `NOTICE`).
Oryginalny opis: `README_original.md`.

Bez zmian: `monolit.c` (DSP: 3 piły polyBLEP + sub, filtr drabinkowy zero-delay z MOAI, makra; + kod ekranu — ścieżka C
`mo_chunk_c`, nie asembler Blackfin), `monolit_tables.h`, `monolit.svg` (→ `monolit_art.h`), `plugin.json`,
`presets.py` (12 presetów). Zaślepki: `op1_plugin.h`, `screens/stroke_font.h` (tekst ekranu oryginału).
`core/synth/monolit_dsp.c` kompiluje `monolit.c` jako C; stan głosu (`br_home.v`, `br_rel`) podmieniany per ścieżka.
Odrzucone: `monolit_dsp.S`, `monolit_fc.S` (Blackfin), `kern_guest.c`, generatory i skrypty kontrolne Pythona.

W DAW-POL: 4 makra (SHAPE, CUTOFF, CONTOUR, HEAT), L3 na SOUND = strona 2 (BODY, VEL, AGE = `Track::push[0..2]`),
AMP ENV, FILTER ENV (na CUTOFF), MOD ENV, 2 LFO, VOICE (POLY/MONO i GLIDE — oryginał zakłada, że daje je gospodarz;
bez UNISON i AGE, decyzja Patryka). Wyjście ścieżki −3 dB (po decymacji, poza DSP): oryginał nie ma trymu, a dwie
nuty dochodziły do pełnej skali. Presety: 12 z oryginału (gałki i strona 2 1:1, obwiednia w przybliżeniu, bez efektów
i oktawy −1 czterech basów) + 20 z MONO/glide/unison i modulacją. Test bit w bit: `tools/test_monolit.sh`.
Częstotliwość: przy 96 kHz CONTOUR i dryf (liczone na blok) ~9% szybciej, filtr ~9% niżej względem dźwięku.
