# DAW-POL 3000 — DAW na Anbernic RG35XX Pro, UI w duchu OP-1

Nazwa projektu: **DAW-POL 3000** (repo/binarka: `dawpol3000`).

## Zasady współpracy
- Właściciel: Patryk. Komunikacja **po polsku**, luźno, konkretnie.
- Claude prowadzi projekt w całości (architektura, kod, build, deploy, testy). Patryk tylko ocenia: „git / nie git”.
- Każdy etap kończy się czymś do sprawdzenia: PNG ekranu, WAV dźwięku albo build na konsoli. Najpierw pokazuj, potem idź dalej.
- **Raport po każdej zmianie (obowiązkowo).** Po każdej zmianie w projekcie (kod, konfiguracja, ten plik, decyzja):
  1. Dopisz wpis na górze `DZIENNIK.md` (najnowsze u góry): data i godzina, **co zostało zmienione** (pliki + jednym zdaniem po co), **co jest kolejnym krokiem**, i czy kolejny krok wymaga czegoś od Patryka.
  2. Zaktualizuj sekcję „Aktualny stan” na końcu tego pliku (etap, co działa, następny krok).
  3. Zakończ odpowiedź w czacie krótkim blokiem: **Zmienione:** … / **Następny krok:** … (po polsku, bez żargonu).
- **Patryk nie zna Linuxa.** Wszystko, co się da, rób sam w terminalu. Gdy potrzebna jest jego akcja (hasło sudo, logowanie do GitHuba, coś na konsoli), podaj dokładną komendę do wklejenia i jednym zdaniem, co robi. Bez żargonu.
- Decyzje nieodwracalne albo wymagające rąk Patryka (flashowanie karty, ustawienia sieci, zakupy) — zapytaj, nie zgaduj.

## Sprzęt docelowy
- Anbernic RG35XX Pro (obudowa pionowa): Allwinner H700, 4× Cortex-A53 @ 1.5 GHz, Mali G31 MP2, 1 GB RAM.
- Ekran 3.5" IPS 640×480 (4:3).
- Sterowanie: D-pad, ABXY, 2 analogi (pod D-padem i ABXY), L1/R1, L2/R2, Start/Select/Menu. Jack 3.5 mm, Wi-Fi, BT.
- Firmware: custom **Knulli** (ew. muOS) na osobnej karcie — SSH + uruchamianie własnych aplikacji. Ścieżki, sposób uruchamiania (Ports), użytkownika SSH, urządzenie audio i kody przycisków **sprawdź na urządzeniu w etapie 0**, nie zakładaj.

## Środowisko dev
- Debian w **VirtualBox** (wykryte: `systemd-detect-virt` = oracle; w planie było VMware), sieć **NAT** (IP VM 10.0.2.15) (Patryk nie chce Bridged — wolno działa). Połączenia wychodzące z VM do LAN przez NAT działają, więc SSH/scp z VM na konsolę powinno iść normalnie; konsola nie łączy się do VM (nie jest potrzebne). Jeśli SSH z VM nie dosięgnie konsoli — plan B: wbudowany w Windows `ssh`/`scp` przez folder współdzielony VirtualBox, plan C: kopiowanie na kartę SD. Claude Code działa w tej VM.
- Toolchain: `gcc-aarch64-linux-gnu` + SDL2 dla arm64 (multiarch apt albo sysroot z urządzenia). CMake.
- Build natywny x86_64 na desktop (okno 640×480) do szybkich iteracji.
- Deploy: `tools/deploy.sh` (build ARM + scp + wpis w Ports), `tools/package_portmaster.sh` (ZIP dla PortMastera do `dist/`), `tools/log.sh` (log z konsoli), `tools/dev.sh '<komenda>'` (komenda na konsoli). Konsola: IP 192.168.50.239, root, hasło w `tools/device.conf` (poza gitem). **Klucze SSH nie działają** (`/userdata` na exFAT → Dropbear odrzuca), stąd hasło przez SSH_ASKPASS. Komendy `!` w Claude Code nie mają okna na hasło.
- Konsola (sprawdzone): Knulli = Batocera 42, kernel 4.9.170, glibc 2.40, SDL 2.32.8, ALSA: audiocodec/ahubdam/ahubhdmi, RAM 973 MB. Program w `/userdata/dawpol3000`, starter w `/userdata/roms/ports/DAW-POL 3000.sh`, odświeżenie listy: `curl http://127.0.0.1:1234/reloadgames`. Jest PortMaster.
- Repo: prywatne na GitHubie — `git@github.com:rpm160219/dawpol3000.git`, gałąź `main`. Każdy zatwierdzony krok: commit + push.

## Architektura
- **C++17 w stylu „C z klasami”** (bo większość bibliotek DSP jest w C++): bez wyjątków i alokacji w wątku audio, bez ciężkiego STL w ścieżce audio. SDL2 tylko jako cienka warstwa platformy: okno/framebuffer, input, audio callback.
- Silniki i efekty to w dużej mierze **portowane gotowe open source** (patrz sekcja „Biblioteki DSP”), opakowane we wspólny interfejs `Engine` / `Effect` (process(block), 4 parametry pod enkodery + parametry ukryte, rysowanie własnego ekranu).
- **Dokumentacja modułów**: każdy moduł w osobnym pliku/folderze w `core/` (gfx, io, util, synth, seq, audio, ui), opis w nagłówku pliku + tabela w `docs/ARCHITEKTURA.md`. Przy każdej zmianie struktury aktualizować ten dokument.
- **Rdzeń niezależny od platformy**: `core/` renderuje UI do bufora 640×480 RGBA i audio do bufora float. Tryb headless (`--render-png`, `--render-wav`) do testów bez urządzenia — tak wysyłamy Patrykowi makiety i próbki.
- **Własny rasteryzer**: linie z antyaliasingiem (grubość ~1.5–2 px), łuki/okręgi, krzywe; własny cienki font wektorowy (stroke font), duże cyfry. Żadnych bitmap UI.
- **Audio**: 48 kHz, float, osobny wątek, bufor 128–256 ramek (zmierzyć latencję w etapie 0). Komunikacja UI→audio przez lock-free ring buffer komend; zero alokacji i locków w wątku audio.
- Budżet: CPU audio < ~50% jednego rdzenia przy pełnym projekcie, RAM < 300 MB.

## Biblioteki DSP (kandydaci do portu)
**Licencja projektu: GPL-3.0-or-later** (decyzja Patryka: otwarte, do własnego użytku). Wolno brać kod GPL/LGPL/MIT/BSD/Apache. Licencja i źródło każdej biblioteki w `third_party/LICENSES.md`; atrybucja na ekranie „about”.

### Obowiązkowe (wybór Patryka)
- **op1r** — https://github.com/pcppcp/op1r-src (GPL-3.0-or-later; części MIT w NOTICE). Własny firmware-mod do OP-1 w C: silniki synth i efekty jako pluginy (`plugin.json` + kod C + ekran), API w `op1_plugin.h`, quad LFO, sekwencery euclid i dots, markery taśmy, count-in, punch in/out, edytor sampli, generatory ekranów SVG, harnessy hostowe renderujące ekrany do PNG i pluginy do WAV.
  - Plan: zaimplementować w DAW-POL **warstwę zgodności z `op1_plugin.h`**, żeby pluginy op1r wchodziły bez przepisywania. Ich harness PNG/WAV wykorzystać w naszym trybie headless.
  - Portujemy tylko samodzielny kod pluginów/funkcji. Wszystko, co zależy od firmware TE (hooki, `hook.S`, `dispatcher.c`, kod z zaszyfrowanej części, tablice skoków), odpada — repo nie zawiera firmware TE i nie wolno go pozyskiwać ani odtwarzać. Uwaga: oryginał na DSP Blackfin — sprawdzić arytmetykę stałoprzecinkową i założenia o sample rate/bloku.
- **PicoVintageSynthCollection** — https://github.com/Michi71/PicoVintageSynthCollection (GPL-3.0-or-later; części Boost/MIT/BSD). Emulacje na RP2350 w C/C++: organy drawbar (YC), e-piano (CP, mdaEPiano), FM 4-op (DX), Juno-6 (J6), Minimoog (MD), String Ensemble (SM), OB-X (OB, z OB-Xf).
  - Wyciągamy sam DSP; wycinamy Pico SDK, I2S, OLED, enkodery, TinyUSB. Każdy instrument jako `Engine` z własnym ekranem w naszym stylu.
  - RD, JV-880, D-50 wymagają oryginalnych ROM-ów. **Patryk ma własne zrzuty ROM z posiadanego sprzętu** → portujemy wszystkie 10 instrumentów. ROM-y nigdy nie trafiają do repo (`.gitignore` na `roms/`); ładowane z karty z `/roms/<instrument>/`, z weryfikacją sumy kontrolnej i czytelnym komunikatem na ekranie, gdy brak pliku. Skrypt Pythona z repo (ekstrakcja paczek RD) uruchamiać lokalnie u Patryka. JV-880/D-50 mogą być ciężkie CPU — zmierzyć na urządzeniu, ew. osobny rdzeń.

### Dodatkowe znaleziska (do portu, kolejność wg wartości)
- **sfizz** (BSD-2, C++): sampler SFZ — ładowanie gotowych instrumentów sample'owych z karty (pianina, smyczki, perkusje SFZ). Biblioteka; używać z dr_libs, bez JACK/UI.
- **CHOW Tape Model** (GPL-3, C++/JUCE): fizyczny model taśmy — efekt „taśmy” na ścieżkach/masterze, pasuje do widoku taśmy. Wyjąć DSP z JUCE; ciężki → tryb „lite”/mniejszy oversampling, zmierzyć.
- **Surge sst-filters / sst-waveshapers** (GPL-3, header-only C++): filtry (ladder, OB-Xd, K35/diode z Odin 2, comb, tri-pole) i waveshapery (saturacja, folding, fuzz). Używają intrinsics SSE → na ARM przez SIMDe (NEON). Wspólna biblioteka filtrów dla wszystkich silników.
- **Munt / mt32emu** (LGPL-2.1+): emulacja Roland MT-32/CM-32L, lekka w trybie integer. Wymaga ROM-ów — tylko jeśli Patryk ma własne.
- **Nuked-SC55** (licencja MAME, niekomercyjna — niezgodna z GPL przy dystrybucji; do prywatnego użytku OK, trzymać jako osobny opcjonalny moduł): SC-55 i JV-880 przez emulację firmware z ROM. Ciężki — na Pi 3 za wolny; raczej odpada na H700. Sprawdzić, czy JV-880 w PicoVintageSynthCollection pochodzi z tego kodu (licencja!).
- Rozważyć: Dexed (GPL-3; rdzeń msfa Apache-2) jako 6-op FM obok 4-op z Pico; Geonkick (GPL-3) syntezator perkusji; Helm (GPL-3) lekki subtraktywny. Vital/Surge w całości — za ciężkie na A53 bez dużego cięcia.

### Eksperyment: The Usual Suspects / gearmulator (GPL-3)
- Emulacja DSP56300 z JIT na aarch64: Osirus (Virus A/B/C), OsTIrus (Virus TI), Vavra (microQ), Xenia (Microwave II/XT), Nodal Red 2x (Nord Lead 2x), JE-8086 (JP-8000). Wymagają ROM/firmware od Patryka.
- Ocena: bardzo ciężkie (na desktopowym i7 ~8–9% CPU dla jednego Virusa; marzeniem społeczności było Pi 5). Na A53 1.5 GHz real-time mało prawdopodobny. Test **dopiero po etapie 0**: zbudować sam rdzeń emulatora (bez JUCE) na aarch64 i zmierzyć Osirus (Virus B/C — jedno DSP, najlżejszy) na jednym rdzeniu. Jeśli nie wyrabia — opcja „render offline/freeze”: nagrywanie partii do klipu audio wolniej niż real-time (sekwencja → bounce). Bez obietnic.

### Pozostali kandydaci
- **Mutable Instruments eurorack** (pichenettes/eurorack, kod STM32 = MIT): Plaits (macro-oscylator → wiele silników synth naraz), Rings (rezonator/string), Elements (modal), Clouds/Beads (granular, tekstury), Warps, Marbles. Pisane pod Cortex-M4 → na A53 lekkie. Nie używać oryginalnych nazw modułów w UI.
- **DaisySP** (MIT; moduły LGPL osobno w DaisySP-LGPL — tych unikamy): oscylatory, filtry, obwiednie, delay, drumy syntetyczne, efekty. Zawiera też porty kodu MI.
- **Airwindows** (MIT): setki efektów — reverby, saturacje, kompresory, EQ, amp/cab (np. lekki amp sim i Cabs). Kod prosty, per-sample, łatwy do wyjęcia.
- **Neural Amp Modeler A2 / A2-Lite** (open source): amp sim gitarowy z modeli .nam; A2-Lite projektowany pod embedded. Do sprawdzenia: licencja aktualnego kodu, wydajność na A53 (budżet CPU razem z cab IR i efektami). Alternatywa: AIDA-X/RTNeural (sprawdzić licencje).
- Do rozważenia: Dexed/MSFA (FM w stylu DX7), Signalsmith Stretch (time-stretch/pitch dla samplera), Freeverb — licencje zweryfikować przed użyciem.
- Linki od Patryka: dopisać tutaj po otrzymaniu i ocenić (licencja, język, zależności, koszt CPU, czy ma sens na 1 GB RAM).

## Styl wizualny
- **Język aplikacji: angielski** (wszystkie napisy w UI, nazwy parametrów, komunikaty i logi programu). Komunikacja z Patrykiem i dokumentacja projektu — po polsku.
- Czarne tło, cienkie linie, duże cienkie cyfry, dużo pustej przestrzeni. Zaokrąglone kształty („pigułki”), siatki z kropek zamiast gęstych linii.
- Paleta menu (wzór OP-1 Field): belki nagłówków jasnoszare/tan z ciemnym napisem, pigułki akcji steel/tan/szary/czerwony, listy w kolorze tan. Kolory ścieżek: 32 barwy (`pal::track`, pierwsze 8: niebieski, zielony, czerwony, tan, fiolet, cyjan, żółty, róż — bez zmian dla zgodności), wybór z siatki 4×8.
- Ilustracje w oknach dialogowych: zawsze 196×150 przy (24, 84), elementy liczone od obudowy.
- Grafiki wzorcowe od Patryka: folder `ref/` (poza gitem — mogą być cudze materiały; tylko inspiracja, nie kopiujemy 1:1). Ikony/ilustracje rysujemy wektorowo w kodzie; obrazki z generatorów (np. Gemini) tylko jako szkic do przerysowania.
- 4 kolory = 4 „enkodery”: **niebieski, zielony, biały, czerwony** — każdy parametr na ekranie ma kolor swojego enkodera.
- Inspiracja OP-1, ale **własne grafiki i ilustracje**. Nie kopiować 1:1 ekranów, ilustracji (bokser, krowa itp.), nazw ani logo Teenage Engineering. Nazwy silników/efektów własne.

## Założenia produktowe (od Patryka)
- Charakter: **notatnik do szkiców wszystkiego** — nie pod jeden gatunek. Szybko od pomysłu do nagrania.
- Workflow: **jak w FL Studio — patterny + okno arrange (playlist)**, z taśmą jako sposobem nagrywania audio:
  - **Pattern**: zawiera nuty dla wielu instrumentów (jak channel rack). Edycja krokowa (step seq) i piano roll. Patterny numerowane, klonowanie, długość dowolna (w krokach/taktach).
  - **Arrange / playlist**: N ścieżek, na których układa się klipy patternów i klipy audio. Pętla, markery, zoom, kopiuj/wklej/klonuj klip, przesuwanie z kwantyzacją.
  - **Taśma**: nagrywanie audio (wejście USB/gitara, resampling wyjścia, live granie syntezy) → powstaje klip audio na ścieżce playlisty. Ekran ze szpulami zostaje jako widok nagrywania. (Potwierdzone przez Patryka.)
  - Tryby: PATTERN (step/piano roll) ↔ ARRANGE ↔ MIXER, plus ekrany silników/efektów.
- Źródła dźwięku: wbudowane syntezatory, własne sample WAV z karty, gotowe zestawy perkusyjne, **nagrywanie z wejścia USB audio — w tym gitara**.
- Łączność: **klawiatura MIDI po USB (MIDI in), MIDI clock sync, MIDI out**.
- Konsola ma jeden port USB-C (też ładowanie) → interfejs audio + MIDI naraz wymaga huba USB-C (najlepiej z PD). W etapie 0 sprawdzić: czy działa USB host/OTG, czy kernel Knulli ma `snd-usb-audio` i `snd-usb-midi` (ALSA rawmidi/seq), czy SDL2 widzi wejście audio USB.

## Mapowanie sterowania
- Lewy analog X/Y = enkoder niebieski/zielony, prawy analog X/Y = biały/czerwony. Tryb względny: wychylenie = prędkość zmiany wartości (z martwą strefą i krzywą przyspieszenia).
- W PATTERN/ARRANGE: D-pad = kursor (kroki/ścieżki/klipy), A = wstaw/przełącz, B = usuń, X/Y = narzędzia (kopiuj, klonuj), analogi = enkodery (wartości, zoom/scroll). Poniższe mapowanie nut dotyczy trybu grania (KEYS).
- D-pad + ABXY = 8 nut skali (skala/tonacja wybieralna), L1/R1 = oktawa −/+. W SYNTH: L1/R1 = OCTAVE instrumentu (−3..+3, zapisywane w projekcie i presecie; wszystkie silniki; odczyt „OCT ±n” tylko gdy ≠ 0).
- L2/R2 = zakładki w lewo/prawo: PROJECT ↔ ARRANGE ↔ PIANO ROLL ↔ SYNTH ↔ MIXER (docelowo też DRUM, TAPE).
- MIXER: krzyżak = kanał / wiersz (EQ, COMP, wysyłki A–F, 4 inserty, PAN, VOL), L1/R1 = strona (5 kanałów + stały MASTER; po ścieżkach powroty AUX A–F), A = otwórz okno (EQ / kompresor / efekt / BUS COMP), Y = mute, X = solo; gałki: niebieska = głośność, zielona = panorama, biała = wybrana wysyłka, czerwona = master. Okno EQ: L3 = warstwa GAIN / FREQ / Q, Y = MAX EQ, X = CLEAN, A = wł./wył., czerwona = OUTPUT. Kompresor: L1/R1 = model OPTO / VCA / FET, Y = przełącznik / ratio. Efekt (insert / AUX): oryginalny ekran efektu + gałki, L3 = strona 2, X = zmiana efektu (lista), Y = usuń; pusty slot otwiera od razu listę. Okna: B = wyjście; w oknach L2/R2 nie zmieniają zakładek.
- SYNTH: Select = następna strona gałek (SOUND, VOICE, AMP ENV, FILTER ENV, MOD ENV, LFO 1, LFO 2, PRESETS — ostatnia); MOD ENV: L3 = biała/czerwona gałka → DEST + AMOUNT ±; VOICE = MODE POLY/MONO, GLIDE (L3 = synchronizacja z BPM: 1/1 … 1/128T), UNISON (w MONO), AGE; PRESETS = lewo/prawo kolumna, góra/dół wybór, A wczytaj, X zapisz, B usuń (2×), Y INIT (2×); L3 na stronach obwiedni = czerwona gałka RELEASE ↔ AMOUNT (−100..+100); L3 na stronach LFO = synchronizacja z BPM (RATE = podział 1/1 … 1/128 i triole). D-pad + ABXY = odsłuch (C4, bez opisu i bez zmiany oktawy — decyzja Patryka). WAVETABLE (oryginał 1:1) i WAVETABLE+ to osobne silniki wybierane na karcie ENGINE okna TRACK (R3 w SYNTH usunięty); WAVETABLE+ ma strony OSC 1 / OSC 2 / FILTER ( L3 na OSC 1 = LEVEL ↔ SUB; fala OSC 1 zielona, OSC 2 czerwona). Nazwy na gałkach skracać (WAVE, DTUN, CUT, RES, PTCH, VOL; SINE, TRI, SAW, SQR, RND). Silnik ścieżki (WAVETABLE, WAVETABLE+, TAFFY, PEW, ELG, EVA, MBIRA, REACTOR, OXBOW, OXBOW+, SID, MONOLIT, VAPORA, SUPERSAW, MIXTUR, BRASSOPHONE, HOLLOW SAINT, BLOODMOON, BLOT, AETHER, ARC, DDDD, GIN, JUNI, FELT, GRAND, LOUNGE, OUTATIME, PARALLAX, TEST SAW; na liście alfabetycznie) wybiera się na karcie ENGINE (L2/R2) — w oknie NEW TRACK i w oknie TRACK (zmiana silnika = domyślne gałki brzmienia nowego silnika); na pierwszej karcie tylko nazwa silnika. R2 z ARRANGE otwiera pattern pod kursorem (albo ostatni na ścieżce).
- L3 (klik lewej gałki) = pasek NARZĘDZI w ARRANGE i PIANO ROLL: B UNDO, A REDO, Y IN/OUT (raz = IN, drugi raz = OUT i włącza pętlę), X LOOP; L3 ponownie = wyjście.
- Okno TRACK (Select na ścieżce w ARRANGE) ma karty przełączane L2/R2: TRACK (nazwa, kolor, usuń; silnik tylko jako nazwa) i ENGINE (wybór silnika: lista w jednej kolumnie, ilustracja + opis po prawej). Decyzja Patryka — wybór silnika tylko tam. Pętla w ARRANGE w taktach (wyłączona = utwór gra raz), w PIANO ROLL w krokach patternu.
- R3 (klik prawej gałki) = tryb ZAZNACZANIA w piano rollu, 2 warstwy (L2/R2 — w tym menu nie zmieniają zakładek): 1) B SELECT / A DELETE / Y COPY / X PASTE; 2) B TRANSPOSE (trzymaj + góra/dół) / A MOVE (trzymaj + krzyżak) / Y OCTAVE (trzymaj + góra/dół) / X DUPLICATE; R3 ponownie = wyjście. (Y/X w warstwie 2 dobrane przez Claude — do potwierdzenia przez Patryka.)
- Paski akcji (L3/R3) zawsze tym samym elementem `actionBar`: 4 pigułki w kolumnach gałek, kolejność B, A, Y, X, kolory steel / czerwony / tan / szary. Na PC: U = L3, O = R3. RG35XX: L3 = przycisk 12, R3 = 15.
- Pozycja w nagłówkach zawsze w formacie „BAR takt.miara”, wyśrodkowana (wspólny `screenHeader`). Wskaźnik zakładek: 3 kropki pod nagłówkiem (y≈38, w PROJECT y≈60). Kropki warstw paska akcji (L1/R1): na samym dole ekranu (y≈474). Małe elementy (kółka z literą) wyrównywać do środka piksela. Teksty w klipach/polach przycinane do obrysu (`drawTextFit`). Start = play/stop, Select = rec (później). Menu+Select = wyjście.
- W oknach dialogowych: A = zatwierdź, B = anuluj, X = edycja nazwy (góra/dół litera, lewo/prawo pozycja); pigułki na dole pokazują literę przycisku.
- Mapowanie trzymać w jednej tabeli (łatwo przestawić po testach na urządzeniu).

## Etapy
0. **Setup i test sprzętu**: toolchain, hello-world na konsoli (ekran, dźwięk sinusa, wszystkie przyciski i osie wypisane na ekranie), pomiar latencji audio, skrypt deploy.
1. **Silnik + pierwszy synth**: subtraktywny (osc → filtr → ADSR), polifonia ~6 głosów, klawiatura na przyciskach, ekran z obwiednią i 4 enkoderami. Tu zatwierdzamy styl wizualny (najpierw makiety PNG).
2. **Perkusja + patterny** (priorytet Patryka zaraz po synth): drum sampler z wbudowanymi zestawami, pattern z wieloma kanałami, step seq, piano roll, tempo/swing.
3. **Arrange + taśma**: playlist z klipami patternów i audio, pętla/markery; nagrywanie audio przez widok taśmy → klip audio na playliście, overdub, punch in/out.
4. **Silniki**: FM, string (Karplus-Strong), sampler (własne WAV), LFO z przypisaniem do parametrów.
5. **Efekty i mixer**: delay, reverb, drive, master EQ (3 pasma) i kompresor; mixer 4 ścieżek.
6. **Pliki**: zapis/odczyt projektów, import WAV z karty, eksport miksu do WAV.
7. **USB audio i MIDI**: nagrywanie z interfejsu USB na taśmę. Gitara: **amp sim + łańcuch efektów na wejściu** (gain/drive, cab IR lub filtr kolumny, EQ, delay, reverb), monitoring przez konsolę — cel latencji round-trip < ~10 ms, full duplex na jednym urządzeniu ALSA, MIDI in z klawiatury, MIDI out, MIDI clock (master i slave). Jeśli test w etapie 0 pokaże, że USB działa, prosty MIDI in można wciągnąć wcześniej (etap 1).
8. **Polish**: animowane ilustracje, wydajność, bateria, ekran startowy.

## Do zrobienia przez Patryka (stan na start)
- [ ] Druga karta microSD + Knulli wgrane, konsola w Wi-Fi, SSH działa
- [x] ~~Sieć Bridged~~ — zostaje NAT (decyzja Patryka)
- [x] Claude Code w VM, git + SSH do GitHuba
- [x] Prywatne repo: github.com/rpm160219/dawpol3000 (push po SSH z VM)
- [ ] Słuchawki na jacka do testów
- [x] Hub USB-C z PD, interfejs audio USB, klawiatura MIDI USB — Patryk ma; testujemy w etapie 0 (zapytaj o modele)
- [x] Nazwa projektu — DAW-POL 3000

## Aktualny stan
- Etap: 4 (silniki) w toku (29 silników, w tym pierwsza perkusja GIN) — **WAVETABLE z op1r przeniesiony 1:1** (DSP zgodny bit w bit z oryginałem, ekran przeniesiony). Etapy 2/3 (sekwencer, arrange) działają.
- WAVETABLE ma też obwiednię filtra i 2 LFO z wyborem celu (dodatki DAW-POL; przy zerowych ustawieniach brzmi bit w bit jak oryginał).
- Działa: PROJECT (zapis/odczyt `.dpp`, autozapis), ARRANGE (16 MIDI + 4 audio, klipy, VU, mute/solo, 32 kolory, pętla IN/OUT), PIANO ROLL (wielotaktowy, zaznaczanie 2 warstwy, pętla), SYNTH (WAVETABLE: 4 gałki + ADSR, ekran oscyloskopowy), undo/redo, paski akcji L3/R3.
- WAVETABLE: ~1,4% rdzenia na głos (6 głosów/ścieżkę ≈ 8%); ~5 w pełni obciążonych ścieżek mieści się w budżecie 50%.
- TAFFY (port 1:1 z op1r, CZ phase distortion, ekran „bubblegum”): gałki WAVE/DCW/ENV/RES, druga obwiednia = DCW ENV, ta sama infrastruktura co WAVETABLE (ADSR, MOD ENV, 2 LFO, VOICE), ilustracja na karcie ENGINE = twarz z oryginalnego SVG z ustami, nićmi gumy i gwiazdkami; ~7% rdzenia przy 6 głosach.
- WAVETABLE+ (tryb rozszerzony): 2 oscylatory, sub, filtr LP24/LP12/BP/HP + drive, cele LFO W1/W2/P1/P2/LV1/LV2 (WAVE = obie fale naraz); ~10% rdzenia przy 6 głosach.
- Presety instrumentów osobno dla silnika (`presets/wavetable/`, `presets/wavetableplus/`, `presets/taffy/`; zestawy fabryczne w `core/io/factory_presets.cpp`: 37 / 45 / 41, kategorie z ikonami); głosy POLY/MONO, glide, unison, AGE.
- Brakuje: zakładka CREDITS z atrybucją (decyzja Patryka: na końcu prac; podpis z ekranu startowego usunięty), kolejne silniki op1r/Pico, perkusja/step seq, LFO, efekty, audio/taśma, MIDI.
- PEW (port 1:1 z op1r, organy z 9 suwakami + Leslie, ekran „jazz club”): gałki REGISTER/PERCUSSION/DRIVE/LESLIE, strona BARS (wysunięcie suwaków LOW/MID/HIGH), bez drugiej obwiedni (decyzja Patryka), 31 presetów w `presets/pew/`; ~5,6% rdzenia przy 6 głosach.
- Ekran startowy z nazwą programu (~4,5 s, dowolny przycisk pomija).
- ELG (port 1:1 z op1r, dron z 3 piłami, ekran „łoś”): „sam silnik” — gałki INTERVAL/DETUNE/TIMBRE/MOVEMENT (L3 = czerwona gałka SYNC: ruch w tempie, OFF / 1/1 … 1/128T), tylko obwiednia głośności, strony SOUND / AMP ENV / PRESETS (31 presetów); ~7,6% rdzenia przy 6 głosach.
- EVA (port 1:1 z op1r, „kosmiczny chór” z radiem w hełmie, ekran astronautki): PRESSURE/FREQ/OXYGEN/COM, L3 = strona 2 COM (oktawa sygnału Quindar), strony SOUND / AMP ENV / MOD ENV / LFO 1 / LFO 2 / PRESETS (cele PRES, FREQ, OXY, COM, PTCH, VOL; bez drugiej obwiedni i trybów głosu), 28 presetów (8 z oryginału + 20 z modulacją); C (`eva_dsp.c`); ~7,6% rdzenia przy 6 głosach.
- MBIRA (port 1:1 z op1r, pianino kciukowe — synteza modalna, ekran kalimby): STRIKE/TINE/BOX/BUZZ, L3 = strona 2 (RING / DOUBLE / WAH), strony jak EVA (AMP ENV, MOD ENV, 2 LFO), 28 presetów (8 z oryginału + 20 z modulacją); C (`mbira_dsp.c`); ~5,3% rdzenia przy 6 głosach.
- Utwór demo „ALL ENGINES” (`core/seq/demo_song.cpp`): wszystkie silniki, 16 taktów, 96 BPM; dodawany raz do listy PROJECT; na konsoli ~18% rdzenia.
- REACTOR (port 1:1 z op1r 63, FM ze sprzężeniem i „temperaturą rdzenia”, ekran elektrowni z drżeniem): COOLING/RADIATION/RODS/HEAT, strony jak EVA, 11 presetów oryginału; C (`reactor_dsp.c`); ~7,8% rdzenia przy 6 głosach. Głośniejszy od innych silników (jak oryginał).
- MBIRA: ekran 1,85× z marginesami. Ekran startowy bez podpisu (atrybucja w przyszłej zakładce CREDITS).
- OXBOW (Hundekraft wg OberVave zvensona — kod z paczki źródeł Hundekraft 3.0.1, nie z op1r; **GPL-3.0-only**, przy dystrybucji całość jako GPL-3.0-only): poli w stylu OB-X, CUTOFF/RESONANCE/ENV AMOUNT/DETUNE, L3 = WAVE/FILTER/DRIVE, strony jak EVA, 16 presetów oryginału, ekran byka (tło w cache); C (`oxbow_dsp.c`); ~5–6% rdzenia. Demo z partią OXBOW (v3).
- OXBOW+ (rozszerzony OXBOW, jak WAVETABLE+): 2 niezależne oscylatory (fala, interwał, strojenie, poziom, PW), szum, SYNC, X-MOD, ENV→P2, keytrack/velocity/VINTAGE, ADSR obwiedni filtra, VOICE (POLY/MONO, glide, unison, AGE), cele LFO PW1/PW2/P1/P2/LV1/LV2; strony OSC 1 (L3: PW/NOISE), OSC 2 (L3: SYNC/X-MOD/PW/ENV>PITCH), FILTER (L3: KEYTRACK/VELOCITY/VINTAGE), FILTER ENV (AMOUNT = ENV AMOUNT); ekran: odczyty FILTER i ENV>PITCH, ptaki z ENV>PITCH, oddech z typem filtra; 30 presetów; DSP = zmodyfikowana kopia `oxbow_plus.c`; ~5–7% rdzenia.
- SID (port 1:1 z op1r, cRSID Hermita): 3 oscylatory na głos + filtr 8580, WAVE (krokowo, także modulacja) / VOICES-WIDTH-INTERVAL / CUTOFF / RES, strony SOUND / AMP ENV / FILTER ENV / MOD ENV / LFO 1 / LFO 2 / PRESETS, 20 presetów (8 z oryginału + 12 z modulacją); C (`sid_dsp_wrap.c`); ~6–8% rdzenia. Demo z partią SID (v4, ~22% rdzenia).
- MONOLIT (port 1:1 z op1r, bas/lead: 3 piły + sub, filtr drabinkowy): makra SHAPE/CUTOFF/CONTOUR/HEAT, L3 = BODY/VEL/AGE, strony SOUND / VOICE (tylko MODE i GLIDE) / AMP ENV / FILTER ENV (na CUTOFF) / MOD ENV / LFO 1 / LFO 2 / PRESETS, wyjście −3 dB, 32 presety (12 + 20); C (`monolit_dsp.c`); ~13% rdzenia. Demo z partią ACID (v5, ~24% rdzenia).
- VAPORA (port 1:1 z op1r, AiAAAA — pad PADsynth, ekran oceanu ze słońcem): MORPH/THICKNESS/BRIGHTNESS/MOTION, L3 = CHARACTER (14 widm) / INTERVAL / SUN (wzór słońca, tylko obraz) / SPEED, strony SOUND / AMP ENV / FILTER ENV (na BRIGHTNESS) / MOD ENV / LFO 1 / LFO 2 / PRESETS, 31 presetów (INIT + 15 z oryginału + 15 nowych), wyjście +9,5 dB; ekran = oryginalny kod C; C (`vapora_dsp.c`); ~5,5% rdzenia. Demo z partią TIDE (v6, ~27% rdzenia).
- OCTAVE instrumentu (−3..+3) dla wszystkich silników: L1/R1 w SYNTH, zapis w projekcie/presecie (`oct`); 4 basy MONOLIT dostały oktawę −1 jak w oryginale (presety fabryczne v3 zastępują stare).
- Karta ENGINE: przewijana lista silników w pełnym rozmiarze (6 widocznych).
- SUPERSAW (port 1:1 z op1r, pcppcp — 2 rozstrojone piły → kwadrat, sub, przester; ekran piły łańcuchowej): DETUNE/MORPH/SUB/DRIVE, strona FILTER po SOUND = filtr DAW-POL (CUTOFF / RESONANCE / TYPE LP24-LP12-BP-HP + wykres; otwarty = oryginał bit w bit), strony SOUND / FILTER / VOICE (tylko MODE i GLIDE — decyzja Patryka) / AMP ENV / FILTER ENV / MOD ENV / LFO 1 / LFO 2 / PRESETS (cele CUT, RES), 34 presety (oryginał bez presetów); ekran bez zdublowanych tarcz OP-1, wyjście −3 dB; C (`supersaw_dsp.c`); ~8,3% rdzenia z filtrem. Demo z partią SAWS (TRANCE PLUCK, v8).
- MIXTUR (port 1:1 z op1r, ultralazr — organy subharmoniczne Mixtur-Trautonium, ekran drabiny undertonów): UNDERTONE A..D krokowo (OFF, 1/1 … 1/24), L3 = FORMANT 1 / FORMANT 2 / RESONANCE, strony SOUND / AMP ENV / MOD ENV / LFO (jedno, decyzja Patryka) / PRESETS, cele UT A..D, PTCH, VOL, F1, F2, RES, 14 presetów (oryginał bez presetów); C (`mixtur_dsp.c`); ~6% rdzenia. Demo z partią BIRDS (v9, ~31% rdzenia).
- BRASSOPHONE (port 1:1 z op1r, nvmsocool — sinus z wyginaną fazą, barwa i podjazd z obwiedni AMP ENV, wibrato od podtrzymania; ekran rogu Dr. Seussa): sam silnik (decyzja Patryka) — TIMBRE / SWEEP / VIBRATO / VIB RATE, strony SOUND / AMP ENV / PRESETS, 12 presetów (oryginał bez presetów); C (`brass_dsp.c`); ~6,8% rdzenia. Demo z partią HORN (v10, 16 ścieżek = komplet MIDI, ~32% rdzenia).
- HOLLOW SAINT (port 1:1 z op1r, kennel — nawiedzona lira korbowa: struna, burdon, brzęczący mostek w rytm tempa, katedra, taśma Lo-Fi Relic; ekran liry w gotyckim łuku): sam silnik — DRONE / STRINGS / WHEEL BUZZ / CATHEDRAL, L3 = DRONE KEY / RELIC AGE / STROKES / RELIC (wł./wył.), strony SOUND / AMP ENV / PRESETS, 15 presetów (8 z oryginału + 6 z RELIC); cały stan per ścieżka; C (`hollowsaint_dsp.c`); ~6–7% rdzenia. Nie ma go w demo (16 ścieżek MIDI = komplet).
- BLOODMOON (port 1:1 z op1r, Nola Drift — gotyckie organy z nawiedzonym dzwonem, wibrato po ataku; ekran krwawego księżyca za katedrą): sam silnik + AMP ENV — BLOOD / ECLIPSE / HAUNTING / MOONRISE, strony SOUND / AMP ENV / PRESETS, 9 presetów (8 autorki); C (`bloodmoon_dsp.c`); ~5,7% rdzenia.
- BLOT (port 1:1 z op1r, pcppcp — TB-303: piła/kwadrat, filtr wg Open303, obwiednia filtra, akcent przy velocity ≥ 100; ekran kwasowego smileya): CUTOFF / RESONANCE / ENV MOD / DECAY, L3 = WAVE / ACCENT / DRIVE, strony SOUND / VOICE (MODE, GLIDE = slide jak portamento OP-1) / AMP ENV / PRESETS, 15 presetów (oryginał bez presetów), wyjście +6 dB; C (`blot_dsp.c`); ~10% rdzenia.
- AETHER (port 1:1 z op1r, pcppcp — teremin grany przechyleniem OP-1; ekran anteny): RG35XX nie ma czujnika ruchu → wirtualny czujnik (decyzja Patryka): strona TILT (PITCH TILT, VOL TILT, SHAKE) + modulacja MOD ENV / 2 LFO (cele PTLT, VTLT, SHAK); ORBIT / FORM / SPACE / DEPTH, L3 = DEAD ZONE / TILT RANGE / VIB RATE, strony SOUND / TILT / VOICE (MODE, GLIDE) / AMP ENV / MOD ENV / LFO 1 / LFO 2 / PRESETS, 11 presetów, wyjście −3 dB; ~6,8% rdzenia.
- ARC (port 1:1 z op1r, ultralazr — śpiewający łuk Duddella: oscylator Van der Pola od sinusa po wrzaskliwą relaksację, szum plazmy, zapłon i trzaski, sprzężona 2. lampa; ekran portretu fazowego między elektrodami): CURRENT / CARBON / HISS / STRIKE, L3 = RATIO / COUPLING / BRIGHTNESS, strony SOUND / AMP ENV / MOD ENV / LFO (jedno, decyzja Patryka) / PRESETS, cele też RAT / CPL / BRT, 13 presetów, wyjście −3 dB; ~9,5% rdzenia.
- Karta ENGINE: silniki alfabetycznie.
- DDDD (port 1:1 z op1r, pcppcp — parafoniczna ściana ostrego szumu: źródła, pętla sprzężenia z odczepem na klawisz, filtr, fold/clip/crush, losowe „ręce”; ekran zniekształcanej osy): sam silnik — SOURCE / FEEDBACK / DESTROY / HANDS, L3 = FILTER / SPREAD / RATE, strony SOUND / AMP ENV / PRESETS, 12 presetów; cały stan per ścieżka; ~5% rdzenia.
- JUNI (port 1:1 z op1r, Hundekraft — poli w stylu jednego DCO: piła/puls/sub/PWM, filtr drabinkowy, HPF, chorus I/II/I+II; ekran białej nocy nad fiordem): sam silnik — TONE / SHAPE / SWEEP / CHORUS, L3 = RESONANCE / HPF / PWM RATE, strony SOUND / AMP ENV (prowadzi też filtr) / PRESETS, 13 presetów (12 z oryginału), wyjście −3 dB; stan per ścieżka; ~7% rdzenia.
- OUTATIME (port 1:1 z paczki źródeł Hundekraft 3.1.4, dźwięk opus/LizardMan245 — wehikuł czasu 1955↔2015; ekran DeLoreana z wyświetlaczami lat, MPH i GW): YEAR / SPEED / POWER / TRAIL, L3 = SPREAD / TONE / FLUX, strony SOUND / VOICE (MODE, GLIDE) / AMP ENV / PRESETS, 12 presetów (4 z oryginału + 7 nowych); licencja opus GPL-3.0 (może być -only, jak OXBOW); ~9% rdzenia.
- GIN (port 1:1 z paczki Hundekraft 3.1.4: perkusja SquareWave Studio na libpo32, ekran butelek gin/tonik): zestaw 24 dźwięków na klawiszach (na stałe C2..B3 = 24 dźwięki, każdy ma swój klawisz; inne nuty milczą), każdy dźwięk ma końce A/B i MORPH; strony SOUND (MORPH / PITCH / DECAY / BALANCE, L3 = MOD / NOISE / CUTOFF), 8 stron edytora oryginału (TONE, TONE ENV, PITCH MOD, NOISE FILTER, NOISE ENV, MIX + LEVEL, EQ, VELOCITY; L3 = wybór dźwięku zieloną), ABOUT, PRESETS (8 zestawów, nowa kategoria DRUMS); L1/R1 = dźwięk, R3 = A/B, uderzenie wybiera dźwięk; własna czcionka zamiast czcionki z firmware; velocity działa (libpo32), wyjście +6 dB; zestaw zapisywany w projekcie i presecie; ~6% rdzenia. Brak: zmiana nazw dźwięków (oryginał ma).
- PARALLAX (własny silnik, clean room wg opisu i zrzutu ekranu „dimension” z Opus — kod oryginału niepubliczny): klasyczny subtraktywny — WAVEFORM (PULSE → SQUARE → SAW → SAW+SUB → NOISE płynnie), UNISON (Juno, 3 oscylatory), FILTER FREQ i RES (drabinka w stylu 303); ekran jak wzorzec (koło fal, oscyloskop XY z cieniem unison, szczyt rezonansu); strony SOUND / AMP ENV / FILTER ENV / MOD ENV / LFO 1 / LFO 2 / PRESETS; 22 presety; ~12% rdzenia w najgorszym przypadku.
- LOUNGE (własny silnik, clean room wg opisu KEYS z „OP-1 Mods” Justina Joe — kod niepubliczny): modelowane klawisze bez sampli — TINE / REED / CLAV, gałki BARK / TONE / DECAY / TYPE; ekran: przekrój mechanizmu pod pokrywą z koktajlem (młotek, drgania, przetwornik); strony SOUND / AMP ENV / MOD ENV / LFO 1 / LFO 2 / PRESETS; 16 presetów (tremolo walizkowe = LFO na VOL); ~5,5% rdzenia.
- FELT (własny model fizyczny fortepianu z filcem, w duchu felt piano): struny-falowody (1–3, sztywność, dudnienie), młotek z filcem, płyta rezonansowa, odgłosy mechaniki i tłumika; FELT / CORDA / DECAY / NOISE, L3 = DETUNE (rozstrojenie strun nuty) / WORN (rozstrojone pianino — klawisze na stałe fałszują) / BODY (płyta + puknięcie); LFO / MOD ENV także na DTUN / WORN / BODY; ekran: przekrój fortepianu (pokrywa otwierana gałką DECAY, świeca, stroik z odchyleniem klawisza, struny wachlarzem z dudnieniem, luźne kołki przy WORN, młotek z warstwami filcu uderza w struny, tłumik, klawisz); strony SOUND / AMP ENV / MOD ENV / LFO 1 / LFO 2 / PRESETS; 19 presetów; ~11% rdzenia.
- GRAND (własny oszczędny model fortepianu koncertowego wzorowany na FigBug/Piano — oryginał na konsoli 38% rdzenia na nutę): falowody strun rozcięte w miejscu uderzenia, niealikwotowość jak w fortepianie (silna w basie, filtry dobierane do nuty), nieliniowy młotek z filcem, puknięcie w płytę, wspólny mostek (wybrzmienie dwuetapowe), pomruk basu, płyta FDN, tłumiki (od E6 brak); HAMMER / STRIKE / DECAY / LID, L3 = UNISON / GROWL / BOARD; ekran: przekrój fortepianu (pokrywa wg LID, fale na strunie z miejsca uderzenia, młotek z filcem, kręgi płyty); 15 presetów; ~16,5% rdzenia; impedancja strun i strata alikwotów dobierane do nuty, płyta z długimi liniami (bez buczenia).
- Utwór demo REVOLUTIONARY: Chopin, Etiuda op. 10 nr 12 (domena publiczna, MIDI z Mutopia nr 743), takty 1–37 (3 strony), 160 BPM, dwie ścieżki GRAND (ręce osobno, po 6 głosów); ~22% rdzenia.
- Utwór demo SYNTHWAVE (`buildDemoSynthwave`): 16 taktów, 104 BPM, Am F C G — GIN, PARALLAX (bas, arpeggio, melodia), JUNI (pad), OXBOW (brass); na liście PROJECT; ~24% rdzenia.
- MIXER (`core/audio/mixer.*`, `core/ui/mixer_view.*`): kanał = 4 inserty → EQ 3-pasmowy (częstotliwość, Q, MAX EQ) → kompresor OPTO (styl LA-2A) / VCA (dbx 160) / FET (1176) → głośność + panorama; 6 wysyłek po tłumiku na AUX A–F (efekt, EQ, poziom); master: BUS COMP w stylu SSL G → głośność → ogranicznik. Efekty z op1r bit w bit: JUNI CHORUS, SISTER (nowy projekt: AUX A = SISTER), edycja na ich oryginalnych ekranach (`core/ui/fx_screen*`). Wysyłki = gałeczka + miernik sygnału, VU na płytkach (kanał ciemnoszara, AUX szara, master biała), symetryczne panele kompresorów. Utwór „SYNTHWAVE MIX” (~30% rdzenia).
- Następny krok: Patryk ocenia MIXER na konsoli i słucha „SYNTHWAVE MIX”; potem kolejne efekty (delay, reverb, drive) i ekrany efektów.
