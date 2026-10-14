# xBR reference check (development only)

Compares `XbrScaler::Scale()` with Hyllian's original xBR-lv2 GLSL shader,
run in headless Chromium (WebGL, SwiftShader). Needs Python 3, numpy and
Playwright with Chromium; runs on any development machine, not on Haiku.

1. Make the WebGL fragment shader from the libretro source
   (`xbr/shaders/xbr-lv2.glsl` in github.com/libretro/glsl-shaders):
   `python3 make_frag.py xbr-lv2.glsl > frag.glsl`
2. `python3 ref.py <in.raw> <width> <height> <factor> <out.raw>`
   (raw = little endian 0xffRRGGBB per pixel)
3. Compare with the output of `XbrScaler::Scale()` for the same input.

Result 2026-10-14 (commit with xBR): MoG scenes (320x200 and 640x400 art,
factors 2, 3, 4): 95.3 to 98.2% of the pixels identical, 99.92 to 99.99%
identical or 1 step apart; the rest are rounding ties. The unit-test pattern
(24x24, factor 3) is identical; its hash is checked in tests/unit.
