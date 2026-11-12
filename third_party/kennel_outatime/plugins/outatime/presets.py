#!/usr/bin/env python3
"""Four OUTATIME sound presets as OP-1 patch files: python3 synth_sdk/plugins/outatime/presets.py [OUT_DIR]
-> OUT_DIR/*.aif (default: presets/ next to this script). Uses the sibling MBIRA writer for the common OP-1 patch format
and range checks; the type name is "outatime". Copy the folder to the OP-1's synth/ folder in disk mode (Shift + COM,
T3), e.g. as synth/outatime/.
"outatime" is upstream's new sound (opus outatime's .knobs: 1985, 18 mph, 0.31 GW, trail 35; spread 40, tone 50,
flux 30): kennel starts a synth's page-2 words at 0, so this preset is how to get upstream's whole default.
Copyright (C) 2026 Hundekraft. GPL-3.0-or-later with the OP-1 firmware permission (see LICENSE and NOTICE).
"""
from pathlib import Path
import importlib.util
import sys

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('kennel_patch_writer', HERE.parent / 'mbira/presets.py')
writer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(writer)

# name: knobs (YEAR, SPEED, POWER, TRAIL), page-2 words (SPREAD, TONE, FLUX), ADSR, FX, LFO, octave
PRESETS = [
    ('outatime', [16549, 6620, 8274, 11584], [13239, 16549, 9929], [64, 9000, 28000, 5000], None, None, 0),
    ('1955 organ', [0, 2000, 0, 3000], [9000, 18000, 16000], [300, 9000, 30000, 6000],
     ('spring', [12000, 14000, 3000, 9000]), None, 0),
    ('88 mph', [16549, 32767, 20000, 26000], [16000, 20000, 12000], [64, 9000, 26000, 4000],
     ('delay', [7000, 14000, 6000, 9000]), None, 0),
    ('2015 glass', [32767, 9000, 4000, 20000], [6000, 12000, 20000], [64, 12000, 20000, 9000],
     ('delay', [9000, 16000, 8000, 12000]), None, 0),
]


def meta(p):
    out = writer.meta(*p)
    out['type'] = 'outatime'
    return out


def main():
    if len(sys.argv) > 2:
        sys.exit(__doc__)
    out = Path(sys.argv[1]) if len(sys.argv) == 2 else HERE / 'presets'
    out.mkdir(parents=True, exist_ok=True)
    for p in PRESETS:
        (out / (p[0] + '.aif')).write_bytes(writer.aifc(meta(p)))
    print(f'{len(PRESETS)} OUTATIME presets in {out}')


if __name__ == '__main__':
    main()
