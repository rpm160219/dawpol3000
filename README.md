# DAW-POL 3000

Szkicownik muzyczny na Anbernic RG35XX Pro (Knulli), UI w duchu OP-1. Licencja: GPL-3.0-or-later.

- `./tools/build.sh` — build desktop (okno 640×480), `./tools/build.sh arm` — build na konsolę
- `build/dawpol3000 --render-png out/x.png` / `--render-wav out/x.wav 5` — tryb bez okna
- `./tools/deploy.sh <IP>` — wysyłka na konsolę i uruchomienie

Opis budowy programu: [docs/ARCHITEKTURA.md](docs/ARCHITEKTURA.md).

Zakładki PROJECT ↔ ARRANGE ↔ PIANO ROLL: L2 / R2 (na PC: klawisze 1 / 2). Start z ostatnim projektem.
PROJECT: góra/dół = lista, X = nazwa, Y = zapisz, A = wczytaj / nowy, B = usuń. Projekty: `projects/<NAZWA>/project.dpp`.
ARRANGE: strzałki = kursor, X(PC)/A(konsola) na „ADD TRACK” = nowa ścieżka, na klipie = otwórz pattern, na pustym = wstaw ostatni pattern;
Y (konsola) = nowy pattern, B = usuń klip, X krótko = klon, X + ←/→ = przesuń klip, Enter/Start = play całości,
Select = ustawienia ścieżki (nazwa, kolor, usuń), L1 = mute, R1 = solo.

Piano roll — klawiatura na PC (konsola: te same funkcje na D-padzie/ABXY/analogach):
strzałki = kursor, X = wstaw nutę (A na konsoli), Z = usuń (B), S + ←/→ = długość (X), A + strzałki = przesuń (Y),
Q/W = oktawa, Enter = play/stop, gałki: F/H długość, T/G velocity, J/L głośność, I/K tempo, Esc = wyjście.
Narzędzia: klik lewej gałki (PC: U) → B undo, A redo, Y IN/OUT pętli, X pętla wł/wył. Zaznaczanie w piano rollu: klik prawej gałki (PC: O) → B zaznacz (trzymaj + strzałki = prostokąt), A usuń, Y kopiuj, X wklej;
L2/R2 (PC: 1/2) = druga warstwa: trzymaj B + góra/dół = transpozycja, trzymaj A + strzałki = przesuń, trzymaj Y + góra/dół = oktawa, X = duplikat.
SYNTH (R2 z piano rolla): silnik (WAVETABLE / WAVETABLE+ / TEST SAW) wybierasz w oknie ścieżki: Select → R2 (karta ENGINE); Select = następna strona; L3: na obwiedniach RELEASE ↔ AMOUNT, na OSC 1 LEVEL ↔ SUB, na LFO synchronizacja z BPM; gałki = parametry; odsłuch z przycisków (PC: strzałki + A/S/Z/X).
Silniki: WAVETABLE / WAVETABLE+, TAFFY, PEW, ELG, EVA, MBIRA, REACTOR, SID, MONOLIT (porty 1:1 z op1r, Hundekraft, GPL-3.0-or-later — `third_party/op1r_*`) i OXBOW (Hundekraft wg OberVave zvensona, GPL-3.0-only — `third_party/kennel_oxbow`), TEST SAW. Przy starcie ekran z nazwą programu (dowolny przycisk pomija).
`--test` uruchamia ekran testu sprzętu. `--render-wav out.wav 42 allengines` — utwór demo „ALL ENGINES” (wszystkie silniki) do WAV. `tools/test_wavetable.sh`, `tools/test_taffy.sh`, `tools/test_pew.sh`, `tools/test_elg.sh`, `tools/test_eva.sh`, `tools/test_mbira.sh`, `tools/test_reactor.sh`, `tools/test_oxbow.sh`, `tools/test_oxbowplus.sh`, `tools/test_sid.sh`, `tools/test_monolit.sh` — testy zgodności z oryginałami.
Presety: ostatnia strona SYNTH (Select); pliki osobno dla silnika: `presets/wavetable/`, `presets/wavetableplus/`, `presets/taffy/`, `presets/pew/`, `presets/elg/`, `presets/eva/`, `presets/mbira/`, `presets/reactor/`, `presets/oxbow/`, `presets/oxbowplus/`, `presets/sid/`, `presets/monolit/` (na konsoli `/userdata/dawpol3000/presets`).

Paczka PortMastera (do instalacji na innej konsoli): `./tools/package_portmaster.sh` → `dist/DAW-POL-3000_portmaster_<data>.zip` (rozpakować do folderu ports na karcie).

## POLIVOKS

Niezależny silnik z własnym DSP, mono/duo, filtrem LP/BP, obwiedniami Repeat
i oversamplingiem 4×. Wybór: TRACK → ENGINE → POLIVOKS. Select przełącza osiem
stron, L3 dodatkowe parametry. Bank: 12 presetów w `presets/polivoks/`.
Ekran: wektorowy Cyber-Niedźwiedź z cewkami i oscyloskopem.
[Opis integracji i testów](docs/POLIVOKS.md). Test: `./tools/test_polivoks.sh`.
