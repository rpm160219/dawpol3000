# Oxbow

OberVave-inspired polysynth adapted from zvenson’s code by Hundekraft. Preserve the original copyright, license and reported permission record in source and `../../../PROVENANCE.md`.

Each voice has two oscillators, sync/cross-modulation, noise, drive, selectable state-variable filtering, a filter envelope and per-card drift. The OP-1 supplies up to six voices and the sole T2 amplitude envelope. Normal OP-1 presets store settings; there is no internal program bank.

Primary controls are Cutoff, Resonance, Env amount and Detune. Page2 exposes Wave, Filter type and Drive in words5–7. `obv_params.h` defines defaults; `presets.py` emits explicit patch metadata.

`gen_svg.py` maps artwork and Q10 animation geometry together. Preserve circle proportions, horn/bell/bird pivots, mapped travel paths and lower-screen clearance. Do not apply an additional effect-screen translation.

Audio runs at 22.05 kHz and reconstructs the native 128-sample voice block with per-voice four-point interpolation. Native pitch decoding stays at 44.1 kHz; generated filter tables use the internal rate. Control/envelope update durations remain unchanged. Preserve these rate relationships when changing tables or interpolation.
