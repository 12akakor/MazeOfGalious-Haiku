#include "video_scaler.h"

#include <string.h>


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


Scaler*
scaler_for_name(const char* /*name*/)
{
	static NearestScaler sNearest;
	return &sNearest;
}
