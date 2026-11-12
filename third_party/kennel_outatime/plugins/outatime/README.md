# outatime

A time-travel synth, ported from opus's `opus.outatime` engine (LizardMan245/opus, commit f0d2216, GPL-3.0). Each
voice has three oscillators, the flux capacitor's arms. YEAR morphs their wave from 1955 to 2015, SPEED sets their
time drift, POWER drives a strong overdrive, and TRAIL is the ignition: each note opens a resonant filter and burns
down. The sound is upstream's `ota_dsp.c` and `ota_tables.h`. `outatime.c` is new glue for kennel's engine API.

## Controls

| Knob | Page 1 | Page 2 (press the encoder) |
|---|---|---|
| Blue | Year, 1955 to 2015: a triangle (round, organ-like), then a saw (1985), then a sine folded up to 8 times (glassy). The filter opens with the year | Spread: the outer arms detuned up to 20 cents (preset word 5) |
| Green | Speed, 0 to 88 mph: the time drift. The arms sit a third of a cycle apart on one slow modulator, up to 30 years either way and up to 1.9 Hz | Tone: darker or brighter within the year (preset word 6) |
| White | Power, 0 to 1.21 GW: overdrive from 1x to 40x, its level matched so any power sounds as loud | Flux: the ensemble, a three-phase pitch modulation up to 22 cents at 0.4 to 3 Hz (preset word 7) |
| Red | Trail: the ignition. Each note opens the filter up to three octaves and lets it burn down over 40 ms to 1.4 s, with more resonance as trail goes up | (none) |

The amp envelope, glide and play modes are the OP-1's own.

## Screen

The owner's time-machine page from upstream (art v3): the car on the road, the speedometer, the time circuits and the
flux capacitor. DEST shows the set year, PRES the first arm's year as the drift moves it, LAST the year before the last
change. The needle follows the car's speed up to the set speed, and the fire starts once it gets there. The rods glow
with power and breathe with the drift while a note plays, and current runs along the time cables. A paged knob's tab
shows its page-2 name and value.

`import_art.py` took the art out of the opus checkout (`outatime-art.svg`, `outatime-art-places.json`). `gen_svg.py`
writes `outatime.svg` (all still, drawn through the static-art cache) and `outatime_geom.h`. `outatime_screen.c` draws
the live parts with upstream's code. The page is scaled once, uniformly (0.946, about x 160 and the top edge), so the
panels' bottom edge ends at y 138 (the OLED bottom margin). Stroke widths are kept.

## What differs from upstream

- **Rate.** The tables are upstream's, made for 88.2 kHz voices with 128-sample blocks. They are not regenerated.
  On the unit the OS's voices run at 88.2 kHz (OP1-Emulator `docs/emu-audio-clock-2026-10-21.md`, and opus's
  `api.md`), so the sound matches upstream there. Kennel's host harness (`audio.py`) and the emulator's default link
  write voices out at 44.1 kHz. In those takes the drift and the ignition run at half speed and the filter sits an
  octave low.
- **The overdrive runs per voice.** Upstream drove the sum of all voices in a `block()` hook, after the OS's envelope.
  Kennel's engine API has no hook on the sum, so the same chain runs at the end of each voice's render: drive, level
  match, DC blocker, fizz lowpass, +4 dB and the soft limiter, with the velocity applied before it as upstream's OS
  did. Its block is 128 samples at 88.2 kHz, the same 1.45 ms. The fizz lowpass, the DC blocker and the followers
  get the same poles per unit of time (`ota_dsp.c`, marked `kennel:`). Effects of the move: a chord is driven note by
  note, with no intermodulation between notes, and the OS's envelope comes after the drive, so releases fade a
  driven sound. A new note resets its voice's followers, as upstream's sum did after silence.
- **Page 2 instead of shift.** Upstream's shift layer (spread, tone, flux) is kennel's page 2 on blue, green and white.
  Page-2 turns move at kennel's rate, not upstream's 3 of 99 per detent. Kennel starts a synth's page-2 words at 0
  (`page2_defaults` is FX-only in the core), so a new sound has spread 0, tone 0 (about two octaves darker than
  upstream's 50) and flux 0. Upstream's new sound is the `outatime` preset.
- **Settings.** Upstream's "always driving" (off) and "overdrive type" (warm, hard, tube, fold) have no synth-plugin
  equivalent in kennel, so they're fixed at their defaults: the car drives only while a note plays, and the drive is
  warm (tanh). The other three shapes are still in `ota_dsp.c`.
- **Voice init.** Kennel has no voice-build callback, so a voice is set up at its first note.
- **Screen.** The time circuits show the year only. Upstream also showed the OP-1's month, day and time
  (`opus_clock`), and kennel has no service for that, so those fields stay dark. The labels (DEST, PRES, LAST, MPH)
  and the tabs' names use kennel's stroke font. Upstream used the OP-1's own font, which is firmware data. The tab
  names shrink to fit the tab. Colours are kennel's fixed encoder colours, not opus themes. Upstream's kept-pixel zones
  are gone: the still art is the static-prefix cache and every live part is drawn each frame. Round line ends became
  flat ones, with segments lengthened by half their width to keep the same ink, and dots are two crossing lines. The
  trail tab stays lit on page 2.

## Presets

`presets.py` writes four sounds to `presets/`: outatime (upstream's new sound: 1985, 18 mph, 0.31 GW, trail 35;
spread 40, tone 50, flux 30), 1955 organ, 88 mph and 2015 glass. Host renders without the OP-1's envelope and effects
land between -12.6 and -18.4 dBFS RMS.

## Status

- Host renders (`harness/audio.py outatime`, sanitizers on) are clean, including every knob and page-2 word at 0,
  half and full and 30 random settings. Across each sweep the level stays within about -13 to -18 dBFS RMS.
- bcore cost (`plugin_cost.py --write`): 24,876 packets per voice call, 1,284 per voice start; six voices at
  C7..C8 about 83 % of the unit block (86 % worst with random knobs and retriggers).
- CPU (2026-10-30, branch `opt-outatime`): on the Blackfin a voice's block runs in `ota_kern.S`, four hand-scheduled
  passes with 16 x 16 multiplies only (the C loops stay in `ota_dsp.c` for the host and as the reference:
  `kern_check.py` compares the two on bcore). The knob values are cached and redone only when a knob moves. The
  voices live in the L1 synth partition (`op1_l1_claim_state`), with the tables after them. Same samples as before
  (exact_check with the pool off and on). bcore: about 82,000 packets for six voices' worst block, down from 153,000.
- Screen: about 270 to 420 raster lines a frame in the harness, and up to about 760 with a long trail of fire.
  Native draw cost is not measured.
- Not done yet: listening on the emulator and the OP-1, a native screen capture, and a native audio review.
