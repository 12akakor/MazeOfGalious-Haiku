/*
 * Scaling of the 640x400 game image to the window or screen size.
 *
 *   nearest  integer factor, centred, black borders (version 1)
 *   sharp    sharp bilinear: fills the window keeping the 16:10 aspect, pixels
 *            stay sharp, no uneven pixel widths
 *   xbr      Hyllian's xBR-lv2 (smooth contours), then resampled to the window
 *
 * 'sourceFactor' tells a filter how many output pixels one art pixel covers in
 * the source: 2 for the "original" set (320x200 art stored doubled), 1 for the
 * other sets. xBR works on the art resolution, so it first undoes the
 * doubling.
 */
#ifndef MOG_VIDEO_SCALER_H
#define MOG_VIDEO_SCALER_H

#include <stdint.h>
#include <vector>


class Scaler {
public:
	virtual					~Scaler() {}
	virtual	const char*		Name() const = 0;
	/* src: srcWidth x srcHeight pixels (0xffRRGGBB). dst has dstWidth x
	   dstHeight pixels, dstStride pixels per row. */
	virtual	void			Process(const uint32_t* src, int srcWidth,
								int srcHeight, int sourceFactor,
								uint32_t* dst, int dstWidth, int dstHeight,
								int dstStride) = 0;
};


class NearestScaler : public Scaler {
public:
	virtual	const char*		Name() const { return "nearest"; }
	virtual	void			Process(const uint32_t* src, int srcWidth,
								int srcHeight, int sourceFactor,
								uint32_t* dst, int dstWidth, int dstHeight,
								int dstStride);

	/* Integer factor and placement used for a given output size. */
	static	void			Layout(int srcWidth, int srcHeight, int dstWidth,
								int dstHeight, int* factor, int* x, int* y);
};


class SharpScaler : public Scaler {
public:
	virtual	const char*		Name() const { return "sharp"; }
	virtual	void			Process(const uint32_t* src, int srcWidth,
								int srcHeight, int sourceFactor,
								uint32_t* dst, int dstWidth, int dstHeight,
								int dstStride);
};


class XbrScaler : public Scaler {
public:
	virtual	const char*		Name() const { return "xbr"; }
	virtual	void			Process(const uint32_t* src, int srcWidth,
								int srcHeight, int sourceFactor,
								uint32_t* dst, int dstWidth, int dstHeight,
								int dstStride);

	/* xBR-lv2 at an integer factor (1..4): out has width*factor x
	   height*factor pixels. Exposed for tests. */
	static	void			Scale(const uint32_t* in, int width, int height,
								int factor, uint32_t* out);

private:
			std::vector<uint32_t> fArt;
			std::vector<uint32_t> fScaled;
};


/* Largest rectangle with the aspect of srcWidth:srcHeight that fits in
   dstWidth x dstHeight, centred. */
void scaler_fit(int srcWidth, int srcHeight, int dstWidth, int dstHeight,
	int* x, int* y, int* width, int* height);

/* Returns the scaler for a name ("nearest", "sharp", "xbr"); unknown names
   give "xbr". Never NULL. */
Scaler* scaler_for_name(const char* name);

/* Next scaler in the cycle nearest -> sharp -> xbr -> nearest. */
Scaler* scaler_next(Scaler* current);

#endif
