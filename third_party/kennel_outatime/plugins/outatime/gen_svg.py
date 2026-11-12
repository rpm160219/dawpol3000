#!/usr/bin/env python3
# Copyright (C) 2026 Hundekraft. GPL-3.0-or-later with the OP-1 firmware permission (see LICENSE and NOTICE).
"""Generate outatime.svg + outatime_geom.h: OUTATIME's screen, opus's time-machine page (the owner's art, v3).

    python3 gen_svg.py

The art is outatime-art.svg (upstream's art/outatime_t1.svg, its OP-1-font labels taken out: import_art.py) and
outatime-art-places.json (the places upstream's ota_page.c draws at, units x 256). It is a vector file on the 320 x 160
page, so it converts 1:1 (docs/screen-art.md 3a) apart from the forced changes below.

The fit (computed here, not picked): the art's lowest ink is the three panels' bottom edge (centre line y 145, width
1.2), below the OLED's bottom margin (screen-art.md: ink at y <= 138). The whole page, art and live parts alike, is
scaled once, uniformly, about (160, the top ink's centre line): S = (138 - w/2 - top) / (bottom - top) on the centre
lines, so the top stays where it is, the bottom edge's ink ends at y 138, and x stays centred. Stroke widths are kept
(scaling them would thin 1.1 px lines on the OLED); only geometry, sizes and travel scale. outatime_geom.h carries S and
its centre with upstream's places unchanged, and outatime_screen.c maps every point through the same fit.

Forced changes, each for the reason given:
  - the labels DEST, PRES, LAST and MPH were the OP-1 font's strokes (firmware data): drawn here with kennel's stroke
    font (screens/stroke_font.h) at the same top-left and cap height, wider than the original letters
  - the colours upstream sets from the encoders at run time (opus themes): the fourth encoder's placeholder #FF4240 (the
    car's red lines) -> kennel's red #FF3A5D; the second's dim #00475C (PRES's rule) -> green #00ED95 at 77/256,
    #00472C; the cables' glow #4358A1 -> blue #698EFF at 154/256, #3F5599 (the same shades as ota_page.c's)
  - curves flattened, two decimals, no turn-backs, filled shapes one contour per path (R246's subset, svgart)
"""
import json
import math
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
SDK = HERE.parents[1]
sys.path.insert(0, str(SDK / 'display'))
import svgart  # noqa: E402
from svgart import f2, shape  # noqa: E402

BOTTOM = 138.0                              # screen-art.md: the lowest ink
RECOLOUR = {'#FF4240': '#FF3A5D', '#00475C': '#00472C', '#4358A1': '#3F5599'}
LABEL_W = 1.1                               # the art's label strokes


def stroke_font():
    """kennel's stroke font: char -> [(x0, y0, x1, y1)] on its 4 x 6 grid"""
    src = (SDK / 'screens' / 'stroke_font.h').read_text()
    out = {}
    for ch, segs in re.findall(r"\{ '(.)', \d+, \{ ((?:\{[\d,]+\},?)+) \} \}", src):
        out[ch] = [tuple(map(int, s.split(','))) for s in re.findall(r'\{([\d,]+)\}', segs)]
    return out


def main():
    art = svgart.art(HERE / 'outatime-art.svg')
    places = json.loads((HERE / 'outatime-art-places.json').read_text())
    # the fit, from the centre lines and the widths of the art's own strokes
    top = min((y, w) for _, s, f, w, geo in art for pts, _ in geo for _, y in pts)
    bot = max((y, w) for _, s, f, w, geo in art for pts, _ in geo for _, y in pts)
    y0 = top[0]
    s = (BOTTOM - bot[1] / 2 - y0) / (bot[0] - y0)
    s12 = int(math.floor(s * 4096))                # the C code's S (Q12); the SVG uses the same rounded value
    s = s12 / 4096
    fit = lambda x, y: (160 + (x - 160) * s, y0 + (y - y0) * s)

    out = []
    for el, stroke, fill, width, geo in art:
        stroke = RECOLOUR.get(stroke, stroke)
        for pts, closed in geo:
            q = [fit(x, y) for x, y in pts]
            if fill and fill.upper() == '#000000':
                out.append(shape(q, stroke or svgart.BLACK, width if stroke else 0.2, True, svgart.BLACK))
            elif fill:                               # coloured fills (the trefoil, the dots): one contour a path
                out.append(shape(q, stroke or fill, width if stroke else 0.2, True, fill))
            else:
                out.append(shape(q, stroke, width, closed))
    font = stroke_font()
    for lab in places['labels']:                     # the labels, in kennel's stroke font
        x, y, _, yb = lab['box']
        u = (yb - y) / 6                              # px per grid unit: the cap height on 6 units
        for ch in lab['text']:
            for a, b, c, d in font[ch]:
                out.append(shape([fit(x + a * u, y + b * u), fit(x + c * u, y + d * u)], '#7C7990', LABEL_W))
            x += 6 * u
    svg = ('<svg xmlns="http://www.w3.org/2000/svg" width="320" height="160" viewBox="0 0 320 160">\n'
           '<g id="static">\n' + '\n'.join(out) + '\n</g>\n</svg>\n')
    (HERE / 'outatime.svg').write_text(svg)

    # the live parts' places: upstream's, unchanged (units x 256), and the fit
    h = ['/* outatime_geom.h: made by gen_svg.py (do not edit). Upstream ota_art.h\'s places (opus outatime, units x 256',
         ' * on the 320 x 160 page, GPL-3.0, PROVENANCE.md) and the fit that maps them onto the screen (gen_svg.py). */',
         '#ifndef OUTATIME_GEOM_H', '#define OUTATIME_GEOM_H',
         f'#define OTA_FIT_S12     {s12}         /* the uniform scale, Q12 ({s:.4f}) */',
         f'#define OTA_FIT_CX      {160 * 256}        /* its centre, units x 256: x 160 */',
         f'#define OTA_FIT_CY      {round(y0 * 256)}         /* y {y0:g}, the top ink\'s centre line */']
    for k, v in places.items():
        if isinstance(v, int):
            h.append(f'#define {k:<15} {v}')
    ctype = {'FX_EL': 'int', 'TAB': 'int', 'WHEEL': 'int', 'SPOKE': 'short', 'CABLE_AT': 'short', 'CABLE': 'int',
             'SEG7': 'signed char', 'MAP7': 'unsigned char'}
    dims = {'FX_EL': '[3][2]', 'TAB': '[4][3]', 'WHEEL': '[2][2]', 'SPOKE': '[2][SPOKES][3]',
            'CABLE_AT': '[CABLES + 1]', 'CABLE': '[CABLE_N][3]', 'SEG7': '[7][4]', 'MAP7': '[10]'}
    lit = lambda v: '{ ' + ', '.join(lit(x) for x in v) + ' }' if isinstance(v, list) else str(v)
    for k in ctype:
        h.append(f'static const {ctype[k]} {k}{dims[k]} = {lit(places[k])};')
    h.append('#endif')
    (HERE / 'outatime_geom.h').write_text('\n'.join(h) + '\n')
    print(f'fit S {s:.4f} about (160, {y0:g}); bottom centre line {bot[0]:g} (width {bot[1]:g}) -> '
          f'{fit(0, bot[0])[1] + bot[1] / 2:.2f}; {len(out)} shapes')


if __name__ == '__main__':
    main()
