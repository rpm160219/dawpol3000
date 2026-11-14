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

- MONO: ostatni naciśnięty klawisz steruje oboma VCO; Note Off wraca do ostatniego nadal trzymanego.
- DUO: najwyższy klawisz → I, najniższy → II; jeden wspólny filtr i VCA.
- Powrót do trzymanych nut i glide pochodzą z Polivoksa. Glide obejmuje oba VCO w MONO, VCO I w DUO; pierwsza nuta po pełnym wygaśnięciu nie ślizga się od starego tonu.
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
złożone kontury i 32 kolory z zatwierdzonej palety. Używany jest natywny
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

Select: dziewięć stron, start na OSC 1. L3 przełącza dodatkowe kontrolki
sekcji; MIXER i LFO mają jeden komplet kontrolek.

| Strona | Podstawowe parametry | L3 |
|---|---|---|
| OSC 1 | wave, range, level, cross-mod II→I | własna głębokość LFO I |
| OSC 2 | wave, range, level, detune | LFO II, fine II, PWM II |
| FILTER | cutoff, resonance, env mod, LP/BP | keytrack, LFO→filtr |
| MIXER | poziom I, poziom II, szum, ring | jedna warstwa |
| FILTER ENV | ADSR filtra | single/repeat |
| AMP ENV | ADSR VCA | single/repeat, VCA hold |
| LFO | rate/SYNC division, depth, shape, dest | FREE/SYNC do BPM |
| VOICE | master tune, glide, mono/duo, VINTAGE | velocity, EXCITE, VCA hold |
| PRESETS | standardowa przeglądarka DAW-POL | standardowe akcje |

LFO ma własne cztery przebiegi: TRI/SQR/S&H/NOISE i cztery równolegle aktywne
głębokości: OSC 1, OSC 2, FILTER, AMP. DEST wybiera tylko parametr edytowany
przez DEPTH; nie wyłącza pozostałych dróg. Po wejściu na stronę wybierana jest
najsilniejsza modulacja. Wykres przedstawia wybraną falę, obok są poziomy
wszystkich dróg i wskaźnik aktualnej wartości własnego LFO z telemetrii.
Nie używamy LFO ani obwiedni WavetableSynth.

Włączenie Repeat ustawia sustain danej obwiedni na zero. Ręczne zwiększenie
sustain wybiera SINGLE, więc podpis trybu odpowiada rzeczywistemu działaniu.
Wykres ADSR jest tylko na stronach obwiedni; oscylatory mają własny kształt
fali, filtr schemat LP/BP, mikser wskaźniki poziomów, VOICE opis mono/duo.
SAW ma liniowy rampowy przebieg z pionowym resetem, również przy sprężynie.

Sprężyny oraz podpisy i symbole fal przechodzą od białego przy poziomie 0 do
niebieskiego/zielonego przy poziomie 100%. Pokazują poziom VCO w mikserze:
OSC II nadal może modulować I przy poziomie II=0. Z grafiki bazowej usunięto
pozostałości zielono-niebieskiej wiązki; oscyloskop rysowany jest osobno,
także przy zamkniętej paszczy i wąskim BP. Paleta głowy/futra pozostaje stała.

Rejestry mają pięć pozycji: 32'/16'/8'/4'/2'. Zakresy DSP: A do 3 s,
D/R do 12 s. UI udostępnia 42 parametry. EXTERNAL i OUTPUT są schowane;
wszystkie 44 pola nadal zapisuje się w projekcie/presecie. Wartość OUTPUT
z poprzednich presetów pozostaje bez zmian. EXTERNAL w adapterze otrzymuje
ciszę, ponieważ aplikacja nie ma obecnie toru wejścia audio. Ring, PWM,
velocity i filter excite są dodatkami programowymi; EXCITE wzbudza filtr
mikroszumem i nie jest oryginalnym przełącznikiem samooscylacji.

## Presety i pliki

24 presety są instalowane w `presets/polivoks/` jako `.dpr`: pierwsze 12
zatwierdzonych barw zachowuje parametry i wersję fabryczną 1; kolejne 12 ma
wersję 2. Aktualizacja dopisuje nowe pozycje bez podmiany starych barw ani
istniejących własnych presetów o nazwach nowych pozycji. Test obejmuje obie
sytuacje. Lista dodatków: [POLIVOKS_PRESETS.md](POLIVOKS_PRESETS.md).
INIT w tym instrumencie przywraca FOUNDATION. Factory bank jest wersjonowany
standardowym mechanizmem projektu. Parametry użytkownika nie są podmieniane
przy ponownym starcie programu.

`Track::polivoks[44]` i `polivoksSet` zawierają komplet parametrów. Zapis instrumentu
ma token `engine polivoks` i pole `pvox 3 <44 wartości>`. Te same pola przechodzą
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

Dodatkowe ekrany: `polivoksosc1`, `polivoksosc2`, `polivoksmixer`, `polivoksenv`, `polivoksamp`,
`polivokslfo`, `polivokscontrol`, `polivoksloop`, `polivoksbp`.
`POLIVOKS_ALT=1` pokazuje drugą warstwę, `POLIVOKS_PRESET=0..23` wybiera preset.
`POLIVOKS_CUTOFF=0..32767` i `POLIVOKS_RESONANCE=0..32767` ustawiają kontrolki
w podglądzie PNG, żeby porównać skrajne ustawienia animacji.
Główny podgląd `polivoks` otwiera filtr i zwiększa rezonans dla ilustracji;
nie zmienia to fabrycznych danych banku.

Test integracji porównuje 24 presety z samodzielnym rdzeniem, z nutami,
legato, release i odcinkami 1/37/61/113/128 próbek. Sprawdza wszystkie 41 pól
w pliku projektu i presecie, cały 24-pozycyjny factory bank, Note Off po zmianie oktawy,
usunięcie/ponowne użycie ścieżki, STOP oraz osiem stron edycji UI i ich warstwy L3.
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

Dodatkowe parametry podglądów: `POLIVOKS_LEVEL1/LEVEL2=0..32767`,
`POLIVOKS_LFO_WAVE=0..32767` (znormalizowana pozycja, nie indeks).
`tools/export_polivoks_factory.py` odtwarza tabelę z JSON samodzielnego banku,
sprawdzając zgodność pierwszych 12 tablic z `tests/polivoks_approved.inc`.
Testy sprawdzają wszystkie 39 dostępnych kontrolek, nienaruszanie dwóch ukrytych,
cztery niezależne głębokości LFO, cztery różne wykresy fal, SINGLE/REPEAT,
kolor sprężyn oraz brak barwnych odnóg poza wąskim trójkątem BP.

Przy poprawce wiązki porównano górną część zaakceptowanej ilustracji
(x=230..409, y=100..196) z nowym renderem dla tych samych cutoff/resonance:
0 zmienionych pikseli. Porównanie dotyczy tej części głowy; sprężyny i dolny
obszar są celowo animowane/oczyszczone.

## Kolejna aktualizacja — SYNC, VINTAGE i animacja

L3 na stronie LFO przełącza FREE/SYNC. FREE zachowuje osobny parametr Hz;
SYNC używa 24 podziałów od 1/1 D do 1/128 T (proste, z kropką D, triolowe T).
Natywne LFO otrzymuje BPM i pozycję transportu z sekwencera. START restartuje
jego zegar; przy odtwarzaniu faza jest przypięta do pozycji względem START.
Wysokie podziały nie są ograniczane limitem 35 Hz trybu FREE. Przy zatrzymanym
transporcie LFO nadal biegnie z częstotliwością wyliczoną z BPM.

VINTAGE w VOICE steruje dodatkowymi tolerancjami VCO, nasileniem wolnego dryfu,
rozrzutem czasów obwiedni oraz charakterystyki/nasycenia filtra. Kalibracja jest
stała w obrębie instancji, a dryf ciągły. Osobny strumień losowy nie zmienia
bazowego generatora szumu. VINTAGE=0 wyłącza dodatkowe tolerancje. Ring i PWM
pozostają we własnym torze. Zmiany zachowania MONO/glide są zamierzonymi poprawkami.

Format v3 dopisuje VINTAGE, LFO SYNC i LFO DIV do 41 pól v2. Parser zachowuje
obsługę v1/v2. Dotychczasowe patche wczytują FREE, VINTAGE=0 i podział 1/4;
pełne ustawienia v3 zapisują się w projektach i presetach. Fabryczne 24 pozycje
nie są nadpisywane tylko z powodu nowego formatu.

Nazwa presetu jest nad nazwą strony, wyrównana do x=20, w dotychczasowym
kolorze i foncie. Wskaźniki przełączników używają indeksu pozycji, więc WAVE,
RANGE, SHAPE i podziały SYNC osiągają oba końce łuku. Pod wykresem LFO są
pełne podpisy OSC 1 / OSC 2 / FLT / AMP. Z VOICE usunięto HIGH NOTE i LEGATO.

Gwiazdy pulsują własnym sygnałem LFO tylko przy LFO→FLT > 0; głębokość określa
siłę pulsowania. Jasne iskry zmieniają położenie, czerwone pioruny pozostają
widoczne i krótko przygasają podczas grania. Sześć małych planet w przygaszonej
palecie ma losowe pozycje w rozłącznych strefach wolnego nieba, z zapasem na
pierścienie i oddzieleniem od nagłówka, gwiazd, czapki, cewek i piorunów.
Losowość grafiki jest wyłącznie po stronie UI i nie dotyka audio.

Walidacja tej aktualizacji: build ARM i wysyłka ze zgodną sumą SHA-256.
Na wyraźną prośbę Patryka nie wykonywano renderów ani testów; wcześniejsze
wyniki powyżej dotyczą poprzedniej wersji. Dostosowano istniejące oczekiwania
źródeł testów do nowego API/priorytetu, bez uruchamiania ich.
