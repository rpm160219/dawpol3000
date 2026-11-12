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
Narzędzia: klik lewej gałki (PC: U) → B undo, A redo, Y IN/OUT pętli, X pętla wł/wył. Zaznaczanie w piano rollu: klik prawej gałki (PC: O) → B zaznacz (trzymaj + strzałki = prostokąt), A usuń, Y kopiuj, X wklej.
`--test` uruchamia ekran testu sprzętu.
