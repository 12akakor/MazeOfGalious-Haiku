/*
 * Scalers, see video_scaler.h.
 *
 * XbrScaler::Scale() is a C++ port of Hyllian's xBR-lv2 shader
 * (libretro glsl-shaders, xbr/shaders/xbr-lv2.glsl, MIT licence, see
 * src/third_party/LICENSE.xbr.txt), with the default parameters of that
 * shader: CORNER_C, SMOOTH_TIPS, XBR_SCALE 3 (edge width), EQ threshold 15,
 * LV2 coefficient 2, small details off.
 *
 * The shader evaluates everything per output pixel. Here the edge rules,
 * which only depend on the input pixel and its neighbours, are computed once
 * per input pixel; the per-subpixel line inequations come from a table.
 */
#include "video_scaler.h"

#include <algorithm>
#include <math.h>
#include <string.h>
#include <thread>


namespace {

/* Runs body(begin, end) over [0, count) on a few threads. */
template<typename Body>
void
parallel_rows(int count, Body body)
{
	unsigned int cores = std::thread::hardware_concurrency();
	int threads = cores == 0 ? 2 : (int)std::min(cores, 4u);
	if (threads <= 1 || count < 64) {
		body(0, count);
		return;
	}
	std::thread workers[4];
	int chunk = (count + threads - 1) / threads;
	for (int t = 1; t < threads; t++) {
		int begin = t * chunk;
		int end = std::min(count, begin + chunk);
		if (begin < end)
			workers[t] = std::thread(body, begin, end);
	}
	body(0, std::min(count, chunk));
	for (int t = 1; t < threads; t++) {
		if (workers[t].joinable())
			workers[t].join();
	}
}


inline void
clear_rows(uint32_t* dst, int dstWidth, int stride, int fromRow, int toRow)
{
	for (int y = fromRow; y < toRow; y++)
		memset(dst + (size_t)y * stride, 0, sizeof(uint32_t) * (size_t)dstWidth);
}


/* Black borders around the rectangle (rx, ry, rw, rh). */
void
clear_borders(uint32_t* dst, int dstWidth, int dstHeight, int stride, int rx,
	int ry, int rw, int rh)
{
	clear_rows(dst, dstWidth, stride, 0, ry);
	clear_rows(dst, dstWidth, stride, ry + rh, dstHeight);
	for (int y = ry; y < ry + rh; y++) {
		uint32_t* row = dst + (size_t)y * stride;
		memset(row, 0, sizeof(uint32_t) * (size_t)rx);
		memset(row + rx + rw, 0,
			sizeof(uint32_t) * (size_t)(dstWidth - rx - rw));
	}
}


inline uint32_t
lerp_pixel(uint32_t a, uint32_t b, int t /* 0..256 */)
{
	uint32_t rb = (((a & 0xff00ff) * (256 - t) + (b & 0xff00ff) * t) >> 8)
		& 0xff00ff;
	uint32_t g = (((a & 0x00ff00) * (256 - t) + (b & 0x00ff00) * t) >> 8)
		& 0x00ff00;
	return 0xff000000u | rb | g;
}


/* Bilinear resample of src (sw x sh) into the rectangle at dst. 'sharpness'
   > 1 makes it sharp bilinear: pixels keep a flat centre of that scale. */
void
resample(const uint32_t* src, int sw, int sh, uint32_t* dst, int stride,
	int rx, int ry, int rw, int rh, bool sharp)
{
	/* Per column / row: first source index and weight (0..256). */
	std::vector<int> colIndex(rw), colWeight(rw), rowIndex(rh), rowWeight(rh);
	double scaleX = (double)rw / sw;
	double scaleY = (double)rh / sh;
	for (int pass = 0; pass < 2; pass++) {
		int count = pass == 0 ? rw : rh;
		int size = pass == 0 ? sw : sh;
		double scale = pass == 0 ? scaleX : scaleY;
		std::vector<int>& index = pass == 0 ? colIndex : rowIndex;
		std::vector<int>& weight = pass == 0 ? colWeight : rowWeight;
		for (int i = 0; i < count; i++) {
			double texel = (i + 0.5) / scale;
			double pos;
			if (sharp && scale > 1.0) {
				/* libretro sharp-bilinear-simple */
				double floored = floor(texel);
				double s = texel - floored;
				double range = 0.5 - 0.5 / scale;
				double dist = s - 0.5;
				double clamped = std::min(std::max(dist, -range), range);
				pos = floored + (dist - clamped) * scale + 0.5;
			} else
				pos = texel;
			double q = pos - 0.5;
			int i0 = (int)floor(q);
			double t = q - i0;
			if (i0 < 0) {
				i0 = 0;
				t = 0;
			}
			if (i0 >= size - 1) {
				i0 = size - 1;
				t = 0;
			}
			index[i] = i0;
			weight[i] = (int)(t * 256.0 + 0.5);
		}
	}

	parallel_rows(rh, [&](int begin, int end) {
		for (int y = begin; y < end; y++) {
			int sy = rowIndex[y];
			int wy = rowWeight[y];
			const uint32_t* r0 = src + (size_t)sy * sw;
			const uint32_t* r1 = sy + 1 < sh ? r0 + sw : r0;
			uint32_t* out = dst + (size_t)(ry + y) * stride + rx;
			for (int x = 0; x < rw; x++) {
				int sx = colIndex[x];
				int wx = colWeight[x];
				int sx1 = sx + 1 < sw ? sx + 1 : sx;
				uint32_t top = lerp_pixel(r0[sx], r0[sx1], wx);
				uint32_t bottom = lerp_pixel(r1[sx], r1[sx1], wx);
				out[x] = lerp_pixel(top, bottom, wy);
			}
		}
	});
}


/* Copies every 'factor'-th pixel (undoes pixel doubling). */
const uint32_t*
art_image(const uint32_t* src, int srcWidth, int srcHeight, int factor,
	std::vector<uint32_t>& buffer, int* width, int* height)
{
	if (factor < 2) {
		*width = srcWidth;
		*height = srcHeight;
		return src;
	}
	int w = srcWidth / factor;
	int h = srcHeight / factor;
	buffer.resize((size_t)w * h);
	for (int y = 0; y < h; y++) {
		const uint32_t* in = src + (size_t)y * factor * srcWidth;
		uint32_t* out = buffer.data() + (size_t)y * w;
		for (int x = 0; x < w; x++)
			out[x] = in[x * factor];
	}
	*width = w;
	*height = h;
	return buffer.data();
}


/* xBR-lv2 ------------------------------------------------------------------ */

const float kAo[4] = { 1.0f, -1.0f, -1.0f, 1.0f };
const float kBo[4] = { 1.0f, 1.0f, -1.0f, -1.0f };
const float kCo[4] = { 1.5f, 0.5f, -0.5f, 0.5f };
const float kAx[4] = { 1.0f, -1.0f, -1.0f, 1.0f };
const float kBx[4] = { 0.5f, 2.0f, -0.5f, -2.0f };
const float kCx[4] = { 1.0f, 1.0f, -0.5f, 0.0f };
const float kAy[4] = { 1.0f, -1.0f, -1.0f, 1.0f };
const float kBy[4] = { 2.0f, 0.5f, -2.0f, -0.5f };
const float kCy[4] = { 2.0f, 0.0f, -1.0f, 0.5f };
const float kCi = 0.25f;
const float kXbrScale = 3.0f;
const float kEqThreshold = 15.0f;
const float kLv2Coefficient = 2.0f;

struct SubpixelWeights {
	float	f45i[4];
	float	f45[4];
	float	f30[4];
	float	f60[4];
};


inline float
clamp01(float value)
{
	return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
}


void
subpixel_weights(int factor, std::vector<SubpixelWeights>& table)
{
	table.resize((size_t)factor * factor);
	const float delta = 1.0f / kXbrScale;
	const float deltaL[4] = { 0.5f / kXbrScale, 1.0f / kXbrScale,
		0.5f / kXbrScale, 1.0f / kXbrScale };
	const float deltaU[4] = { deltaL[1], deltaL[0], deltaL[3], deltaL[2] };
	for (int sy = 0; sy < factor; sy++) {
		for (int sx = 0; sx < factor; sx++) {
			float fpx = (sx + 0.5f) / factor;
			float fpy = (sy + 0.5f) / factor;
			SubpixelWeights& w = table[(size_t)sy * factor + sx];
			for (int k = 0; k < 4; k++) {
				float fx = kAo[k] * fpy + kBo[k] * fpx;
				float fxl = kAx[k] * fpy + kBx[k] * fpx;
				float fxu = kAy[k] * fpy + kBy[k] * fpx;
				w.f45i[k] = clamp01((fx + delta - kCo[k] - kCi) / (2 * delta));
				w.f45[k] = clamp01((fx + delta - kCo[k]) / (2 * delta));
				w.f30[k] = clamp01((fxl + deltaL[k] - kCx[k]) / (2 * deltaL[k]));
				w.f60[k] = clamp01((fxu + deltaU[k] - kCy[k]) / (2 * deltaU[k]));
			}
		}
	}
}


struct Rgb {
	float	r, g, b;
};


inline Rgb
to_rgb(uint32_t c)
{
	Rgb out = { (float)((c >> 16) & 0xff), (float)((c >> 8) & 0xff),
		(float)(c & 0xff) };
	return out;
}


inline Rgb
mix(const Rgb& a, const Rgb& b, float t)
{
	Rgb out = { a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t,
		a.b + (b.b - a.b) * t };
	return out;
}


inline float
color_distance(const Rgb& a, const Rgb& b)
{
	return fabsf(a.r - b.r) + fabsf(a.g - b.g) + fabsf(a.b - b.b);
}


inline uint32_t
to_pixel(const Rgb& c)
{
	int r = (int)(c.r + 0.5f);
	int g = (int)(c.g + 0.5f);
	int b = (int)(c.b + 0.5f);
	return 0xff000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

}	// namespace


void
XbrScaler::Scale(const uint32_t* in, int width, int height, int factor,
	uint32_t* out)
{
	if (factor < 1)
		factor = 1;
	const int outWidth = width * factor;

	/* Luma as in the shader: dot(rgb / 255, (14.352, 28.176, 5.472)). */
	std::vector<float> luma((size_t)width * height);
	for (size_t i = 0; i < luma.size(); i++) {
		uint32_t c = in[i];
		luma[i] = ((c >> 16) & 0xff) / 255.0f * 14.352f
			+ ((c >> 8) & 0xff) / 255.0f * 28.176f
			+ (c & 0xff) / 255.0f * 5.472f;
	}

	std::vector<SubpixelWeights> weights;
	subpixel_weights(factor, weights);

	parallel_rows(height, [&](int begin, int end) {
		for (int y = begin; y < end; y++) {
			int ym2 = std::max(y - 2, 0), ym1 = std::max(y - 1, 0);
			int yp1 = std::min(y + 1, height - 1), yp2 = std::min(y + 2, height - 1);
			for (int x = 0; x < width; x++) {
				int xm2 = std::max(x - 2, 0), xm1 = std::max(x - 1, 0);
				int xp1 = std::min(x + 1, width - 1);
				int xp2 = std::min(x + 2, width - 1);
#define L(px, py) luma[(size_t)(py) * width + (px)]
#define P(px, py) in[(size_t)(py) * width + (px)]
				float A1 = L(xm1, ym2), B1 = L(x, ym2), C1 = L(xp1, ym2);
				float A = L(xm1, ym1), B = L(x, ym1), C = L(xp1, ym1);
				float D = L(xm1, y), E = L(x, y), F = L(xp1, y);
				float G = L(xm1, yp1), H = L(x, yp1), I = L(xp1, yp1);
				float G5 = L(xm1, yp2), H5 = L(x, yp2), I5 = L(xp1, yp2);
				float A0 = L(xm2, ym1), D0 = L(xm2, y), G0 = L(xm2, yp1);
				float C4 = L(xp2, ym1), F4 = L(xp2, y), I4 = L(xp2, yp1);

				uint32_t* block = out + (size_t)y * factor * outWidth
					+ (size_t)x * factor;
				uint32_t centre = P(x, y);

				/* The four rotations (bottom-right, top-right, top-left,
				   bottom-left), as the vec4 components of the shader. */
				const float b[4] = { B, D, H, F };
				const float c[4] = { C, A, G, I };
				const float d[4] = { D, H, F, B };
				const float f[4] = { F, B, D, H };
				const float g[4] = { G, I, C, A };
				const float h[4] = { H, F, B, D };
				const float i[4] = { I, C, A, G };
				const float i4[4] = { I4, C1, A0, G5 };
				const float i5[4] = { I5, C4, A1, G0 };
				const float h5[4] = { H5, F4, B1, D0 };
				const float f4[4] = { F4, B1, D0, H5 };

				bool edri[4], edr[4], edrL[4], edrU[4], px[4];
				bool any = false;
				for (int k = 0; k < 4; k++) {
					auto df = [](float p, float q) { return fabsf(p - q); };
					auto eq = [&](float p, float q) { return df(p, q) <= kEqThreshold; };
					bool irlv0 = E != f[k] && E != h[k];
					edri[k] = edr[k] = edrL[k] = edrU[k] = false;
					px[k] = df(E, h[k]) >= df(E, f[k]);
					if (!irlv0)
						continue;
					/* CORNER_C */
					float irlv1 = (float)(!eq(f[k], b[k]) && !eq(f[k], c[k]))
						+ (float)(!eq(h[k], d[k]) && !eq(h[k], g[k]))
						+ (float)eq(E, i[k]) * ((float)(!eq(f[k], f4[k]) && !eq(f[k], i4[k]))
							+ (float)(!eq(h[k], h5[k]) && !eq(h[k], i5[k])))
						+ (float)eq(E, g[k]) + (float)eq(E, c[k]);
					bool irlv2l = E != g[k] && d[k] != g[k];
					bool irlv2u = E != c[k] && b[k] != c[k];
					float wd1 = df(E, c[k]) + df(E, g[k]) + df(i[k], h5[k])
						+ df(i[k], f4[k]) + 4.0f * df(h[k], f[k]);
					float wd2 = df(h[k], d[k]) + df(h[k], i5[k]) + df(f[k], i4[k])
						+ df(f[k], b[k]) + 4.0f * df(E, i[k]);
					edri[k] = wd2 >= wd1;
					edr[k] = wd2 >= wd1 + 0.1f && irlv1 >= 0.5f;
					edrL[k] = edr[k] && irlv2l
						&& df(h[k], c[k]) >= kLv2Coefficient * df(f[k], g[k]);
					edrU[k] = edr[k] && irlv2u
						&& df(f[k], g[k]) >= kLv2Coefficient * df(h[k], c[k]);
					any = true;
				}

				if (!any) {
					for (int sy = 0; sy < factor; sy++) {
						uint32_t* row = block + (size_t)sy * outWidth;
						for (int sx = 0; sx < factor; sx++)
							row[sx] = centre;
					}
					continue;
				}

				Rgb cE = to_rgb(centre);
				Rgb cB = to_rgb(P(x, ym1)), cD = to_rgb(P(xm1, y));
				Rgb cF = to_rgb(P(xp1, y)), cH = to_rgb(P(x, yp1));
				Rgb pick0 = px[0] ? cF : cH;
				Rgb pick1 = px[1] ? cB : cF;
				Rgb pick2 = px[2] ? cD : cB;
				Rgb pick3 = px[3] ? cH : cD;
#undef L
#undef P
				for (int sy = 0; sy < factor; sy++) {
					uint32_t* row = block + (size_t)sy * outWidth;
					for (int sx = 0; sx < factor; sx++) {
						const SubpixelWeights& w = weights[(size_t)sy * factor + sx];
						float m[4];
						for (int k = 0; k < 4; k++) {
							float v30 = edrL[k] ? w.f30[k] : 0.0f;
							float v60 = edrU[k] ? w.f60[k] : 0.0f;
							float v45 = edr[k] ? w.f45[k] : 0.0f;
							float v45i = edri[k] ? w.f45i[k] : 0.0f;
							m[k] = std::max(std::max(v30, v60), std::max(v45, v45i));
						}
						Rgb res1 = mix(mix(cE, pick0, m[0]), pick2, m[2]);
						Rgb res2 = mix(mix(cE, pick1, m[1]), pick3, m[3]);
						Rgb res = color_distance(cE, res2) >= color_distance(cE, res1)
							? res2 : res1;
						row[sx] = to_pixel(res);
					}
				}
			}
		}
	});
}


/* Scalers ------------------------------------------------------------------ */

void
scaler_fit(int srcWidth, int srcHeight, int dstWidth, int dstHeight, int* x,
	int* y, int* width, int* height)
{
	int w = dstWidth;
	int h = (int)((long long)dstWidth * srcHeight / srcWidth);
	if (h > dstHeight) {
		h = dstHeight;
		w = (int)((long long)dstHeight * srcWidth / srcHeight);
	}
	if (w < 1)
		w = 1;
	if (h < 1)
		h = 1;
	*width = w;
	*height = h;
	*x = (dstWidth - w) / 2;
	*y = (dstHeight - h) / 2;
}


void
NearestScaler::Layout(int srcWidth, int srcHeight, int dstWidth,
	int dstHeight, int* factor, int* x, int* y)
{
	int f = dstWidth / srcWidth;
	if (dstHeight / srcHeight < f)
		f = dstHeight / srcHeight;
	if (f < 1)
		f = 1;
	*factor = f;
	*x = (dstWidth - srcWidth * f) / 2;
	*y = (dstHeight - srcHeight * f) / 2;
}


void
NearestScaler::Process(const uint32_t* src, int srcWidth, int srcHeight,
	int /*sourceFactor*/, uint32_t* dst, int dstWidth, int dstHeight,
	int dstStride)
{
	int factor, offsetX, offsetY;
	Layout(srcWidth, srcHeight, dstWidth, dstHeight, &factor, &offsetX,
		&offsetY);

	for (int y = 0; y < dstHeight; y++) {
		uint32_t* row = dst + (size_t)y * dstStride;
		int sy = y - offsetY;
		if (sy < 0 || sy >= srcHeight * factor) {
			memset(row, 0, sizeof(uint32_t) * (size_t)dstWidth);
			continue;
		}
		const uint32_t* in = src + (size_t)(sy / factor) * srcWidth;
		int x = 0;
		for (; x < offsetX && x < dstWidth; x++)
			row[x] = 0;
		/* Visible part (may be clipped if the output is smaller). */
		for (int sx = 0; sx < srcWidth && x < dstWidth; sx++) {
			uint32_t pixel = in[sx];
			for (int i = 0; i < factor && x < dstWidth; i++) {
				if (x >= 0)
					row[x] = pixel;
				x++;
			}
		}
		for (; x < dstWidth; x++)
			row[x] = 0;
	}
}


void
SharpScaler::Process(const uint32_t* src, int srcWidth, int srcHeight,
	int sourceFactor, uint32_t* dst, int dstWidth, int dstHeight,
	int dstStride)
{
	std::vector<uint32_t> art;
	int width, height;
	const uint32_t* image = art_image(src, srcWidth, srcHeight, sourceFactor,
		art, &width, &height);
	int rx, ry, rw, rh;
	scaler_fit(width, height, dstWidth, dstHeight, &rx, &ry, &rw, &rh);
	clear_borders(dst, dstWidth, dstHeight, dstStride, rx, ry, rw, rh);
	resample(image, width, height, dst, dstStride, rx, ry, rw, rh, true);
}


void
XbrScaler::Process(const uint32_t* src, int srcWidth, int srcHeight,
	int sourceFactor, uint32_t* dst, int dstWidth, int dstHeight,
	int dstStride)
{
	int width, height;
	const uint32_t* image = art_image(src, srcWidth, srcHeight, sourceFactor,
		fArt, &width, &height);
	int rx, ry, rw, rh;
	scaler_fit(width, height, dstWidth, dstHeight, &rx, &ry, &rw, &rh);
	clear_borders(dst, dstWidth, dstHeight, dstStride, rx, ry, rw, rh);

	/* Integer xBR factor that reaches the output size, then resample. */
	int factor = (int)ceil((double)rw / width - 1e-6);
	factor = std::min(std::max(factor, 1), 6);
	if (factor == 1) {
		resample(image, width, height, dst, dstStride, rx, ry, rw, rh, true);
		return;
	}
	fScaled.resize((size_t)width * factor * height * factor);
	Scale(image, width, height, factor, fScaled.data());

	int sw = width * factor;
	int sh = height * factor;
	if (sw == rw && sh == rh) {
		for (int y = 0; y < rh; y++) {
			memcpy(dst + (size_t)(ry + y) * dstStride + rx,
				fScaled.data() + (size_t)y * sw, sizeof(uint32_t) * (size_t)rw);
		}
		return;
	}
	resample(fScaled.data(), sw, sh, dst, dstStride, rx, ry, rw, rh, false);
}


Scaler*
scaler_for_name(const char* name)
{
	static NearestScaler sNearest;
	static SharpScaler sSharp;
	static XbrScaler sXbr;
	if (name != NULL && strcmp(name, "nearest") == 0)
		return &sNearest;
	if (name != NULL && strcmp(name, "sharp") == 0)
		return &sSharp;
	return &sXbr;
}


Scaler*
scaler_next(Scaler* current)
{
	const char* name = current != NULL ? current->Name() : "xbr";
	if (strcmp(name, "nearest") == 0)
		return scaler_for_name("sharp");
	if (strcmp(name, "sharp") == 0)
		return scaler_for_name("xbr");
	return scaler_for_name("nearest");
}
