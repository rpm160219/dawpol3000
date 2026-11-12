# op1r VAPORA (AiAAAA) — port do DAW-POL 3000

Źródło: https://github.com/pcppcp/op1r-src — `synth_sdk/plugins/vapora` (wersja 1.2.4), commit `042716b49e373af9e2d8cc7579ff060a29602726`.
Autor: AiAAAA; op1r: pcppcp. Licencja: **GPL-3.0-or-later** z dodatkowym pozwoleniem dot. firmware OP-1 (`LICENSE`, `NOTICE`,
`THIRD_PARTY_NOTICES.md`). Oryginalny opis: `README_original.md`.

Bez zmian: `vapora.c`, `vapora_dsp.c/.h`, `vapora_params.h`, `vapora_characters.h`, `vapora_mailbox.h`, `vapora_tables.h`
(161 tablic PADsynth), kod ekranu `vapora_screen.c` z nagłówkami (`vapora_visual.h`, `vapora_suns.h`, `vapora_particles.h`,
`vapora_star_logos.h`, `vapora_wave_modes.h`, `vapora_q.h`), `vapora.svg`, `plugin.json`, `PRESET_CATALOG.csv`,
`data/presets.json` (15 presetów). Zaślepki: `op1_plugin.h`, `op1_synth.h`, `seq/seq_text.h`.
Odrzucone: testy, generatory Pythona, pliki `.aif` presetów (dane presetów przepisane z `presets.json` / `recipes.json`).

W DAW-POL: `core/synth/vapora_dsp.c` kompiluje `vapora.c` jako C (stan głosu, licznik nut i skrzynki ekranu per ścieżka),
`vapora_dsp.c` osobno. Ekran: `core/ui/vapora_screen_c.c` kompiluje `vapora_screen.c` jako C; linie i tarcza słońca
idą do naszego Canvas (2×). Stopka z napisami (Shift) i ukryta animacja z napisami wyłączone — gałki rysuje DAW-POL.
Gałki: MORPH / THICKNESS / BRIGHTNESS / MOTION; L3 na SOUND = strona 2: CHARACTER (14 widm, krokowo), INTERVAL
(10, krokowo), SUN (wzór słońca, tylko obraz — w oryginale biała gałka wciśnięta), SPEED. Strony: AMP ENV, FILTER ENV
(na BRIGHTNESS), MOD ENV, 2 LFO. Wyjście ścieżki +9,5 dB po decymacji (oryginał zostawia zapas na 6 głosów).
Presety: 15 z oryginału (gałki, strona 2 i oktawa 1:1, obwiednia z `preview_envelope_ms`, bez efektów OP-1) + 15 nowych.
Test bit w bit: `tools/test_vapora.sh`. Częstotliwość: przy 96 kHz filtr, LFO i wygładzanie (liczone na blok/próbkę) ~9%
szybciej względem 88,2 kHz oryginału.
