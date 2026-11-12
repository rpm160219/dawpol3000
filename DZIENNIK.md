# Dziennik DAW-POL 3000

## 2026-11-12 14:10
**Zmienione:** nowy cel LFO **W1+2** (fale obu oscylatorów naraz; dopisany na końcu listy — zapisane projekty bez zmian) — `core/seq/project.*`, `core/synth/wavetable_synth.*`; ekran pokazuje też ruch fali OSC 1 przy W1 / W1+2. Dokończone z poprzedniej sesji: opis trybu rozszerzonego w `docs/ARCHITEKTURA.md`, README, CLAUDE.md; pomiar na konsoli: 6 głosów — oryginał 9,3%, WAVETABLE+ (2 LFO, sub, BP) 10,1% rdzenia. Zgodność oryginału nadal 0 różnic.
**Następny krok:** Patryk testuje WAVETABLE+ na konsoli.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-12 13:40
**Zmienione:** tryb ROZSZERZONY WAVETABLE (R3 w SYNTH; oryginał 1:1 zostaje domyślny i nadal zgodny bit w bit): własny DSP wzorowany na `wavetable_sample` — 2 niezależne oscylatory (fala, interwał ±24, detune ±50 c, poziom), SUB (sinus −1 okt., L3 na OSC 1), filtr LP24 (oryginał) / LP12 / BP / HP + DRIVE; strony OSC 1, OSC 2, FILTER; nowe cele LFO W1 W2 P1 P2 LV1 LV2; na ekranie fala OSC 2 na czerwono, krzywa filtra wg trybu. Pliki: `core/seq/project.*`, `core/io/project_file.cpp` (naprawiony parser długich linii), `core/synth/wavetable_synth.*`, `core/audio/engine.*`, `core/ui/synth_view.*`, `core/ui/wavetable_screen.*`, `core/headless.*`.
Testy: oryginał 0 różnic; rozszerzony z ustawieniami jak oryginał różni się o ~−77 dB; tryby filtra i SUB działają.
**Do zrobienia:** aktualizacja `docs/ARCHITEKTURA.md` i README o tryb rozszerzony; test wydajności trybu rozszerzonego na konsoli.
**Wymaga od Patryka:** odsłuch na konsoli.

## 2026-11-12 12:20
**Zmienione:**
- `core/seq/project.*` — synchronizacja LFO (`sync`, `div`), 15 podziałów 1/1 … 1/128 z triolami; AMOUNT obwiedni głośności −1..1; `core/io/project_file.*` — zapis/odczyt (zgodnie wstecz).
- `core/synth/wavetable_synth.*` — LFO zsynchronizowane: przy grającym utworze faza = pozycja w ćwierćnutach / długość cyklu (przypięte do bitu), bez transportu biegnie z częstotliwością z tempa; wygładzenie LFO ~3 ms i tremolo interpolowane próbka po próbce (koniec kliknięć przy SAW/SQR → VOL); AMOUNT głośności ujemny = obwiednia odwrócona.
- `core/seq/sequencer.h` — `beatPos()`, `tempo()`; `core/audio/engine.cpp` — przekazywanie transportu do instrumentów przed każdym odcinkiem.
- `core/ui/synth_view.*` — L3 na stronach LFO: SYNC/FREE (RATE → podział, etykieta SYNC), AMOUNT ±100 w AMP ENV.
- Testy: skoki przy SAW/SQR na VOL na poziomie czystego sinusa (brak kliknięć); faza LFO 1/4 zgodna z pozycją w takcie; zgodność z oryginałem 0 różnic.
**Następny krok:** Patryk testuje.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-12 11:30
**Zmienione (przegląd kodu obwiedni i LFO po uwadze Patryka):**
- `core/synth/wavetable_synth.*` — LFO biegnie w czasie rzeczywistym także w ciszy (wcześniej stał między frazami, więc każda fraza „startowała” go od miejsca, gdzie skończyła poprzednia); SUSTAIN śledzi gałkę w trakcie nuty; szybkie powtórzenie tej samej nuty w trakcie wyciszania nie zajmuje drugiego głosu; obwiednia filtra ma własne RELEASE; AMOUNT obwiedni głośności (0 = stała głośność przy trzymaniu, rampa bramki 3 ms); ekran dostaje modulację ostatnio uderzonej nuty (zamiast „najgłośniejszej”, która skakała między głosami).
- `core/seq/project.*`, `core/io/project_file.*` — `envAmt`, obwiednia filtra A/D/S/R + `fenvAmt`; wczytywanie pierwszej wersji zapisu (A D S AMOUNT) zachowane; skrócone nazwy kształtów i celów LFO.
- `core/ui/synth_view.*` — L3 na stronach obwiedni zamienia RELEASE ↔ AMOUNT; usunięte napisy o klawiszach i zmiana oktawy; `core/ui/app.cpp` — L3 trafia do zakładki SYNTH.
- Testy: zgodność z oryginałem 0 różnic; LFO po 0,25 s ciszy w poprawnej fazie; 3× ta sama nuta = 1 głos; SUSTAIN w trakcie nuty; AMOUNT 0 = stała głośność i cisza po puszczeniu; stary plik OK; brak kliknięć.
**Następny krok:** Patryk testuje na konsoli.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-12 10:30
**Zmienione:**
- `core/synth/wavetable_synth.*` — bez kliknięć: przejmowany/ponownie uderzany brzmiący głos najpierw wycisza się ~2 ms (dopiero potem reset DSP i nowa nuta), ogon filtra decymującego wybrzmiewa (zamiast zerowania); obwiednia filtra na głos (A/D/S/AMOUNT, wybrzmienie = R głośności); 2 LFO na ścieżkę (sinus, trójkąt, piła, prostokąt, losowy; 0,05–20 Hz; cele WAVE, DETUNE, CUTOFF, RESONANCE, PITCH, VOLUME); zmodulowane gałki dla ekranu.
- `core/audio/engine.*` — przekazywanie obwiedni filtra i LFO; minimalny atak 1 ms i wybrzmienie 3 ms.
- `core/seq/project.*`, `core/io/project_file.*` — obwiednia filtra i LFO w ścieżce i w pliku (`fenv`, `lfo1`, `lfo2`).
- `core/ui/synth_view.*` — 5 stron gałek (SOUND, AMP ENV, FILTER ENV, LFO 1, LFO 2) z kropkami stron, wykres obwiedni filtra / kształtu LFO z celem, czytelny opis klawiszy („D-PAD + ABXY = C4 MAJOR SCALE, L1/R1 = OCTAVE”), ekran pokazuje ruch modulacji.
- Headless: `synthfenv`, `synthlfo`. Testy: zgodność z oryginałem nadal 0 różnic; pomiar kliknięć — największy skok między próbkami spadł z 10,4% do 5,1% sygnału (poziom naturalnego zbocza fali, czyli brak kliknięć).
**Następny krok:** Patryk testuje na konsoli.
**Wymaga od Patryka:** tak — odsłuch.

## 2026-11-12 09:30
**Zmienione:**
- `third_party/op1r_wavetable/` — oryginalne pliki silnika WAVETABLE z op1r (Hundekraft, GPL-3.0-or-later): `wavetable_dsp.h`, `wavetable_level.h`, `wavetable_geom.h`, `orig_88k/wavetables.h`, LICENSE, NOTICE, README ze źródłem i commitem; `wavetables.h` przeliczony ich generatorem dla 96 kHz (`tools/gen/op1r_wavetable_gen_tables.py`; z 88200 odtwarza oryginał bajt w bajt). `third_party/LICENSES.md`.
- `core/synth/wavetable_synth.*` — instrument: oryginalny DSP (ścieżka C), głos 96 kHz w blokach 128, decymacja 2:1 FIR z harnessa op1r (bufor kołowy), 6 głosów, własny ADSR.
- `core/ui/wavetable_screen.*` — ekran przeniesiony 1:1 (statyka wg `gen_svg.py` w cache, ruch wg `wavetable_screen.c`), skala 2×; `core/ui/synth_view.*` — zakładka SYNTH (strony SOUND/ENVELOPE, wykres ADSR, nuty z przycisków).
- `core/seq/project.*`, `core/io/project_file.*` — instrument ścieżki (silnik, 4 gałki, ADSR) w projekcie i w pliku; `core/audio/engine.*` — instrument wg ścieżki; `core/ui/dialogs.*` — wybór silnika L1/R1; `core/ui/app.*` — 4. zakładka; `core/gfx/canvas.*` — `copyRows`.
- `tests/wavetable_match.cpp`, `tools/test_wavetable.sh` — test zgodności: **0 różnic na 105 984 próbkach** względem oryginalnego harnessa op1r.
- Wydajność na konsoli: 1,37% rdzenia na głos (6 głosów 8,2%).
**Następny krok:** Patryk testuje brzmienie i sterowanie WAVETABLE na konsoli.
**Wymaga od Patryka:** tak — odsłuch i ocena sterowania.

## 2026-11-12 08:00
**Zmienione:**
- `core/gfx/canvas.h` — 32 kolory ścieżek (8 dotychczasowych + 24 nowe); `core/io/project_file.cpp`, `core/seq/project.*`, ekrany — obsługa 32 barw.
- `core/ui/dialogs.*` — wybór koloru ścieżki z siatki 4×8 (krzyżak), zamiast listy nazw.
- `core/synth/saw_synth.*` — pomiar szczytu ścieżki; `core/audio/engine.*` — poziomy ścieżek (szczyt + opadanie ~24 dB/s) do VU, przekazywane atomowo.
- `core/ui/arrange.cpp` — segmentowy VU (−48..0 dB, zielony/żółty od −12 dB/czerwony od −3 dB) pod paskiem głośności każdej ścieżki.
**Następny krok:** ocena Patryka; potem step seq/perkusja albo syntezator.
**Wymaga od Patryka:** tak — ocena.

## 2026-11-12 07:35
**Zmienione:** `core/ui/piano_roll.*` — warstwy menu zaznaczania (R3) przełączane L2/R2 zamiast L1/R1 (L2 = warstwa 1, R2 = warstwa 2); `core/ui/app.cpp` — gdy menu R3 jest otwarte, L2/R2 nie zmieniają zakładek. README, CLAUDE.md, ARCHITEKTURA.md zaktualizowane. Test: R2 w menu zmienia warstwę i zostaje w piano rollu; po wyjściu z menu L2 przechodzi do ARRANGE.
`tools/deploy.sh` — wysyłka pod tymczasową nazwę i podmiana (`mv`), bo nadpisanie działającego programu na konsoli się nie udawało.
**Następny krok:** ocena Patryka; potem step seq/perkusja albo syntezator.
**Wymaga od Patryka:** tak — ocena (aplikację uruchomić ponownie, żeby weszła nowa wersja).

## 2026-11-12 07:20
**Zmienione:** poprawka mojej pomyłki — `core/ui/app.cpp`: kropki zakładek (PROJECT/ARRANGE/PIANO ROLL) wróciły pod nagłówek; `core/ui/piano_roll.cpp`: kropki warstw paska R3 (L1/R1) przywrócone i przeniesione na dół ekranu (wcześniej leżały na linii). CLAUDE.md — opis położenia obu wskaźników.
**Następny krok:** ocena Patryka; potem step seq/perkusja albo syntezator.
**Wymaga od Patryka:** tak — ocena.

## 2026-11-12 07:05
**Zmienione:** `core/ui/widgets.cpp` — kółko i litera przycisku w pigułkach wyrównane do środka piksela (litera na środku koła), napis BAR w nagłówku wyśrodkowany; `core/ui/app.cpp` — kropki zakładek przeniesione na dół ekranu; `core/ui/piano_roll.cpp` — usunięte kropki warstw spod linii (warstwę pokazuje status „1/2” / „2/2”). CLAUDE.md — zasady.
**Następny krok:** ocena Patryka; potem step seq/perkusja albo syntezator.
**Wymaga od Patryka:** tak — ocena.

## 2026-11-12 06:45
**Zmienione:** `core/ui/piano_roll.*` — druga warstwa paska zaznaczania (L1/R1): B TRANSPOSE (trzymaj + góra/dół, półtony), A MOVE (trzymaj + krzyżak, czas i wysokość), Y OCTAVE (trzymaj + góra/dół), X DUPLICATE (kopia zaraz za zaznaczeniem); przesunięcia „wszystko albo nic” (kolizja albo wyjście poza pattern = brak zmiany), kursor podąża za zaznaczeniem, odsłuch najwyższej nuty; kropki warstw nad paskiem. Headless: `transform`. Dokumentacja/README/CLAUDE.md.
Testy: transpozycja, oktawa, przesuwanie z blokadą na końcu patternu, duplikat bez miejsca, undo całego gestu — OK.
**Następny krok:** Patryk ocenia (i potwierdza, czy Y = OCTAVE i X = DUPLICATE mu pasują); potem step seq/perkusja albo syntezator.
**Wymaga od Patryka:** tak — test i decyzja o Y/X.

## 2026-11-12 06:10
**Zmienione:**
- `core/ui/widgets.*` — pigułki z tekstem dopasowanym do obrysu; nowy kompaktowy `actionBar` (4 pigułki w kolumnach gałek, linia oddzielająca, linia statusu).
- `core/ui/piano_roll.*` — pasek zaznaczania na `actionBar`; podświetlenie zakresu pętli + flagi IN/OUT na siatce.
- `core/ui/app.*` — pasek NARZĘDZI pod L3: B UNDO, A REDO, Y IN/OUT, X LOOP; redo (druga historia); ekran dostaje wtedy tylko D-pad/Start/R3.
- `core/ui/buttons.h` — `masked()`; `core/ui/arrange.*` — linia i podświetlenie pętli, `cursorBar()`.
- `core/seq/project.*`, `pattern.h` — pętla utworu (takty) i patternu (kroki); `core/seq/sequencer.cpp` — odtwarzanie w zakresie pętli, utwór bez pętli zatrzymuje się na końcu; `core/io/project_file.*` — linie `loop`.
- Headless: `tools`, `rolltools`. Dokumentacja/README/CLAUDE.md.
- Testy: pętla taktów 3–5 i kroków 5–8 grają tylko swój zakres; bez pętli utwór staje; undo/redo w obie strony; pętla przetrwa zapis/odczyt.
**Następny krok:** ocena Patryka; potem wybór: step seq/perkusja, syntezator.
**Wymaga od Patryka:** tak — test na konsoli.

## 2026-11-12 05:15
**Zmienione:**
- `core/ui/input.h`, `platform/controls_map.h`, `core/ui/test_screen.cpp` — nowe przyciski L3/R3 (klik gałek; konsola: 12 i 15, PC: U i O).
- Nowy `core/seq/undo_stack.h` — historia 32 migawek projektu.
- `core/ui/app.*` — undo pod L3; krok historii zamyka się, gdy przyciski i gałki są puszczone (jedno wciśnięcie / pokręcenie = jeden krok, okno dialogowe = jeden krok); po undo poprawka kursorów/ekranów; komunikaty „toast”; historia czyszczona przy wczytaniu/nowym projekcie.
- `core/ui/piano_roll.*` — tryb zaznaczania pod R3: dolny pasek → pigułki B SELECT / A DELETE / Y COPY / X PASTE; B = zaznacz nutę, B + D-pad = prostokąt, B na pustym = wyczyść; schowek działa między patternami; wklejone nuty zostają zaznaczone.
- `core/ui/arrange.*` — `clampCursor()`. Headless: ekran `select`. Dokumentacja/README/CLAUDE.md.
- Testy (symulacja przycisków): pokręcenie BPM = 1 krok undo; wklej → usuń → undo → undo poprawnie; undo dodania ścieżki + patternu OK.
**Następny krok:** ocena Patryka; potem wybór: step seq/perkusja, pętla/markery, syntezator.
**Wymaga od Patryka:** tak — test na konsoli.

## 2026-11-12 04:30
**Zmienione:**
- `core/gfx/icons.*` — dyskietka zamiast notatnika (ilustracja w polu 196×150 + mała ikona projektu).
- `core/ui/widgets.*` — wspólny `screenHeader` (ARRANGE i PIANO ROLL: ten sam układ, „BAR takt.miara” w tym samym miejscu) i `drawTextFit` (tekst przycinany do szerokości).
- `core/ui/arrange.*` — nazwy klipów przycinane do widocznej części klipu przy każdym zoomie; `highlightedPattern()`.
- `core/ui/app.cpp` — R2 z ARRANGE otwiera pattern pod kursorem (albo ostatni na ścieżce).
- `core/ui/piano_roll.cpp` — nagłówek ze wspólnego `screenHeader` (pozycja kursora/głowicy jako takt.miara zamiast „BAR 1/2”), mniejsze etykiety miar nad siatką.
**Następny krok:** ocena Patryka; potem wybór: step seq/perkusja, pętla/markery, undo, syntezator.
**Wymaga od Patryka:** tak — test i wybór kolejnego kroku.

## 2026-11-12 04:00
**Zmienione:**
- Nowe: `core/io/fs.*` (katalogi), `core/io/project_file.*` (format `.dpp`: tekstowy, wersjonowany, zapis atomowy z fsync), `core/io/project_store.*` (projekty w `projects/<NAZWA>/`, lista od najnowszego, ostatnio otwarty), `core/ui/project_view.*` (zakładka PROJECT).
- `core/gfx/icons.*` — ilustracja notatnika (196×150, jak syntezator/magnetofon) i ikona projektu.
- `core/ui/app.*` — zakładki PROJECT ↔ ARRANGE ↔ PIANO ROLL (L2/R2) z kropkami pod nagłówkiem, stan „niezapisane”, zapis/odczyt/nowy, autozapis przy wyjściu, start z ostatnim projektem.
- `platform/main_sdl.cpp` — projekty w `projects/` (na konsoli `/userdata/dawpol3000/projects`), autozapis przy wyjściu, SIGTERM (Hotkey+Start) kończy program normalnie.
- Headless: ekran `project`. Testy: zapis → nowy → odczyt daje identyczny projekt; autozapis i start z ostatnim projektem OK; zapis na karcie konsoli (exFAT) OK.
- Dokumentacja, README, CLAUDE.md zaktualizowane; `projects/` w `.gitignore`.
**Następny krok:** ocena Patryka; potem do wyboru: step seq/perkusja, pętla i markery, undo, prawdziwy syntezator.
**Wymaga od Patryka:** tak — test na konsoli i wybór kolejnego kroku.

## 2026-11-12 03:10
**Zmienione:**
- `core/gfx/icons.cpp` — ilustracje narysowane od nowa na sztywnej siatce liczonej od obudowy: syntezator (galki mieszczą się w obrysie, usunięty kabel, poprawny układ czarnych klawiszy 2+3), magnetofon (4 wskaźniki VU równe i w osi, wycentrowane szpule, zamiast suwaków przyciski transportu).
- `core/ui/dialogs.cpp` — ikony na liście TYPE wyrównane do środka wierszy (MIDI/AUDIO); nowe proporcje ilustracji.
- `core/ui/arrange.cpp` — miniatury nut w klipach mapowane dokładnie na oś taktów i przycinane do klipu; siatka (kropki taktów) rysowana pod klipami, klipy z nieprzezroczystym tłem (kropki nie udają nut).
**Następny krok:** ocena Patryka; potem zapis/odczyt projektu.
**Wymaga od Patryka:** tak — test na konsoli.

## 2026-11-12 02:30
**Zmienione:**
- `core/gfx/icons.cpp` — ikony przerysowane od nowa (siatka 16×16, jedna grubość kreski, zaokrąglone końce, mniej detali): klawiatura 3+2 klawisze, fala = jeden okres sinusa, aranżacja = 3 rzędy kresek, czytelniejsze play/pętla/metronom/taśma.
- `core/seq/project.*` — mute/solo na ścieżce, `audible()`, `removeTrack()` (z klipami).
- `core/seq/sequencer.cpp`, `core/audio/engine.*` — mute/solo w trybie SONG (nie wyzwala nut, wycisza grające).
- `core/ui/dialogs.*` — okno TRACK (edycja): nazwa, kolor z listy z próbkami, usuwanie (Y dwa razy), OK/CANCEL.
- `core/ui/arrange.*` — Select = ustawienia ścieżki, L1 = mute, R1 = solo; wskaźniki M (żółty) / S (cyjan) przy nazwie; wyciszone ścieżki przygaszone. `core/ui/app.*` — `openTrackEdit()`.
- Headless: ekran `trackedit`. Dokumentacja, README, CLAUDE.md zaktualizowane. Testy: zmiana nazwy, kolor, mute/solo (cisza po ~10 ms), usuwanie — OK.
**Następny krok:** ocena Patryka; potem zapis/odczyt projektu.
**Wymaga od Patryka:** tak — test na konsoli.

## 2026-11-12 01:40
**Zmienione:**
- Nowe moduły: `seq/project.*` (ścieżki 16 MIDI + 4 audio, pula patternów, klipy), `ui/app.*` (zarządzanie ekranami i projektem), `ui/view.h`, `ui/arrange.*` (ekran ARRANGE z BPM), `ui/dialogs.*` (NEW TRACK, NEW PATTERN), `ui/widgets.*` (belki, pigułki, pola, listy, edycja nazw), `gfx/icons.*` (ikony + ilustracje syntezatora i magnetofonu).
- Zmienione: `seq/pattern` (nazwa, 1–16 taktów, 512 nut), `seq/sequencer` (tryby SONG i PATTERN, wiele ścieżek), `audio/engine` (migawka całego projektu, synth na ścieżkę, master + łagodny limiter), `synth/saw_synth` (dodaje do miksu z głośnością ścieżki, nic nie liczy bez nut), `ui/piano_roll` (pattern z projektu, wiele taktów), `gfx/canvas.h` (nowa paleta: kolory menu i ścieżek), `headless` (ekrany arrange/track/pattern/roll), `main_sdl.cpp` (start w ARRANGE z projektem demo).
- Referencje Patryka skopiowane do `ref/`. Zaktualizowane `docs/ARCHITEKTURA.md` i CLAUDE.md (mapowanie L2/R2, paleta menu, stan).
- Test przepływu (symulacja przycisków): dodanie ścieżki → pattern 3 takty → klip → nuty → klon klipu → odtwarzanie — OK. CPU na konsoli ~2.5% rdzenia.
**Następny krok:** Patryk testuje na konsoli; potem zapis/odczyt projektu, usuwanie/zmiana nazwy ścieżek, mute/solo.
**Wymaga od Patryka:** tak — test i ocena.

## 2026-11-12 03:20
**Zmienione:**
- `core/gfx/font.cpp` — dodany znak `#` (brakowało go, więc „C#5” rysowało się jako „C5”).
- `core/gfx/canvas.*` — zaokrąglone prostokąty (wypełnienie i obrys, SDF z antyaliasingiem).
- `core/ui/piano_roll.*` — styl bliżej OP-1: nuty jako pigułki, **kolor nuty zależy od velocity** (ciemna zieleń → zieleń → mięta, zieleń = kolor enkodera VELOCITY), siatka z kropek + linie tylko na miarach, mini-klawiatura z zaokrąglonymi klawiszami, biały kursor, zaznaczona nuta w białej obwódce z niebieskim uchwytem długości, głowica z trójkątem; `setView()` do ustawienia widoku.
- Folder `ref/` na grafiki wzorcowe (w `.gitignore`). CLAUDE.md: zasady stylu (pigułki, kropki) i grafik wzorcowych.
**Następny krok:** Patryk ocenia nowy wygląd i podsyła grafiki wzorcowe do `ref/`.
**Wymaga od Patryka:** tak — ocena i grafiki.

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
