# POLIVOKS — 12 nowych barw (bank fabryczny v2)

Łącznie 24 presety. Pierwsze 12 zatwierdzonych barw pozostaje bez zmian.
Dodatki instalują się po uruchomieniu nowego buildu; istniejące własne presety,
także o takich samych nazwach, pozostają zachowane.

| Preset | Kategoria | Charakter |
|---|---|---|
| RED HAMMER | BASS | Szybki atak filtra, piła 16' i sub-puls 32'. |
| PULSE ENGINE | BASS | Wąski puls z wolną modulacją szerokości OSC II. |
| BURIED COPPER | BASS | Głęboki trójkąt/piła 32', mały rezonans, dłuższy ogon. |
| RAZOR FIFTH | LEAD | Wysoka kwinta, puls/piła, delikatny cross-mod i glide. |
| GLASS CURRENT | BELL | Wygasający dzwon: trójkąty, FM i ring. |
| VOWEL PRESS | LEAD | BP, impulsy i odwrócona obwiednia filtra. |
| DUO BRASS | PAD | Duofoniczny square/saw, miękki atak, osobne vibrato. |
| PWM HORIZON | PAD | Powolna duofonia, pulsująca szerokość i oddech filtra. |
| RADIO STEPS | FX | S&H na wysokości i filtrze, szum oraz odrobina metalu. |
| GEIGER HEART | FX | Obie obwiednie Repeat, krótkie rezonansowe impulsy. |
| STEEL CLAP | DRUMS | Krótki szum BP z metalicznym dodatkiem. |
| SNOW CIRCUIT | PAD | Wolna szumowa tekstura BP z nieregularną modulacją. |

Wszystkie barwy pochodzą wyłącznie z własnego DSP Polivoksa. Brak dodatkowych
obwiedni/LFO WavetableSynth. Ring i PWM są jawnymi rozszerzeniami programu.
Zapis zawiera komplet 41 pól v2. Bank C jest w `core/synth/polivoks_presets.c`,
tabela przeglądarki w `core/io/polivoks_factory.inc` (porównywana testami).

Odsłuch nowych 12: `out/polivoks/new_presets.wav`, spis czasów
`out/polivoks/new_presets.md`. Pełny samodzielny bank i 24 osobne klipy są
w katalogu nadrzędnym `presets/`; `make presets` i `python3 package_presets.py`
odtwarzają odsłuchy i paczkę. Odsłuchy używają stałej korekty wzmocnienia
bez kompresji; nie zmienia to parametrów presetów. Barwy perkusyjne i dzwon
mają naturalne wygaszanie, więc niski RMS długiej przytrzymanej nuty jest celowy.
