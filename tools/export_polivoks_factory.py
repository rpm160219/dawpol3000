#!/usr/bin/env python3
"""Export the standalone 24-preset JSON bank to the native factory table.

Usage: python3 tools/export_polivoks_factory.py /path/to/polivoks_bank.json
The first twelve entries keep factory version 1; additions use version 2.
"""
import argparse
import json
from pathlib import Path
import re

root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('bank',type=Path)
args=parser.parse_args()
bank=json.loads(args.bank.read_text())
assert (bank['param_version'],bank['param_count']) in ((2,41),(3,44))
entries=bank['presets']
assert len(entries)==24 and len({p['name'] for p in entries})==24
header=(root/'core/synth/polivoks_dsp.h').read_text()
keys=re.findall(r'^\s*(PVX_[A-Z0-9_]+)\b',header.split('enum PolivoksParam {')[1].split('};')[0],re.M)
assert len(keys)==44
if bank['param_version']==2: keys=keys[:41]
approved=re.findall(r'\{([0-9,]+)\}',(root/'tests/polivoks_approved.inc').read_text())
categories={'BASS':'CAT_BASS','SUB':'CAT_BASS','LEAD':'CAT_LEAD','DUO':'CAT_PAD','PAD':'CAT_PAD','FM':'CAT_LEAD','MOD':'CAT_FX','FX':'CAT_FX','PERC':'CAT_DRUMS','BELL':'CAT_BELL'}
rows=['// Standalone bank: approved 12 v1 entries unchanged; 12 additions in factory v2.','static const FactoryPreset kPolivoks[] = {']
for i,preset in enumerate(entries):
    params=preset['params']
    assert set(params)==set(keys) and all(type(v) is int and 0<=v<=32767 for v in params.values())
    words=[params[key] for key in keys]
    if i<12: assert words[:41]==list(map(int,approved[i].split(','))), 'Approved preset changed'
    assert re.fullmatch(r'[A-Z0-9 ]{1,31}',preset['name'])
    rows.append('    {"%s", %s, %d, "pvox %d %s"},'%(preset['name'],categories[preset['category']],1 if i<12 else 2,bank['param_version'],' '.join(map(str,words))))
rows+=['};','']
(root/'core/io/polivoks_factory.inc').write_text('\n'.join(rows))
print('Exported 24 presets, first 12 unchanged')
