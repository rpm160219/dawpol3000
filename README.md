# DAW-POL 3000

Szkicownik muzyczny na Anbernic RG35XX Pro (Knulli), UI w duchu OP-1. Licencja: GPL-3.0-or-later.

- `./tools/build.sh` — build desktop (okno 640×480), `./tools/build.sh arm` — build na konsolę
- `build/dawpol3000 --render-png out/x.png` / `--render-wav out/x.wav 5` — tryb bez okna
- `./tools/deploy.sh <IP>` — wysyłka na konsolę i uruchomienie

Opis budowy programu: [docs/ARCHITEKTURA.md](docs/ARCHITEKTURA.md).

Piano roll — klawiatura na PC (konsola: te same funkcje na D-padzie/ABXY/analogach):
strzałki = kursor, X = wstaw nutę (A na konsoli), Z = usuń (B), S + ←/→ = długość (X), A + strzałki = przesuń (Y),
Q/W = oktawa, Enter = play/stop, gałki: F/H długość, T/G velocity, J/L głośność, I/K tempo, Esc = wyjście.
`--test` uruchamia ekran testu sprzętu.
