/*
 * Scaling of the 640x400 game image to the window or screen size.
 *
 * Version 1 ships NearestScaler only (integer factor, centred, black borders).
 * Later filters (e.g. xBR, see docs/port/05-toekomst-beeldkwaliteit.md) are
 * additional Scaler classes. 'sourceFactor' tells a filter how many output
 * pixels one art pixel covers in the source (2 for the "original" set, which
 * is 320x200 art stored doubled; 1 for the other sets).
 */
#ifndef MOG_VIDEO_SCALER_H
#define MOG_VIDEO_SCALER_H

#include <stdint.h>

class Scaler {
public:
	virtual					~Scaler() {}
	virtual	const char*		Name() const = 0;
	/* dst has dstWidth x dstHeight pixels, dstStride pixels per row. */
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


/* Returns the scaler for a settings name ("nearest"); never NULL. */
Scaler* scaler_for_name(const char* name);

#endif
