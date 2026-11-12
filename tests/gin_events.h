// Wspolny scenariusz testu GIN: wszystkie 8 zestawow, kazdy klawisz, gesto (limit 4 glosow syntezy, wyciszanie tego
// samego klawisza, kradziez odbiornikow), potem wybrzmienie.
#pragma once
#define GE_KITS 8
#define GE_BLOCKS 420
// klawisz uderzony w bloku b (albo -1); drugi klawisz w tym samym bloku dla co 5. uderzenia
static int ge_key(int kit, int b, int which) {
    if (b >= 300) return -1;
    if (b % 6) return -1;
    const int i = b / 6;
    if (which == 0) return (i * 7 + kit * 3) % 24;
    return i % 5 == 0 ? (i * 11 + 5) % 24 : (i % 9 == 0 ? (i * 7 + kit * 3) % 24 : -1);  // ten sam klawisz = wyciszenie
}
