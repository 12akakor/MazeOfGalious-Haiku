#!/usr/bin/env python3
"""Writes an HVIF file as an rdef vector_icon resource.
usage: hvif2rdef.py build/haiku/icon.hvif > build/haiku/icon.rdef

The icon source is build/haiku/icon.svg (own design). Convert it with
icon2icon from https://github.com/threedeyes/hvif-tools:
    icon2icon build/haiku/icon.svg build/haiku/icon.hvif -f hvif"""
import sys

data = open(sys.argv[1], 'rb').read()
if data[:4] != b'ncif':
    sys.exit('not an HVIF file')
hexdata = data.hex().upper()
print('resource vector_icon array {')
for i in range(0, len(hexdata), 64):
    print('\t$"%s"' % hexdata[i:i + 64])
print('};')
