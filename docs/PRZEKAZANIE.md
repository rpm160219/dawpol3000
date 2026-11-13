# DAW-POL 3000 — przekazanie projektu (stan na 2026-11-15)

Dokument dla kolejnego asystenta AI (np. Gemini) albo programisty, który przejmuje projekt.
**Najpierw przeczytaj `CLAUDE.md`**: są tam wszystkie zasady współpracy, mapowanie przycisków i sekcja „Aktualny stan”.
Szczegóły modułów są w `docs/ARCHITEKTURA.md`, a historia zmian w `DZIENNIK.md` (najnowsze wpisy u góry).

## 1. Co to jest

DAW-POL 3000 to szkicownik muzyczny („DAW”) w stylu OP-1 dla konsoli **Anbernic RG35XX Pro** (firmware Knulli, ekran 640×480, sterowanie padem i dwoma analogami).
- Język: C++17 w stylu „C z klasami”. SDL2 służy tylko jako cienka warstwa platformy.
- Rdzeń `core/` rysuje UI do bufora 640×480 i liczy audio do bufora float. Ma tryb headless (`--render-png`, `--render-wav`) do testów bez urządzenia.
- Właściciel: Patryk. Rozmawia **po polsku**, nie zna Linuksa i ocenia wyniki słowami „git / nie git”.
- **UI programu jest po angielsku**, dokumentacja projektu po polsku.

## 2. Zasady, których trzeba pilnować

1. **Po każdej zmianie:**
   - wpis na górze `DZIENNIK.md` (co zmieniono, następny krok, czy trzeba czegoś od Patryka),
   - aktualizacja „Aktualny stan” w `CLAUDE.md`,
   - commit i push do `main`.
2. **ROM-y i banki SysEx nigdy nie trafiają do repo.** Katalog `roms/` jest w `.gitignore`.
   - ROM-y D-50 (LINEA) leżą lokalnie w `roms/linea/`, a na konsoli w `/userdata/dawpol3000/roms/linea/`.
   - Banki `.syx` na konsoli: `/userdata/dawpol3000/presets/linea/banks/` (zainstalowane) i `/userdata/dawpol3000/sysex/` (do importu).
3. **Hasło do konsoli** jest tylko w `tools/device.conf` (poza gitem). Nie commitować, nie wypisywać.
4. **Porty cudzego kodu robimy 1:1**, z testem zgodności bit w bit względem oryginału (`tools/test_*.sh`).
   - Dodatki DAW-POL są wyłączone przy ustawieniach neutralnych, więc oryginał brzmi identycznie.
   - Licencja i źródło każdej biblioteki trafiają do `third_party/LICENSES.md`.
5. **Bez nazw i grafik Teenage Engineering ani Rolanda.** Silniki i efekty mają własne nazwy (np. LINEA zamiast D-50), ekrany są rysowane wektorowo w kodzie.
6. **Licencja projektu:** GPL-3.0-or-later. OXBOW i OUTATIME są GPL-3.0-only, więc całość rozpowszechniana jest jako GPL-3.0-only.

## 3. Budowanie, testy, wgrywanie

| Co | Polecenie |
|---|---|
| Build na PC (okno 640×480) | `./tools/build.sh` → `build/dawpol3000` |
| Build ARM (konsola) | `./tools/build.sh arm` (cross `aarch64-linux-gnu`) |
| Wgranie na konsolę | `./tools/deploy.sh` (build ARM + scp + starter w Ports) |
| Komenda na konsoli | `./tools/dev.sh '<komenda>'`; log: `./tools/log.sh` |
| Paczka PortMaster | `./tools/package_portmaster.sh` → `dist/` |
| Podgląd ekranu | `build/dawpol3000 --render-png out/x.png <ekran>` (lista ekranów w `core/headless.cpp`) |
| Nagranie WAV | `build/dawpol3000 --render-wav out/x.wav <sekundy> <utwór>` (`synthwave`, `synthwavemix`, `synthwavefx`, `etude`, `allengines`, …) |
| CPU efektów miksera | `build/dawpol3000 --bench-fx` |
| Testy zgodności | `for t in tools/test_*.sh; do bash $t; done` (28 skryptów, każdy kończy się „roznych: 0”) |

Konsola: Knulli (Batocera 42), IP 192.168.50.239, użytkownik root. Logowanie kluczem nie działa (exFAT), więc hasło idzie przez SSH_ASKPASS (`tools/ssh_env.sh`).

## 4. Co jest zrobione

### Struktura aplikacji

Zakładki przełączane L2/R2: **PROJECT ↔ ARRANGE ↔ PIANO ROLL ↔ SYNTH ↔ MIXER**.
- **PROJECT:** zapis i odczyt projektów `.dpp`, autozapis, utwory demo.
- **ARRANGE:** 16 ścieżek MIDI i 4 audio, klipy, pętla IN/OUT, VU, mute/solo, 32 kolory, undo/redo.
- **PIANO ROLL:** wielotaktowy, zaznaczanie w 2 warstwach (transpozycja, przesuwanie, kopiowanie).
- **SYNTH:** instrument bieżącej ścieżki. Ma strony gałek (Select = następna), presety per silnik i ekran silnika.
- **MIXER:**
  - na stronie 5 kanałów plus stały master,
  - kanał: VU, EQ 3-pasmowy (GAIN/FREQ/Q, MAX EQ), kompresor, 4 inserty, 6 wysyłek AUX, panorama,
  - master: kompresor szyny w stylu SSL G.

### Silniki (31 + TEST SAW; okno TRACK → karta ENGINE, lista alfabetyczna)

- **Porty 1:1 z op1r (OP-1), bit w bit:** WAVETABLE (+ rozszerzony WAVETABLE+), TAFFY, PEW, ELG, EVA, MBIRA, REACTOR, SID, MONOLIT, VAPORA, SUPERSAW, MIXTUR, BRASSOPHONE, HOLLOW SAINT, BLOODMOON, BLOT, AETHER, ARC, DDDD, JUNI.
- **Z paczek Hundekraft:** OXBOW (+ OXBOW+), OUTATIME, GIN (perkusja).
- **Własne:** PARALLAX, LOUNGE, FELT, GRAND (modele fizyczne i subtraktywny).
- **VELOUR:** port PicoFaceSM (Michi71), string ensemble w stylu Solina. Każdy instrument ma własną gałkę-poziom. Ekran to scena orkiestry.
- **LINEA:** port PicoFaceD5 (Michi71), synteza LA na ROM-ach D-50 Patryka.
  - ROM-y czytane z karty i rozpoznawane po CRC32, PCM dekodowany w C++.
  - Banki SysEx: import, zapis do banku USER, przeglądarka.
  - Ekran PLAY: generatywna konstelacja, lewa gałka = wektor, prawa = CUTOFF/RES.
  - Strony edycji w stylu wyświetlacza D-50.
  - Na konsoli jest oryginalny bank fabryczny (Fantasia) jako FACTORY.

### Efekty miksera (op1r / Hundekraft, bit w bit, wiele egzemplarzy przez `core/fx/fxw.h`)

Każdy efekt ma swój oryginalny ekran.
- **Inserty:** JUNI CHORUS, STUTTER, CRUSH, ENTE (sidechain: T1..T4 = ścieżki 1..4, TAPE = suma ścieżek, BEAT = tempo).
- **AUX:** SISTER, SNOWGLOBE, RACKVERB, TAPE, FATHER.
- **Kompresory kanału** mają animowane sceny, a stary panel jest pod L3:
  - OPTO: lampka i przesłona aparatu,
  - VCA: tama,
  - FET: prasa hydrauliczna.

### Obciążenie CPU na konsoli (H700, jeden rdzeń)

- Silniki: zwykle 5–17% rdzenia na 6 głosów.
- Efekty: 1,2–2,6% na egzemplarz.
- Demo SYNTHWAVE: 24%, SYNTHWAVE FX: około 40%.
- CPU silnika LINEA nie było jeszcze mierzone.

## 5. Gdzie co jest (skrót)

- `core/synth/`: silniki. `wavetable_synth.*` to wspólny tor głosów, a `kind_` wybiera DSP:
  - GIN (23), VELOUR (28) i LINEA (29) mają własne tory renderowania,
  - nowy silnik dodaje się w ok. 15 miejscach; wzorem są ostatnie commity VELOUR i LINEA.
- `core/ui/`: ekrany. `synth_view.*` (strony SYNTH; GIN, VELOUR i LINEA mają osobne pliki `synth_*.cpp`), `mixer_view.*`, `*_screen.*` (ekrany silników).
- `core/fx/`: wrappery efektów op1r i rejestr efektów.
- `core/audio/`: `engine.*` (wątek audio) i `mixer.*` (tor miksera).
- `core/io/`: zapis projektów i presetów, `factory_presets.cpp`, `linea_banks.*`.
- `third_party/`: kod źródłowy portów, licencje w `LICENSES.md`.
- `tools/gen/`: generatory, np. `svg_to_art2.py` (SVG → nagłówek z rysunkiem) i `midi_to_demo.py`.

## 6. Otwarte sprawy / pomysły na dalej

- LINEA: zmierzyć CPU na konsoli. Do rozważenia: zmiana nazwy patcha w BANKS przed zapisem do USER.
- Efekty z CLAUDE.md jeszcze nieprzeniesione: delay i reverb DAW-POL, master EQ, efekty Airwindows / Mutable Instruments (kandydaci w CLAUDE.md).
- Etap 3, taśma: nagrywanie audio na ścieżki audio (widok szpul), overdub, punch in/out.
- Etap 7: wejście USB audio (gitara z amp sim), MIDI in/out, MIDI clock.
- Zakładka CREDITS z atrybucją wszystkich portów (decyzja Patryka: na koniec prac).
- Pozostałe instrumenty z PicoVintageSynthCollection (JV-880, RD, DX, YC, CP, J6, MD, OB) wymagają oceny CPU, a część także ROM-ów Patryka.

## 7. Jak pracować z Patrykiem

- Każdy krok kończy się czymś do sprawdzenia: PNG, WAV albo build na konsoli. Najpierw pokazać, potem iść dalej.
- Odpowiedzi krótkie, po polsku, bez żargonu. Na końcu blok: **Zmienione:** … / **Następny krok:** …
- Jeśli potrzebna jest jego ręka (włączyć konsolę, hasło, akceptacja licencji), podać dokładnie, co zrobić.
