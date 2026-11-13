#!/usr/bin/env python3
"""Convert Maiko #250's custom Crush alpha layers; no TE SVG/font is read.
Project-maintainer-authorized Maiko artwork reconstruction; inputs are supplied separately.
Screen placement: translate static art up twelve logical pixels.
Usage: gen_svg.py PRIVATE/OP1_vdk/01e40000.bin [OUTPUT]
"""
import hashlib
import struct
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'display'))
from pixel_svg import pixel_svg
REGION_SHA = '09c22c233dc9a84b6e9674f0a1670f5c8f0ca8331dc819d97d4133fd58681c7a'

def generate(source, output):
    region = Path(source).read_bytes()
    if hashlib.sha256(region).hexdigest() != REGION_SHA:
        raise ValueError('wrong private Maiko #250 region')
    palette = struct.unpack_from('<8H', region, 0x60de0)
    pixels = [[(0, 0, 0) for _ in range(320)] for _ in range(160)]
    for index in range(6):
        color, x, y, width, height, pointer = struct.unpack_from('<IiiIII', region, 0x6118c + index * 24)
        if color == 0xffffffff:
            continue
        if color >= 8 or width > 320 or height > 200:
            raise ValueError('custom art bounds/format mismatch')
        value = palette[color]
        rgb = ((value >> 11) * 255 // 31, ((value >> 5) & 63) * 255 // 63, (value & 31) * 255 // 31)
        start, size = pointer - 0x01e40000, (width * height + 1) // 2
        if start < 0 or start + size > len(region):
            raise ValueError('custom art pointer outside private region')
        data = region[start:start + size]
        for row in range(height):
            for col in range(width):
                if not 0 <= y + row < 160 or not 0 <= x + col < 320:
                    continue
                n = row * width + col
                alpha = (data[n // 2] >> ((n & 1) * 4)) & 15
                old = pixels[y + row][x + col]
                pixels[y + row][x + col] = tuple((rgb[c] * alpha + old[c] * (15-alpha) + 7) // 15 for c in range(3))
    Path(output).write_text(pixel_svg(pixels,'Maiko #250 custom Crush alpha art; owner-authorized port. No TE SVG/font.',y_offset=-12))

if __name__ == '__main__':
    generate(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else Path(__file__).with_name('crush.svg'))
