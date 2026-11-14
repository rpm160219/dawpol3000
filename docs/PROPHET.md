# PROPHET-5 — niezależny silnik DAW-POL

Punktem odniesienia integracji jest ukończony POLIVOKS. Prophet ma własny cały tor:
przydział pięciu głosów → dwa VCO na głos → mixer → nieliniowy czterobiegunowy LPF
→ własny VCA/ADSR → FIR → mikser DAW. WavetableSynth nie uczestniczy w syntezie,
obwiedniach, modulacji, glide, unison, rozstrojeniu ani decymacji.

## Źródła i zakres zgodności

Podstawa funkcjonalna to **oryginalna rodzina Rev 3 / CEM**, rozszerzona o model Rev 2 / SSM:

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
| OSC A | WAVE, FREQUENCY, PULSE WIDTH, HARD SYNC | — |
| OSC B | WAVE, FREQUENCY, FINE, PULSE WIDTH | LO FREQ, KEYBOARD |
| FILTER | CUTOFF, RESONANCE, ENV AMOUNT, KEYBOARD | REVISION: REV 3 CEM / REV 2 SSM |
| MIXER | OSC A, OSC B, WHITE NOISE | — |
| FILTER ENV | ATTACK, DECAY, SUSTAIN, RELEASE | — |
| AMP ENV | ATTACK, DECAY, SUSTAIN, RELEASE | RELEASE switch, VELOCITY |
| POLY MOD | FILTER ENV source, OSC B source, FREQ A, PW A | FILTER destination |
| LFO | RATE/SYNC division, SHAPES, MOD WHEEL, LFO/PINK mix | BPM SYNC, DIVISION |
| WHEEL MOD | FREQ A, FREQ B, PW A, PW B | MOD WHEEL, LFO/PINK, FILTER |
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
Projekt i preset: `engine prophet5 pfive 2` + 51 niezależnych wartości. Odczyt `pfive 1` zachowuje poprzednie 50 wartości i ustawia REVISION na Rev 3. Dotychczasowe 24 presety fabryczne zostają w swoim formacie v1, bez podmiany nastaw. Nie
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

## Aktualizacja Rev 2 / animacje (2026-11-14)

**Wybór trybu:** FILTER → L3 → niebieski REVISION. Domyślny REV 3 CEM pozostaje
zatwierdzonym brzmieniem. REV 2 SSM jest drugim charakterem tego samego niezależnego
instrumentu; tryb zapisuje się w presecie/projekcie. Parametr P5_REVISION dopisany
na końcu, format v2 ma 51 wartości. Stare projekty i banki v1 automatycznie zachowują Rev 3.

Podstawy modelu SSM:

- [Oryginalny Technical Manual TM1000C, styczeń 1980](https://manuals.fdiskc.com/tree/Sequential%20Circuits/Sequential%20Circuits%20Prophet%205%20Service%20Manual.pdf), m.in. schemat toru na stronie 2-9: filtr 2040, obwiednia 2050, VCA 2020.
- [Sound Semiconductor SSI2140, dokumentacja oryginalnego projektanta SSM2040](https://www.soundsemiconductor.com/downloads/ssi2140datasheet.pdf), sekcja Unity-Gain Overdrive, strony 16–18. Dokument opisuje asymetryczne nasycenie pierwszej komórki i parzyste harmoniczne jako cechę rodziny SSM2040. SSI2140 jest nowszym układem; nie utożsamiamy jego parametrów pomiarowych z konkretnym starym P5.

Tryb Rev 2 zmienia **pierwszą nieliniową komórkę wewnątrz pętli filtra**, nie
nakłada saturatora na gotowe audio: sterowanie drive 1,32 i bias 0,18 przy pełnym
Rev 2, usunięcie składowej stałej i normalizacja małosygnałowego nachylenia.
Pochodna zmienionej komórki uczestniczy w tym samym rozwiązaniu Newtona, zarówno
w NEON, jak i skalarze. Przełączenie płynnie zmienia współczynniki przez około 3 ms,
zachowuje fazy i obwiednie. Przy VINTAGE zwiększa rozrzut/dryft VCO do 1,45×.
To **przybliżenie charakteru rodziny SSM**, z dobranymi współczynnikami — nie
pełna emulacja SSM2030/2040/2050, nie kalibracja pomiarowa i nie wykonanie firmware'u Z80.
Sam kształt bazowych VCO i obwiedni pozostaje wspólny; nie dodajemy różnic bez uzasadnienia.

ROM użytkownika: `Sequential Circuits Prophet 5 Rev 2.0 v7.0c.zip`. Zachowany w
ignorowanym `slooped_engines/prophet/rev2`, bez zmian w Downloads. Trzy pliki po
2048 bajtów, każdy ma identyczne połówki 1024 bajtów. To fragmenty firmware'u
Z80, bez próbek oscylatorów; nie są wbudowane w binarkę ani repo.

| Plik | SHA-256 |
|---|---|
| p5_v7.0c_0.bin | d26e3d4eb48fab7b2734b3a94f55dca48e85f35a2166520a673e84885144b4c4 |
| p5_v7.0c_1.bin | 45efddcc25ba936f0b0cf7cb0635b23acecfd4aa2b05e030b27b960bd3914f7b |
| p5_v7.0c_2.bin | 99b359bb47aa65d760b77b64c88ae893be5d26a30c376f8a147e8b0c5d9463c8 |

Grafika Rev 2: bursztynowa laska A, fioletowa B, cyjanowe pioruny i chłodne szaty.
Pozostają te same kontury postaci i dekoracji. Źrenica otwiera się z cutoffem, z zachowaniem oryginalnych powiek,
tęczówka zmienia biel w czerwień z rezonansem; promienie reagują na obwiednię
filtra i przypisany Wheel-Mod → FILTER. Wnętrza lasek przewijają wybraną falę/PW,
jasność wiąże się z poziomem oscylatora, atak daje rozbłysk, szaty rozświetla VCA.
Sześć gwiazdek mieni się **losowo**, niezależnie od głosów, LFO i Poly-Mod.
Stałe jasne wyładowania przemieszczają się wyłącznie po istniejących piorunach,
także w ciszy i przy wyłączonych trasach Poly-Mod; czerwone/cyjanowe podstawy nie gasną.

MIXER pokazuje trzy poziomy A/B/NOISE, zgodnie ze stylem Polivoksa. LEVEL jest
wyłącznie w mikserze; VINTAGE na VOICE. WHEEL MOD ma rzeczywisty rysunek koła,
obrót/znacznik i procent zgodny z P5_WHEEL, mieszankę LFO/PINK i pięć tras.
L3 na WHEEL MOD udostępnia koło, mieszankę i przełącznik FILTER. Miniatura koła
jest też widoczna obok głównej ilustracji.

Kontrole wykonano na ARM: 24 presety w obu trybach, skrajne ustawienia i zmiana
rewizji przy trzymanej nucie, zapis Rev 2, migracja starych 50 pól do Rev 3,
segmenty sekwencera i niezależny tor. Porównanie z zatwierdzonym commitem
`3762cd4`: **294 912 próbek Rev 3, maksymalna różnica 0**. Ten sam patch/ziarno
w SSM daje inny sygnał (suma bezwzględnych różnic 48,209 w porównanym przebiegu).
Średni koszt obu trybów około 0,738 s CPU/s pięciogłosowego audio, maksimum
0,756 w tym przebiegu. Podglądy ARM: `out/prophet/rev2`.

Wgrany build: `3991e0eee80040fa199e56a1dce52e303f410039bf3b0869f8e447665435b29b`
(SHA-256 lokalnie i na 192.168.50.239); nie wymaga instalowania ROM-u na konsoli.
