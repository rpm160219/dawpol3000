# Dziennik DAW-POL 3000

## 2026-11-12 02:50
**Zmienione:** `core/ui/piano_roll.cpp` — poprawione kolory wierszy (były odwrócone: czarne klawisze jaśniejsze); teraz białe klawisze jaśniejsze, czarne ciemne, wiersz kursora delikatnie zielony (nie myli się z klawiszem); dodany pasek mini-klawiatury po lewej i znacznik kursora; mocniejsza linia na granicy oktawy (B/C) i słabsza między E/F. Wysłane na konsolę.
**Następny krok:** Patryk sprawdza na konsoli; potem wiele taktów i kanałów, step seq.
**Wymaga od Patryka:** tak — ocena.

## 2026-11-12 02:30
**Zmienione:**
- Reorganizacja `core/` na foldery: `gfx/` (canvas, font), `io/` (PNG/WAV), `util/` (spsc, triple_buffer), `synth/` (note_sink, saw_synth, tone_synth), `seq/` (pattern, sequencer), `audio/` (engine), `ui/` (input, buttons, encoder, piano_roll, test_screen).
- Nowe: `seq/pattern` (nuty w tickach, PPQ 96, 1 takt 4/4), `seq/sequencer` (odtwarzanie z dokładnością do próbki), `synth/saw_synth` (piła polyBLEP, 8 głosów), `audio/engine` (łączy sekwencer i instrument, komendy lock-free), `util/triple_buffer.h` (pattern UI→audio), `ui/piano_roll` (ekran edycji), `ui/buttons.h`, `ui/encoder.h`.
- `docs/ARCHITEKTURA.md` — opis wątków, przepływu danych i każdego modułu. README — sterowanie piano rolla.
- `platform/main_sdl.cpp` — domyślnie piano roll, `--test` = ekran testu sprzętu; headless `--render-png plik [roll|test]`.
- Wysłane na konsolę. Wyniki: `out/pianoroll.png`, `out/pianoroll.wav` (8 s demo). CPU na konsoli ~1.5% rdzenia.
**Następny krok:** Patryk testuje piano roll na konsoli; potem wiele taktów i kanałów, step seq.
**Wymaga od Patryka:** tak — test na konsoli i ocena.

## 2026-11-12 01:20
**Zmienione:** podpięte repo GitHub (`origin` = git@github.com:rpm160219/dawpol3000.git), wysłana cała historia; CLAUDE.md — adres repo i zasada „commit + push po każdym kroku”.
**Następny krok:** etap 1 — makiety PNG ekranu syntezatora (osc → filtr → ADSR) do oceny stylu.
**Wymaga od Patryka:** nie (tylko ocena makiet, gdy będą).

## 2026-11-12 01:05
**Zmienione:** UI i logi programu po angielsku (`core/hello_app.cpp`, `core/input.h`, `platform/main_sdl.cpp`); wpisane kody przycisków RG35XX Pro (`platform/controls_map.h`); awaryjne wyjście Menu+Select na surowych kodach; `tools/knulli/DAW-POL 3000.sh.keys` — wyjście Hotkey+Start (Knulli zabija program); `tools/deploy.sh` wysyła też plik `.keys`. CLAUDE.md: zasada „aplikacja po angielsku”. Wymuszone zamknięcie zawieszonej aplikacji przez SSH.
**Stan:** Patryk potwierdził — na konsoli działa obraz, dźwięk, przyciski, analogi.
**Następny krok:** podpięcie GitHuba, potem etap 1 (makiety ekranu syntezatora).
**Wymaga od Patryka:** tak — dodać klucz SSH na GitHubie i utworzyć puste prywatne repo.

## 2026-11-12 00:40
**Zmienione:** `tools/ssh_env.sh` (wspólne SSH z hasłem z `tools/device.conf`, który jest poza gitem), `tools/deploy.sh`, `tools/log.sh`, `tools/dev.sh` (komenda na konsoli). Program wysłany na konsolę i dodany do menu Ports; test headless na konsoli OK (PNG zgodny z PC, WAV OK).
**Ustalenia o konsoli:** Knulli = Batocera 42, kernel 4.9.170, glibc 2.40, SDL 2.32.8, karty ALSA: audiocodec / ahubdam / ahubhdmi, RAM 973 MB. `/userdata` na exFAT (fuseblk) → Dropbear odrzuca klucze SSH, logujemy się hasłem. Porty: `/userdata/roms/ports/*.sh`, przeładowanie listy: `curl http://127.0.0.1:1234/reloadgames`.
**Następny krok:** Patryk uruchamia Ports → DAW-POL 3000; czytam log (ekran, audio, pad), mapuję przyciski.
**Wymaga od Patryka:** tak — uruchomić z menu i powiedzieć, co widać/słychać.

## 2026-11-12 00:10
**Zmienione:** `tools/deploy.sh` (kopiuje program + wpis w menu Ports, zapamiętuje IP), `tools/knulli/DAW-POL 3000.sh` (uruchamianie z menu Ports, log do pliku), `tools/log.sh` (podgląd logu z konsoli). Patryk dostał instrukcję wgrania Knulli i włączenia SSH.
**Następny krok:** pierwszy deploy na konsolę i test (ekran, dźwięk, przyciski).
**Wymaga od Patryka:** tak — karta z Knulli, Wi-Fi, IP konsoli.

## 2026-11-11 23:55
**Zmienione:** `platform/main_sdl.cpp` — logi wypisywane od razu linia po linii (potrzebne do czytania logów z konsoli przez SSH). Uruchomiona wersja desktop do testu przez Patryka.
**Następny krok:** ocena okna/dźwięku przez Patryka; potem Knulli na karcie i IP konsoli.
**Wymaga od Patryka:** tak — ocena i info o karcie z Knulli.

## 2026-11-11 23:45
**Zmienione:**
- Toolchain zainstalowany (cmake, aarch64-linux-gnu-g++, SDL2 arm64) — buduje się desktop i ARM.
- `core/` — rdzeń niezależny od platformy: rasteryzer z antyaliasingiem (`canvas`), własny font kreskowy (`font`), kolejka lock-free UI→audio (`spsc.h`), ton testowy (`tone_synth`), zapis PNG/WAV (`image_io`), ekran testowy etapu 0 (`hello_app`), tryb headless (`headless`).
- `platform/main_sdl.cpp` + `platform/controls_map.h` — okno, audio 48 kHz/256 ramek, pad/klawiatura, logowanie surowych kodów przycisków (do zmapowania konsoli); całe mapowanie w jednej tabeli.
- `CMakeLists.txt`, `cmake/aarch64.cmake`, `tools/build.sh`, `tools/deploy.sh`, `README.md`, `LICENSE` (GPL-3), `.gitignore`; repo git założone lokalnie.
- Wynik: `out/etap0.png` (makieta ekranu testowego), `out/etap0.wav` (6 s: skala C-dur, w drugiej połowie rośnie drive). Binarka ARM wymaga glibc ≥ 2.38, libstdc++ wbudowane.

**Następny krok:** uruchomienie na konsoli — potrzebne Knulli na karcie, Wi-Fi i IP konsoli; potem odczyt kodów przycisków i pomiar latencji.
**Wymaga od Patryka:** tak — ocena PNG/WAV („git / nie git”), karta z Knulli i IP konsoli.

## 2026-11-11 23:10
**Zmienione:**
- `DZIENNIK.md` — utworzony (dziennik zmian, najnowsze u góry).
- `CLAUDE.md` — poprawione: VM to VirtualBox (nie VMware), sieć zostaje NAT (usunięta sprzeczna pozycja „Bridged”), zaktualizowany „Aktualny stan”.

**Sprawdzone w VM:** Debian 13, 16 rdzeni, 15 GB RAM, 37 GB wolnego. Jest: gcc/g++, git, python3, ssh/scp, SDL2 2.32. Brak: cmake, aarch64-linux-gnu-gcc, SDL2 arm64, `~/.ssh`, git user.name/email. Sudo wymaga hasła.

**Następny krok:** instalacja toolchaina (cmake + cross-kompilator aarch64 + SDL2 arm64).
**Wymaga od Patryka:** tak — wklejenie jednej komendy z sudo (hasło).
