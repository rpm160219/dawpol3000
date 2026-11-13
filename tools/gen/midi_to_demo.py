#!/usr/bin/env python3
"""MIDI (Mutopia, domena publiczna) -> core/seq/demo_etude_data.h: nuty pierwszych N taktow w siatce DAW-POL (PPQ 96).
Uzycie: python3 tools/gen/midi_to_demo.py plik.mid takty wyjscie.h"""
import struct, sys

def vlq(d, i):
    v = 0
    while True:
        b = d[i]; i += 1
        v = (v << 7) | (b & 0x7f)
        if b < 0x80: return v, i

def parse(path):
    d = open(path, 'rb').read()
    assert d[:4] == b'MThd'
    fmt, ntr, div = struct.unpack('>HHH', d[8:14])
    i = 14; tracks = []
    for t in range(ntr):
        assert d[i:i+4] == b'MTrk'
        ln = struct.unpack('>I', d[i+4:i+8])[0]; j = i + 8; end = j + ln; i = end
        tick = 0; run = 0; on = {}; notes = []; tempo = None
        while j < end:
            dt, j = vlq(d, j); tick += dt
            st = d[j]
            if st == 0xff:
                typ = d[j+1]; l, j2 = vlq(d, j+2)
                if typ == 0x51: tempo = (d[j2] << 16) | (d[j2+1] << 8) | d[j2+2]
                j = j2 + l; continue
            if st in (0xf0, 0xf7):
                l, j2 = vlq(d, j+1); j = j2 + l; continue
            if st & 0x80: run = st; j += 1
            ev = run & 0xf0
            if ev in (0x80, 0x90):
                n, v = d[j], d[j+1]; j += 2
                if ev == 0x90 and v > 0: on.setdefault(n, []).append((tick, v))
                elif on.get(n):
                    s, vv = on[n].pop(0); notes.append((s, tick - s, n, vv))
            elif ev in (0xc0, 0xd0): j += 1
            else: j += 2
        tracks.append(notes)
    return div, tracks

div, tracks = parse(sys.argv[1])
bars = int(sys.argv[2])
q = div // 96  # 384 -> 96
lim = bars * 4 * div
hands = [t for t in tracks if t]
out = ['// Wygenerowane przez tools/gen/midi_to_demo.py z third_party/mutopia_chopin_op10_12/etude.mid',
       '// (Chopin, Etiuda c-moll op. 10 nr 12, Mutopia Project nr 743, domena publiczna). Takty 1-%d.' % bars,
       '#pragma once', '#include <stdint.h>',
       'struct EtudeNote { uint16_t tick; uint8_t len16, pitch, vel; };  // tick w PPQ 96 od poczatku utworu; len w tickach/2',
       ]
for h, notes in enumerate(hands):
    sel = sorted(x for x in notes if x[0] < lim)
    rows = []
    for s, l, n, v in sel:
        t = s // q; L = max(1, min(510, l // q)) // 2 or 1
        rows.append('{%d,%d,%d,%d}' % (t, L, n, v))
    out.append('static const EtudeNote kEtude%s[] = {' % ('RH' if h == 0 else 'LH'))
    for k in range(0, len(rows), 12): out.append('    ' + ','.join(rows[k:k+12]) + ',')
    out.append('};')
    print('reka', h, 'nut', len(rows), 'zakres', min(x[2] for x in sel), max(x[2] for x in sel), file=sys.stderr)
open(sys.argv[3], 'w').write('\n'.join(out) + '\n')
