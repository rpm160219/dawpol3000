# op1r BRASSOPHONE (nvmsocool) — port do DAW-POL 3000

Źródło: https://github.com/pcppcp/op1r-src — `synth_sdk/plugins/brassophone`, commit `46dfde8`.
Autor: nick (nvmsocool); op1r: pcppcp. Licencja: **GPL-3.0-or-later** z dodatkowym pozwoleniem dot. firmware OP-1
(`LICENSE`, `NOTICE` — pliki z katalogu głównego op1r-src).

Bez zmian: `brass.c` (DSP: sinus z fazą wyginaną tablicą `warp_table.h`, barwa i podjazd ~50 centów z obwiedni, wibrato
od podtrzymania), `horn.c` (ekran rogu), `warp_table.h`, `brass-handmade.svg` (→ `brass_art.h`, grupy white, blue-2,
blue-1, green-move, red-spinner), `plugin.json`, `gen_warp_table.py`. Zaślepki: `op1_plugin.h`, `op1_synth.h`.
Odrzucone: `brass_dsp.S` (jądro Blackfin; pętla C daje te same próbki).

W DAW-POL: `core/synth/brass_dsp.c` kompiluje oba pliki jako C; stan głosu = slot 0x24 B w instancji ścieżki (self).
Silnik czyta obwiednię głośności głosu (`OP1_VOICE_ENV` w OP-1) — u nas wartość AMP ENV głosu na początku bloku
(opóźnienie jednego bloku, 1,3 ms). Sam silnik 1:1 (decyzja Patryka): strony SOUND / AMP ENV / PRESETS; gałki TIMBRE,
SWEEP, VIBRATO, VIB RATE. Oryginał nie ma presetów — 12 własnych z gałek i obwiedni. Test bit w bit (wrapper z zastępczą
obwiednią harnessa): `tools/test_brass.sh`. Częstotliwość: przy 96 kHz wibrato i filtr wyjściowy (na próbkę) ~9% szybciej.
