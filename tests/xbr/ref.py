import base64, json, sys
import numpy as np
from playwright.sync_api import sync_playwright

frag = open('frag.glsl').read()
vert = '''
attribute vec2 pos;
uniform vec2 TextureSize;
varying vec2 TEX0; varying vec4 TEX1; varying vec4 TEX2; varying vec4 TEX3;
varying vec4 TEX4; varying vec4 TEX5; varying vec4 TEX6; varying vec4 TEX7;
void main() {
    gl_Position = vec4(pos, 0.0, 1.0);
    vec2 TexCoord = pos * 0.5 + 0.5;
    float dx = 1.0 / TextureSize.x;
    float dy = 1.0 / TextureSize.y;
    TEX0 = TexCoord; TEX0.x *= 1.00000001;
    TEX1 = TexCoord.xxxy + vec4(-dx, 0, dx, -2.0*dy);
    TEX2 = TexCoord.xxxy + vec4(-dx, 0, dx, -dy);
    TEX3 = TexCoord.xxxy + vec4(-dx, 0, dx, 0);
    TEX4 = TexCoord.xxxy + vec4(-dx, 0, dx, dy);
    TEX5 = TexCoord.xxxy + vec4(-dx, 0, dx, 2.0*dy);
    TEX6 = TexCoord.xyyy + vec4(-2.0*dx, -dy, 0, dy);
    TEX7 = TexCoord.xyyy + vec4(2.0*dx, -dy, 0, dy);
}'''
js = '''
([vert, frag, w, h, n, b64]) => {
  const bin = atob(b64); const data = new Uint8Array(bin.length);
  for (let i = 0; i < bin.length; i++) data[i] = bin.charCodeAt(i);
  const c = document.createElement('canvas'); c.width = w*n; c.height = h*n;
  const gl = c.getContext('webgl', {preserveDrawingBuffer: true, antialias: false});
  if (!gl) return 'no webgl';
  function sh(type, s) { const o = gl.createShader(type); gl.shaderSource(o, s); gl.compileShader(o);
    if (!gl.getShaderParameter(o, gl.COMPILE_STATUS)) throw gl.getShaderInfoLog(o); return o; }
  const p = gl.createProgram();
  gl.attachShader(p, sh(gl.VERTEX_SHADER, vert)); gl.attachShader(p, sh(gl.FRAGMENT_SHADER, frag));
  gl.linkProgram(p); if (!gl.getProgramParameter(p, gl.LINK_STATUS)) return gl.getProgramInfoLog(p);
  gl.useProgram(p);
  const prec = gl.getShaderPrecisionFormat(gl.FRAGMENT_SHADER, gl.HIGH_FLOAT);
  const buf = gl.createBuffer(); gl.bindBuffer(gl.ARRAY_BUFFER, buf);
  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1,-1, 1,-1, -1,1, 1,1]), gl.STATIC_DRAW);
  const loc = gl.getAttribLocation(p, 'pos'); gl.enableVertexAttribArray(loc);
  gl.vertexAttribPointer(loc, 2, gl.FLOAT, false, 0, 0);
  const tex = gl.createTexture(); gl.bindTexture(gl.TEXTURE_2D, tex);
  gl.pixelStorei(gl.UNPACK_ALIGNMENT, 1);
  gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, w, h, 0, gl.RGBA, gl.UNSIGNED_BYTE, data);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
  gl.uniform2f(gl.getUniformLocation(p, 'TextureSize'), w, h);
  gl.uniform1i(gl.getUniformLocation(p, 'decal'), 0);
  gl.viewport(0, 0, w*n, h*n); gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
  const out = new Uint8Array(w*n*h*n*4);
  gl.readPixels(0, 0, w*n, h*n, gl.RGBA, gl.UNSIGNED_BYTE, out);
  let s = ''; for (let i = 0; i < out.length; i += 8192) s += String.fromCharCode.apply(null, out.subarray(i, i+8192));
  return JSON.stringify({prec: prec.precision, renderer: gl.getParameter(gl.RENDERER), data: btoa(s)});
}'''
inp, w, h, n, outp = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4]), sys.argv[5]
a = np.fromfile(inp, dtype='<u4').reshape(h, w)
rgba = np.stack([(a >> 16) & 255, (a >> 8) & 255, a & 255, np.full_like(a, 255)], -1).astype(np.uint8)
with sync_playwright() as pw:
    br = pw.chromium.launch(args=['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'])
    page = br.new_page()
    res = page.evaluate(js, [vert, frag, w, h, n, base64.b64encode(rgba.tobytes()).decode()])
    br.close()
if not res.startswith('{'):
    sys.exit('error: ' + res)
r = json.loads(res)
print('renderer:', r['renderer'], 'highp precision bits:', r['prec'])
o = np.frombuffer(base64.b64decode(r['data']), np.uint8).reshape(h*n, w*n, 4)
v = (0xff000000 | (o[..., 0].astype(np.uint32) << 16) | (o[..., 1].astype(np.uint32) << 8) | o[..., 2]).astype('<u4')
v.tofile(outp)
