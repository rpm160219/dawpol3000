# POLIVOKS — niezależny instrument DAW-POL

Silnik i zatwierdzone 12 presetów pochodzą ze samodzielnego projektu Polivoksa.
W DAW-POL pracują jako osobny `PolivoksSynth : NoteSink`, obok `WavetableSynth`.
Kod `wavetable_synth.*` nie został zmieniony przy tej integracji.

## Tor audio

Sekwencer/przyciski → `PolivoksSynth` → własny `PolivoksCore` → float stereo
(identyczny sygnał mono na L/R) → kanał miksera DAW-POL.

Polivoks zachowuje własne oscylatory, mikser, nieliniowy filtr LP/BP, dwie obwiednie,
Repeat, LFO, glide, duofonię, nasycenie i FIR. Wspólne obwiednie, LFO, AGE,
unison i decymator WavetableSynth nie są wywoływane dla tego instrumentu.
Pozostaje zwykły kanał miksera: głośność, panorama, EQ, kompresor, inserty i AUX.

Host pracuje przy 48 kHz, rdzeń Polivoksa przy 192 kHz (4×). Nie ma dodatkowej
konwersji 96→48 kHz. FIR rdzenia ma nominalnie 12 próbek opóźnienia hosta.
Do integracji dodano `BeginFrames/RenderFrames` dla odcinków 1–128 próbek,
bez zmiany równań syntezy. Zdarzenie sekwencera może więc przypaść wewnątrz
standardowego bloku 128 i nie musi czekać na opróżnienie gotowego bufora.
Stare API 128-próbkowe pozostaje zgodne.

Instancje przydzielane są w init, poza callbackiem. Render nie alokuje pamięci.
Telemetria (obwiednie, fazy, LFO, klawisze i 128 punktów oscyloskopu) przechodzi
z audio do UI przez potrójny bufor. UI nie odczytuje bezpośrednio stanu DSP.

## Granie i zatrzymywanie

- MONO: najwyższy trzymany klawisz steruje oboma VCO.
- DUO: najwyższy klawisz → I, najniższy → II; jeden wspólny filtr i VCA.
- Legato, powrót do trzymanych nut i glide pochodzą z Polivoksa.
- L1/R1 transponuje następne nuty instrumentu. Adapter pamięta rzeczywistą nutę
  uruchomioną przez każdy klawisz, więc zmiana oktawy nie zawiesza Note Off.
- STOP puszcza również nuty odsłuchowe. Usunięcie ścieżki zatrzymuje transport,
  puszcza aktywne nuty i resetuje instancje Polivoksa, żeby przenumerowane ścieżki
  nie odziedziczyły trzymanych klawiszy.
- Przełączenie z Polivoksa czyści jego bramki i ogony; fazy i dryf rdzenia nie
  są resetowane przy zwykłym Note On/Off.

## UI

Kontury wektorowe pochodzą bezpośrednio z referencji Gemini przekazanej przez
Patryka (2400×1792): zachowane detale futra, uszanki/CCCP, nosa, zębów,
mechaniki, kurtek, gwiazd, cewek i błyskawic. `ui/polivoks_art.inc` zawiera
2387 złożonych konturów, 58 533 wierzchołki i 32 kolory. Używany jest natywny
Canvas z antyaliasingiem; obie ilustracje (instrument i karta ENGINE) korzystają
z tego samego bufora konturów, inicjalizowanego raz po stronie UI.

Paszcza reaguje wyłącznie na parametr CUTOFF, ze złagodzeniem ruchu między
klatkami. Malejący cutoff podnosi dolne zęby i brodę, rosnący otwiera pysk.
Czapka, brwi, oczy, nos, górne kły i ramiona zachowują swoje kontury.
Obwiednia ani poziom dźwięku nie sterują ruchem szczęki. REZONANS płynnie
barwi optykę oczu oraz jej poświatę: biały przy 0%, czerwony przy 100%.
W stożku jest próbka rzeczywistego wyjścia instrumentu; BP zwęża wiązkę.

Narzędzie odtwarzania geometrii: `tools/art/trace_polivoks.py` (instrukcja
w `tools/art/README.md`). Źródłowe napisy UI są pomijane. Fragment prawego
policzka, brody i kurtki zasłonięty oscyloskopem w pliku źródłowym odtworzono
z widocznej lewej połowy; aplikacja nakłada własny oscyloskop. JPEG nie jest
potrzebny do budowania ani uruchamiania. Przy skalowaniu do 640×480 używana
jest paleta konturów, więc nie deklarujemy identyczności pikseli z JPEG.

Select: osiem stron. L3: druga warstwa, kolory enkoderów jak w reszcie aplikacji.

| Strona | Podstawowe parametry | L3 |
|---|---|---|
| SOUND / FILTER | cutoff, resonance, env mod, LP/BP | keytrack, auto-osc, glide, mono/duo |
| OSCILLATORS | fala I, fala II, detune II, FM | rejestr I, rejestr II, fine II, master tune |
| MIXER | poziom I, poziom II, ring, szum | output, external, PWM II, velocity |
| FILTER ENV | ADSR filtra | A, D, env mod, single/repeat |
| AMP ENV | ADSR VCA | A, D, VCA hold, single/repeat |
| MODULATOR | rate, wave, filtr, pitch I | rate, pitch II, tremolo, PWM II |
| CONTROL | tune, glide, mono/duo, output | keytrack, velocity, hold, auto-osc |
| PRESETS | standardowa przeglądarka DAW-POL | standardowe akcje |

Włączenie Repeat ustawia sustain danej obwiedni na zero, zgodnie z podpisem w UI.
Rejestry mają pięć pozycji, także 32'. Zakresy czasu odpowiadają aktualnemu DSP
(A do 3 s, D/R do 12 s), a nie starszym zakresom z makiety. Host nie ma jeszcze
toru nagrywania/wejścia USB, więc EXTERNAL zachowuje parametr i API rdzenia,
ale adapter ścieżki obecnie podaje na wejście zewnętrzne ciszę.

## Presety i pliki

12 zatwierdzonych presetów jest instalowanych w `presets/polivoks/` jako `.dpr`.
INIT w tym instrumencie przywraca FOUNDATION. Factory bank jest wersjonowany
standardowym mechanizmem projektu. Parametry użytkownika nie są podmieniane
przy ponownym starcie programu.

`Track::polivoks[41]` i `polivoksSet` zawierają komplet parametrów. Zapis instrumentu
ma token `engine polivoks` i pole `pvox 2 <41 wartości>`. Te same pola przechodzą
przez zapis projektu i presetów. `pvox 1 <34 wartości>` korzysta z migracji v1.
Istniejące identyfikatory innych silników zachowano, nowy dopisano na końcu enum.

## Weryfikacja i podglądy

```sh
./tools/build.sh
./tools/build.sh arm
./tools/test_polivoks.sh
build/dawpol3000 --render-png out/polivoks.png polivoks
build/dawpol3000 --render-png out/polivoks_presets.png polivokspresets
build/dawpol3000 --render-png out/polivoks_engine.png polivoksengine
build/dawpol3000 --render-wav out/polivoks.wav 12 polivoks
```

Dodatkowe ekrany: `polivoksosc`, `polivoksmixer`, `polivoksenv`, `polivoksamp`,
`polivokslfo`, `polivokscontrol`, `polivoksloop`, `polivoksbp`.
`POLIVOKS_ALT=1` pokazuje drugą warstwę, `POLIVOKS_PRESET=0..11` wybiera preset.
`POLIVOKS_CUTOFF=0..32767` i `POLIVOKS_RESONANCE=0..32767` ustawiają kontrolki
w podglądzie PNG, żeby porównać skrajne ustawienia animacji.
Główny podgląd `polivoks` otwiera filtr i zwiększa rezonans dla ilustracji;
nie zmienia to fabrycznych danych banku.

Test integracji porównuje 12 presetów z samodzielnym rdzeniem, z nutami,
legato, release i odcinkami 1/37/61/113/128 próbek. Sprawdza wszystkie 41 pól
w pliku projektu i presecie, cały factory bank, Note Off po zmianie oktawy,
usunięcie/ponowne użycie ścieżki, STOP oraz siedem stron UI i ich warstwy L3.
Przy integracji wykonano dodatkowe porównanie z kopią DSP sprzed zmian API:
921 600 próbek float, maksymalna różnica 0 na desktopie. Nowy kod DSP i adapter
przeszły ASan/UBSan (LeakSanitizer wyłączony ze względu na ptrace środowiska).
Wierność z fizycznym Polivoksem nadal wymaga odrębnej kalibracji sprzętowej.

Regresja wspólnego toru: 8-sekundowy `synthwavefx` porównany z renderem
sprzed integracji — pliki WAV identyczne bajtowo.

Testy obrazu kontrolują biały/czerwony kolor obu oczu, ograniczenie zmian
rezonansu do optyki, zmianę wysokości dolnej szczęki przez cutoff, stałość górnej
części twarzy oraz niezależność szczęki od obwiedni. Po zmianie grafiki render
dema Polivoksa jest identyczny bajtowo z poprzednim WAV (12 s).
