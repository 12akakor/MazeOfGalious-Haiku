#!/usr/bin/env python3
"""Turns libretro's xbr-lv2.glsl into a GLSL ES 1.00 fragment shader for
ref.py (default parameters, small_details branches removed)."""
import sys
src = open(sys.argv[1]).read()
frag = src[src.index('#elif defined(FRAGMENT)') + len('#elif defined(FRAGMENT)'):src.rindex('#endif')]
i = frag.index('\tif (small_details < 0.5)\n\t{\n\t\ti4'); j = frag.index('    f4 = h5.yzwx;')
frag = frag[:i] + '''\t\ti4 = vec4(dot(I4,rgbw), dot(C1,rgbw), dot(A0,rgbw), dot(G5,rgbw));
\t\ti5 = vec4(dot(I5,rgbw), dot(C4,rgbw), dot(A1,rgbw), dot(G0,rgbw));
\t\th5 = vec4(dot(H5,rgbw), dot(F4,rgbw), dot(B1,rgbw), dot(D0,rgbw));
''' + frag[j:]
i = frag.index('\tif (small_details < 0.5)\n\t{\n\t\twd1'); j = frag.index('    edri  = step')
frag = frag[:i] + '''\t\twd1 = wd( e, c,  g, i, h5, f4, h, f);
\t\twd2 = wd( h, d, i5, f, i4,  b, e, i);
''' + frag[j:]
frag = frag.replace('#define PRECISION mediump', '#define PRECISION highp')
header = ''.join('#define %s\n' % d for d in [
    'XBR_SCALE 3.0', 'lv2_cf XBR_LV2_COEFFICIENT', 'CORNER_C', 'SMOOTH_TIPS',
    'texCoord TEX0', 't1 TEX1', 't2 TEX2', 't3 TEX3', 't4 TEX4', 't5 TEX5',
    't6 TEX6', 't7 TEX7'])
sys.stdout.write(header + frag)
