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
Silniki: WAVETABLE / WAVETABLE+ i TAFFY (porty 1:1 z op1r, Hundekraft, GPL-3.0-or-later — `third_party/op1r_wavetable`, `third_party/op1r_taffy`), TEST SAW.
`--test` uruchamia ekran testu sprzętu. `tools/test_wavetable.sh`, `tools/test_taffy.sh` — testy zgodności z oryginałami.
Presety: ostatnia strona SYNTH (Select); pliki osobno dla silnika: `presets/wavetable/`, `presets/wavetableplus/`, `presets/taffy/` (na konsoli `/userdata/dawpol3000/presets`).
