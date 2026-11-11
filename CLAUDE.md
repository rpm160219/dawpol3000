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
- Deploy: `tools/deploy.sh` (build ARM + scp + wpis w Ports), `tools/log.sh` (log z konsoli), `tools/dev.sh '<komenda>'` (komenda na konsoli). Konsola: IP 192.168.50.239, root, hasło w `tools/device.conf` (poza gitem). **Klucze SSH nie działają** (`/userdata` na exFAT → Dropbear odrzuca), stąd hasło przez SSH_ASKPASS. Komendy `!` w Claude Code nie mają okna na hasło.
- Konsola (sprawdzone): Knulli = Batocera 42, kernel 4.9.170, glibc 2.40, SDL 2.32.8, ALSA: audiocodec/ahubdam/ahubhdmi, RAM 973 MB. Program w `/userdata/dawpol3000`, starter w `/userdata/roms/ports/DAW-POL 3000.sh`, odświeżenie listy: `curl http://127.0.0.1:1234/reloadgames`. Jest PortMaster.
- Repo: prywatne na GitHubie Patryka.

## Architektura
- **C++17 w stylu „C z klasami”** (bo większość bibliotek DSP jest w C++): bez wyjątków i alokacji w wątku audio, bez ciężkiego STL w ścieżce audio. SDL2 tylko jako cienka warstwa platformy: okno/framebuffer, input, audio callback.
- Silniki i efekty to w dużej mierze **portowane gotowe open source** (patrz sekcja „Biblioteki DSP”), opakowane we wspólny interfejs `Engine` / `Effect` (process(block), 4 parametry pod enkodery + parametry ukryte, rysowanie własnego ekranu).
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
- Czarne tło, cienkie linie, duże cienkie cyfry, dużo pustej przestrzeni.
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
- D-pad + ABXY = 8 nut skali (skala/tonacja wybieralna), L1/R1 = oktawa −/+.
- L2/R2 = shift / przełączanie trybów: SYNTH, DRUM, TAPE, MIXER. Start = play/stop taśmy, Select = rec.
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
- [ ] Claude Code w VM (działa ✓), git + SSH do GitHuba (brak klucza SSH i konfiguracji git)
- [ ] Puste prywatne repo na GitHubie + dodany klucz SSH z VM (github.com/settings/keys)
- [ ] Słuchawki na jacka do testów
- [x] Hub USB-C z PD, interfejs audio USB, klawiatura MIDI USB — Patryk ma; testujemy w etapie 0 (zapytaj o modele)
- [x] Nazwa projektu — DAW-POL 3000

## Aktualny stan
- Etap: 0 — **działa na konsoli** (obraz, dźwięk, wszystkie przyciski i analogi; potwierdzone przez Patryka). Pad: „Anbernic RG35XX-Pro Controller”, rozpoznany jako GameController (kody surowe w `platform/controls_map.h`). Wideo SDL: mali, audio: alsa 48 kHz / 256 ramek.
- UI przestawione na angielski. Wyjście: Menu+Select (też surowo) albo Hotkey+Start (plik `.keys` Knulli).
- Do zrobienia w etapie 0: pomiar realnej latencji audio i obciążenia CPU, test mniejszego bufora (128), USB (hub, interfejs audio, MIDI) — po podaniu modeli.
- Następny krok: GitHub (Patryk dodaje klucz + tworzy repo), potem etap 1 — makiety PNG ekranu syntezatora do akceptacji stylu.
