#!/usr/bin/env python3
"""Przepisuje statyczna grafike SVG op1r (tylko <polyline> i <circle>) do naglowka C++:
   python3 tools/gen/svg_to_art.py third_party/op1r_taffy/taffy.svg TF > third_party/op1r_taffy/taffy_art.h"""
import re, sys
src, pre = sys.argv[1], sys.argv[2]
svg = open(src).read()
def col(s):
    s = s.lstrip('#'); return '0x%s' % s.upper()
lines, pts = [], []
for m in re.finditer(r'<polyline points="([^"]+)"[^>]*stroke="(#[0-9A-Fa-f]+)"[^>]*stroke-width="([0-9.]+)"', svg):
    p = [float(v) for v in m.group(1).replace(',', ' ').split()]
    lines.append((len(pts) // 2, len(p) // 2, col(m.group(2)), float(m.group(3))))
    pts += p
circles = [(float(m.group(1)), float(m.group(2)), float(m.group(3)), col(m.group(4)))
           for m in re.finditer(r'<circle cx="([0-9.]+)" cy="([0-9.]+)" r="([0-9.]+)" fill="(#[0-9A-Fa-f]+)"', svg)]
print('/* wygenerowane przez tools/gen/svg_to_art.py z %s (grafika: Hundekraft, GPL-3.0-or-later) */' % src.split('/')[-1])
print('#pragma once')
print('static const float %s_PTS[] = {%s};' % (pre, ', '.join('%.2ff' % v for v in pts)))
print('struct %sLine { int first, count; unsigned rgb; float width; };' % pre)
print('static const %sLine %s_LINES[] = {' % (pre, pre))
for f, n, c, w in lines: print('    {%d, %d, %s, %.2ff},' % (f, n, c, w))
print('};')
print('struct %sCircle { float x, y, r; unsigned rgb; };' % pre)
print('static const %sCircle %s_CIRCLES[] = {' % (pre, pre))
for x, y, r, c in circles: print('    {%.2ff, %.2ff, %.2ff, %s},' % (x, y, r, c))
print('};')
