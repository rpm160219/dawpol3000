"""SVG geometry for the font glyphs: the bounding box SVG's getBBox gives each top-level element with an id.
Usage: python3 font_bounds.py input.svg output.json
Replaces font_bounds.cjs (Chromium through Playwright). Pure Python, no network, no browser.

getBBox is the tight geometric box in the element's own user space: no stroke, curve extrema included, a group the
union of its children. Values come out in single precision like Chromium's (coordinates parsed as float32, width and
height as float32 differences), so prepare_stock_assets.py's header check sees the same numbers.
"""
import json
import math
import re
import struct
import sys
import xml.etree.ElementTree as ET

NUM = r'[-+]?(?:\d*\.\d+|\d+\.?\d*)(?:[eE][-+]?\d+)?'
f32 = lambda v: struct.unpack('<f', struct.pack('<f', v))[0]


def cubic_extrema(p0, p1, p2, p3):
    """t in (0, 1) where one coordinate of the cubic has a zero derivative"""
    a = -p0 + 3 * p1 - 3 * p2 + p3
    b = 2 * (p0 - 2 * p1 + p2)
    c = p1 - p0
    if abs(a) < 1e-12:
        return [-c / b] if abs(b) > 1e-12 else []
    disc = b * b - 4 * a * c
    if disc < 0:
        return []
    s = math.sqrt(disc)
    return [(-b + s) / (2 * a), (-b - s) / (2 * a)]


class Box:
    def __init__(self):
        self.pts = []

    def add(self, x, y):
        self.pts.append((f32(x), f32(y)))

    def cubic(self, a, b, c, d):
        self.add(*d)
        for j in (0, 1):
            for t in cubic_extrema(a[j], b[j], c[j], d[j]):
                if 0 < t < 1:
                    q = 1 - t
                    self.add(*(q ** 3 * a[k] + 3 * q * q * t * b[k] + 3 * q * t * t * c[k] + t ** 3 * d[k] for k in (0, 1)))

    def merge(self, other):
        self.pts += other.pts


def path_box(d, box):
    tokens = re.findall(r'[A-Za-z]|' + NUM, d)
    p = (0.0, 0.0); start = p; old_control = None; old_cmd = None; i = 0; cmd = None
    while i < len(tokens):
        if tokens[i].isalpha():
            cmd = tokens[i]; i += 1
        rel = cmd.islower(); code = cmd.upper()
        if code == 'Z':
            p = start; old_control = None; old_cmd = code; cmd = None
            continue
        n = {'M': 2, 'L': 2, 'H': 1, 'V': 1, 'C': 6, 'S': 4}.get(code)
        if n is None:
            raise ValueError('Unsupported path command ' + cmd)
        v = [f32(float(t)) for t in tokens[i:i + n]]; i += n
        pt = lambda x, y: (f32(p[0] + x), f32(p[1] + y)) if rel else (x, y)
        if code in ('M', 'L'):
            q = pt(*v)
            if code == 'M':
                start = q; cmd = 'l' if rel else 'L'
            box.add(*q); p = q; old_control = None
        elif code == 'H':
            q = (f32(p[0] + v[0]) if rel else v[0], p[1]); box.add(*q); p = q; old_control = None
        elif code == 'V':
            q = (p[0], f32(p[1] + v[0]) if rel else v[0]); box.add(*q); p = q; old_control = None
        elif code == 'C':
            b, c, q = pt(*v[:2]), pt(*v[2:4]), pt(*v[4:6]); box.add(*p); box.cubic(p, b, c, q); p = q; old_control = c
        else:
            b = tuple(f32(2 * p[j] - old_control[j]) for j in (0, 1)) if old_cmd in ('C', 'S') else p
            c, q = pt(*v[:2]), pt(*v[2:4]); box.add(*p); box.cubic(p, b, c, q); p = q; old_control = c
        old_cmd = code


def element_box(n):
    tag = n.tag.rsplit('}', 1)[-1]
    if n.get('transform') is not None:
        raise ValueError('transform on ' + tag)
    g = lambda k: f32(float(n.get(k, 0)))
    box = Box()
    if tag == 'g':
        for c in n:
            box.merge(element_box(c))
    elif tag == 'path':
        path_box(n.get('d', ''), box)
    elif tag == 'line':
        box.add(g('x1'), g('y1')); box.add(g('x2'), g('y2'))
    elif tag in ('polyline', 'polygon'):
        v = [f32(float(t)) for t in re.findall(NUM, n.get('points', ''))]
        for x, y in zip(v[::2], v[1::2]):
            box.add(x, y)
    elif tag == 'rect':
        x, y = g('x'), g('y'); box.add(x, y); box.add(f32(x + g('width')), f32(y + g('height')))
    elif tag in ('circle', 'ellipse'):
        cx, cy = g('cx'), g('cy')
        rx = g('r') if tag == 'circle' else g('rx'); ry = g('r') if tag == 'circle' else g('ry')
        box.add(cx - rx, cy - ry); box.add(cx + rx, cy + ry)
    else:
        raise ValueError('Unsupported element ' + tag)
    return box


def bbox(n):
    pts = element_box(n).pts
    if not pts:
        return {'x': 0, 'y': 0, 'width': 0, 'height': 0}
    x0 = min(x for x, _ in pts); x1 = max(x for x, _ in pts)
    y0 = min(y for _, y in pts); y1 = max(y for _, y in pts)
    return {'x': x0, 'y': y0, 'width': f32(x1 - x0), 'height': f32(y1 - y0)}


def bounds(svg_text):
    root = ET.fromstring(svg_text)
    return {n.get('id'): bbox(n) for n in root if n.get('id')}


if __name__ == '__main__':
    if len(sys.argv) != 3:
        raise SystemExit('input.svg and output.json required')
    with open(sys.argv[1], 'rb') as f:
        out = bounds(f.read())
    with open(sys.argv[2], 'w') as f:
        json.dump(out, f)
