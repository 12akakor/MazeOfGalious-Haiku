#!/usr/bin/env python3
"""Writes the HVIF vector icon (own design: shield and sword) as an rdef
resource. Format: see src/libs/icon/flat_icon in the Haiku/RenkuOS sources.
usage: make_icon.py > build/haiku/icon.rdef"""

STYLES = [  # solid colours, no alpha (type 3): r, g, b
    (0x1a, 0x3a, 0x9c), (0x4a, 0x7a, 0xe0), (0xf0, 0xc0, 0x30),
    (0xdc, 0xdc, 0xe6), (0x60, 0x30, 0x10)]
PATHS = [
    [(12, 6), (52, 6), (52, 30), (32, 60), (12, 30)],          # shield
    [(17, 11), (47, 11), (47, 29), (32, 53), (17, 29)],        # inner
    [(12, 49), (15, 52), (52, 15), (57, 7), (49, 12)],          # blade
    [(10, 40), (14, 36), (27, 49), (23, 53)],                   # cross guard
    [(14, 45), (18, 49), (10, 57), (6, 53)],                    # grip
    [(5, 54), (9, 58), (6, 61), (2, 57)],                       # pommel
]
SHAPES = [(0, [0]), (1, [1]), (3, [2]), (4, [4]), (2, [3]), (2, [5])]


def coord(v):
    if float(v).is_integer() and -32 <= v <= 95:
        return bytes([int(v) + 32])
    raw = int(round((v + 128.0) * 102.0))
    return bytes([0x80 | (raw >> 8), raw & 0xff])


data = bytearray(b'ncif')
data.append(len(STYLES))
for r, g, b in STYLES:
    data += bytes([3, r, g, b])
data.append(len(PATHS))
for points in PATHS:
    data += bytes([2 | 8, len(points)])           # closed, no curves
    for x, y in points:
        data += coord(x) + coord(y)
data.append(len(SHAPES))
for style, paths in SHAPES:
    data += bytes([10, style, len(paths)]) + bytes(paths) + bytes([0])

hexdata = data.hex()
lines = [hexdata[i:i + 64] for i in range(0, len(hexdata), 64)]
print('resource vector_icon array {')
for line in lines:
    print('\t$"%s"' % line.upper())
print('};')
