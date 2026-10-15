/*
 * Windowed sinc resampler, see resampler.h.
 */
#include "resampler.h"

#include <math.h>


namespace {

const int kPhases = 512;
const int kZeroCrossings = 16;	/* per side of the kernel */
const double kCutoff = 0.90;	/* of the lower Nyquist frequency */
const double kBeta = 8.0;		/* Kaiser window, about 80 dB stopband */


double
bessel_i0(double x)
{
	double sum = 1.0, term = 1.0;
	for (int k = 1; k < 50; k++) {
		term *= (x / (2.0 * k)) * (x / (2.0 * k));
		sum += term;
		if (term < sum * 1e-12)
			break;
	}
	return sum;
}

}	// namespace


Resampler::Resampler()
	:
	fInRate(1),
	fOutRate(1),
	fHalf(1),
	fTaps(1)
{
}


void
Resampler::Init(int inRate, int outRate)
{
	fInRate = inRate > 0 ? inRate : 1;
	fOutRate = outRate > 0 ? outRate : 1;
	fTable.clear();
	if (Passthrough()) {
		fHalf = 1;
		fTaps = 1;
		return;
	}

	/* Cutoff in units of the input Nyquist frequency. When the output rate
	   is lower, the output Nyquist frequency is the limit. */
	double fc = kCutoff;
	if (fOutRate < fInRate)
		fc *= (double)fOutRate / fInRate;
	fHalf = (int)ceil(kZeroCrossings / fc);
	fTaps = 2 * fHalf;

	fTable.resize((size_t)(kPhases + 1) * fTaps);
	double norm = bessel_i0(kBeta);
	for (int p = 0; p <= kPhases; p++) {
		double phase = (double)p / kPhases;
		float* row = &fTable[(size_t)p * fTaps];
		double sum = 0.0;
		for (int j = 0; j < fTaps; j++) {
			int n = j - (fHalf - 1);
			double d = phase - n;	/* distance in input frames */
			double x = fc * d;
			double sinc = fabs(x) < 1e-9 ? 1.0 : sin(M_PI * x) / (M_PI * x);
			double r = d / fHalf;
			double window = fabs(r) >= 1.0
				? 0.0 : bessel_i0(kBeta * sqrt(1.0 - r * r)) / norm;
			double value = fc * sinc * window;
			row[j] = (float)value;
			sum += value;
		}
		/* Exact unity gain for every phase (no ripple on constant
		   signals). */
		for (int j = 0; j < fTaps; j++)
			row[j] = (float)(row[j] / sum);
	}
}


void
Resampler::Position(int64_t outFrame, int64_t* index, double* fraction) const
{
	int64_t scaled = outFrame * fInRate;
	*index = scaled / fOutRate;
	*fraction = (double)(scaled % fOutRate) / fOutRate;
}


int64_t
Resampler::OutputFrames(int64_t inFrames) const
{
	return (inFrames * fOutRate + fInRate - 1) / fInRate;
}


void
Resampler::Frame(const int16_t* window, double fraction, int16_t* out) const
{
	if (Passthrough()) {
		out[0] = window[0];
		out[1] = window[1];
		return;
	}
	double position = fraction * kPhases;
	int p = (int)position;
	if (p >= kPhases)
		p = kPhases - 1;
	float t = (float)(position - p);
	const float* row0 = &fTable[(size_t)p * fTaps];
	const float* row1 = row0 + fTaps;
	float left = 0.0f, right = 0.0f;
	for (int j = 0; j < fTaps; j++) {
		float w = row0[j] + (row1[j] - row0[j]) * t;
		left += w * window[j * 2];
		right += w * window[j * 2 + 1];
	}
	long l = lrintf(left);
	long r = lrintf(right);
	out[0] = (int16_t)(l > 32767 ? 32767 : (l < -32768 ? -32768 : l));
	out[1] = (int16_t)(r > 32767 ? 32767 : (r < -32768 ? -32768 : r));
}
