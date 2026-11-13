# Dziennik DAW-POL 3000

## 2026-11-13 11:00 — FELT: koniec z narastaniem („samooscylacją”)
- **Co zmienione:** `core/synth/felt_dsp.c` — przyczyna: impuls młotka miał składową stałą, która krążyła w pętli struny i rosła z kolejnymi uderzeniami (DC do 1/3 skali, sygnał po 19 s głośniejszy niż po 1 s), a rezonatory płyty miały w basie wzmocnienie ~40×. Poprawki: impuls bez składowej stałej, filtr DC na wyjściu, rezonatory pasmowe o stałym wzmocnieniu, ponowne uderzenie trzymanej nuty pochłania 40% drgań (młotek), uderzenie po puszczeniu klawisza zaczyna od wyciszonej struny (tłumik), łagodne kolano nigdy nie przekracza pełnej skali, poziom wyjścia dopasowany. Po co: zgłoszenie Patryka.
- Testy: trzymane nuty 20 s w całym zakresie i skrajnych ustawieniach — brak DC, wzrost ograniczony do ~1,7× tylko przy ciągłym uderzaniu tej samej trzymanej nuty (jak w fortepianie); granie z puszczaniem + szybka modulacja DECAY / DETUNE / CORDA — bez narastania; bez przesterów; strój bez zmian. Wgrane na konsolę (razem z pokrywą na DECAY i modulacją DTUN / WORN / BODY).
- **Następny krok:** Patryk sprawdza FELT na konsoli. **Od Patryka:** czy problem zniknął.

## 2026-11-13 10:20 — FELT: DECAY otwiera pokrywę, LFO / MOD ENV na DETUNE, WORN, BODY
- **Co zmienione:** `core/ui/felt_screen.cpp` — kąt pokrywy zależy od DECAY (krótko = prawie zamknięta, długo = otwarta na podpórce; LFO na DECAY ją porusza); `core/synth/wavetable_synth.cpp`, `core/ui/synth_view.cpp`, `core/seq/project.cpp` — cele modulacji DTUN / WORN / BODY (LFO 1, LFO 2, MOD ENV) dla strony 2 FELT. Po co: prośba Patryka.
- **Następny krok:** Patryk testuje. **Od Patryka:** ocena.

## 2026-11-13 03:50 — FELT: rozstrojenie strun, rozstrojone pianino, więcej fortepianu
- **Co zmienione:** `core/synth/felt_dsp.*` — strona 2 (L3): DETUNE (rozstrojenie strun nuty 0,1–14 c, oddzielone od CORDA), WORN (każdy klawisz na stałe inaczej rozstrojony: większość lekko, kilka wyraźnie, struny nuty też się rozjeżdżają), BODY (płyta rezonansowa + puknięcie w ataku); brzmienie bardziej fortepianowe: twardszy i jaśniejszy młotek przy forte, szersza dynamika, mniej tłumienia wysokich alikwotów, 6 rezonansów płyty. `core/ui/felt_screen.*` — stroik (wskazówka = odchylenie ostatniego klawisza, czerwona gdy fałszuje; cienkie = rozstrojenie strun), struny rozchodzą się wachlarzem i drgają z różną prędkością (dudnienie), przy WORN chwiejne kołki i czerwone załamania. `synth_view` (L3 DETUNE / WORN / BODY), zapis `push`, domyślne, presety (HONKY TONK v2 + OLD UPRIGHT, SALOON, FORGOTTEN ROOM). Po co: prośba Patryka.
- Strój bez WORN dokładny; bez przesterów; CPU ~11% rdzenia. Testy bez zmian. Wgrane na konsolę.
- **Następny krok:** Patryk ocenia, czy FELT brzmi bardziej jak fortepian. **Od Patryka:** ocena.

## 2026-11-13 03:10 — utwór demo SYNTHWAVE
- **Co zmienione:** `core/seq/demo_song.*` — nowy utwór `buildDemoSynthwave`: 16 taktów w pętli, 104 BPM, a-moll (Am F C G); GIN (LINN + TONIC), PARALLAX (bas oktawowy, arpeggio, melodia), JUNI (pad ENSEMBLE), OXBOW (akordy brass); aranżacja: wstęp, bębny + bas, melodia, kulminacja. `core/ui/app.*` — dodawany raz do listy PROJECT (znacznik `_demo_synthwave_v1`); `core/headless.cpp` — `--render-wav plik 37 synthwave`.
- Szczyt miksu 0,86, na konsoli ~24% rdzenia (render 37 s w 8,7 s). Wgrane na konsolę.
- **Następny krok:** Patryk słucha SYNTHWAVE (PROJECT → SYNTHWAVE). **Od Patryka:** ocena.

## 2026-11-13 02:40 — silnik FELT (własny model fortepianu z filcem)
- **Co zmienione:** nowy silnik FELT — `core/synth/felt_dsp.*` (falowody strun ze sztywnością, młotek z filcem, grzebień miejsca uderzenia, płyta rezonansowa, mechanika i tłumik; strojenie poprawione o opóźnienia fazowe), `core/ui/felt_screen.*` (przekrój fortepianu: pokrywa, świeca, struny wg CORDA, młotek z warstwami filcu uderzający w struny, tłumik, klawisz, pył mechaniki), wpięcie (rodzaj 26, tłumik po puszczeniu klawisza), projekt, zapis, widok SYNTH, karta ENGINE, 16 presetów, CMake, podglądy, testy.
- Strój sprawdzony widmem: ±2 centy (1–3 struny, cały zakres). Brak przesterów w przeglądzie wszystkich ustawień (łagodne kolano na wyjściu). CPU na konsoli: ~10% rdzenia (6 głosów × 3 struny). Wszystkie 24 testy zgodności bez zmian. Wgrane na konsolę.
- **Następny krok:** Patryk testuje FELT (filc, una corda → tre corde, odgłosy mechaniki). **Od Patryka:** ocena brzmienia i ekranu.

## 2026-11-13 01:50 — LOUNGE: poprawki ekranu
- **Co zmienione:** `core/ui/lounge_screen.cpp` — usunięte kratki VOICES i dolny pasek z podpisami (napisy wchodziły na obramowanie); obudowa z zaokrąglonymi dolnymi rogami; podpisy HAMMER i PICKUP przy samych elementach w środku obudowy (CLAV: rozsunięte, bez nachodzenia); `core/ui/dialogs.cpp` — w opisie silnika bez „6 VOICES”. Po co: uwagi Patryka.
- **Następny krok:** Patryk ocenia ekran LOUNGE.

## 2026-11-13 01:30 — silnik LOUNGE (własne modelowane klawisze w stylu KEYS z „OP-1 Mods”)
- **Co zmienione:** nowy silnik LOUNGE — `core/synth/lounge_dsp.*` (własne modele fizyczne: ząb Rhodesa z przetwornikiem magnetycznym, języczek Wurlitzera z przetwornikiem elektrostatycznym, struna clavu Karplus-Strong; głos 48 kHz → 96 kHz), `core/ui/lounge_screen.*` (własny rysunek przekroju w układzie zrzutu KEYS: DECAY, LID z koktajlem, młotek, drgania, przetwornik, głosy), wpięcie (rodzaj 25), projekt, zapis, widok SYNTH (SOUND / AMP ENV / MOD ENV / 2 LFO), karta ENGINE, 16 presetów, CMake, podglądy, testy (`tools/gin_objs.sh`). Clean room: kod KEYS niepubliczny (repo j3threejay/op1-mod = tylko łatka binarna bez licencji).
- Strój sprawdzony (TINE i CLAV ±1 cent). CPU na konsoli: 5,3–5,8% rdzenia (6 głosów). Wszystkie 24 testy zgodności bez zmian. Wgrane na konsolę.
- **Następny krok:** Patryk testuje LOUNGE. **Od Patryka:** ocena brzmień i ekranu.

## 2026-11-13 00:40 — silnik PARALLAX (własny, w stylu „dimension” z Opus)
- **Co zmienione:** nowy silnik PARALLAX — `core/synth/parallax_dsp.*` (własny DSP od zera: fala PULSE/SQUARE/SAW/SAW+SUB/NOISE płynnie, unison Juno 3 oscylatory, filtr drabinkowy w stylu 303; głos przy 48 kHz → 96 kHz), `core/ui/parallax_screen.*` (ekran wg zrzutu wzorca: koło fal, oscyloskop XY z cieniem unison, szczyt rezonansu), wpięcie w silnik głosów (rodzaj 24), projekt, zapis, widok SYNTH (strony jak SID z obwiedniami i 2 LFO), kartę ENGINE, 22 presety, CMake (`-O3 -ffast-math`), podgląd headless; `tools/gin_objs.sh` linkuje też PARALLAX w testach. Clean room: kod „dimension” nie jest publiczny — tylko opis („subtraktywny z filtrem 303 i unisonem Juno”) i zrzut ekranu; nazwa własna (Dimension = silnik TE OP-1 Field).
- CPU na konsoli: 12,4% rdzenia (6 głosów, pełny unison, wysoki rezonans). Filtr stabilny w całym zakresie. Wszystkie 24 testy zgodności bez zmian. Wgrane na konsolę.
- **Następny krok:** Patryk testuje PARALLAX (brzmienie, ekran). **Od Patryka:** ocena, ew. nazwa.

## 2026-11-12 23:50 — GIN: stałe mapowanie klawiszy
- **Co zmienione:** `core/synth/gin_dsp.c/.h` — 24 dźwięki zestawu na stałe na C2..B3 (każdy ma swój jeden klawisz); nuty poza tym zakresem milczą (wcześniej co 24 półtony mapowanie się powtarzało). Opis w `CLAUDE.md`, `docs/ARCHITEKTURA.md`. Po co: prośba Patryka.
- **Następny krok:** Patryk testuje GIN. **Od Patryka:** ocena.

## 2026-11-12 23:30 — silnik GIN (perkusja, port 1:1 z paczki Hundekraft 3.1.4)
- **Co zmienione:** nowy silnik GIN — `third_party/kennel_gin/` (oryginalne źródła SquareWave Studio / libpo32 / kennel bez zmian), `core/synth/gin_dsp.*` (wrapper: zestaw, przydział 12 głosów, 88,2 → 48 kHz, dynamika), `core/ui/gin_screen_c.c` + `ginscreen.*` + `gin_shim/` (oryginalny ekran butelek, napisy naszą czcionką — nowa funkcja `textStrokes` w `gfx/font.cpp`), `Track.gin` + zapis `gink`/`gins` w projekcie i presetach, kategoria presetów DRUMS (ikona bębna), widok SYNTH (11 stron), karta ENGINE, 8 zestawów jako presety, podglądy headless, CMake; test `tools/test_gin.sh` (referencja = oryginalny `gin_fw.c` ze słownikiem współczynników z ich skryptu) + `tools/gin_objs.sh` w pozostałych testach. Po co: Patryk poprosił o GIN z własną czcionką.
- Zgodność bit w bit: 0 różnic (430 tys. próbek, 8 zestawów) na PC i na konsoli; pozostałe 23 testy też 0 różnic. CPU na konsoli: 5,9% rdzenia (gęste uderzenia z dynamiką). Wgrane na konsolę.
- **Następny krok:** Patryk testuje GIN (granie z przycisków, piano roll od C2, strony edytora, MORPH, A/B). Do decyzji: zmiana nazw dźwięków (w oryginale jest), mapowanie klawiszy.
- **Od Patryka:** ocena „git / nie git”.

## 2026-11-12 21:40 — silnik OUTATIME (port 1:1 z paczki Hundekraft 3.1.4)
- **Co zmienione:** nowy silnik OUTATIME — `third_party/kennel_outatime/` (oryginalny kod opus/Hundekraft bez zmian), `core/synth/outatime_dsp.*` (wrapper), `core/ui/outatime_screen*` (ekran DeLoreana), wpięcie w `wavetable_synth`, `engine`, `project`, `project_file`, `synth_view`, `dialogs`, `app.h`, `factory_presets` (12 presetów: 4 z oryginału + 7 nowych + INIT), `headless`, `CMakeLists.txt`; test `tools/test_outatime.sh` (+ obiekt OUTATIME dopisany do pozostałych skryptów testowych); dokumentacja i licencje. Po co: Patryk poprosił o OUTATIME.
- Zgodność bit w bit z oryginałem: 0 różnic (wszystkie 23 testy przechodzą). CPU na konsoli: 9,1% rdzenia przy 6 głosach. Wgrane na konsolę.
- **Następny krok:** Patryk testuje OUTATIME; decyzja o GIN (syntezator perkusji z zestawami — inna architektura niż silniki klawiszowe).
- **Od Patryka:** ocena brzmienia/ekranu + odpowiedź w sprawie GIN.

## 2026-11-12 21:08
**Zmienione — port silnika JUNI (op1r, Hundekraft), 1:1:**
- `third_party/op1r_juni/` — oryginalne `juni.c`, geometria, tablice, `juni.svg`, test hosta, generatory, `plugin.json`, `presets.py`, LICENSE, NOTICE, README; zaślepka `op1_plugin.h`; `juni_art.h`.
- `core/synth/juni_dsp.*` — juni.c jako C, stan pliku per ścieżka; `core/synth/wavetable_synth.*`, `core/audio/engine.*` — rodzaj 21, sam silnik, obwiednia AMP ENV prowadzi też filtr (SWEEP), wyjście −3 dB (głośny: akordy by przesterowały).
- `core/ui/juni_screen*` — ekran fiordu 1:1 (druga kompilacja) + ilustracja; `core/ui/synth_view.*` — TONE / SHAPE (5 mieszanek) / SWEEP / CHORUS (OFF, I, II, I+II), L3: RESONANCE / HPF / PWM RATE; `core/ui/dialogs.cpp` — JUNI na liście.
- `core/seq/project.*`, `core/io/project_file.cpp`, `core/io/factory_presets.cpp`, `core/ui/app.h` — silnik `juni`, 13 presetów (12 z oryginału).
- OUTATIME: nie ma go w op1r-src (aktualne repo) ani w wyszukiwarce — potrzebny link / plik od Patryka.
- `tests/juni_match.cpp`, `tools/test_juni.sh`; wszystkie skrypty testów linkują JUNI.
- Testy: JUNI bit w bit (0 różnic), wszystkie 22 testy 0 różnic; CPU na konsoli 7% przy 6 głosach z chorusem. Wysłane na konsolę.
**Następny krok:** Patryk testuje JUNI; link do OUTATIME.
**Wymaga od Patryka:** tak — odsłuch, link do OUTATIME.

## 2026-11-12 20:56
**Zmienione — port silnika DDDD (op1r, pcppcp), 1:1:**
- `third_party/op1r_dddd/` — oryginalne `plugins/dddd/dddd.c`, `dddd.svg`, `plugin.json`, `dddd/dd_core.c/h`, LICENSE, NOTICE, README; zaślepki `op1_plugin.h`, `op1_synth.h`.
- `core/synth/dddd_dsp.*` — dddd.c jako C, cały stan (ściana szumu z pętlą, kolejność nut) per ścieżka; `core/synth/wavetable_synth.*`, `core/audio/engine.*` — rodzaj 20, sam silnik, obwiednia AMP ENV otwiera filtr.
- `core/ui/dddd_screen*` — ekran osy 1:1 (druga kompilacja; bez dolnego rzędu z napisami) + ilustracja; `core/ui/synth_view.*` — SOURCE (NOISE/CRACKLE/CHAOS/METAL) / FEEDBACK / DESTROY (FOLD/CLIP/CRUSH) / HANDS, L3: FILTER (LP/BP/HP) / SPREAD / RATE (1/n); `core/ui/dialogs.cpp` — DDDD na liście.
- `core/seq/project.*`, `core/io/project_file.cpp`, `core/io/factory_presets.cpp`, `core/ui/app.h` — silnik `dddd`, 12 presetów (oryginał nie ma żadnych).
- `tests/dddd_match.cpp`, `tools/test_dddd.sh`; wszystkie skrypty testów linkują DDDD.
- Testy: DDDD bit w bit (0 różnic), wszystkie 21 testów 0 różnic; CPU na konsoli ok. 5% (ściana gra jeden głos). Wysłane na konsolę.
**Następny krok:** Patryk testuje DDDD.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-12 20:26
**Zmienione (prośba Patryka): ARC dostał MOD ENV i jedno LFO.**
- `core/ui/synth_view.*` — strony ARC: SOUND, AMP ENV, MOD ENV, LFO, PRESETS; cele modulacji: CUR, CARB, HISS, STRK, PTCH, VOL oraz RAT, CPL, BRT (strona 2: proporcja 2. lampy, sprzężenie, jasność).
- `core/synth/wavetable_synth.cpp` — modulacja słów 5..7 ARC per głos; `core/audio/engine.cpp` — bez drugiego LFO i trybów głosu; `core/seq/project.cpp` — nazwy celów; `core/ui/dialogs.cpp` — opis na karcie ENGINE.
- Bez modulacji nadal bit w bit. Wysłane na konsolę.
**Następny krok:** Patryk testuje ARC z modulacją.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-12 20:13
**Zmienione — port silnika ARC (op1r, ultralazr), 1:1; lista silników alfabetycznie:**
- `core/ui/dialogs.cpp` — lista silników na karcie ENGINE (okno TRACK / NEW TRACK) w kolejności alfabetycznej (indeksy silników bez zmian; strzałki i przewijanie po nowej kolejności).
- `third_party/op1r_arc/` — oryginalne `arc.c`, tablice, kalibracja, geometria, `duddell.svg`, generatory, `plugin.json`, LICENSE, NOTICE, README; zaślepki `op1_plugin.h`, `seq/seq_text.h`; `duddell_art.h`.
- `core/synth/arc_dsp.*` — arc.c jako C, stan głosu i dane ekranu per ścieżka; `core/synth/wavetable_synth.*`, `core/audio/engine.*` — rodzaj 19, sam silnik (AMP ENV), wyjście −3 dB (był ok. 3 dB głośniejszy od reszty).
- `core/ui/arc_screen*` — ekran 1:1 (portret fazowy, elektrody, łuk, iskry; bez tarcz) + ilustracja; `core/ui/synth_view.*` — CURRENT / CARBON / HISS / STRIKE, L3: RATIO (zapadki 1:2 … 2:1) / COUPLING / BRIGHTNESS.
- `core/seq/project.*`, `core/io/project_file.cpp`, `core/io/factory_presets.cpp`, `core/ui/app.h` — silnik `arc`, 13 presetów (oryginał nie ma żadnych).
- `tests/arc_match.cpp`, `tools/test_arc.sh`; wszystkie skrypty testów linkują ARC.
- Testy: ARC bit w bit (0 różnic), wszystkie 20 testów 0 różnic; CPU na konsoli 9,5% przy 6 głosach (z 2. lampą). Wysłane na konsolę.
**Następny krok:** Patryk testuje ARC i kolejność listy.
**Wymaga od Patryka:** tak — odsłuch / ocena.

## 2026-11-12 19:56
**Zmienione — port silnika AETHER (op1r, pcppcp), 1:1 + wirtualny czujnik przechylenia (wybór Patryka: strona TILT + modulacja):**
- `third_party/op1r_aether/` — oryginalne `aether.c`, geometria, `aether.svg`, `orbit-1.svg`, generator, `plugin.json`, LICENSE, NOTICE, README; zaślepki `op1_plugin.h`, `op1_synth.h`; `aether_art.h`.
- `core/synth/aether_dsp.*` — aether.c jako C z czujnikiem wirtualnym (przechylenie przód/tył, bok, potrząsanie skalibrowane do detektora oryginału), stan per ścieżka; `core/synth/wavetable_synth.*`, `core/audio/engine.*` — rodzaj 18, TILT + modulacja (PTLT / VTLT / SHAK), MODE/GLIDE, MOD ENV, 2 LFO, wyjście −3 dB.
- `core/ui/aether_screen*` — ekran anteny 1:1 (druga kompilacja oryginału; bez tarcz, suwaków i etykiet) + ilustracja; `core/ui/synth_view.*` — nowa strona TILT, SOUND (ORBIT / FORM / SPACE / DEPTH, L3: DEAD ZONE / TILT RANGE / VIB RATE); `core/ui/dialogs.cpp` — AETHER na liście.
- `tools/gen/svg_to_art2.py` — polecenia ścieżki H / V.
- `core/seq/project.*`, `core/io/project_file.cpp`, `core/io/preset_store.cpp`, `core/io/factory_presets.cpp`, `core/ui/app.h` — silnik `aether`, token `tilt`, 11 presetów (oryginał nie ma żadnych).
- `tests/aether_match.cpp`, `tools/test_aether.sh`; wszystkie skrypty testów linkują AETHER.
- Testy: AETHER bit w bit (0 różnic, bez przechylenia jak harness), wszystkie 19 testów 0 różnic; CPU na konsoli 6,8% przy 6 głosach. Wysłane na konsolę.
**Następny krok:** Patryk testuje AETHER (strona TILT, presety SIREN / SPOOKY GLIDE / DIVE).
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-12 19:43
**Zmienione — port silnika BLOT (op1r, pcppcp), 1:1:**
- `third_party/op1r_blot/` — oryginalne `blot.c`, `blot_tables.h`, `blot.svg`, `gen_tables.py`, `plugin.json`, LICENSE, NOTICE, README; zaślepki `op1_plugin.h`, `op1_synth.h`.
- `core/synth/blot_dsp.*` — blot.c jako C (DSP + ekran), stan głosu per ścieżka; `core/synth/wavetable_synth.*`, `core/audio/engine.*` — rodzaj 17, AMP ENV + MODE/GLIDE (slide 303 to w oryginale portamento OP-1), bez MOD ENV / LFO, wyjście +6 dB (oryginał był ok. 6–9 dB ciszej od reszty).
- `core/ui/blot_screen.*` — ekran smileya 1:1 (bez dolnego rzędu z napisami) + ilustracja; `core/ui/synth_view.*` — CUTOFF (Hz) / RESONANCE / ENV MOD (oktawy) / DECAY (ms), L3: WAVE / ACCENT / DRIVE, strony SOUND / VOICE / AMP ENV / PRESETS; `core/ui/dialogs.cpp` — BLOT na liście.
- `core/seq/project.*`, `core/io/project_file.cpp`, `core/io/factory_presets.cpp`, `core/ui/app.h` — silnik `blot`, 15 presetów (oryginał nie ma żadnych).
- `tests/blot_match.cpp`, `tools/test_blot.sh`; wszystkie skrypty testów linkują BLOT.
- Testy: BLOT bit w bit (0 różnic, z akcentem), wszystkie 18 testów 0 różnic; CPU na konsoli 10,4% przy 6 głosach. Wysłane na konsolę.
**Następny krok:** Patryk testuje BLOT.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-12 19:33
**Zmienione — port silnika BLOODMOON (op1r, Nola Drift), 1:1 + AMP ENV:**
- `third_party/op1r_bloodmoon/` — oryginalne `bloodmoon.c`, `moon.c`, geometria, SVG, generator, `plugin.json`, LICENSE, NOTICE, README; zaślepki `op1_plugin.h`, `op1_synth.h`; `bloodmoon_art.h`.
- `core/synth/bloodmoon_dsp.*` — oba pliki jako C, stan głosu per ścieżka; `core/synth/wavetable_synth.*`, `core/audio/engine.*` — rodzaj 16, sam silnik (AMP ENV).
- `core/ui/bloodmoon_screen.*` — ekran 1:1 (księżyc, rozeta, dzwony, gwiazdy; bez dublujących napisów) + ilustracja; `core/ui/synth_view.*` — BLOOD / ECLIPSE / HAUNTING / MOONRISE (Hz); `core/ui/dialogs.cpp` — BLOODMOON na liście.
- `tools/gen/svg_to_art2.py` — koło z wypełnieniem i obrysem rysuje oba (wcześniej obrys ginął — księżyc był bez czerwonej obwódki).
- `core/seq/project.*`, `core/io/project_file.cpp`, `core/io/factory_presets.cpp`, `core/ui/app.h` — silnik `bloodmoon`, 9 presetów (8 autorki).
- `tests/bloodmoon_match.cpp`, `tools/test_bloodmoon.sh`; wszystkie skrypty testów linkują BLOODMOON.
- Testy: BLOODMOON bit w bit (0 różnic), wszystkie 17 testów 0 różnic; CPU na konsoli 5,7% przy 6 głosach. Wysłane na konsolę.
**Następny krok:** Patryk testuje BLOODMOON.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-12 19:23
**Zmienione — port silnika HOLLOW SAINT (op1r, build kennel), 1:1 bez dodatków:**
- `third_party/op1r_hollowsaint/` — oryginalne `hollow_saint.c`, geometria, `hollow_saint.svg`, `plugin.json`, `presets.py`, generator ekranu, LICENSE, NOTICE, README; zaślepka `op1_plugin.h`; `hollow_saint_art.h`.
- `core/synth/hollowsaint_dsp.*` — hollow_saint.c jako C z flagami kennel; cały stan oryginału (wspólny pogłos, taśma, burdon) osobno dla każdej ścieżki (podmiana przy zmianie ścieżki; sprawdzone: dwie ścieżki nie wpływają na siebie). Obwiednia z AMP ENV, zegar taktu z transportu (`core/synth/wavetable_synth.*`, rodzaj 15); `core/audio/engine.*` — sam silnik, RELIC z `Track::relic`.
- `core/ui/hollowsaint_screen.*` — ekran liry 1:1 (bez dublujących wskaźników gałek) + ilustracja; `core/ui/synth_view.*` — SOUND: DRONE / STRINGS / WHEEL BUZZ / CATHEDRAL, L3: DRONE KEY / RELIC AGE / STROKES / RELIC; `core/ui/dialogs.cpp` — HOLLOW SAINT na liście; zmiana silnika ustawia domyślną stronę 2 (`engineDefaultPush`).
- `core/seq/project.*`, `core/io/project_file.cpp`, `core/io/preset_store.cpp`, `core/io/factory_presets.cpp`, `core/ui/app.h` — silnik `hollowsaint`, token `relic`, 15 presetów (8 z oryginału + 6 z taśmą RELIC).
- `tests/hollowsaint_match.cpp`, `tools/test_hollowsaint.sh`; wszystkie skrypty testów linkują HOLLOW SAINT.
- Testy: HOLLOW SAINT bit w bit (0 różnic), wszystkie 16 testów 0 różnic; CPU na konsoli 6–7% przy 6 głosach. Demo bez zmian (16 ścieżek MIDI = komplet). Wysłane na konsolę.
**Następny krok:** Patryk testuje HOLLOW SAINT.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-12 19:11
**Zmienione — port silnika BRASSOPHONE (op1r, nvmsocool), 1:1 bez dodatków:**
- `third_party/op1r_brass/` — oryginalne `brass.c`, `horn.c`, `warp_table.h`, `brass-handmade.svg`, `plugin.json`, generator, LICENSE, NOTICE, README; zaślepki `op1_plugin.h`, `op1_synth.h`; `brass_art.h`.
- `core/synth/brass_dsp.*` — brass.c + horn.c jako C, stan głosu per ścieżka; silnik czyta obwiednię głośności głosu — podajemy wartość AMP ENV (`core/synth/wavetable_synth.*`, rodzaj 14); `core/audio/engine.*` — sam silnik (jak ELG).
- `core/ui/brass_screen.*` — ekran rogu 1:1 (tłumiki, suwak, balon, spinner) + ilustracja; `core/ui/synth_view.*` — SOUND: TIMBRE / SWEEP / VIBRATO / VIB RATE; `core/ui/dialogs.cpp` — BRASSOPHONE na liście.
- `core/seq/project.*`, `core/io/project_file.cpp`, `core/io/factory_presets.cpp`, `core/ui/app.h` — silnik `brass`, 12 presetów (oryginał nie ma żadnych); demo ALL ENGINES z partią HORN (v10; 16 ścieżek = komplet MIDI).
- `tests/brass_match.cpp`, `tools/test_brass.sh`; wszystkie skrypty testów linkują BRASSOPHONE.
- Testy: BRASSOPHONE bit w bit (0 różnic), wszystkie 15 testów 0 różnic; CPU na konsoli 6,8% przy 6 głosach; demo ~32% rdzenia, szczyt 0,89. Wysłane na konsolę.
**Następny krok:** Patryk testuje BRASSOPHONE (i MIXTUR z MOD ENV / LFO).
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-12 19:05
**Zmienione (prośba Patryka): MIXTUR dostał MOD ENV i jedno LFO.**
- `core/ui/synth_view.*` — strony MIXTUR: SOUND, AMP ENV, MOD ENV, LFO, PRESETS; cele modulacji: UT A..D (undertony), PTCH, VOL oraz F1, F2, RES (formanty ze strony 2).
- `core/synth/wavetable_synth.cpp` — modulacja słów 5..7 (F1 / F2 / RES) per głos; `core/audio/engine.cpp` — MIXTUR bez drugiego LFO i bez trybów głosu; `core/seq/project.cpp` — nazwy celów.
- Bez modulacji nadal bit w bit. Wysłane na konsolę.
**Następny krok:** port BRASSOPHONE 1:1.
**Wymaga od Patryka:** nie.

## 2026-11-12 18:59
**Zmienione — port silnika MIXTUR (op1r, ultralazr), wybór Patryka: 1:1 bez dodatków:**
- `third_party/op1r_mixtur/` — oryginalne `mixtur.c`, `mixtur_dsp.h`, tablice, geometria, `mixtur.svg`, generatory, LICENSE, NOTICE, README; zaślepki `op1_plugin.h`, `seq/seq_text.h`; `mixtur_art.h`.
- `core/synth/mixtur_dsp.*` — mixtur.c jako C (DSP + ekran), pamięć głosu per ścieżka; `core/synth/wavetable_synth.*`, `core/audio/engine.*` — rodzaj 13, sam silnik (tylko AMP ENV, jak ELG).
- `core/ui/mixtur_screen.*` — ekran drabiny undertonów 1:1 (bez dublujących napisów) + ilustracja; `core/ui/synth_view.*` — SOUND: UNDERTONE A..D krokowo (OFF, 1/1 … 1/24, „=” przy powtórzonym — wyciszony jak w oryginale), L3: FORMANT 1 / FORMANT 2 / RESONANCE; `core/ui/dialogs.cpp` — MIXTUR na liście.
- `core/seq/project.*`, `core/io/project_file.cpp`, `core/io/factory_presets.cpp`, `core/ui/app.h` — silnik `mixtur`, 14 presetów (oryginał nie ma żadnych); demo ALL ENGINES z partią BIRDS (v9).
- Poprawka VAPORA: wejście w L3 nie zmienia już SPEED „0 = domyślne” na minimum bez ruszania gałki (ten sam błąd wyłapany przy MIXTUR).
- `tests/mixtur_match.cpp`, `tools/test_mixtur.sh`; wszystkie skrypty testów linkują MIXTUR.
- Testy: MIXTUR bit w bit (0 różnic), wszystkie 14 testów 0 różnic; CPU na konsoli ~6% przy 6 głosach; demo ~31% rdzenia. Wysłane na konsolę.
**Następny krok:** Patryk testuje MIXTUR.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-12 18:48
**Zmienione (uwaga Patryka):** `core/ui/synth_view.cpp` — odczyt OCTAVE w SYNTH widoczny tylko, gdy oktawa jest przesunięta (L1/R1, np. „OCT −1”); przy 0 nic nie jest wyświetlane. Wysłane na konsolę.
**Następny krok:** Patryk testuje SUPERSAW.
**Wymaga od Patryka:** tak — ocena.

## 2026-11-12 18:46
**Zmienione (uwaga Patryka):** `core/ui/synth_view.*` — filtr SUPERSAW na osobnej stronie FILTER zaraz po SOUND (CUTOFF / RESONANCE / TYPE + wykres charakterystyki filtra), zamiast pod L3 na SOUND. Strony: SOUND, FILTER, VOICE, AMP ENV, FILTER ENV, MOD ENV, LFO 1, LFO 2, PRESETS. Wysłane na konsolę.
**Następny krok:** Patryk sprawdza stronę FILTER w SUPERSAW.
**Wymaga od Patryka:** tak — ocena.

## 2026-11-12 18:43
**Zmienione — filtr w SUPERSAW (wybór Patryka: opcja 1, filtr w samym silniku):**
- `core/synth/wavetable_synth.*` — filtr DAW-POL za oryginalnym silnikiem: SVF (LP24 / LP12 / BP / HP), rezonans do Q 10 z kompensacją głośności, wygładzanie; obwiednia filtra i cele LFO/MOD ENV CUT/RES. Filtr w pełni otwarty bez rezonansu = dźwięk oryginału bit w bit (test 0 różnic).
- `core/ui/synth_view.*` — L3 na SOUND = CUTOFF (OPEN na końcu) / RESONANCE / TYPE; strona FILTER ENV; LFO i MOD ENV z celami CUT i RES. `core/audio/engine.cpp`, `core/io/project_file.cpp`, `core/seq/project.*` — parametry filtra w `push` (zamknięcie, RES, typ; stare projekty = otwarty).
- `core/ui/dialogs.cpp` — zmiana silnika zeruje stronę 2 (`push`), żeby nie przechodziły wartości poprzedniego silnika.
- `core/io/factory_presets.cpp` — 9 presetów z filtrem (v3): FILTER PLUCK, TRANCE PLUCK, SWEEP PAD, PUMP PAD, THIN AIR, ACID SAW, WARM BASS, WAH LEAD, FILTER RISE; demo: SAWS gra TRANCE PLUCK (v8).
- CPU na konsoli: 8,3% przy 6 głosach (filtr +2,5%). Wysłane na konsolę.
**Następny krok:** Patryk sprawdza filtr w SUPERSAW.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-12 18:45
**Zmienione (uwagi Patryka do SUPERSAW):**
- `core/ui/supersaw_screen.*` — usunięte zdublowane tarcze (rząd gałek OP-1 z oryginalnego ekranu; mamy własne gałki na dole), piła przesunięta niżej, ilustracja bez tarcz.
- `core/ui/synth_view.cpp`, `core/audio/engine.cpp` — VOICE w SUPERSAW tylko MODE i GLIDE (UNISON i AGE usunięte, w dźwięku wyłączone); `core/ui/dialogs.cpp` — opis bez UNISON/AGE.
- `core/io/factory_presets.cpp` — presety używające UNISON/AGE poprawione (wersja 2 zastępuje stare: TRANCE LEAD, HOOVER, SCREAM LEAD, GLIDE LEAD, SUPER PAD, STACK PAD, DRIFT PAD — dryf z LFO, HYPER CHORD, REESE BASS).
- Wysłane na konsolę; SUPERSAW nadal bit w bit.
**Następny krok:** Patryk sprawdza SUPERSAW.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-12 18:32
**Zmienione — port silnika SUPERSAW (op1r, pcppcp), standardowo: 1:1 + pakiet modulacji + VOICE, własne presety:**
- `third_party/op1r_supersaw/` — oryginalne `supersaw.c`, `supersaw_geom.h`, `supersaw.svg`, `plugin.json`, `gen_svg.py`, LICENSE, NOTICE, README; zaślepka `op1_plugin.h`; `supersaw_art.h` (rysunek ze SVG).
- `tools/gen/svg_to_art2.py` — obsługa krzywych Béziera i elementów z własnym id (potrzebne do rysunku piły).
- `core/synth/supersaw_dsp.*` — supersaw.c jako C (DSP + ekran); `core/synth/wavetable_synth.*`, `core/audio/engine.*` — rodzaj 12, wyjście −3 dB (gęste piły z przesterem, jak MONOLIT).
- `core/ui/supersaw_screen.*` — ekran piły łańcuchowej 1:1 + ilustracja; `core/ui/synth_view.*` — SOUND (DETUNE w %, MORPH SAW/MIX/SQR, SUB, DRIVE 1–5X), VOICE z UNISON i AGE, AMP ENV, MOD ENV, 2 LFO; `core/ui/dialogs.cpp` — SUPERSAW na liście.
- `core/seq/project.*`, `core/io/project_file.cpp`, `core/io/factory_presets.cpp`, `core/ui/app.h` — silnik `supersaw`, 25 presetów (oryginał nie ma żadnych); demo ALL ENGINES z partią SAWS (akordy na off-beat), wersja 7.
- `tests/supersaw_match.cpp`, `tools/test_supersaw.sh`; wszystkie skrypty testów linkują SUPERSAW.
- Testy: SUPERSAW bit w bit (0 różnic), wszystkie 13 testów 0 różnic; CPU na konsoli 5,8% przy 6 głosach; demo ~29% rdzenia, szczyt 0,89. Wysłane na konsolę.
**Następny krok:** Patryk testuje SUPERSAW.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-12 18:40
**Zmienione (uwaga Patryka):** `core/ui/dialogs.cpp` — lista silników na karcie ENGINE znowu w normalnym rozmiarze (jak na początku), przewijana: 6 pozycji naraz, zaznaczenie zawsze widoczne, strzałki ^ / v gdy jest coś wyżej/niżej. Podpis SID: FENV zamiast AMP. Wysłane na konsolę.
**Następny krok:** Patryk testuje VAPORA, OCTAVE i listę; ewentualnie build pod Windows (wymaga jednej komendy z hasłem sudo).
**Wymaga od Patryka:** tak — odsłuch / ocena.

## 2026-11-12 18:15
**Zmienione — port silnika VAPORA (op1r, AiAAAA), wybór Patryka: 1:1 + modulacja + FILTER ENV, presety 15 + 15, OCTAVE dla wszystkich silników:**
- `third_party/op1r_vapora/` — oryginalne `vapora.c`, `vapora_dsp.c/.h`, tablice PADsynth, kod ekranu `vapora_screen.c` z nagłówkami, `vapora.svg`, `plugin.json`, `presets.json`, LICENSE, NOTICE, README; zaślepki `op1_plugin.h`, `op1_synth.h`, `seq/seq_text.h`.
- `core/synth/vapora_dsp.*` — vapora.c jako C, stan głosu per ścieżka; `core/synth/wavetable_synth.*`, `core/audio/engine.*` — rodzaj 11, FILTER ENV na BRIGHTNESS, wyjście +9,5 dB (oryginał był ~10 dB ciszej od reszty).
- `core/ui/vapora_screen*` — ekran = oryginalny kod C rysujący na naszym Canvas (ocean, słońce, gwiazdy, fala przy nucie) + ilustracja na karcie ENGINE; `core/ui/synth_view.*` — SOUND (MORPH/THICKNESS/BRIGHTNESS/MOTION, L3: CHARACTER/INTERVAL/SUN/SPEED, krokowo), strony jak SID; `core/ui/dialogs.cpp` — VAPORA na liście (14 pozycji).
- OCTAVE: `Track::octave` (−3..+3), L1/R1 w SYNTH dla każdego silnika, odczyt „OCT” obok nazwy strony, zapis `oct` w projekcie i presecie (`core/seq/project.h`, `core/io/project_file.cpp`, `core/synth/saw_synth.*`).
- `core/io/factory_presets.cpp`, `core/io/preset_store.cpp` — 31 presetów VAPORA; MONOLIT v3: LADDER BASS, PLUCK BASS, SUB DROP, LADDER PING z oktawą −1 jak w oryginale (wpis nowszej wersji zastępuje stary preset o tej samej nazwie).
- `core/seq/demo_song.cpp`, `core/ui/app.cpp` — demo ALL ENGINES z partią TIDE (VAPORA, wstęp i zakończenie), wersja 6.
- `tests/vapora_match.cpp`, `tools/test_vapora.sh`; wszystkie skrypty testów linkują VAPORA.
- Testy: VAPORA bit w bit (0 różnic), wszystkie 12 testów 0 różnic; CPU na konsoli 5,5% przy 6 głosach; demo ~27% rdzenia, szczyt 0,89. Wysłane na konsolę.
**Następny krok:** Patryk testuje VAPORA i OCTAVE (L1/R1 w SYNTH).
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-13 19:00
**Zmienione (uwagi Patryka do MONOLIT):**
- `core/ui/synth_view.*`, `core/audio/engine.cpp`, `core/synth/wavetable_synth.cpp` — MONOLIT ma stronę FILTER ENV (wspólna obwiednia filtra na CUTOFF = gałka 1, AMOUNT ± pod L3, działa obok CONTOUR); strona VOICE tylko MODE i GLIDE (UNISON i AGE usunięte, w dźwięku wyłączone).
- `core/ui/widgets.cpp` — puste pozycje paska gałek nie są rysowane (MONOLIT VOICE, PEW BARS).
- MONOLIT bez obwiedni filtra nadal bit w bit; WAVETABLE, TAFFY bez zmian. Wysłane na konsolę.
**Następny krok:** Patryk sprawdza MONOLIT.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-13 18:30
**Zmienione — port silnika MONOLIT (op1r, Hundekraft), wybór Patryka: 1:1 + modulacja + VOICE, presety 12 + 20:**
- `third_party/op1r_monolit/` — oryginalne `monolit.c`, `monolit_tables.h`, `monolit.svg`, `plugin.json`, `presets.py`, LICENSE, NOTICE, README; zaślepki `op1_plugin.h`, `screens/stroke_font.h`; `monolit_art.h`.
- `core/synth/monolit_dsp.*` — monolit.c jako C, stan głosu per ścieżka; `core/synth/wavetable_synth.*`, `core/audio/engine.*` — rodzaj 10, note_off = wybrzmienie CONTOUR, VOICE bez AGE, wyjście −3 dB (oryginał bez trymu był za głośny).
- `core/ui/monolit_screen.*` — ekran monolitu 1:1 + ilustracja; `core/ui/synth_view.*` — SOUND (SHAPE/CUTOFF/CONTOUR/HEAT, L3: BODY/VEL/AGE), VOICE z AGE wyłączonym; `core/ui/dialogs.cpp` — MONOLIT na liście (13 pozycji, ciaśniej).
- `core/seq/project.*`, `core/io/project_file.cpp`, `core/io/factory_presets.cpp`, `core/ui/app.h` — silnik `monolit`, 32 presety; demo ALL ENGINES z partią ACID (MONOLIT, MONO + glide), wersja 5.
- `tests/monolit_match.cpp`, `tools/test_monolit.sh`; wszystkie skrypty testów linkują MONOLIT.
- Testy: MONOLIT bit w bit (0 różnic, z wybrzmieniem po note_off), wszystkie 11 testów 0 różnic; presety bez przesterowania; CPU na konsoli 13% przy 6 głosach (najcięższy silnik); demo ~24% rdzenia, szczyt 0,89. Wysłane na konsolę.
**Następny krok:** Patryk testuje MONOLIT.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-13 17:00
**Zmienione (uwagi Patryka do SID):**
- `core/ui/synth_view.*`, `core/audio/engine.cpp` — SID ma stronę FILTER ENV (wspólna obwiednia filtra na CUTOFF, AMOUNT ± pod L3); strony: SOUND, AMP ENV, FILTER ENV, MOD ENV, LFO 1, LFO 2, PRESETS.
- `core/ui/synth_view.cpp` — gałka WAVE w SID krokowa (TRIANGLE … RING, jedno wychylenie = jedna fala, bez zawijania); wcześniej fale powtarzały się co 512 jednostek i analog przelatywał przez nie lawinowo. `core/synth/wavetable_synth.cpp` — modulacja WAVE (LFO / MOD ENV) też skokowo o całe fale.
- SID bez obwiedni filtra nadal bit w bit. Wysłane na konsolę.
**Następny krok:** Patryk sprawdza SID.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-13 16:30
**Zmienione — port silnika SID (op1r; cRSID 1.58 Hermita, adaptacja Hundekraft):**
- `third_party/op1r_sid/` — oryginalne `sid.c`, `sid_dsp.c/.h`, tablice filtra i szumu, `sid_screen.c`, `sid.svg`, `plugin.json`, `presets.py`, LICENSE, NOTICE, README; zaślepki `op1_plugin.h`, `op1_l1.h`; `sid_art.h`.
- `core/synth/sid_dsp_wrap.*` — sid.c jako C (stan głosu per ścieżka), `sid_dsp.c` osobno; `core/synth/wavetable_synth.*`, `core/audio/engine.*` — SID jako rodzaj 9 z pakietem modulacji (AMP ENV, MOD ENV, 2 LFO).
- `core/ui/sid_screen.*` — ekran (rysunek układu SID + odczyty WAVE / VOICES-WIDTH-INTERVAL / CUTOFF / RES) i ilustracja; `core/ui/synth_view.*` — strona SOUND; `core/ui/dialogs.cpp` — SID na liście (12 pozycji, lista ciaśniej).
- `core/seq/project.*`, `core/io/project_file.cpp`, `core/io/factory_presets.cpp`, `core/ui/app.h` — silnik `sid`, 20 presetów (8 z oryginału + 12 z modulacją); demo ALL ENGINES z partią CHIP (SID, arpeggio), wersja 4.
- `tests/sid_match.cpp`, `tools/test_sid.sh`; pozostałe skrypty testów linkują SID. `CMakeLists.txt` — sid_dsp.c bez ostrzeżeń o wcięciach (oryginał 1:1).
- Testy: SID bit w bit (0 różnic), wszystkie 10 testów zgodności 0 różnic; presety bez przesterowania; CPU na konsoli 6,3–7,8% przy 6 głosach; demo ~22% rdzenia, szczyt 0,87. Wysłane na konsolę.
**Następny krok:** Patryk testuje SID.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-13 15:30
**Zmienione:** `tools/package_portmaster.sh` (nowy) i `tools/portmaster/DAW-POL 3000.sh` — paczka ZIP dla PortMastera (do pokazania koledze): starter szukający PortMastera jak inne porty, folder `dawpol3000/` z programem, `port.json`, README (instalacja, sterowanie) i licencje. Wynik w `dist/` (poza gitem) i kopia na konsoli w `/userdata/dawpol3000/`. Sprawdzone na konsoli w katalogu tymczasowym (rozpakowanie, PortMaster znaleziony, program startuje); obecna instalacja nienaruszona.
**Następny krok:** Patryk przekazuje paczkę koledze.
**Wymaga od Patryka:** tak — skopiowanie pliku.

## 2026-11-13 15:00
**Zmienione:** `core/ui/oxbow_screen.*`, `core/ui/synth_view.cpp` — ekran OXBOW+ (uwagi Patryka): pod bykiem zamiast ENV jest typ filtra (biały), zamiast DETUNE — ENV>PITCH (czerwony); ptaki chodzą po rogach z ENV>PITCH (środek = 0), oddech (biały) rośnie z typem filtra (LP12 najmniejszy … SEM największy). Zwykły OXBOW bez zmian. Wysłane na konsolę.
**Następny krok:** Patryk sprawdza ekran OXBOW+.
**Wymaga od Patryka:** tak — ocena.

## 2026-11-13 14:30
**Zmienione — OXBOW+ (rozszerzona wersja OXBOW, jak WAVETABLE+):**
- `third_party/kennel_oxbow/plugins/oxbow/oxbow_plus.c`, `oxbow_plus.h` — zmodyfikowana kopia oxbow.c: stałe neutralnego głosu → parametry ścieżki (2 niezależne oscylatory: fala, interwał, strojenie, poziom, PW; szum, SYNC, X-MOD, ENV→P2, keytrack, velocity, VINTAGE, ADSR obwiedni filtra). Oryginalny oxbow.c bez zmian.
- `core/synth/oxbow_plus_dsp.*` — opakowanie (stan ścieżki, przeliczenie czasów obwiedni na indeksy tablic); `core/synth/wavetable_synth.*`, `core/audio/engine.*` — rodzaj 8 = OXBOW+, modulacje PW1/PW2/P1/P2/LV1/LV2 per głos, VOICE (unison, glide, AGE) działa.
- `core/seq/project.*` (`ObExt`), `core/io/project_file.cpp` (`obx`), `core/io/preset_store.cpp` (INIT = neutralny głos oryginału) — dane OXBOW+.
- `core/ui/synth_view.*` — strony OSC 1 / OSC 2 / FILTER z warstwami L3, FILTER ENV (AMOUNT = ENV AMOUNT), grafika poziomów, nagłówek „OXBOW+”; `core/ui/dialogs.cpp` — OXBOW+ na liście (11 pozycji, lista ciaśniej); `core/ui/app.h` — presety w `presets/oxbowplus/`.
- `core/io/factory_presets.cpp` — 30 presetów OXBOW+ (unison, sync, X-MOD, PWM, fale per oscylator, szum, filtry), poziomy sprawdzone.
- Testy: OXBOW+ z domyślnymi = bit w bit oryginalny OXBOW (`tools/test_oxbowplus.sh`); wszystkie 8 silników nadal 0 różnic; CPU na konsoli 5,2% (6 głosów), 6,3% (wszystko włączone), 6,6% (MONO + unison). Wysłane na konsolę.
**Następny krok:** Patryk testuje OXBOW+.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-13 12:30
**Zmienione — port silnika OXBOW (Hundekraft/kennel wg OberVave zvensona):**
- Kod OXBOW nie jest w repo op1r — tylko w paczce źródeł Hundekraft 3.0.1 (link w README `Hundekraft/op1-workshop`); pobrana do `~/src_ext/hundekraft-3.0.1/` (suma SHA-256 zgodna).
- `third_party/kennel_oxbow/` — oryginalne `oxbow.c`, `obv_dsp.h`, `obv_params.h`, `obv_tables.h`, `oxbow_geom.h`, `oxbow.svg`, `plugin.json`, `presets.py`, LICENSE, NOTICE, PROVENANCE, README (licencja: OberVave GPL-3.0-only — zapisane w `third_party/LICENSES.md`); zaślepki `op1_plugin.h`, `seq/seq_text.h`; `oxbow_art.h`.
- `core/synth/oxbow_dsp.*` — oxbow.c jako C, stan ścieżki zamieniany przy przełączaniu ścieżek; `core/synth/wavetable_synth.*`, `core/audio/engine.*` — OXBOW jako ósmy rodzaj (note_off = wybrzmienie obwiedni filtra).
- `core/ui/oxbow_screen.*` — ekran byka 1:1 (rogi, ptaki, dzwonek, oddech, gwiazdy, odczyty) w skali 1,82× (bez nachodzenia na napisy), tło w cache (4 ms/klatkę zamiast 12 na konsoli) + ilustracja; `core/ui/synth_view.*` — SOUND + L3 (WAVE/FILTER/DRIVE); `core/ui/dialogs.cpp` — OXBOW na liście (10 pozycji).
- `core/seq/project.*`, `core/io/project_file.cpp`, `core/io/factory_presets.cpp`, `core/ui/app.*` — silnik `oxbow`, 16 presetów oryginału; demo ALL ENGINES z partią BRASS (OXBOW POLY BRASS), wersja 3.
- `tests/oxbow_match.cpp`, `tools/test_oxbow.sh` — wzorzec z oryginalnego oxbow.c + harness op1r.
- Testy: OXBOW bit w bit (0 różnic, z wybrzmieniem po note_off), pozostałe 7 silników nadal 0; CPU na konsoli 5,1–6,3% przy 6 głosach; demo bez przesterowania (szczyt 0,85), ~19% rdzenia. Wysłane na konsolę.
**Następny krok:** Patryk testuje OXBOW i demo.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-13 11:00
**Zmienione — port silnika REACTOR (op1r 63, Hundekraft):**
- `third_party/op1r_reactor/` — oryginalne `reactor.c`, `reactor_dsp.h`, `reactor_sin.h`, `reactor_geom.h`, `reactor.svg`, `plugin.json`, `presets.py` (układ katalogów jak w op1r), LICENSE, NOTICE, README; zaślepki `op1_plugin.h` i `seq/seq_text.h`; `reactor_art.h` z SVG.
- `core/synth/reactor_dsp.*` — reactor.c jako C, stan głosu podmieniany przed wywołaniem; `core/synth/wavetable_synth.*`, `core/audio/engine.*` — REACTOR jako siódmy rodzaj (AMP ENV, MOD ENV, 2 LFO).
- `core/ui/reactor_screen.*` — ekran elektrowni 1:1 (pręty, wskazówki, wskaźniki, para, miganie w stanie krytycznym, drżenie) + ilustracja; `core/ui/synth_view.*` — SOUND (COOLING/RADIATION/RODS/HEAT); `core/ui/dialogs.cpp` — REACTOR na liście (9 pozycji, lista ciaśniej).
- `core/seq/project.*`, `core/io/project_file.cpp`, `core/io/factory_presets.cpp`, `core/ui/app.*` — silnik `reactor`, 11 presetów oryginału (z trymem); utwór demo ALL ENGINES z partią REACTOR (CORE, preset SCRAM) — wersja 2 zastępuje poprzednią raz.
- op1r zaktualizowany do „op1r 63” — wcześniej przeniesione silniki się nie zmieniły (wszystkie testy 0 różnic).
- Testy: REACTOR bit w bit (0 różnic), pozostałe 6 nadal 0; presety bez przesterowania (głośniejsze od innych silników, jak oryginał); CPU na konsoli przy 6 głosach 7,8% (też w stanie krytycznym); demo ~18%. Wysłane na konsolę.
**Następny krok:** Patryk testuje REACTOR i demo.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-13 09:30
**Zmienione:** `core/ui/mbira_screen.*` — ekran MBIRA skalowany 1,85× z marginesami 24 px (przy 2× scena sięgała obu krawędzi, wskaźniki dotykały prawej krawędzi ekranu; uwaga Patryka). `core/ui/splash.cpp` — usunięty podpis o silnikach z ekranu startowego (atrybucja trafi do zakładki CREDITS na końcu).
**Następny krok:** port REACTOR z op1r.
**Wymaga od Patryka:** nie.

## 2026-11-13 09:00
**Zmienione:**
- `core/io/factory_presets.cpp` — 20 nowych presetów MBIRA (wersja 2) z MOD ENV, LFO i stroną 2 (RING/DOUBLE/WAH); poziomy sprawdzone; `factoryInstrument()` — brzmienie z presetu fabrycznego po nazwie.
- `core/seq/demo_song.*` (nowy) — utwór demo „ALL ENGINES”: A-moll, 96 BPM, 16 taktów (Am–F–C–G), każdy silnik ma partię (ELG dron, EVA chór, MBIRA arpeggio, WAVETABLE bas, WAVETABLE+ stopa i pluck, PEW organy, TAFFY melodia); intro → groove → całość → wyciszenie.
- `core/ui/app.*`, `core/io/project_store.*` — utwór dodawany raz do listy PROJECT (znacznik; usunięty nie wraca); `core/headless.*`, `platform/main_sdl.cpp` — render WAV/PNG utworu demo (`out/all_engines.wav`, `out/allengines.png`).
- Testy: WAV bez przesterowania (szczyt 0,78); na konsoli cały utwór ~18% jednego rdzenia. Wysłane na konsolę.
**Następny krok:** Patryk słucha utworu demo (PROJECT → ALL ENGINES) i presetów MBIRA.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-13 08:00
**Zmienione:** `core/ui/eva_screen.cpp` — astronautka na ekranie EVA (i na ilustracji) zmniejszona do 88%, nie wchodzi już na napisy strony pod ekranem; wskaźniki bez zmian (uwaga Patryka). Wysłane na konsolę.
**Następny krok:** Patryk sprawdza ekran EVA i MBIRA.
**Wymaga od Patryka:** tak — ocena.

## 2026-11-13 07:30
**Zmienione — port silnika MBIRA (op1r, Hundekraft):**
- `third_party/op1r_mbira/` — oryginalne `mbira.c`, `mbira_noise.h`, `mbira_geom.h`, `mbira.svg`, `plugin.json`, `presets.py`, LICENSE, NOTICE, README; zaślepka `op1_plugin.h`; `mbira_art.h` (`tools/gen/svg_to_art2.py` — pusty wpis, gdy brak wypełnień/okręgów).
- `core/synth/mbira_dsp.*` — mbira.c jako C, stan ścieżki podmieniany przed wywołaniem; `core/synth/wavetable_synth.*`, `core/audio/engine.*` — MBIRA jako szósty rodzaj (AMP ENV, MOD ENV, 2 LFO; bez drugiej obwiedni i trybów głosu).
- `core/ui/mbira_screen.*` — ekran kalimby 1:1 + ilustracja; `core/ui/synth_view.*` — SOUND (STRIKE/TINE/BOX/BUZZ), L3 = RING/DOUBLE/WAH; `core/ui/dialogs.cpp` — MBIRA na liście (8 pozycji).
- `core/seq/project.*`, `core/io/project_file.cpp`, `core/io/factory_presets.cpp`, `core/ui/app.h` — silnik `mbira`, 8 presetów oryginału w `presets/mbira/`; `tests/mbira_match.cpp`, `tools/test_mbira.sh`.
- Testy: MBIRA bit w bit z oryginałem (0 różnic), pozostałe 5 silników nadal 0; presety bez przesterowania; CPU na konsoli przy 6 głosach: MBIRA 5,3%. Wysłane na konsolę.
**Następny krok:** Patryk testuje MBIRA.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-13 06:30
**Zmienione:** `core/io/factory_presets.cpp` — 20 nowych presetów EVA (wersja 2 zestawu) z MOD ENV i LFO: przejścia samogłosek, oddychanie, bramkowanie w tempie, zaniki radia, stab/pluck/lead/bass; poziomy sprawdzone (szczyt ≤ 0,44). Wysłane na konsolę.
**Następny krok:** kolejny silnik — Patryk nie podał linku.
**Wymaga od Patryka:** tak — link do silnika.

## 2026-11-13 06:00
**Zmienione:** EVA — dodane strony MOD ENV (z wyborem celu) i LFO 1 / LFO 2 (`core/ui/synth_view.*`, `core/audio/engine.cpp`; cele PRES/FREQ/OXY/COM/PTCH/VOL w `core/seq/project.cpp`); bez modulacji nadal bit w bit. `core/ui/widgets.cpp` — wartość gałki zmniejsza się, gdy nie mieści się w kolumnie (np. „RADIO 51” w COM). Wysłane na konsolę.
**Następny krok:** Patryk testuje EVA z modulacją.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-13 05:30
**Zmienione — port silnika EVA (op1r, Hundekraft), sam silnik:**
- `third_party/op1r_eva/` — oryginalne `eva.c`, `eva_tables.h`, `eva_geom.h`, `eva.svg`, `eva-art.svg`, `plugin.json`, `presets.py`, LICENSE, NOTICE, README; zaślepka `op1_plugin.h`; `eva_art.h` z SVG (`tools/gen/svg_to_art2.py` — dodana kolejność elementów z dokumentu, potrzebna dla masek wskaźników).
- `core/synth/eva_dsp.*` — eva.c kompilowany jako C (CMake: dodany język C), stan każdej ścieżki podmieniany przed wywołaniem; `core/synth/wavetable_synth.*`, `core/audio/engine.*` — EVA jako piąty rodzaj (tylko obwiednia głośności), puszczenie klawisza = note_off oryginału (końcowy sygnał Quindar).
- `core/ui/eva_screen.*` — ekran astronautki 1:1 + ilustracja; `core/ui/synth_view.*` — SOUND (PRESSURE/FREQ/OXYGEN/COM), L3 = BEEP OCT (strona 2 COM), AMP ENV, PRESETS; `core/ui/dialogs.cpp` — EVA na liście (lista ciaśniej, 7 pozycji).
- `core/seq/project.*`, `core/io/project_file.cpp`, `core/io/factory_presets.cpp`, `core/ui/app.h` — silnik `eva`, 8 presetów oryginału (gałki 1:1, obwiednia w przybliżeniu, bez efektów); `tests/eva_match.cpp`, `tools/test_eva.sh`; pozostałe testy kompilują EVA jako C.
- Testy: EVA bit w bit z oryginałem (0 różnic), WAVETABLE/TAFFY/PEW/ELG nadal 0; dwie ścieżki EVA niezależne (0 różnic); sygnał Quindar przy puszczeniu gra; presety bez przesterowania; CPU na konsoli przy 6 głosach: EVA 7,6%. Wysłane na konsolę.
**Następny krok:** Patryk testuje ELG i EVA.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-13 04:00
**Zmienione:** ELG — synchronizacja ruchu (MOVEMENT) z tempem pod L3: czerwona gałka = SYNC (OFF, 1/1 … 1/128T), MOVEMENT dalej ustawia głębokość; faza przypięta do bitu jak w LFO (`core/synth/elg_dsp.*`, `wavetable_synth.*`, `audio/engine.cpp`, `ui/synth_view.*`, `seq/project.h`, `io/project_file.cpp`; `elg.c` bez zmian, bez sync nadal bit w bit). 30 presetów ELG (`io/factory_presets.cpp`), poziomy sprawdzone. Test: 120 BPM, 1/1 → okres ruchu 2,000 s. Wysłane na konsolę.
**Następny krok:** port EVA z op1r.
**Wymaga od Patryka:** nie.

## 2026-11-13 03:00
**Zmienione — port silnika ELG (op1r, Hundekraft), „sam silnik”:**
- `third_party/op1r_elg/` — oryginalne `elg.c`, `elg_level.h`, `elg_geom.h`, `screen.c`, `elg.svg`, `plugin.json`, LICENSE, NOTICE, README; zaślepka `hk/engine.h`; `elg_art.h` z SVG (`tools/gen/svg_to_art2.py`). `third_party/LICENSES.md`.
- `core/synth/elg_dsp.*` — DSP 1:1 w klasie (instancja na ścieżkę); `core/synth/wavetable_synth.*` — ELG jako czwarty rodzaj; `core/audio/engine.*` — dla ELG wyłączone dodatki (druga obwiednia, MOD ENV, LFO, tryby głosu), zostaje tylko obwiednia głośności.
- `core/ui/elg_screen.*` — ekran „łoś” 1:1 (oddech, księżyc, zorza, poroże, gwiazdy) + ilustracja na karcie ENGINE; `core/ui/synth_view.*` — strony SOUND (INTERVAL/DETUNE/TIMBRE/MOVEMENT), AMP ENV, PRESETS; `core/ui/dialogs.cpp` — ELG na liście silników.
- `core/seq/project.*`, `core/io/project_file.cpp`, `core/io/factory_presets.cpp`, `core/ui/app.h` — silnik `elg`, presety w `presets/elg/` (tylko INIT); `tests/elg_match.cpp`, `tools/test_elg.sh`.
- Testy: ELG bit w bit z oryginałem (105 984 próbki, 0 różnic), WAVETABLE/TAFFY/PEW nadal 0 różnic; CPU na konsoli przy 6 głosach: ELG 7,6%. Wysłane na konsolę.
**Następny krok:** Patryk testuje ELG.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-13 02:00
**Zmienione:** `core/ui/synth_view.*`, `core/audio/engine.cpp` — PEW bez drugiej obwiedni (usunięta strona DRIVE ENV, jej AMOUNT nie działa na organy; uwaga Patryka). `core/ui/splash.*` — ekran startowy dłuższy: ~4,5 s (logo stoi ~2,4 s po animacji). PEW nadal bit w bit. Wysłane na konsolę.
**Następny krok:** kolejny silnik z op1r — port ściśle 1:1, bez dodatków (decyzja Patryka).
**Wymaga od Patryka:** tak — wybór silnika.

## 2026-11-13 01:30
**Zmienione — port silnika PEW (op1r, Hundekraft) + ekran startowy:**
- `third_party/op1r_pew/` — oryginalne `pew.c`, `pew_tables.h`, `pew_macro.h`, `pew_geom.h`, `screen.c`, `pew.svg`, `plugin.json`, LICENSE, NOTICE, README; zaślepka `hk/engine.h`; `pew_art.h` wygenerowany z SVG nowym skryptem `tools/gen/svg_to_art2.py` (ścieżki, wypełnienia, grupy). `third_party/LICENSES.md`.
- `core/synth/pew_dsp.*` — DSP organów 1:1 w klasie (osobne organy i Leslie na każdą ścieżkę); `core/synth/wavetable_synth.*` — PEW jako trzeci rodzaj silnika głosów (ADSR, DRIVE ENV, MOD ENV, 2 LFO, POLY/MONO/GLIDE/AGE); bramki perkusji zwalniane przy puszczeniu klawisza.
- `core/ui/pew_screen.*` — ekran „jazz club” (suwaki, nuty, dym, lampa, Leslie, klawisze); `core/gfx/canvas.*` — wypełnianie wielokątów z antyaliasingiem; `core/ui/synth_view.*` — strona SOUND (REGISTER/PERCUSSION/DRIVE/LESLIE) i nowa BARS (LOW/MID/HIGH), grafika suwaków; `core/ui/dialogs.cpp`, `core/gfx/icons.*` — PEW na liście silników z ilustracją (cała scena).
- `core/seq/project.*`, `core/io/project_file.cpp` — silnik `pew`, pole `push` (strona 2); `core/io/factory_presets.cpp`, `core/ui/app.h` — 31 presetów PEW w `presets/pew/`.
- `core/ui/splash.*`, `core/ui/app.*`, `platform/main_sdl.cpp` — ekran startowy z nazwą DAW-POL 3000 (fala w 4 kolorach, ~2,6 s, przycisk pomija).
- Poprawka: INIT na WAVETABLE+ przełączał silnik na zwykły WAVETABLE (`preset_store.cpp`).
- Testy: PEW bit w bit z oryginalnym harnessem (105 984 próbki, 0 różnic), WAVETABLE i TAFFY nadal 0 różnic; 31 presetów PEW bez przesterowania (szczyt ≤ 0,72); CPU na konsoli przy 6 głosach: PEW 5,6%, TAFFY 6,9%, WAVETABLE 8,6%. Wysłane na konsolę.
**Następny krok:** Patryk testuje PEW i ekran startowy.
**Wymaga od Patryka:** tak — odsłuch i ocena.

## 2026-11-12 23:45
**Zmienione:** `core/gfx/icons.cpp` — ilustracja TAFFY na karcie ENGINE uzupełniona: usta, zielone nici gumy wylatujące z ust (liczone jak na ekranie silnika, bez balonu), kreski przy ustach i gwiazdki obok twarzy (uwaga Patryka). Wysłane na konsolę.
**Następny krok:** Patryk sprawdza ikonę i odsłuchuje nowe presety.
**Wymaga od Patryka:** tak — ocena.

## 2026-11-12 23:30
**Zmienione:**
- `core/gfx/icons.cpp` — ilustracja TAFFY na karcie ENGINE to teraz twarz dziewczyny z oryginalnego SVG (zamiast balonu).
- `core/io/preset_store.*`, `core/ui/app.h`, `core/ui/synth_view.cpp` — osobne presety dla WAVETABLE+ (`presets/wavetableplus/`); przy pierwszym starcie presety WAVETABLE+ ze wspólnego katalogu są przenoszone (własne presety Patryka zostają tam, gdzie pasują).
- `core/io/factory_presets.cpp` (nowy) — zestawy fabryczne jako linie tekstu; po 30 nowych presetów: WAVETABLE (37 razem), WAVETABLE+ (45), TAFFY (41). Istniejące presety nie są nadpisywane.
- `core/headless.cpp` — ekran `pluspresets`.
- Testy: wszystkie 123 presety sprawdzone pod kątem głośności (szczyt < 0,8, żaden cichy); przeniesienie starych presetów sprawdzone; WAVETABLE i TAFFY nadal bit w bit (0 różnic). Wysłane na konsolę.
**Następny krok:** Patryk odsłuchuje presety.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-12 22:00
**Zmienione — port silnika TAFFY (op1r, Hundekraft):**
- `third_party/op1r_taffy/` — oryginalne `taffy.c`, `taffy_tables.h`, `taffy_geom.h`, `screen.c`, `taffy.svg`, LICENSE, NOTICE, README (źródło + commit); zaślepki `hk/engine.h`, `op1_l1.h`; `taffy_art.h` wygenerowany z SVG (`tools/gen/svg_to_art.py`). `third_party/LICENSES.md`.
- `core/synth/taffy_dsp.*` — osobna jednostka z oryginalnym DSP (CZ phase distortion); `core/synth/wavetable_synth.*` — silnik głosów obsługuje TAFFY (`setKind`): ADSR, druga obwiednia (DCW), MOD ENV, 2 LFO, POLY/MONO/UNISON/GLIDE/AGE, decymacja — spójnie z WAVETABLE.
- `core/ui/taffy_screen.*` — ekran „bubblegum” 1:1 (statyka z SVG w cache, na żywo balon-fala, nici, odblaski, iskry); `core/ui/synth_view.*` — TAFFY: gałki WAVE/DCW/ENV/RES, strona DCW ENV, nazwy celów modulacji; `core/ui/dialogs.cpp` — TAFFY na liście silników z ilustracją i opisem, zmiana silnika = domyślne gałki nowego silnika; `core/gfx/icons.cpp` — ilustracja TAFFY.
- `core/io/preset_store.*`, `core/ui/app.h` — presety osobno dla silnika (`presets/wavetable/`, `presets/taffy/`), 11 fabrycznych presetów TAFFY; `core/seq/project.*`, `core/io/project_file.cpp` — silnik `taffy`, domyślne gałki silnika, nazwy celów.
- Testy: TAFFY bit w bit z oryginalnym harnessem (105 984 próbki, 0 różnic), WAVETABLE nadal 0 różnic; 11 presetów TAFFY bez przesterowania; CPU na konsoli: 6 głosów TAFFY 7,1%, WAVETABLE 8,6%.
**Następny krok:** Patryk testuje TAFFY.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-12 20:30
**Zmienione:** `core/ui/dialogs.*` — okno NEW TRACK ma kartę ENGINE (L2/R2, tylko dla MIDI) jak okno TRACK; na pierwszej karcie tylko nazwa silnika (usunięty wybór L1/R1); CREATE działa z obu kart; nowa ścieżka MIDI domyślnie WAVETABLE. Headless `newengine`. Test: WAVETABLE+ z karty → nowa ścieżka ma WAVETABLE+; bez karty — WAVETABLE; ścieżka AUDIO bez karty ENGINE.
**Następny krok:** Patryk sprawdza.
**Wymaga od Patryka:** nie.

## 2026-11-12 20:15
**Zmienione:** `core/ui/synth_view.*` — usunięty przełącznik R3 (oryginał ↔ rozszerzony) i jego podpowiedź w zakładce SYNTH; WAVETABLE i WAVETABLE+ wybiera się tylko jako silniki na karcie ENGINE okna TRACK (propozycja Patryka). Dokumentacja, README, CLAUDE.md zaktualizowane.
**Następny krok:** Patryk sprawdza.
**Wymaga od Patryka:** nie.

## 2026-11-12 20:00
**Poprawka po uwadze Patryka (źle zrozumiałem poprzednią prośbę):**
- `core/ui/app.*` — cofnięta dodatkowa warstwa TRACK w pasku narzędzi L3 (pasek znów ma jedną warstwę EDIT; L2/R2 przy otwartym pasku znów przełączają zakładki).
- `core/ui/dialogs.*` — wybór silnika jako druga karta okna TRACK (Select na ścieżce): L2/R2 przełączają karty TRACK ↔ ENGINE; karta ENGINE = lista silników w jednej kolumnie + ilustracja i opis; na karcie TRACK silnik tylko jako nazwa (usunięta zmiana przez L1/R1); A = OK zapisuje nazwę, kolor i silnik, B = anuluj; ścieżki audio bez karty ENGINE; kropki kart na dole. Usunięte osobne okno EngineDialog.
- Test: Select → R2 → WAVETABLE+ → A; TEST SAW i powrót L2 → A; L1/R1 nie zmieniają silnika; B anuluje; ścieżka AUDIO bez karty ENGINE — OK.
**Następny krok:** Patryk sprawdza.
**Wymaga od Patryka:** tak — ocena.

## 2026-11-12 19:30
**Zmienione:**
- `core/ui/synth_view.cpp` — nazwa presetu usunięta z nagłówka SYNTH (zostaje w przeglądarce PRESETS, zielona kropka).
- `core/ui/app.*` — pasek narzędzi (L3, „shift”) ma 2 warstwy przełączane L2/R2 (decyzja Patryka): EDIT (undo/redo/IN-OUT/loop) i TRACK (MUTE, ENGINE, SOLO, ustawienia ścieżki) dla ścieżki pod kursorem ARRANGE albo ścieżki piano rolla; kropki warstw na dole.
- `core/ui/dialogs.*` — nowe okno ENGINE: jedna kolumna (WAVETABLE — oryginał 1:1, WAVETABLE+ — rozszerzony, TEST SAW), po prawej ilustracja i opis, zielona kropka = silnik ścieżki; A wybór, B powrót.
- `core/gfx/icons.*` — ilustracja silnika w stylu ekranu WAVETABLE (stos klatek fal w perspektywie, kropkowana podłoga, nawias; WAVETABLE+ z czerwoną klatką OSC 2).
- Headless: `engine`, `tracklayer`. Test: wybór WAVETABLE+ / TEST SAW i MUTE z warstwy TRACK działają; oryginał 0 różnic.
**Następny krok:** Patryk testuje.
**Wymaga od Patryka:** tak — ocena okna silników.

## 2026-11-12 18:30
**Zmienione:**
- GLIDE zsynchronizowany z tempem (decyzja Patryka: strona VOICE, L3 = FREE ↔ SYNC, gałka GLIDE = podział 1/1 … 1/128T, czas liczony z bieżącego BPM) — `core/seq/project.*` (`glideSync`, `glideDiv`, `glideSeconds`), `core/io/project_file.cpp`, `core/io/preset_store.cpp`, `core/audio/engine.cpp`, `core/ui/synth_view.*`.
- 10 nowych presetów fabrycznych dla trybu rozszerzonego: HOOVER, REESE, KICK DRUM, SYNC PLUCK, SOFT STRINGS, GATED KEYS, PORTA LEAD, DRIFT PAD, BELL TREE, NOISE SWEEP; zestaw wersjonowany (`_factory_v2`) — na konsoli dojdą tylko brakujące, istniejące i własne presety zostają nietknięte.
- `core/synth/wavetable_synth.cpp` — kompensacja głośności UNISON (6 głosów ≈ jeden głos, 1/√6); wcześniej HOOVER/SUPERSAW dobijały do granicy.
- Testy: świeży katalog 21 presetów; aktualizacja starego katalogu dopisuje 10 i nie nadpisuje zmienionego ACID ani własnego presetu; wszystkie presety grają, szczyty ≤ 0,81; GLIDE SYNC 1/16 @120 = 0,125 s, @90 = 0,167 s; oryginał 0 różnic.
**Następny krok:** Patryk testuje nowe presety.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-12 17:30
**Zmienione:**
- `core/synth/wavetable_synth.*` — naprawiony glide w unisonie: przy starcie nuty tylko pierwszy z 6 głosów zaczynał od poprzedniej wysokości (pozostałe od razu na docelowej), teraz cały stos ma wspólny punkt startu (pomiar: unison podjeżdża identycznie jak pojedynczy głos; przed poprawką 5 głosów skakało od razu na 440 Hz).
- Nowa obwiednia **MOD ENV** (na głos): A/D/S/R + AMOUNT ±100% + cel (te same co LFO, w tym PTCH, CUT, WAVE, W1/W2, P1/P2, LV1/LV2, VOL); modulacje liczone per głos (LFO + MOD ENV), cel VOL interpolowany próbka po próbce (bez kliknięć) — `core/synth/wavetable_synth.*`, `core/seq/project.h`, `core/io/project_file.cpp`, `core/io/preset_store.cpp`, `core/audio/engine.cpp`.
- `core/ui/synth_view.*` — strona MOD ENV (po FILTER ENV); L3 zamienia białą/czerwoną gałkę na DEST + AMOUNT; wykres i opis celu; wybór strony po rodzaju (`setPageKind`) w trybie headless.
- Testy: oryginał 0 różnic; MOD ENV → PTCH (skok i powrót do 220 Hz), → VOL bez kliknięć; MOD ENV zapisuje się w presecie i projekcie.
**Następny krok:** Patryk testuje.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-12 16:30
**Zmienione:**
- Strona **VOICE** w SYNTH: MODE (POLY/MONO — stos nut, legato bez ponownego startu obwiedni, powrót do trzymanej nuty), GLIDE (portamento 5 ms–2 s, POLY i MONO), UNISON (w MONO 6 głosów na nutę, rozstrojenie symetryczne do ±50 c), AGE (jak „vintage” Prophet-6/OB-6: stały charakter głosu + powolny losowy dryf strojenia i filtra) — `core/synth/wavetable_synth.*`, `core/seq/project.*`, `core/audio/engine.cpp`.
- **PRESETS** (ostatnia strona Select): przeglądarka jak na podglądzie OP-1 Field (`ref/op1field_presets.jpg`) — kategorie z ikonami (ALL, BASS, LEAD, PAD, KEYS, PLUCK, BELL, FX, OTHER) i lista presetów; A wczytaj, X zapisz (nazwa + kategoria), B usuń, Y INIT; nazwa presetu w nagłówku — `core/io/preset_store.*` (nowy), `core/gfx/icons.*` (ikony kategorii), `core/ui/synth_view.*`, `core/ui/app.*`, `platform/main_sdl.cpp`; 11 presetów fabrycznych.
- `core/io/project_file.*` — wspólny zapis instrumentu dla projektu i presetów (+ głosy, nazwa presetu); `core/io/fs.*` — lista plików; `core/ui/widgets.cpp` — przycinany podtytuł nagłówka.
- Testy: oryginał 0 różnic; MONO legato/powrót, UNISON 6 głosów, GLIDE, AGE; obieg presetów (zapis w kategorii, wczytanie na inną ścieżkę, zapis w projekcie, usuwanie); presety fabryczne powstają na karcie konsoli.
**Następny krok:** Patryk testuje.
**Wymaga od Patryka:** tak — odsłuch i ocena przeglądarki presetów.

## 2026-11-12 15:00
**Zmienione:** `core/synth/wavetable_synth.cpp` — przester nowych filtrów (WAVETABLE+) usunięty: zdjęte sztuczne wzmocnienia BP ×1,5 / HP ×2 sprzed soft clipa, skale wyjść LP12/BP/HP + kompensacja zależna od rezonansu dobrane pomiarem nasycenia soft clipa (teraz zawsze ≤ LP24 = oryginał), suma OSC1+OSC2+SUB normalizowana ponad poziom oryginału, głośność trybów wyrównana czysto za soft clipem (LP12 ×1,35, BP ×2, HP ×1,8; bez dobijania do granicy). Usunięty cel LFO W1+2 (WAVE już przesuwa obie fale; zapisane W1+2 wczytują się jako WAVE) — `core/seq/project.*`. Dokumentacja.
Pomiar nasycenia wyjścia przy RES 30000 + SUB: przed — LP12 59%, BP 90%, HP 80% (LP24 29%); po — LP12 8,5%, BP 0%, HP 18% (LP24 25%). Oryginał nadal 0 różnic.
**Następny krok:** Patryk testuje filtry na konsoli.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-12 14:10
**Zmienione:** nowy cel LFO **W1+2** (fale obu oscylatorów naraz; dopisany na końcu listy — zapisane projekty bez zmian) — `core/seq/project.*`, `core/synth/wavetable_synth.*`; ekran pokazuje też ruch fali OSC 1 przy W1 / W1+2. Dokończone z poprzedniej sesji: opis trybu rozszerzonego w `docs/ARCHITEKTURA.md`, README, CLAUDE.md; pomiar na konsoli: 6 głosów — oryginał 9,3%, WAVETABLE+ (2 LFO, sub, BP) 10,1% rdzenia. Zgodność oryginału nadal 0 różnic.
**Następny krok:** Patryk testuje WAVETABLE+ na konsoli.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-12 13:40
**Zmienione:** tryb ROZSZERZONY WAVETABLE (R3 w SYNTH; oryginał 1:1 zostaje domyślny i nadal zgodny bit w bit): własny DSP wzorowany na `wavetable_sample` — 2 niezależne oscylatory (fala, interwał ±24, detune ±50 c, poziom), SUB (sinus −1 okt., L3 na OSC 1), filtr LP24 (oryginał) / LP12 / BP / HP + DRIVE; strony OSC 1, OSC 2, FILTER; nowe cele LFO W1 W2 P1 P2 LV1 LV2; na ekranie fala OSC 2 na czerwono, krzywa filtra wg trybu. Pliki: `core/seq/project.*`, `core/io/project_file.cpp` (naprawiony parser długich linii), `core/synth/wavetable_synth.*`, `core/audio/engine.*`, `core/ui/synth_view.*`, `core/ui/wavetable_screen.*`, `core/headless.*`.
Testy: oryginał 0 różnic; rozszerzony z ustawieniami jak oryginał różni się o ~−77 dB; tryby filtra i SUB działają.
**Do zrobienia:** aktualizacja `docs/ARCHITEKTURA.md` i README o tryb rozszerzony; test wydajności trybu rozszerzonego na konsoli.
**Wymaga od Patryka:** odsłuch na konsoli.

## 2026-11-12 12:20
**Zmienione:**
- `core/seq/project.*` — synchronizacja LFO (`sync`, `div`), 15 podziałów 1/1 … 1/128 z triolami; AMOUNT obwiedni głośności −1..1; `core/io/project_file.*` — zapis/odczyt (zgodnie wstecz).
- `core/synth/wavetable_synth.*` — LFO zsynchronizowane: przy grającym utworze faza = pozycja w ćwierćnutach / długość cyklu (przypięte do bitu), bez transportu biegnie z częstotliwością z tempa; wygładzenie LFO ~3 ms i tremolo interpolowane próbka po próbce (koniec kliknięć przy SAW/SQR → VOL); AMOUNT głośności ujemny = obwiednia odwrócona.
- `core/seq/sequencer.h` — `beatPos()`, `tempo()`; `core/audio/engine.cpp` — przekazywanie transportu do instrumentów przed każdym odcinkiem.
- `core/ui/synth_view.*` — L3 na stronach LFO: SYNC/FREE (RATE → podział, etykieta SYNC), AMOUNT ±100 w AMP ENV.
- Testy: skoki przy SAW/SQR na VOL na poziomie czystego sinusa (brak kliknięć); faza LFO 1/4 zgodna z pozycją w takcie; zgodność z oryginałem 0 różnic.
**Następny krok:** Patryk testuje.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-12 11:30
**Zmienione (przegląd kodu obwiedni i LFO po uwadze Patryka):**
- `core/synth/wavetable_synth.*` — LFO biegnie w czasie rzeczywistym także w ciszy (wcześniej stał między frazami, więc każda fraza „startowała” go od miejsca, gdzie skończyła poprzednia); SUSTAIN śledzi gałkę w trakcie nuty; szybkie powtórzenie tej samej nuty w trakcie wyciszania nie zajmuje drugiego głosu; obwiednia filtra ma własne RELEASE; AMOUNT obwiedni głośności (0 = stała głośność przy trzymaniu, rampa bramki 3 ms); ekran dostaje modulację ostatnio uderzonej nuty (zamiast „najgłośniejszej”, która skakała między głosami).
- `core/seq/project.*`, `core/io/project_file.*` — `envAmt`, obwiednia filtra A/D/S/R + `fenvAmt`; wczytywanie pierwszej wersji zapisu (A D S AMOUNT) zachowane; skrócone nazwy kształtów i celów LFO.
- `core/ui/synth_view.*` — L3 na stronach obwiedni zamienia RELEASE ↔ AMOUNT; usunięte napisy o klawiszach i zmiana oktawy; `core/ui/app.cpp` — L3 trafia do zakładki SYNTH.
- Testy: zgodność z oryginałem 0 różnic; LFO po 0,25 s ciszy w poprawnej fazie; 3× ta sama nuta = 1 głos; SUSTAIN w trakcie nuty; AMOUNT 0 = stała głośność i cisza po puszczeniu; stary plik OK; brak kliknięć.
**Następny krok:** Patryk testuje na konsoli.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-12 10:30
**Zmienione:**
- `core/synth/wavetable_synth.*` — bez kliknięć: przejmowany/ponownie uderzany brzmiący głos najpierw wycisza się ~2 ms (dopiero potem reset DSP i nowa nuta), ogon filtra decymującego wybrzmiewa (zamiast zerowania); obwiednia filtra na głos (A/D/S/AMOUNT, wybrzmienie = R głośności); 2 LFO na ścieżkę (sinus, trójkąt, piła, prostokąt, losowy; 0,05–20 Hz; cele WAVE, DETUNE, CUTOFF, RESONANCE, PITCH, VOLUME); zmodulowane gałki dla ekranu.
- `core/audio/engine.*` — przekazywanie obwiedni filtra i LFO; minimalny atak 1 ms i wybrzmienie 3 ms.
- `core/seq/project.*`, `core/io/project_file.*` — obwiednia filtra i LFO w ścieżce i w pliku (`fenv`, `lfo1`, `lfo2`).
- `core/ui/synth_view.*` — 5 stron gałek (SOUND, AMP ENV, FILTER ENV, LFO 1, LFO 2) z kropkami stron, wykres obwiedni filtra / kształtu LFO z celem, czytelny opis klawiszy („D-PAD + ABXY = C4 MAJOR SCALE, L1/R1 = OCTAVE”), ekran pokazuje ruch modulacji.
- Headless: `synthfenv`, `synthlfo`. Testy: zgodność z oryginałem nadal 0 różnic; pomiar kliknięć — największy skok między próbkami spadł z 10,4% do 5,1% sygnału (poziom naturalnego zbocza fali, czyli brak kliknięć).
**Następny krok:** Patryk testuje na konsoli.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-12 09:30
**Zmienione:**
- `third_party/op1r_wavetable/` — oryginalne pliki silnika WAVETABLE z op1r (Hundekraft, GPL-3.0-or-later): `wavetable_dsp.h`, `wavetable_level.h`, `wavetable_geom.h`, `orig_88k/wavetables.h`, LICENSE, NOTICE, README ze źródłem i commitem; `wavetables.h` przeliczony ich generatorem dla 96 kHz (`tools/gen/op1r_wavetable_gen_tables.py`; z 88200 odtwarza oryginał bajt w bajt). `third_party/LICENSES.md`.
- `core/synth/wavetable_synth.*` — instrument: oryginalny DSP (ścieżka C), głos 96 kHz w blokach 128, decymacja 2:1 FIR z harnessa op1r (bufor kołowy), 6 głosów, własny ADSR.
- `core/ui/wavetable_screen.*` — ekran przeniesiony 1:1 (statyka wg `gen_svg.py` w cache, ruch wg `wavetable_screen.c`), skala 2×; `core/ui/synth_view.*` — zakładka SYNTH (strony SOUND/ENVELOPE, wykres ADSR, nuty z przycisków).
- `core/seq/project.*`, `core/io/project_file.*` — instrument ścieżki (silnik, 4 gałki, ADSR) w projekcie i w pliku; `core/audio/engine.*` — instrument wg ścieżki; `core/ui/dialogs.*` — wybór silnika L1/R1; `core/ui/app.*` — 4. zakładka; `core/gfx/canvas.*` — `copyRows`.
- `tests/wavetable_match.cpp`, `tools/test_wavetable.sh` — test zgodności: **0 różnic na 105 984 próbkach** względem oryginalnego harnessa op1r.
- Wydajność na konsoli: 1,37% rdzenia na głos (6 głosów 8,2%).
**Następny krok:** Patryk testuje brzmienie i sterowanie WAVETABLE na konsoli.
**Wymaga od Patryka:** tak — odsłuch i ocena sterowania.

## 2026-11-12 08:00
**Zmienione:**
- `core/gfx/canvas.h` — 32 kolory ścieżek (8 dotychczasowych + 24 nowe); `core/io/project_file.cpp`, `core/seq/project.*`, ekrany — obsługa 32 barw.
- `core/ui/dialogs.*` — wybór koloru ścieżki z siatki 4×8 (krzyżak), zamiast listy nazw.
- `core/synth/saw_synth.*` — pomiar szczytu ścieżki; `core/audio/engine.*` — poziomy ścieżek (szczyt + opadanie ~24 dB/s) do VU, przekazywane atomowo.
- `core/ui/arrange.cpp` — segmentowy VU (−48..0 dB, zielony/żółty od −12 dB/czerwony od −3 dB) pod paskiem głośności każdej ścieżki.
**Następny krok:** ocena Patryka; potem step seq/perkusja albo syntezator.
**Wymaga od Patryka:** tak — ocena.

## 2026-11-12 07:35
**Zmienione:** `core/ui/piano_roll.*` — warstwy menu zaznaczania (R3) przełączane L2/R2 zamiast L1/R1 (L2 = warstwa 1, R2 = warstwa 2); `core/ui/app.cpp` — gdy menu R3 jest otwarte, L2/R2 nie zmieniają zakładek. README, CLAUDE.md, ARCHITEKTURA.md zaktualizowane. Test: R2 w menu zmienia warstwę i zostaje w piano rollu; po wyjściu z menu L2 przechodzi do ARRANGE.
`tools/deploy.sh` — wysyłka pod tymczasową nazwę i podmiana (`mv`), bo nadpisanie działającego programu na konsoli się nie udawało.
**Następny krok:** ocena Patryka; potem step seq/perkusja albo syntezator.
**Wymaga od Patryka:** tak — ocena (aplikację uruchomić ponownie, żeby weszła nowa wersja).

## 2026-11-12 07:20
**Zmienione:** poprawka mojej pomyłki — `core/ui/app.cpp`: kropki zakładek (PROJECT/ARRANGE/PIANO ROLL) wróciły pod nagłówek; `core/ui/piano_roll.cpp`: kropki warstw paska R3 (L1/R1) przywrócone i przeniesione na dół ekranu (wcześniej leżały na linii). CLAUDE.md — opis położenia obu wskaźników.
**Następny krok:** ocena Patryka; potem step seq/perkusja albo syntezator.
**Wymaga od Patryka:** tak — ocena.

## 2026-11-12 07:05
**Zmienione:** `core/ui/widgets.cpp` — kółko i litera przycisku w pigułkach wyrównane do środka piksela (litera na środku koła), napis BAR w nagłówku wyśrodkowany; `core/ui/app.cpp` — kropki zakładek przeniesione na dół ekranu; `core/ui/piano_roll.cpp` — usunięte kropki warstw spod linii (warstwę pokazuje status „1/2” / „2/2”). CLAUDE.md — zasady.
**Następny krok:** ocena Patryka; potem step seq/perkusja albo syntezator.
**Wymaga od Patryka:** tak — ocena.

## 2026-11-12 06:45
**Zmienione:** `core/ui/piano_roll.*` — druga warstwa paska zaznaczania (L1/R1): B TRANSPOSE (trzymaj + góra/dół, półtony), A MOVE (trzymaj + krzyżak, czas i wysokość), Y OCTAVE (trzymaj + góra/dół), X DUPLICATE (kopia zaraz za zaznaczeniem); przesunięcia „wszystko albo nic” (kolizja albo wyjście poza pattern = brak zmiany), kursor podąża za zaznaczeniem, odsłuch najwyższej nuty; kropki warstw nad paskiem. Headless: `transform`. Dokumentacja/README/CLAUDE.md.
Testy: transpozycja, oktawa, przesuwanie z blokadą na końcu patternu, duplikat bez miejsca, undo całego gestu — OK.
**Następny krok:** Patryk ocenia (i potwierdza, czy Y = OCTAVE i X = DUPLICATE mu pasują); potem step seq/perkusja albo syntezator.
**Wymaga od Patryka:** tak — test i decyzja o Y/X.

## 2026-11-12 06:10
**Zmienione:**
- `core/ui/widgets.*` — pigułki z tekstem dopasowanym do obrysu; nowy kompaktowy `actionBar` (4 pigułki w kolumnach gałek, linia oddzielająca, linia statusu).
- `core/ui/piano_roll.*` — pasek zaznaczania na `actionBar`; podświetlenie zakresu pętli + flagi IN/OUT na siatce.
- `core/ui/app.*` — pasek NARZĘDZI pod L3: B UNDO, A REDO, Y IN/OUT, X LOOP; redo (druga historia); ekran dostaje wtedy tylko D-pad/Start/R3.
- `core/ui/buttons.h` — `masked()`; `core/ui/arrange.*` — linia i podświetlenie pętli, `cursorBar()`.
- `core/seq/project.*`, `pattern.h` — pętla utworu (takty) i patternu (kroki); `core/seq/sequencer.cpp` — odtwarzanie w zakresie pętli, utwór bez pętli zatrzymuje się na końcu; `core/io/project_file.*` — linie `loop`.
- Headless: `tools`, `rolltools`. Dokumentacja/README/CLAUDE.md.
- Testy: pętla taktów 3–5 i kroków 5–8 grają tylko swój zakres; bez pętli utwór staje; undo/redo w obie strony; pętla przetrwa zapis/odczyt.
**Następny krok:** ocena Patryka; potem wybór: step seq/perkusja, syntezator.
**Wymaga od Patryka:** tak — test na konsoli.

## 2026-11-12 05:15
**Zmienione:**
- `core/ui/input.h`, `platform/controls_map.h`, `core/ui/test_screen.cpp` — nowe przyciski L3/R3 (klik gałek; konsola: 12 i 15, PC: U i O).
- Nowy `core/seq/undo_stack.h` — historia 32 migawek projektu.
- `core/ui/app.*` — undo pod L3; krok historii zamyka się, gdy przyciski i gałki są puszczone (jedno wciśnięcie / pokręcenie = jeden krok, okno dialogowe = jeden krok); po undo poprawka kursorów/ekranów; komunikaty „toast”; historia czyszczona przy wczytaniu/nowym projekcie.
- `core/ui/piano_roll.*` — tryb zaznaczania pod R3: dolny pasek → pigułki B SELECT / A DELETE / Y COPY / X PASTE; B = zaznacz nutę, B + D-pad = prostokąt, B na pustym = wyczyść; schowek działa między patternami; wklejone nuty zostają zaznaczone.
- `core/ui/arrange.*` — `clampCursor()`. Headless: ekran `select`. Dokumentacja/README/CLAUDE.md.
- Testy (symulacja przycisków): pokręcenie BPM = 1 krok undo; wklej → usuń → undo → undo poprawnie; undo dodania ścieżki + patternu OK.
**Następny krok:** ocena Patryka; potem wybór: step seq/perkusja, pętla/markery, syntezator.
**Wymaga od Patryka:** tak — test na konsoli.

## 2026-11-12 04:30
**Zmienione:**
- `core/gfx/icons.*` — dyskietka zamiast notatnika (ilustracja w polu 196×150 + mała ikona projektu).
- `core/ui/widgets.*` — wspólny `screenHeader` (ARRANGE i PIANO ROLL: ten sam układ, „BAR takt.miara” w tym samym miejscu) i `drawTextFit` (tekst przycinany do szerokości).
- `core/ui/arrange.*` — nazwy klipów przycinane do widocznej części klipu przy każdym zoomie; `highlightedPattern()`.
- `core/ui/app.cpp` — R2 z ARRANGE otwiera pattern pod kursorem (albo ostatni na ścieżce).
- `core/ui/piano_roll.cpp` — nagłówek ze wspólnego `screenHeader` (pozycja kursora/głowicy jako takt.miara zamiast „BAR 1/2”), mniejsze etykiety miar nad siatką.
**Następny krok:** ocena Patryka; potem wybór: step seq/perkusja, pętla/markery, undo, syntezator.
**Wymaga od Patryka:** tak — test i wybór kolejnego kroku.

## 2026-11-12 04:00
**Zmienione:**
- Nowe: `core/io/fs.*` (katalogi), `core/io/project_file.*` (format `.dpp`: tekstowy, wersjonowany, zapis atomowy z fsync), `core/io/project_store.*` (projekty w `projects/<NAZWA>/`, lista od najnowszego, ostatnio otwarty), `core/ui/project_view.*` (zakładka PROJECT).
- `core/gfx/icons.*` — ilustracja notatnika (196×150, jak syntezator/magnetofon) i ikona projektu.
- `core/ui/app.*` — zakładki PROJECT ↔ ARRANGE ↔ PIANO ROLL (L2/R2) z kropkami pod nagłówkiem, stan „niezapisane”, zapis/odczyt/nowy, autozapis przy wyjściu, start z ostatnim projektem.
- `platform/main_sdl.cpp` — projekty w `projects/` (na konsoli `/userdata/dawpol3000/projects`), autozapis przy wyjściu, SIGTERM (Hotkey+Start) kończy program normalnie.
- Headless: ekran `project`. Testy: zapis → nowy → odczyt daje identyczny projekt; autozapis i start z ostatnim projektem OK; zapis na karcie konsoli (exFAT) OK.
- Dokumentacja, README, CLAUDE.md zaktualizowane; `projects/` w `.gitignore`.
**Następny krok:** ocena Patryka; potem do wyboru: step seq/perkusja, pętla i markery, undo, prawdziwy syntezator.
**Wymaga od Patryka:** tak — test na konsoli i wybór kolejnego kroku.

## 2026-11-12 03:10
**Zmienione:**
- `core/gfx/icons.cpp` — ilustracje narysowane od nowa na sztywnej siatce liczonej od obudowy: syntezator (galki mieszczą się w obrysie, usunięty kabel, poprawny układ czarnych klawiszy 2+3), magnetofon (4 wskaźniki VU równe i w osi, wycentrowane szpule, zamiast suwaków przyciski transportu).
- `core/ui/dialogs.cpp` — ikony na liście TYPE wyrównane do środka wierszy (MIDI/AUDIO); nowe proporcje ilustracji.
- `core/ui/arrange.cpp` — miniatury nut w klipach mapowane dokładnie na oś taktów i przycinane do klipu; siatka (kropki taktów) rysowana pod klipami, klipy z nieprzezroczystym tłem (kropki nie udają nut).
**Następny krok:** ocena Patryka; potem zapis/odczyt projektu.
**Wymaga od Patryka:** tak — test na konsoli.

## 2026-11-12 02:30
**Zmienione:**
- `core/gfx/icons.cpp` — ikony przerysowane od nowa (siatka 16×16, jedna grubość kreski, zaokrąglone końce, mniej detali): klawiatura 3+2 klawisze, fala = jeden okres sinusa, aranżacja = 3 rzędy kresek, czytelniejsze play/pętla/metronom/taśma.
- `core/seq/project.*` — mute/solo na ścieżce, `audible()`, `removeTrack()` (z klipami).
- `core/seq/sequencer.cpp`, `core/audio/engine.*` — mute/solo w trybie SONG (nie wyzwala nut, wycisza grające).
- `core/ui/dialogs.*` — okno TRACK (edycja): nazwa, kolor z listy z próbkami, usuwanie (Y dwa razy), OK/CANCEL.
- `core/ui/arrange.*` — Select = ustawienia ścieżki, L1 = mute, R1 = solo; wskaźniki M (żółty) / S (cyjan) przy nazwie; wyciszone ścieżki przygaszone. `core/ui/app.*` — `openTrackEdit()`.
- Headless: ekran `trackedit`. Dokumentacja, README, CLAUDE.md zaktualizowane. Testy: zmiana nazwy, kolor, mute/solo (cisza po ~10 ms), usuwanie — OK.
**Następny krok:** ocena Patryka; potem zapis/odczyt projektu.
**Wymaga od Patryka:** tak — test na konsoli.

## 2026-11-12 01:40
**Zmienione:**
- Nowe moduły: `seq/project.*` (ścieżki 16 MIDI + 4 audio, pula patternów, klipy), `ui/app.*` (zarządzanie ekranami i projektem), `ui/view.h`, `ui/arrange.*` (ekran ARRANGE z BPM), `ui/dialogs.*` (NEW TRACK, NEW PATTERN), `ui/widgets.*` (belki, pigułki, pola, listy, edycja nazw), `gfx/icons.*` (ikony + ilustracje syntezatora i magnetofonu).
- Zmienione: `seq/pattern` (nazwa, 1–16 taktów, 512 nut), `seq/sequencer` (tryby SONG i PATTERN, wiele ścieżek), `audio/engine` (migawka całego projektu, synth na ścieżkę, master + łagodny limiter), `synth/saw_synth` (dodaje do miksu z głośnością ścieżki, nic nie liczy bez nut), `ui/piano_roll` (pattern z projektu, wiele taktów), `gfx/canvas.h` (nowa paleta: kolory menu i ścieżek), `headless` (ekrany arrange/track/pattern/roll), `main_sdl.cpp` (start w ARRANGE z projektem demo).
- Referencje Patryka skopiowane do `ref/`. Zaktualizowane `docs/ARCHITEKTURA.md` i CLAUDE.md (mapowanie L2/R2, paleta menu, stan).
- Test przepływu (symulacja przycisków): dodanie ścieżki → pattern 3 takty → klip → nuty → klon klipu → odtwarzanie — OK. CPU na konsoli ~2.5% rdzenia.
**Następny krok:** Patryk testuje na konsoli; potem zapis/odczyt projektu, usuwanie/zmiana nazwy ścieżek, mute/solo.
**Wymaga od Patryka:** tak — test i ocena.

## 2026-11-12 03:20
**Zmienione:**
- `core/gfx/font.cpp` — dodany znak `#` (brakowało go, więc „C#5” rysowało się jako „C5”).
- `core/gfx/canvas.*` — zaokrąglone prostokąty (wypełnienie i obrys, SDF z antyaliasingiem).
- `core/ui/piano_roll.*` — styl bliżej OP-1: nuty jako pigułki, **kolor nuty zależy od velocity** (ciemna zieleń → zieleń → mięta, zieleń = kolor enkodera VELOCITY), siatka z kropek + linie tylko na miarach, mini-klawiatura z zaokrąglonymi klawiszami, biały kursor, zaznaczona nuta w białej obwódce z niebieskim uchwytem długości, głowica z trójkątem; `setView()` do ustawienia widoku.
- Folder `ref/` na grafiki wzorcowe (w `.gitignore`). CLAUDE.md: zasady stylu (pigułki, kropki) i grafik wzorcowych.
**Następny krok:** Patryk ocenia nowy wygląd i podsyła grafiki wzorcowe do `ref/`.
**Wymaga od Patryka:** tak — ocena i grafiki.

## 2026-11-12 02:50
**Zmienione:** `core/ui/piano_roll.cpp` — poprawione kolory wierszy (były odwrócone: czarne klawisze jaśniejsze); teraz białe klawisze jaśniejsze, czarne ciemne, wiersz kursora delikatnie zielony (nie myli się z klawiszem); dodany pasek mini-klawiatury po lewej i znacznik kursora; mocniejsza linia na granicy oktawy (B/C) i słabsza między E/F. Wysłane na konsolę.
**Następny krok:** Patryk sprawdza na konsoli; potem wiele taktów i kanałów, step seq.
**Wymaga od Patryka:** tak — ocena.

## 2026-11-12 02:30
**Zmienione:**
- Reorganizacja `core/` na foldery: `gfx/` (canvas, font), `io/` (PNG/WAV), `util/` (spsc, triple_buffer), `synth/` (note_sink, saw_synth, tone_synth), `seq/` (pattern, sequencer), `audio/` (engine), `ui/` (input, buttons, encoder, piano_roll, test_screen).
- Nowe: `seq/pattern` (nuty w tickach, PPQ 96, 1 takt 4/4), `seq/sequencer` (odtwarzanie z dokładnością do próbki), `synth/saw_synth` (piła polyBLEP, 8 głosów), `audio/engine` (łączy sekwencer i instrument, komendy lock-free), `util/triple_buffer.h` (pattern UI→audio), `ui/piano_roll` (ekran edycji), `ui/buttons.h`, `ui/encoder.h`.
- `docs/ARCHITEKTURA.md` — opis wątków, przepływu danych i każdego modułu. README — sterowanie piano rolla.
- `platform/main_sdl.cpp` — domyślnie piano roll, `--test` = ekran testu sprzętu; headless `--render-png plik [roll|test]`.
- Wysłane na konsolę. Wyniki: `out/pianoroll.png`, `out/pianoroll.wav` (8 s demo). CPU na konsoli ~1.5% rdzenia.
**Następny krok:** Patryk testuje piano roll na konsoli; potem wiele taktów i kanałów, step seq.
**Wymaga od Patryka:** tak — test na konsoli i ocena.

## 2026-11-12 01:20
**Zmienione:** podpięte repo GitHub (`origin` = git@github.com:rpm160219/dawpol3000.git), wysłana cała historia; CLAUDE.md — adres repo i zasada „commit + push po każdym kroku”.
**Następny krok:** etap 1 — makiety PNG ekranu syntezatora (osc → filtr → ADSR) do oceny stylu.
**Wymaga od Patryka:** nie (tylko ocena makiet, gdy będą).

## 2026-11-12 01:05
**Zmienione:** UI i logi programu po angielsku (`core/hello_app.cpp`, `core/input.h`, `platform/main_sdl.cpp`); wpisane kody przycisków RG35XX Pro (`platform/controls_map.h`); awaryjne wyjście Menu+Select na surowych kodach; `tools/knulli/DAW-POL 3000.sh.keys` — wyjście Hotkey+Start (Knulli zabija program); `tools/deploy.sh` wysyła też plik `.keys`. CLAUDE.md: zasada „aplikacja po angielsku”. Wymuszone zamknięcie zawieszonej aplikacji przez SSH.
**Stan:** Patryk potwierdził — na konsoli działa obraz, dźwięk, przyciski, analogi.
**Następny krok:** podpięcie GitHuba, potem etap 1 (makiety ekranu syntezatora).
**Wymaga od Patryka:** tak — dodać klucz SSH na GitHubie i utworzyć puste prywatne repo.

## 2026-11-12 00:40
**Zmienione:** `tools/ssh_env.sh` (wspólne SSH z hasłem z `tools/device.conf`, który jest poza gitem), `tools/deploy.sh`, `tools/log.sh`, `tools/dev.sh` (komenda na konsoli). Program wysłany na konsolę i dodany do menu Ports; test headless na konsoli OK (PNG zgodny z PC, WAV OK).
**Ustalenia o konsoli:** Knulli = Batocera 42, kernel 4.9.170, glibc 2.40, SDL 2.32.8, karty ALSA: audiocodec / ahubdam / ahubhdmi, RAM 973 MB. `/userdata` na exFAT (fuseblk) → Dropbear odrzuca klucze SSH, logujemy się hasłem. Porty: `/userdata/roms/ports/*.sh`, przeładowanie listy: `curl http://127.0.0.1:1234/reloadgames`.
**Następny krok:** Patryk uruchamia Ports → DAW-POL 3000; czytam log (ekran, audio, pad), mapuję przyciski.
**Wymaga od Patryka:** tak — uruchomić z menu i powiedzieć, co widać/słychać.

## 2026-11-12 00:10
**Zmienione:** `tools/deploy.sh` (kopiuje program + wpis w menu Ports, zapamiętuje IP), `tools/knulli/DAW-POL 3000.sh` (uruchamianie z menu Ports, log do pliku), `tools/log.sh` (podgląd logu z konsoli). Patryk dostał instrukcję wgrania Knulli i włączenia SSH.
**Następny krok:** pierwszy deploy na konsolę i test (ekran, dźwięk, przyciski).
**Wymaga od Patryka:** tak — karta z Knulli, Wi-Fi, IP konsoli.

## 2026-11-11 23:55
**Zmienione:** `platform/main_sdl.cpp` — logi wypisywane od razu linia po linii (potrzebne do czytania logów z konsoli przez SSH). Uruchomiona wersja desktop do testu przez Patryka.
**Następny krok:** ocena okna/dźwięku przez Patryka; potem Knulli na karcie i IP konsoli.
**Wymaga od Patryka:** tak — ocena i info o karcie z Knulli.

## 2026-11-11 23:45
**Zmienione:**
- Toolchain zainstalowany (cmake, aarch64-linux-gnu-g++, SDL2 arm64) — buduje się desktop i ARM.
- `core/` — rdzeń niezależny od platformy: rasteryzer z antyaliasingiem (`canvas`), własny font kreskowy (`font`), kolejka lock-free UI→audio (`spsc.h`), ton testowy (`tone_synth`), zapis PNG/WAV (`image_io`), ekran testowy etapu 0 (`hello_app`), tryb headless (`headless`).
- `platform/main_sdl.cpp` + `platform/controls_map.h` — okno, audio 48 kHz/256 ramek, pad/klawiatura, logowanie surowych kodów przycisków (do zmapowania konsoli); całe mapowanie w jednej tabeli.
- `CMakeLists.txt`, `cmake/aarch64.cmake`, `tools/build.sh`, `tools/deploy.sh`, `README.md`, `LICENSE` (GPL-3), `.gitignore`; repo git założone lokalnie.
- Wynik: `out/etap0.png` (makieta ekranu testowego), `out/etap0.wav` (6 s: skala C-dur, w drugiej połowie rośnie drive). Binarka ARM wymaga glibc ≥ 2.38, libstdc++ wbudowane.

**Następny krok:** uruchomienie na konsoli — potrzebne Knulli na karcie, Wi-Fi i IP konsoli; potem odczyt kodów przycisków i pomiar latencji.
**Wymaga od Patryka:** tak — ocena PNG/WAV („git / nie git”), karta z Knulli i IP konsoli.

## 2026-11-11 23:10
**Zmienione:**
- `DZIENNIK.md` — utworzony (dziennik zmian, najnowsze u góry).
- `CLAUDE.md` — poprawione: VM to VirtualBox (nie VMware), sieć zostaje NAT (usunięta sprzeczna pozycja „Bridged”), zaktualizowany „Aktualny stan”.

**Sprawdzone w VM:** Debian 13, 16 rdzeni, 15 GB RAM, 37 GB wolnego. Jest: gcc/g++, git, python3, ssh/scp, SDL2 2.32. Brak: cmake, aarch64-linux-gnu-gcc, SDL2 arm64, `~/.ssh`, git user.name/email. Sudo wymaga hasła.

**Następny krok:** instalacja toolchaina (cmake + cross-kompilator aarch64 + SDL2 arm64).
**Wymaga od Patryka:** tak — wklejenie jednej komendy z sudo (hasło).
