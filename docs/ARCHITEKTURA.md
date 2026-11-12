# Architektura DAW-POL 3000

Dokument opisuje, z czego składa się program i jak części ze sobą rozmawiają.
Aktualizowany przy każdej zmianie struktury. Kod: C++17 „C z klasami”, bez wyjątków i RTTI.

## Wątki
- **Wątek UI** (główny): wejście → logika ekranu → rysowanie do `Canvas` → wysłanie na ekran (SDL).
- **Wątek audio** (callback SDL, 48 kHz, blok 256 ramek): `AudioEngine::process`. Zero alokacji, zero blokad.
- Komunikacja UI → audio: kolejka komend `Spsc` oraz `TripleBuffer<Project>` (szczegóły pod diagramem).
- Audio → UI: zmienne atomowe (pozycja głowicy, czy gra).

```
 [Input SDL] -> InputState -> App -> bieżący ekran (Arrange / PianoRoll / okna dialogowe)
                               |  edytuje Project (UI)
                               |--Spsc komendy (play/stop, podgląd nut)--> AudioEngine
                               |--TripleBuffer<Project> (migawka)------->  |  Sequencer -> SawSynth x20 -> miks -> limiter
                               |<--atomic: tick, tryb, czy gra-----------  |
                               v
                         Canvas 640x480 -> SDL tekstura
```
- `UI → audio: komendy` z kanałem `Spsc` (play/stop, podgląd nut), plus `TripleBuffer<Project>`: UI po każdej edycji publikuje kopię całego projektu (~200 KB, kopiowanie < 0.1 ms); tempo, głośności ścieżek i master silnik bierze z migawki.

## Moduły (`core/` — niezależne od platformy)

| Plik | Co robi |
|---|---|
| `gfx/canvas.*` | Bufor 640×480 RGBA i rasteryzer z antyaliasingiem: linie, łuki, koła, prostokąty, zaokrąglone prostokąty/„pigułki” (SDF). Kreski wsadowe (`beginStroke/strokeSeg/endStroke`), żeby nakładające się segmenty nie ciemniały. Paleta 4 kolorów enkoderów (`pal::`). |
| `gfx/icons.*` | Ilustracje silników (stos klatek fal jak ekran WAVETABLE; WAVETABLE+ z czerwoną klatką OSC 2; TEST SAW). Ikony kategorii presetów (ALL, BASS, LEAD, PAD, KEYS, PLUCK, BELL, FX, OTHER). Ikony wektorowe (siatka 16×16, jedna grubość kreski, minimum detali — czytelne od 16 px) (klawiatura/MIDI, fala/audio, pattern, aranżacja, play/stop/rec, plus, taśma, metronom, pętla, nuta) i ilustracje do okien (syntezator, magnetofon, dyskietka) — wszystkie w polu 196×150. |
| `gfx/font.*` | Własny font kreskowy (wektorowy) na siatce 8×12, wygładzanie Chaikina. Wielkie litery, cyfry, interpunkcja. |
| `io/image_io.*` | Zapis PNG (bez zlib) i WAV 16-bit — do trybu headless. |
| `io/fs.*` | Pliki/katalogi (POSIX): istnienie, mkdir -p, usuwanie drzewa, lista podkatalogów od najnowszego. |
| `io/project_file.*` | Wspólny zapis/odczyt instrumentu (`writeInstrument`/`parseInstrument`, także dla presetów). Format projektu `.dpp` — tekstowy, linia po linii (opis w nagłówku). Nieznane linie pomijane, wersja formatu w pierwszej linii. Zapis atomowy: `.tmp` → fsync → rename. |
| `io/preset_store.*` | Presety instrumentów: `presets/wavetable/<NAZWA>.dpr` (tekst: nazwa, kategoria, linia `inst` w tym samym zapisie co instrument w projekcie), lista z kategoriami, wczytaj/zapisz/usuń, INIT, zestaw fabryczny wersjonowany znacznikiem `_factory_vN` — nowsze wersje dopisują tylko brakujące presety, niczego nie nadpisując (v1: 11 presetów, v2: +10 presetów trybu rozszerzonego). |
| `io/project_store.*` | Projekty na karcie: `projects/<NAZWA>/project.dpp` (później też `audio/*.wav`), lista, zapis/odczyt/usuwanie, ostatnio otwarty (`projects/_last.txt`). |
| `util/spsc.h` | Kolejka lock-free jeden producent / jeden konsument. |
| `util/triple_buffer.h` | Potrójny bufor bez blokad do przekazywania migawek danych (pattern) do audio. |
| `synth/note_sink.h` | Interfejs instrumentu: `noteOn / noteOff / allOff` (wołany w wątku audio). |
| `synth/wavetable_synth.*` | **WAVETABLE** — port 1:1 silnika op1r (Hundekraft, `third_party/op1r_wavetable`): oryginalny DSP w C bez zmian (2 oscylatory wavetable z morfingiem 8 kształtów, detune, ladder z rezonansem, soft clip, kompensacja poziomu), głos 96 kHz (2× jak na OP-1) w blokach po 128, decymacja 2:1 FIR z harnessa op1r (bufor kołowy, ta sama kolejność sumowania). 6 głosów na ścieżkę; POLY/MONO (stos nut, legato), portamento (w unisonie wspólny punkt startu dla całego stosu; opcjonalnie zsynchronizowane z BPM), UNISON z kompensacją głośności 1/√6, UNISON (6 głosów na nutę w MONO), AGE (stały „charakter” głosu + powolny losowy dryf strojenia i filtra, jak „vintage” w Prophet-6/OB-6); własny ADSR (na OP-1 robił to firmware), obwiednia filtra na głos (A/D/S/R + AMOUNT ±), obwiednia modulacji MOD ENV na głos (A/D/S/R + AMOUNT ± + cel jak w LFO; cel VOL interpolowany próbka po próbce), AMOUNT obwiedni głośności −100..+100 % (bramka ↔ obwiednia, ujemny = odwrócona), 2 LFO na ścieżkę biegnące w czasie rzeczywistym także w ciszy, z synchronizacją z tempem (1/1 … 1/128T; przy grającym utworze faza przypięta do pozycji w takcie), wygładzone ~3 ms (krawędzie SAW/SQR bez kliknięć), tremolo interpolowane próbka po próbce (cele: WAVE, DTUN, CUT, RES, PTCH, VOL) — modulacja przez gałki silnika, które oryginalny DSP wygładza w bloku; wyciszanie ~2 ms przy przejmowaniu brzmiącego głosu, min. atak 1 ms / wybrzmienie 3 ms (bez kliknięć). Przy zerowych modulacjach wynik bit w bit jak oryginał. Zgodność bit w bit z oryginałem: `tools/test_wavetable.sh`. ~1,4% rdzenia A53 na głos. **Tryb rozszerzony** (R3, `Track::alt`): własny DSP wzorowany na `wavetable_sample` (te same tabele, arytmetyka, ladder, soft clip, kompensacja) — 2 niezależne oscylatory (fala, interwał ±24, detune ±50 c, poziom), SUB (sinus −1 okt.), filtr LP24 (= oryginał) / LP12 / BP / HP z wyjść stopni ladderu (skale wyjść + kompensacja rezonansu dobrane pomiarem: nasycenie nie większe niż w LP24; wyrównanie głośności za soft clipem), suma OSC1+OSC2+SUB normalizowana, DRIVE; cele LFO W1, W2, P1, P2, LV1, LV2 (WAVE = obie fale). Z ustawieniami jak oryginał różnica ~−77 dB; koszt ~10% rdzenia przy 6 głosach (oryginał 9,3%). |
| `synth/saw_synth.*` | Testowy instrument: 8 głosów, piła polyBLEP, łagodny filtr dolnoprzepustowy, obwiednia A/D/S/R. |
| `synth/tone_synth.*` | Pojedynczy ton sinus z drive — tylko dla ekranu testowego sprzętu. |
| `seq/pattern.*` | Pattern: nazwa + lista nut (start, długość w tickach, wysokość, velocity), 1..16 taktów 4/4, max 512 nut. Pilnuje, by nuty o tej samej wysokości nie nachodziły na siebie. |
| `seq/project.*` | Projekt: BPM, master, ścieżki (max 16 MIDI + 4 audio; nazwa, kolor, głośność, mute/solo, instrument: silnik + 4 gałki + ADSR + obwiednia filtra + 2 LFO; usuwanie razem z klipami), pula patternów (max 64), klipy na playliście (ścieżka, pattern, takt startu; max 512). Kolizje klipów, długość utworu, domyślne nazwy. |
| `seq/undo_stack.h` | Historia zmian: pierścień 32 migawek projektu (~7 MB, przydzielone raz); App ma dwie — undo i redo. |
| `seq/sequencer.*` | (pozycja w ćwierćnutach dla LFO z synchronizacją) Odtwarzanie z dokładnością do próbki w dwóch trybach: SONG (pętla IN/OUT w taktach albo cały utwór; bez pętli gra raz i staje) i PATTERN (jeden pattern, opcjonalna pętla IN/OUT w krokach). Sam pamięta grające nuty, więc edycja w trakcie nie zawiesza dźwięku. |
| `audio/engine.*` | Silnik audio (instrument wg ścieżki: WAVETABLE albo TEST SAW, przełączany z migawki projektu; + poziomy ścieżek do VU: szczyt z opadaniem ~24 dB/s, po głośności ścieżki): komendy z UI, migawka projektu, mute/solo (w trybie SONG; nie wyzwala nut i wycisza grające), instrument (SawSynth) na każdą ścieżkę, blok dzielony na odcinki między tickami, miks, master i łagodny ogranicznik. |
| `ui/input.h` | Logiczne przyciski i osie (niezależne od sprzętu). |
| `ui/buttons.h` | Wciśnięcie / puszczenie / auto-powtarzanie przycisków dla ekranów. |
| `ui/encoder.h` | Analog jako enkoder względny: martwa strefa, krzywa, tryb ciągły i „ząbkowy”. |
| `ui/view.h` | Bazowa klasa ekranu (update z wspólnym `Buttons`, render). |
| `ui/app.*` | Właściciel projektu i ekranów: zakładki PROJECT ↔ ARRANGE ↔ PIANO ROLL (L2/R2, kropki pod nagłówkiem), publikacja projektu do silnika, stan „niezapisane zmiany”, zapis/odczyt/nowy, autozapis przy wyjściu, start z ostatnim projektem (albo demo). Pasek narzędzi pod L3 (ARRANGE/PIANO ROLL), 2 warstwy przełączane L2/R2 (zakładki wtedy nie działają): 1) EDIT — B UNDO, A REDO, Y IN/OUT, X LOOP; 2) TRACK — B MUTE, A ENGINE, Y SOLO, X ustawienia ścieżki — ekran dostaje wtedy tylko D-pad/Start/R3. Krok historii zamyka się, gdy przyciski i gałki są puszczone (jedno wciśnięcie / jedno pokręcenie = jeden krok; okno dialogowe = jeden krok). Komunikaty „toast”. |
| `ui/project_view.*` | Ekran PROJECT (pierwsza zakładka): nazwa, statystyki, stan zapisu, lista projektów; zapis/wczytanie/nowy/usuń z potwierdzeniami. |
| `ui/arrange.*` | Ekran ARRANGE: ścieżki (nazwa, pasek głośności, segmentowy VU −48..0 dB, M/S), klipy z miniaturą nut, BPM w nagłówku, dodawanie ścieżek, wstawianie/klonowanie/przesuwanie/usuwanie klipów (sterowanie w nagłówku pliku). |
| `ui/dialogs.*` | Okno ENGINE (lista silników w jednej kolumnie + ilustracja i opis; z paska narzędzi, warstwa TRACK). Okna NEW TRACK, TRACK (edycja: nazwa, kolor z siatki 32 barw, usuwanie z potwierdzeniem) i NEW PATTERN w stylu menu (belki, ilustracja, pola, lista, pigułki akcji). |
| `ui/widgets.*` | Wspólne elementy: kompaktowy pasek 4 pigułek akcji (`actionBar`, w kolumnach gałek, z linią statusu), pigułki z tekstem dopasowanym do obrysu, nagłówek ekranów ARRANGE/PIANO ROLL (ten sam układ, pozycja „BAR takt.miara”), tekst przycinany do szerokości, belka nagłówka, pigułka z literą przycisku, pole wartości, lista, mini-gałka, pasek 4 enkoderów, edycja nazwy przyciskami. |
| `ui/piano_roll.*` | Ekran PIANO ROLL (tryb zaznaczania pod R3, 2 warstwy przełączane L2/R2 (wtedy L2/R2 nie zmieniają zakładek): 1) zaznacz/prostokąt, usuń, kopiuj, wklej — schowek między patternami; 2) transpozycja, przesuwanie, oktawa, duplikat — „wszystko albo nic” przy kolizjach): pattern z projektu, wiele taktów (widok 1 taktu + pasek pozycji), nuty jako pigułki, kolor = velocity, podgląd nut na instrumencie ścieżki (sterowanie w nagłówku pliku). |
| `ui/synth_view.*` | Zakładka SYNTH (R3 = oryginał ↔ rozszerzony; strony wg „rodzaju”: SOUND / OSC 1 / OSC 2 / FILTER / AMP ENV / FILTER ENV / LFO 1 / LFO 2): instrument bieżącej ścieżki — ekran silnika (gałki po modulacji ostatnio uderzonej nuty), strony gałek (Select): SOUND, VOICE, AMP ENV, FILTER ENV, MOD ENV (L3: DEST + AMOUNT), LFO 1, LFO 2, PRESETS (przeglądarka: kategorie z ikonami + presety, A wczytaj / X zapisz / B usuń / Y INIT); L3 na stronach obwiedni: RELEASE ↔ AMOUNT; wykresy obwiedni/LFO, granie nut z przycisków. |
| `ui/wavetable_screen.*` | (tryb rozszerzony: fala OSC 2 na czerwono z nawiasem, krzywa filtra wg trybu) Ekran WAVETABLE przeniesiony 1:1 z op1r (`wavetable_screen.c` + statyczna grafika wg `gen_svg.py`, geometria `wavetable_geom.h`), skala 2×, część statyczna w cache. |
| `ui/test_screen.*` | Ekran testu sprzętu (etap 0): ton, gałki, oscyloskop, podgląd przycisków i surowych kodów. Uruchamiany `--test`. |
| `headless.*` | Tryb bez okna: `--render-png plik [arrange|track|pattern|roll|test]`, `--render-wav plik [sekundy]` (cała aranżacja demo). |

## Platforma (`platform/`)
| Plik | Co robi |
|---|---|
| `main_sdl.cpp` | Okno/pełny ekran, tekstura, audio SDL (48 kHz float), zdarzenia pada i klawiatury, wybór ekranu. SIGTERM/SIGINT (np. Hotkey+Start w Knulli) = normalne wyjście z autozapisem. |
| `controls_map.h` | **Jedyna** tabela mapowania: klawiatura, pad SDL, surowe kody RG35XX Pro. |

## Testy (`tests/`)
- `wavetable_match.cpp` + `tools/test_wavetable.sh` — port WAVETABLE vs oryginalny harness op1r, porównanie próbka po próbce.

## Narzędzia (`tools/`)
`build.sh [desktop|arm]`, `deploy.sh` (ARM + wysyłka + wpis w Ports), `log.sh`, `dev.sh '<komenda>'`, `ssh_env.sh` (wspólne SSH z hasłem), `knulli/` (starter Ports i plik `.keys` z wyjściem Hotkey+Start).

## Czas i jednostki
- Tick = 1/96 ćwierćnuty. Szesnastka = 24 ticki. Takt 4/4 = 384 ticki.
- Wysokość = numer MIDI (60 = C4). Velocity 1..127.
