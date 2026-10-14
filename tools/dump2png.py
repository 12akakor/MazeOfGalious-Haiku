#!/usr/bin/env python3
"""Convert framebuffer dumps (frame_N.raw + .pal) to PNG, or a contact sheet.
usage: dump2png.py <dumpdir> <out.png> [cols]"""
import sys, glob, os
from PIL import Image, ImageDraw
d, out = sys.argv[1], sys.argv[2]
cols = int(sys.argv[3]) if len(sys.argv) > 3 else 5
frames = sorted(glob.glob(os.path.join(d, 'frame_*.raw')))
imgs = []
for f in frames:
    raw = open(f, 'rb').read(); pal = open(f[:-4] + '.pal', 'rb').read()
    im = Image.frombytes('P', (640, 400), raw); im.putpalette(pal)
    imgs.append((os.path.basename(f)[6:11], im.convert('RGB')))
w, h = 320, 200
rows = (len(imgs) + cols - 1) // cols
sheet = Image.new('RGB', (cols * w, rows * (h + 14)), (30, 30, 30))
dr = ImageDraw.Draw(sheet)
for i, (n, im) in enumerate(imgs):
    x, y = (i % cols) * w, (i // cols) * (h + 14)
    sheet.paste(im.resize((w, h)), (x, y + 14)); dr.text((x + 2, y), n, fill=(255, 255, 0))
sheet.save(out)
