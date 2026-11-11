# Architektura DAW-POL 3000

Dokument opisuje, z czego składa się program i jak części ze sobą rozmawiają.
Aktualizowany przy każdej zmianie struktury. Kod: C++17 „C z klasami”, bez wyjątków i RTTI.

## Wątki
- **Wątek UI** (główny): wejście → logika ekranu → rysowanie do `Canvas` → wysłanie na ekran (SDL).
- **Wątek audio** (callback SDL, 48 kHz, blok 256 ramek): `AudioEngine::process`. Zero alokacji, zero blokad.
- Komunikacja UI → audio: kolejka komend `Spsc` (nuty, play/stop, tempo, głośność) oraz
  `TripleBuffer<Pattern>` (UI publikuje całą kopię patternu po każdej edycji).
- Audio → UI: zmienne atomowe (pozycja głowicy, czy gra).

```
 [Input SDL] -> InputState -> Ekran (PianoRoll) --Spsc komendy--> AudioEngine
                                   |           --TripleBuffer---> |  Sequencer -> SawSynth -> wyjście
                                   v           <--atomic tick---- |
                               Canvas 640x480 -> SDL tekstura
```

## Moduły (`core/` — niezależne od platformy)

| Plik | Co robi |
|---|---|
| `gfx/canvas.*` | Bufor 640×480 RGBA i rasteryzer z antyaliasingiem: linie, łuki, koła, prostokąty. Kreski wsadowe (`beginStroke/strokeSeg/endStroke`), żeby nakładające się segmenty nie ciemniały. Paleta 4 kolorów enkoderów (`pal::`). |
| `gfx/font.*` | Własny font kreskowy (wektorowy) na siatce 8×12, wygładzanie Chaikina. Wielkie litery, cyfry, interpunkcja. |
| `io/image_io.*` | Zapis PNG (bez zlib) i WAV 16-bit — do trybu headless. |
| `util/spsc.h` | Kolejka lock-free jeden producent / jeden konsument. |
| `util/triple_buffer.h` | Potrójny bufor bez blokad do przekazywania migawek danych (pattern) do audio. |
| `synth/note_sink.h` | Interfejs instrumentu: `noteOn / noteOff / allOff` (wołany w wątku audio). |
| `synth/saw_synth.*` | Testowy instrument: 8 głosów, piła polyBLEP, łagodny filtr dolnoprzepustowy, obwiednia A/D/S/R. |
| `synth/tone_synth.*` | Pojedynczy ton sinus z drive — tylko dla ekranu testowego sprzętu. |
| `seq/pattern.*` | Pattern: lista nut (start, długość w tickach, wysokość, velocity). PPQ 96, siatka szesnastek, 1 takt 4/4. Pilnuje, by nuty o tej samej wysokości nie nachodziły na siebie. |
| `seq/sequencer.*` | Odtwarzanie patternu w pętli z dokładnością do próbki. Sam pamięta grające nuty, więc usunięcie/skrócenie nuty w trakcie nie zawiesza dźwięku. |
| `audio/engine.*` | Silnik audio: zbiera komendy z UI, podmienia pattern, dzieli blok na odcinki między tickami sekwencera i renderuje instrument. |
| `ui/input.h` | Logiczne przyciski i osie (niezależne od sprzętu). |
| `ui/buttons.h` | Wciśnięcie / puszczenie / auto-powtarzanie przycisków dla ekranów. |
| `ui/encoder.h` | Analog jako enkoder względny: martwa strefa, krzywa, tryb ciągły i „ząbkowy”. |
| `ui/piano_roll.*` | Ekran PIANO ROLL (opis sterowania w nagłówku pliku). |
| `ui/test_screen.*` | Ekran testu sprzętu (etap 0): ton, gałki, oscyloskop, podgląd przycisków i surowych kodów. Uruchamiany `--test`. |
| `headless.*` | Tryb bez okna: `--render-png plik [roll|test]`, `--render-wav plik [sekundy]`. |

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
