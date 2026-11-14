# Kontury POLIVOKS

`trace_polivoks.py` odtwarza kontury z referencji Patryka: plik Gemini JPEG
2400×1792, SHA-256 zapisane w nagłówku `core/ui/polivoks_art.inc`.
Referencja jest lokalnym materiałem autora; nie jest wymagana w buildzie.

Narzędzie offline wymaga Pythona oraz `numpy`, `pillow`,
`opencv-python-headless`. Uruchomienie z katalogu projektu:

```sh
python3 tools/art/trace_polivoks.py /sciezka/do/referencji.jpeg core/ui/polivoks_art.inc
```

Wyjściem są współrzędne złożonych konturów i ich kolory dla natywnego Canvas.
Pomijane są napisy/kontrolki makiety. Prawa część postaci zasłonięta źródłowym
oscyloskopem jest uzupełniana z widocznej lewej strony. Paleta ma 32 kolory;
kontury mają jednopikselowe zakładki w rozdzielczości źródła, co zapobiega
ciemnym szwom po rasteryzacji 1:4. Liczby konturów i punktów są wypisywane po eksporcie. Paleta jest stała,
pochodzi z zatwierdzonej wersji grafiki; lokalna korekta nie zmienia kolorów głowy.
Jest to wierna geometria referencji z kwantyzacją kolorów, nie pikselowa
identyczność z JPEG. Zachować kształty przy kolejnych zmianach UI.

`PolivoksScreen` buforuje kontury raz. Paszcza używa ciągłego odkształcenia
dolnej części twarzy (cutoff), oczy zmiany barwy (rezonans). Osiem stron
parametrów oraz DSP pozostają niezależne od geometrii ilustracji.

Podglądy skrajnych wartości:

```sh
POLIVOKS_CUTOFF=0 POLIVOKS_RESONANCE=0 build/dawpol3000 --render-png out/polivoks_closed.png polivoks
POLIVOKS_CUTOFF=32767 POLIVOKS_RESONANCE=32767 build/dawpol3000 --render-png out/polivoks_open.png polivoks
```

Oryginalny oscyloskop jest usuwany z całej części centralnej, również w etapie
przypisania palety. Samo usunięcie jasnych pikseli pozostawiało słabą poświatę,
która po kwantyzacji wracała jako kolorowe odnogi. Prawy dolny obszar zasłonięty
wiązką jest uzupełniany z lewej połowy przed wydzieleniem konturów.
