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
| `gfx/icons.*` | Ikony wektorowe (siatka 16×16, jedna grubość kreski, minimum detali — czytelne od 16 px) (klawiatura/MIDI, fala/audio, pattern, aranżacja, play/stop/rec, plus, taśma, metronom, pętla, nuta) i ilustracje do okien (syntezator, magnetofon). |
| `gfx/font.*` | Własny font kreskowy (wektorowy) na siatce 8×12, wygładzanie Chaikina. Wielkie litery, cyfry, interpunkcja. |
| `io/image_io.*` | Zapis PNG (bez zlib) i WAV 16-bit — do trybu headless. |
| `util/spsc.h` | Kolejka lock-free jeden producent / jeden konsument. |
| `util/triple_buffer.h` | Potrójny bufor bez blokad do przekazywania migawek danych (pattern) do audio. |
| `synth/note_sink.h` | Interfejs instrumentu: `noteOn / noteOff / allOff` (wołany w wątku audio). |
| `synth/saw_synth.*` | Testowy instrument: 8 głosów, piła polyBLEP, łagodny filtr dolnoprzepustowy, obwiednia A/D/S/R. |
| `synth/tone_synth.*` | Pojedynczy ton sinus z drive — tylko dla ekranu testowego sprzętu. |
| `seq/pattern.*` | Pattern: nazwa + lista nut (start, długość w tickach, wysokość, velocity), 1..16 taktów 4/4, max 512 nut. Pilnuje, by nuty o tej samej wysokości nie nachodziły na siebie. |
| `seq/project.*` | Projekt: BPM, master, ścieżki (max 16 MIDI + 4 audio; nazwa, kolor, głośność, mute/solo; usuwanie razem z klipami), pula patternów (max 64), klipy na playliście (ścieżka, pattern, takt startu; max 512). Kolizje klipów, długość utworu, domyślne nazwy. |
| `seq/sequencer.*` | Odtwarzanie z dokładnością do próbki w dwóch trybach: SONG (cała aranżacja w pętli) i PATTERN (jeden pattern na wybranej ścieżce). Sam pamięta grające nuty, więc edycja w trakcie nie zawiesza dźwięku. |
| `audio/engine.*` | Silnik audio: komendy z UI, migawka projektu, mute/solo (w trybie SONG; nie wyzwala nut i wycisza grające), instrument (SawSynth) na każdą ścieżkę, blok dzielony na odcinki między tickami, miks, master i łagodny ogranicznik. |
| `ui/input.h` | Logiczne przyciski i osie (niezależne od sprzętu). |
| `ui/buttons.h` | Wciśnięcie / puszczenie / auto-powtarzanie przycisków dla ekranów. |
| `ui/encoder.h` | Analog jako enkoder względny: martwa strefa, krzywa, tryb ciągły i „ząbkowy”. |
| `ui/view.h` | Bazowa klasa ekranu (update z wspólnym `Buttons`, render). |
| `ui/app.*` | Właściciel projektu i ekranów: przekazuje wejście, przełącza ekrany (L2 = ARRANGE, R2 = PIANO ROLL), publikuje projekt do silnika, projekt demo. |
| `ui/arrange.*` | Ekran ARRANGE: ścieżki, klipy z miniaturą nut, BPM w nagłówku, dodawanie ścieżek, wstawianie/klonowanie/przesuwanie/usuwanie klipów (sterowanie w nagłówku pliku). |
| `ui/dialogs.*` | Okna NEW TRACK, TRACK (edycja: nazwa, kolor, usuwanie z potwierdzeniem) i NEW PATTERN w stylu menu (belki, ilustracja, pola, lista, pigułki akcji). |
| `ui/widgets.*` | Wspólne elementy: belka nagłówka, pigułka z literą przycisku, pole wartości, lista, mini-gałka, pasek 4 enkoderów, edycja nazwy przyciskami. |
| `ui/piano_roll.*` | Ekran PIANO ROLL: pattern z projektu, wiele taktów (widok 1 taktu + pasek pozycji), nuty jako pigułki, kolor = velocity, podgląd nut na instrumencie ścieżki (sterowanie w nagłówku pliku). |
| `ui/test_screen.*` | Ekran testu sprzętu (etap 0): ton, gałki, oscyloskop, podgląd przycisków i surowych kodów. Uruchamiany `--test`. |
| `headless.*` | Tryb bez okna: `--render-png plik [arrange|track|pattern|roll|test]`, `--render-wav plik [sekundy]` (cała aranżacja demo). |

## Platforma (`platform/`)
| Plik | Co robi |
|---|---|
| `main_sdl.cpp` | Okno/pełny ekran, tekstura, audio SDL (48 kHz float), zdarzenia pada i klawiatury, wybór ekranu. |
| `controls_map.h` | **Jedyna** tabela mapowania: klawiatura, pad SDL, surowe kody RG35XX Pro. |

## Narzędzia (`tools/`)
`build.sh [desktop|arm]`, `deploy.sh` (ARM + wysyłka + wpis w Ports), `log.sh`, `dev.sh '<komenda>'`, `ssh_env.sh` (wspólne SSH z hasłem), `knulli/` (starter Ports i plik `.keys` z wyjściem Hotkey+Start).

## Czas i jednostki
- Tick = 1/96 ćwierćnuty. Szesnastka = 24 ticki. Takt 4/4 = 384 ticki.
- Wysokość = numer MIDI (60 = C4). Velocity 1..127.
