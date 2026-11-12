#!/usr/bin/env python3
"""Przepisuje statyczna grafike SVG op1r (path M/L/Z, line, polyline, circle; grupy <g id="...">) do naglowka C++.
   Kreski: PRE_STROKES {pierwszy punkt, liczba, kolor, grubosc, zamknieta, grupa}; wypelnienia (even-odd):
   PRE_FILLS {pierwszy, liczba, kolor, grupa}; okregi: PRE_CIRCLES {x, y, r, kolor, wypelniony, grubosc, grupa}.
   grupa = indeks w liscie podanych identyfikatorow (-1 = statyczne); PRE_ORDER = kolejnosc elementow z dokumentu.
   python3 tools/gen/svg_to_art2.py third_party/op1r_pew/pew.svg PW note0,note1,note2 > third_party/op1r_pew/pew_art.h"""
import re, sys
src, pre = sys.argv[1], sys.argv[2]
groups = sys.argv[3].split(',') if len(sys.argv) > 3 else []
svg = open(src).read()
def col(s): return '0x%s' % s.lstrip('#').upper()
def attr(tag, name, d=None):
    m = re.search(r'\b%s="([^"]*)"' % name, tag)
    return m.group(1) if m else d
pts, strokes, fills, circles, order = [], [], [], [], []
group = -1
def add_poly(p, tag, closed):
    fill, stroke = attr(tag, 'fill', 'none'), attr(tag, 'stroke', 'none')
    first = len(pts) // 2
    pts.extend(p)
    if fill != 'none':
        order.append((0, len(fills)))
        fills.append((first, len(p) // 2, col(fill), group))
    if stroke != 'none':
        order.append((1, len(strokes)))
        strokes.append((first, len(p) // 2, col(stroke), float(attr(tag, 'stroke-width', '1')), int(closed), group))
for m in re.finditer(r'<(/?)(\w+)([^>]*)>', svg):
    close, name, rest = m.group(1), m.group(2), m.group(3)
    tag = m.group(0)
    if name == 'g':
        if close: group = -1
        else:
            gid = attr(tag, 'id')
            group = groups.index(gid) if gid in groups else -1
        continue
    if close: continue
    gsave = group
    eid = attr(tag, 'id')
    if eid in groups: group = groups.index(eid)  # element z wlasnym id (np. <line id="bar0">) = osobna grupa
    if name == 'path':
        d = attr(tag, 'd')
        for sub in re.findall(r'M[^M]*', d):
            closed = 'Z' in sub or 'z' in sub
            if 'C' in sub:  # krzywe Beziera (bezwzgledne M/L/C/Z): splaszczone na 8 odcinkow
                toks = re.findall(r'[MLCZz]|-?[0-9.]+', sub)
                out, i, cmd, cur = [], 0, 'M', (0.0, 0.0)
                while i < len(toks):
                    t = toks[i]
                    if t in 'MLCZz': cmd = t; i += 1; continue
                    if cmd in 'ML':
                        cur = (float(toks[i]), float(toks[i + 1])); out += cur; i += 2
                    elif cmd == 'C':
                        c1 = (float(toks[i]), float(toks[i + 1])); c2 = (float(toks[i + 2]), float(toks[i + 3]))
                        e = (float(toks[i + 4]), float(toks[i + 5])); i += 6
                        for k in range(1, 9):
                            u = k / 8.0; a = (1 - u) ** 3; b = 3 * u * (1 - u) ** 2; c = 3 * u * u * (1 - u); dd = u ** 3
                            out += (a * cur[0] + b * c1[0] + c * c2[0] + dd * e[0], a * cur[1] + b * c1[1] + c * c2[1] + dd * e[1])
                        cur = e
                    else: i += 1
                nums = out
            else:
                nums = [float(v) for v in re.findall(r'-?[0-9.]+', sub)]
            add_poly(nums, tag, closed)
    elif name == 'polyline':
        add_poly([float(v) for v in attr(tag, 'points').replace(',', ' ').split()], tag, False)
    elif name == 'line':
        add_poly([float(attr(tag, k)) for k in ('x1', 'y1', 'x2', 'y2')], tag, False)
    elif name == 'circle':
        fill, stroke = attr(tag, 'fill', 'none'), attr(tag, 'stroke', 'none')
        cx, cy, r, w = float(attr(tag, 'cx')), float(attr(tag, 'cy')), float(attr(tag, 'r')), float(attr(tag, 'stroke-width', '1'))
        if fill != 'none':  # wypelnienie, a na nim obrys (gdy jest oba)
            order.append((2, len(circles)))
            circles.append((cx, cy, r, col(fill), 1, w, group))
        if stroke != 'none':
            order.append((2, len(circles)))
            circles.append((cx, cy, r, col(stroke), 0, w, group))
    group = gsave
print('/* wygenerowane przez tools/gen/svg_to_art2.py z %s (grafika: autorzy op1r, GPL-3.0-or-later) */' % src.split('/')[-1])
print('#pragma once')
print('static const float %s_PTS[] = {%s};' % (pre, ', '.join('%.2ff' % v for v in pts)))
print('struct %sStroke { int first, count; unsigned rgb; float width; int closed, group; };' % pre)
print('static const %sStroke %s_STROKES[] = {' % (pre, pre))
for s in strokes: print('    {%d, %d, %s, %.2ff, %d, %d},' % s)
if not strokes: print('    {0, 0, 0x000000, 0, 0, -2},  /* pusty wpis */')
print('};')
print('struct %sFill { int first, count; unsigned rgb; int group; };' % pre)
print('static const %sFill %s_FILLS[] = {' % (pre, pre))
for f in fills: print('    {%d, %d, %s, %d},' % f)
if not fills: print('    {0, 0, 0x000000, -2},  /* pusty wpis (brak elementow; poza kolejnoscia rysowania) */')
print('};')
print('struct %sCircle { float x, y, r; unsigned rgb; int filled; float width; int group; };' % pre)
print('static const %sCircle %s_CIRCLES[] = {' % (pre, pre))
for c in circles: print('    {%.2ff, %.2ff, %.2ff, %s, %d, %.2ff, %d},' % c)
if not circles: print('    {0, 0, 0, 0x000000, 0, 0, -2},  /* pusty wpis (brak elementow; poza kolejnoscia rysowania) */')
print('};')
print('/* kolejnosc rysowania jak w dokumencie: rodzaj (0 wypelnienie, 1 kreska, 2 okrag), indeks w swojej tablicy */')
print('struct %sItem { int kind, idx; };' % pre)
print('static const %sItem %s_ORDER[] = {' % (pre, pre))
for k, i in order: print('    {%d, %d},' % (k, i))
print('};')
