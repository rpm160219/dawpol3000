#!/usr/bin/env python3
# Copyright (C) 2026 Hundekraft. GPL-3.0-or-later with the OP-1 firmware permission (see LICENSE and NOTICE).
"""Take outatime's T1 art out of an opus checkout (LizardMan245/opus f0d2216, GPL-3.0), once:

    python3 -I import_art.py OPUS/src/plugin-kit/examples/opus.outatime

writes, next to this script:
  outatime-art.svg          upstream's art/outatime_t1.svg (the owner's time-machine page, v3, as page.js draws it),
                            unchanged except that its label letters are taken out: the grey (#7C7990) square-capped
                            paths spelling DEST, PRES, LAST and MPH are strokes of the OP-1's own font (opfont.svg,
                            via upstream's art/opfont.json), firmware data kennel must not carry. gen_svg.py writes the
                            labels again with kennel's stroke font, at the places recorded here.
  outatime-art-places.json  the places upstream's ota_page.c draws at (ota_art.h, page.js's output: units x 256 on the
                            320 x 160 page), every #define and table there except the art string, the letters (GLYPH*,
                            opfont strokes again) and the fourteen-segment tables (only the date's letters used them),
                            plus the removed labels' words and boxes (screen px, the strokes' centre lines).
Only reads the files (no upstream script runs).
"""
import json
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

HERE = Path(__file__).resolve().parent
NUM = re.compile(r'-?\d+(?:\.\d+)?')


def places(header):
    out = {}
    for name, val in re.findall(r'^#define\s+(\w+)\s+(-?\d+)u?\b', header, re.M):
        if not name.startswith('GLYPH') and name != 'OTA_ART_H':
            out[name] = int(val)
    for name, body in re.findall(r'static const [\w ]+?(\w+)(?:\[[^\]]*\])+ = \{(.*?)\};', header, re.S):
        if name.startswith('GLYPH') or name in ('SEG14', 'MAP14_DIGIT', 'MAP14_ALPHA'):
            continue
        body = re.sub(r'/\*.*?\*/', '', body, flags=re.S)
        body = re.sub(r'\s', '', body).replace('{', '[').replace('}', ']').replace(',]', ']').rstrip(',')
        out[name] = json.loads('[' + body + ']')
    return out


def main():
    src = Path(sys.argv[1])
    svg = (src / 'art' / 'outatime_t1.svg').read_text(encoding='utf-8')
    words = []
    def strip(m):
        el = m.group(0)
        if 'stroke="#7C7990"' in el and 'stroke-linecap="square"' in el:
            d = re.search(r' d="([^"]*)"', el).group(1)
            v = [float(x) for x in NUM.findall(d)]
            words.append((min(v[0::2]), min(v[1::2]), max(v[0::2]), max(v[1::2])))
            return ''
        return el
    out = re.sub(r'<path [^>]*/>', strip, svg)
    assert len(words) == 22, len(words)
    ET.fromstring(out)                                     # still well formed
    # the strokes' boxes into words: boxes on one line of text (their y ranges overlap) less than 2.5 px apart
    merged = list(words)
    near = lambda a, b: a[1] <= b[3] + 0.6 and b[1] <= a[3] + 0.6 and a[0] <= b[2] + 2.5 and b[0] <= a[2] + 2.5
    joined = True
    while joined:
        joined = False
        for i in range(len(merged)):
            for j in range(i + 1, len(merged)):
                a, b = merged[i], merged[j]
                if near(a, b):
                    merged[i] = (min(a[0], b[0]), min(a[1], b[1]), max(a[2], b[2]), max(a[3], b[3]))
                    del merged[j]
                    joined = True
                    break
            if joined:
                break
    assert len(merged) == 4, merged
    mph = [b for b in merged if b[0] < 70]
    rows = sorted((b for b in merged if b[0] >= 70), key=lambda b: b[1])
    labels = [dict(text=t, box=[round(v, 3) for v in b]) for t, b in zip(('MPH', 'DEST', 'PRES', 'LAST'), mph + rows)]
    (HERE / 'outatime-art.svg').write_text(out, encoding='utf-8')
    p = places((src / 'ota_art.h').read_text(encoding='utf-8'))
    p['labels'] = labels
    (HERE / 'outatime-art-places.json').write_text(json.dumps(p, indent=1) + '\n', encoding='utf-8')
    print('labels', labels)
    print('places', sorted(p))


if __name__ == '__main__':
    main()
