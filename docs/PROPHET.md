# PROPHET-5 — niezależny silnik DAW-POL

Punktem odniesienia integracji jest ukończony POLIVOKS. Prophet ma własny cały tor:
przydział pięciu głosów → dwa VCO na głos → mixer → nieliniowy czterobiegunowy LPF
→ własny VCA/ADSR → FIR → mikser DAW. WavetableSynth nie uczestniczy w syntezie,
obwiedniach, modulacji, glide, unison, rozstrojeniu ani decymacji.

## Źródła i zakres zgodności

Podstawa funkcjonalna to **oryginalna rodzina Rev 3 / CEM**:

- [Operation Manual CM1000D, Rev 3.3, 2/82](https://synthfool.com/docs/SequentialCircuits/Prophet_Series/Sequential%20Circuits%20Prophet%205%20Owners%20Manual.pdf), strony drukowane 2-4…2-11: oscylatory, mikser, filtr, obie obwiednie, UNISON/GLIDE, Poly-Mod/Wheel-Mod. PDF jest skanem — strony przeczytano jako obrazy, nie z pustego wyniku pdftotext.
- [Oryginalny Technical Manual Rev 3](https://www.synfo.nl/servicemanuals/Sequential/SEQUENTIAL_PROPHET-5-REV3_SERVICE_MANUAL.pdf).
- [Sequential: oryginalne karty barw](https://sequential.com/wp-content/uploads/2020/10/Original-Prophet-5-Patch-Sheets-1.1.pdf). Nowy bank DAW-POL jest autorski, nie stanowi rzekomego eksportu ROM-u ani rekonstrukcji fabrycznych nastaw.
- [Instrukcja obecnego Rev 4](https://sequential.com/wp-content/uploads/2021/02/Prophet-5-Users-Guide-1.3.pdf) służyła porównaniu; jej HALF keytracking, dodatkowe tryby przydziału i chord memory nie zostały przedstawione jako funkcje starego Rev 3.

Odwzorowane są funkcje panelu i przebieg sygnału. To **model DSP inspirowany
układami CEM, nie komponentowa ani pomierzona emulacja konkretnego egzemplarza**.
Nieliniowości, poziomy modulacji, krzywe gałek i tolerancje są dobrane do modelu;
nie mamy pomiarów prawdziwego P5 do potwierdzenia zgodności brzmienia 1:1.
Programator, pamięć i interfejs kasetowy oryginału zastępuje zapis projektu/presetów DAW.
Nie emulujemy nieobecnych gniazd CV, zdalnej klawiatury ani Z80.

## Rewizja dostarczonego kodu

Wejściowe pliki pozostają nietknięte w prywatnym `slooped_engines/prophet`.
Nowa implementacja: `core/synth/prophet_dsp.[ch]`, adapter `prophet_synth.[ch]`.
Najważniejsze problemy starego silnika:

- Sześć zamiast pięciu głosów; brak własnego VCA/ADSR i przydziału klawiatury.
- Wspólne LFO przesuwane przy renderowaniu każdego głosu, zatrzymywane przy zerowym kole.
- Synteza przy połowie częstotliwości hosta i interpolacja do jego bufora.
- Niepełne kombinacje fal B, rozbieżne przełączniki modulacji, szum biały zamiast różowego w Wheel-Mod.
- Błędna aproksymacja strojenia, niezgodny predyktor nieliniowej pętli filtra,
  ryzykowny wielomian wyjściowy i sztywne przycinanie do int16.
- Dane opisane jako „zmierzone tolerancje” bez źródła pomiarów.

Nowy silnik pracuje w float przy 48 kHz hosta, wewnątrz 4×. BLEP dla piły/pulsu,
cubic BLAMP dla trójkąta; hard sync resetuje A z B z pozycją ułamkową w próbce.
Sync jest przybliżeniem z interpolacją powierzchniową, nie pełnym minBLEP.
Filtr: cztery nieliniowe sekcje OTA-C, spójne rozwiązanie sprzężenia metodą Newtona,
limit pięciu ocen i próg reszty 1e-6; brak nieograniczonego wielomianu na wyjściu.
FIR 97 próbek usuwa pasmo nadpróbkowania. Pozostawiono zapas w float przed mikserem.

Obliczenia ARM czterech kart filtra są wektorowe NEON, piątej skalarne.
NEON oblicza tę samą charakterystykę wymierną; skalar interpoluje ją z błędem
poniżej 1e-6 pełnej skali. Tablice exp2 i współczynnika TPT zachowują dokładność
strojenia; ich generator to `tools/dsp/prophet_tables.py`. To stałe matematyczne,
a nie dane fal z innego instrumentu. Każda ścieżka ma osobne fazy, stany i ziarno szumu.
Nie ma alokacji ani blokad w callbacku audio.

LFO jest wspólne dla pięciu głosów, stale pracuje i sumuje dowolne kombinacje
SAW/TRI/SQUARE. Wheel-Mod miesza je z różowym szumem i ma pięć niezależnych
przełączników: FREQ A, FREQ B, PW A, PW B, FILTER. Poly-Mod sumuje obwiednię
filtra z wybraną kombinacją fal B; trzy niezależne cele to FREQ A, PW A i FILTER.
Fale są sumowane przy pełnym poziomie; mikser zmienia również nasycenie filtra.

UNISON = pięć kart, najniższa nuta i powrót do najniższej pozostałej. Glide wpływa
na oba VCO oraz śledzenie filtra wyłącznie w UNISON. Polifonia kradnie najcichszy
zwolniony głos, potem najstarszy trzymany. Przełączenie trybu uzgadnia trzymane nuty.
RELEASE OFF daje krótkie zamknięcie obu obwiedni bez natychmiastowego skoku VCA.
Czasy nominalne obwiedni: około 1 ms…30 s, środek gałki około 0,5 s.
VCO A/B: cztery oktawy w półtonach; B FINE: 0…+1 półtonu; PW około 1…99%.
OSC B LO FREQ: około 0,4…30 Hz przy odniesieniu C4; KEYBOARD jest niezależnym przełącznikiem.

Przy całkowicie zamkniętych VCA wygaszane są nieużywane pamięci filtra i tanio
utrzymywane zegary/fazy. Zwolnienia, trzymane klawisze i filtr w zakresie
samowzbudzenia zawsze liczą pełny tor. Nie jest to elektryczna symulacja
nieobserwowanego stanu pięciu stale zasilanych kart oryginału.

## Sterowanie

SELECT zmienia stronę, L3 otwiera powiązane dodatkowe ustawienia, L1/R1 zmienia
oktawę. D-pad/ABXY służą odsłuchowi. Nazwa presetu nad nazwą strony, cztery
kolory enkoderów, domknięte skraje przełączników, kolorowe ADSR z kulkami jak w POLIVOKS.

| Strona | Główne gałki | L3 |
|---|---|---|
| OSC A | WAVE, FREQUENCY, PULSE WIDTH, LEVEL | HARD SYNC |
| OSC B | WAVE, FREQUENCY, FINE, PULSE WIDTH | LO FREQ, KEYBOARD, LEVEL |
| FILTER | CUTOFF, RESONANCE, ENV AMOUNT, KEYBOARD | — |
| MIXER | OSC A, OSC B, WHITE NOISE, VINTAGE | — |
| FILTER ENV | ATTACK, DECAY, SUSTAIN, RELEASE | — |
| AMP ENV | ATTACK, DECAY, SUSTAIN, RELEASE | RELEASE switch, VELOCITY |
| POLY MOD | FILTER ENV source, OSC B source, FREQ A, PW A | FILTER destination |
| LFO | RATE/SYNC division, SHAPES, MOD WHEEL, LFO/PINK mix | BPM SYNC, DIVISION |
| WHEEL MOD | FREQ A, FREQ B, PW A, PW B | FILTER |
| VOICE | UNISON, GLIDE, MASTER TUNE, VINTAGE | RELEASE, VELOCITY, PITCH WHEEL, A-440 |
| PRESETS | standardowa przeglądarka DAW | zapis/odczyt jak w POLIVOKS |

R3 na VOICE wykonuje TUNE: koryguje bieżące odstrojenie VCO i filtrów, zachowując
swobodne fazy, obwiednie i dalszy dryft. Jest to kalibracja modelu, nie wykonanie
oryginalnej procedury pomiarowej firmware'u.

VINTAGE (różnice strojenia, powolny dryft, filtr, czasy obwiedni i poziomy kart),
VELOCITY i BPM SYNC są dodatkami programowymi. VELOCITY=0 daje oryginalną
niewrażliwość na siłę uderzenia. LFO FREE jest domyślne; SYNC ma 24 podziały
od 1/1 do 1/128, także D/T, z restartem przy START. Koło modulacji, pitch wheel
oraz strojenie są dostępne jako parametry DAW; mogą zostać zapisane razem z barwą.

## Grafika i presety

`prophet_art.inc`: 1605 konturów / 31 664 punkty, bezpośredni ślad ilustracji Patryka.
Zachowane postać, twarz, kaptur, dłonie, laski, czerwone wyładowania, oko i sześć
**dekoracyjnych** gwiazdek. To nie sześć głosów. Wskaźniki rzeczywistych głosów
są osobnym rzędem pięciu punktów. Napisy i atrapy kontrolek ze źródła są wycięte.
Paleta i rasteryzacja przy ekranie 640×480 wprowadzają różnice pikselowe — nie
zastępujemy jednak postaci nowym rysunkiem. Narzędzie: `tools/art/trace_prophet.py`;
Python/Pillow/OpenCV potrzebne tylko przy autorowaniu, JPEG nie jest zasobem aplikacji.
Poziom oscylatora barwi odpowiednią laskę, obok są jego prawdziwe kombinacje fal,
wykres audio i wspólny zegar LFO. Cache rysunku powstaje w wątku UI.

24 barwy: PROPHET BRASS, FIVE STRINGS, VELVET POLY, FAT FIVE BASS, ROUND WOOD BASS,
SYNC PROPHET, SINGING SYNC, LIQUID RESO, POLY PLUCK, GOLDEN KEYS, PULSE CHOIR,
CINEMA SWELL, POLYMOD BELLS, GLASS FILTER FM, BRONZE WIRES, SLOW ORBIT,
PER VOICE PWM, SILVER NOISE, SINE TEMPLE, PINK MOTION, OCTAVE FANFARE,
UNISON GLIDER, CLOCKWORK POLY, DARK ANALOG SEA.

Bank instalowany automatycznie w `presets/prophet5`, zachowuje istniejące pliki.
Projekt i preset: `engine prophet5 pfive 1` + 50 niezależnych wartości. Nie
interpretujemy parametrów dostarczonego, nigdy wcześniej niewpiętego DSP jako
istniejącego formatu projektów. Nie zmieniamy ID poprzednich silników ani banku POLIVOKS.

## ROM

Prywatne firmware'y nie trafiają do repo ani buildu. Plik `v81_2732.bin` ma
3072 bajty, SHA-256 `7990d7667b3b08a06755ba6ce98b57cc8e20afcd162c096bb1bcfb9dc6707eb5`.
Każdy z trzech plików Bank ma 2048 bajtów z dwoma jednakowymi blokami 1024.
Połączenie pierwszych bloków bank0+bank1+bank2 daje dokładnie `v81_2732.bin`.
To trzy segmenty tego samego kodu sterującego Z80, nie trzy banki próbek/brzmień.
Nie zawierają PCM do odtwarzania oscylatorów analogowego P5.

`p5diags.bin` i `P5diaginstrs.pdf` to osobny program diagnostyczny Boba Griega
(RAM, DAC/ADC, LED, strojenie/S&H), z warunkami niekomercyjnymi. Żaden jego kod
nie został użyty w silniku. `.gitignore` wyłącza cały dostarczony katalog źródłowy.

## Sprawdzenie i dostarczenie

`tools/check_prophet_arm.sh` buduje wyłącznie ARM i uruchamia krótką diagnostykę
na konsoli. Obejmuje stabilność i sygnał wszystkich 24 barw, skrajne nuty/Poly-Mod,
UNISON i powrót klawisza, RELEASE, LFO w ciszy i podział 1/128, zapis/odczyt
projektu i banku, dowolne granice segmentów sekwencera oraz brak wpływu wspólnych
ADSR/LFO/AGE/unison/glide WavetableSynth. Skalarne DSP sprawdzone także ASan/UBSan.

Pomiar na H700 przy około 1,416 GHz: pięciogłosowy akord, 24 presety po około
0,5 s — około 0,72 s CPU na sekundę sygnału (najcięższy w tym przebiegu 0,73).
To **sam DSP**, nie pełny projekt z efektami; wiele jednoczesnych ścieżek Prophet
w jednym callbacku ma istotny koszt. Priorytetem był pełny tor, nie dawny limit kilku procent.
Nie wykonano desktopowej wersji aplikacji ani renderów WAV do zatwierdzania brzmienia.
Ostateczna ocena barw i zgodności referencji należy do odsłuchu/oglądu na konsoli.

Końcowy build wgrany na `192.168.50.239:/userdata/dawpol3000/dawpol3000`.
SHA-256 lokalnie i na urządzeniu: `745ac3daf53cc156b79749b943a940c0a1ad83a9bbbc18bf7dad2d4a3bfc1127`.
Podglądy OSC/FILTER ENV/LFO wykonane binarką ARM: `out/prophet/{osc,env,lfo}.png`; obejrzane bez budowania aplikacji PC.
