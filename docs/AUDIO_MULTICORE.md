# Audio wielordzeniowe

Od 2026-11-14 `AudioEngine` rozdziela niezależne ścieżki między callback SDL i maksymalnie trzech stałych pomocników pthread. Domyślnie wybiera liczbę dostępnych CPU, ograniczoną do czterech wątków łącznie. H700 konsoli udostępnia cztery CPU.

## Przepływ i zachowanie brzmienia

1. Callback pobiera migawkę projektu i komendy UI, przygotowuje instrumenty i sekwencer.
2. Dzieli bufor na odcinki do 256 ramek oraz dokładne granice ticków sekwencera, jak wcześniej.
3. Każda dopuszczona ścieżka jest osobnym zadaniem z własnym stanem i buforem. Najdroższe silniki są na początku kolejki; kolejność przy równych kosztach jest stabilna. Atomowy licznik przydziela każde zadanie dokładnie raz.
4. Pomocnicy renderują ścieżki, a callback równocześnie obsługuje szeregową grupę starszych instrumentów i podejmuje pozostałe zadania.
5. Bariera kończy wszystkie zadania przed kolejnym zdarzeniem sekwencera. Dopiero potem callback przesuwa sekwencer. Komendy NOTE ON/OFF, TUNE, transport i konfiguracja nie zmieniają stanu renderowanego równolegle.
6. Mikser, inserty, EQ, kompresory, sidechain, AUX i master pozostają szeregowe. Kolejność sumowania ścieżek jest identyczna z poprzednią wersją.

Żaden DSP instrumentu nie został zmodyfikowany. PROPHET i POLIVOKS zachowują całkowicie niezależne tory. Praca jest dzielona między **ścieżki**, nie między głosy jednego instrumentu. Jeden pięciogłosowy Prophet nadal zajmuje jeden wątek; zysk pojawia się przy kilku ścieżkach.

## Dopuszczone silniki

| Grupa | Wykonanie |
|---|---|
| PROPHET, POLIVOKS | Równolegle, stan całego instrumentu per ścieżka |
| WAVETABLE i WAVETABLE+, TEST SAW | Równolegle |
| PARALLAX, LOUNGE, FELT | Równolegle |
| Pozostałe porty, w tym GRAND | Jedna grupa w callbacku, oryginalna kolejność ścieżek |
| Mikser i efekty | W callbacku po zakończeniu instrumentów |

Lista jest jawna: `AudioEngine::parallelSafe`. W starszych wrapperach op1r występują współdzielone wskaźniki bieżącej instancji, bufory robocze lub generatory losowe. GRAND ma modyfikowany `static uint32_t rs` w młotku (`grand_dsp.c`). Przeniesienie tych portów na osobne wątki wymaga osobnej rewizji ich stanu. Niewybrane ścieżki również zachowują dotychczasowy przebieg renderowania.

## Synchronizacja i cykl życia

`TrackWorkers::start/stop` działa poza aktywnym callbackiem: alokacja stosów, tworzenie wątków, semaforów i `pthread_join` wyłącznie przy inicjalizacji/zamknięciu. Niepowodzenie utworzenia pomocnika zmniejsza dostępną pulę; aplikacja może działać szeregowo. SDL zamyka urządzenie audio przed zniszczeniem silnika; pula jest ostatnim polem silnika i niszczy się przed stanem DSP.

W nowej ścieżce renderowania brak alokacji, mutexów i tworzenia wątków. Semafory budzą pomocników, publikacja deskryptora używa release/acquire, przydział zadań atomowego licznika. Po krótkim aktywnym oczekiwaniu callback może zasnąć na `FUTEX_WAIT_PRIVATE`, aż ostatni pomocnik wykona `FUTEX_WAKE_PRIVATE`. Porównanie i zaśnięcie są atomowe w jądrze; wcześniejsze zakończenie nie gubi pobudki. Bariera **może blokować** do zakończenia DSP. Nie należy opisywać całego callbacku jako nieblokującego. Istniejące `mixer_.apply` nadal może przygotowywać efekty przy zmianie projektu.

Callback przy pierwszym użyciu próbuje ustawić `SCHED_FIFO`, priorytet 8, jeśli dotąd działał jako `SCHED_OTHER`. Pomocnicy kopiują uzyskany priorytet i środowisko FPU callbacku. Na konsoli uprawnienia pozwalają na FIFO; szybkie budzenie pomocników ma duży wpływ na wynik. Na systemie bez uprawnień pozostaje dotychczasowy scheduler; gain z pomiarów H700 nie jest gwarancją dla innych systemów. Brak zmian globalnej konfiguracji OS, governorów i affinity. Implementacja puli wymaga Linuksa/POSIX, zgodnie z obecnymi platformami konsoli i PC Linux.

Konfiguracja przed startem procesu:

```sh
DAWPOL_AUDIO_THREADS=1 ./dawpol3000  # powrót szeregowy
DAWPOL_AUDIO_THREADS=4 ./dawpol3000  # maksymalnie trzy helpery + callback
```

Domyślny auto bez zmiennej; `AudioEngine::init(sr, 1)` jawnie wymusza wariant szeregowy. Liczba faktycznych wątków trafia do stdout: `Audio render: N threads`. Format projektów i sterowanie UI nie zmieniają się.

## Weryfikacja

- `tools/check_audio_workers_tsan.sh`: osobny test puli na hoście pod ThreadSanitizer, bez kompilowania aplikacji PC. 1–4 wątki, 300 partii na wariant, 0–32 zadań, każde wykonane raz, prawdziwa praca pomocników, bariery, FPU, stop/start. Wynik: PASS, brak zgłoszonych wyścigów. Ten sam test przechodzi na ARM.
- `tools/check_audio_parallel_arm.sh`: build ARM, wykonanie na konsoli, porównanie starej orkiestracji `AudioEngine` z zatwierdzonego `fdee48c` oraz nowej z jednym i czterema wątkami. Wszystkie korzystają z niezmienionego DSP. Każdy z 32 identyfikatorów silnika, także jednoczesne warianty WAVETABLE/+ i OXBOW/+, wielokrotne instancje, współpraca z Prophet/Polivoks, nuty, częściowe bloki 1/17/64/127/256/513/31 ramek, sekwencer, zmiana BPM, pętla, mute, zmiana instrumentu, usuwanie ścieżek, TUNE i Rev 2. Dalej JUNI CHORUS, AUX SISTER, VCA, EQ i master.
- **94 976 ramek stereo float (759 808 bajtów)**: stary silnik, nowy szeregowy i nowy równoległy identyczne bajt w bajt. SHA-256 wszystkich trzech strumieni: `b0935ad7bdf23bdedb6415400211deba64413e2726317aa784cb711bd2d1d497`. Pliki są surowym PCM do porównania, bez renderowania WAV i bez zmiany presetów użytkownika.
- ARC w oryginale wplata adres instancji w seed. Fixture porównania ustawia istniejącym `arcSetSeed` stałe seedy, żeby ASLR nie zmieniał referencji. Produkcyjny ARC bez zmian.
- Fixture wygasza hostowe głosy krótkim release przed zmianą silnika. Przy pozostawieniu długich ogonów poprzedni `AudioEngine` z `fdee48c` także kończy się SIGSEGV przy przejściu MBIRA → REACTOR (potwierdzony status 139). To istniejący problem zmiany portu z aktywnymi ogonami, poza zakresem równoległości; nie deklarujemy naprawienia tej ścieżki.
- LINEA w tym teście nie ładuje prywatnych ROM-ów: sprawdzona ścieżka braku ROM/silence, nie dźwięk PCM D-50.
- Próba uruchomienia pełnego SDL przez dummy video/audio nie jest dostępna w SDL konsoli (`dummy not available`). Nie uruchamiano drugiej aplikacji na fizycznym audio/ekranie ani nie przerywano istniejącej sesji. Cykl start/stop puli sprawdzono osobnym testem; odsłuch realnego wyjścia po restarcie pozostaje oceną użytkownika.

## Pomiary na konsoli

H700, CPU około 1,512 GHz, 48 kHz, bufor 256 ramek = **5,333 ms budżetu**. Aplikacja użytkownika pozostawała uruchomiona w tle. Poniższe wyniki pochodzą z ostatniego pełnego przebiegu, po teście zgodności.

| Projekt / wątki | Mediana | P95 | Maksimum | Przekroczenia budżetu |
|---|---:|---:|---:|---:|
| 3 Prophety × 5 głosów / 1 | 11,241 ms | 14,101 ms | 14,939 ms | 240/240 |
| 3 Prophety × 5 głosów / 4 | 3,783 ms | 3,852 ms | 7,368 ms | 1/240 |
| 4 Prophety × 5 głosów / 4, taktowane co 5,333 ms | 3,799 ms | 3,915 ms | 4,313 ms | 0/600 |

Mediana trzech Prophetów poprawia się około **2,97×**. Energia wyjścia w obu wariantach taka sama: `153.059844292`. W poprzednich dwóch przebiegach wariant trzech Prophetów miał maksima 4,133 i 4,450 ms; czterech taktowanych: raz 1/600 spóźnionych buforów, raz 0/600. Wynik zależy od obciążenia, planisty i temperatury. Pojedyncze przekroczenie także przy trzech instrumentach oznacza, że pomiar nie gwarantuje braku trzasków przy dowolnym projekcie. Ciężkie efekty i pozostałe instrumenty zużywają dalszą część budżetu.

Build ARM wgrany atomowo przez SSH do `/userdata/dawpol3000/dawpol3000`, SHA-256 lokalna i zdalna: `4a704d59e0532c959ecde341099ab092c00946d2398b4970b85a523ac1dfbe34`. Aktywna sesja nie była restartowana. Nowy kod zaczyna działać po ponownym otwarciu programu z Ports.
