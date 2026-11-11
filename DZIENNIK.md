# Dziennik DAW-POL 3000

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
