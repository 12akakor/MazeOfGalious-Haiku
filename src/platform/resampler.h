/*
 * Sample rate conversion for the mixer: windowed sinc (Kaiser window),
 * polyphase table with linear interpolation between phases.
 *
 * Replaces the linear interpolation of version 2. The sound sets have
 * samples at 11025, 22050, 44100 and 48000 Hz; the mixer runs at 44100 Hz.
 * Linear interpolation dulls the high frequencies and leaves mirror images
 * of the sound above the original Nyquist frequency (audible as a slight
 * hiss or metallic edge). The windowed sinc keeps the band up to 90% of the
 * lower Nyquist frequency and removes the images.
 */
#ifndef MOG_RESAMPLER_H
#define MOG_RESAMPLER_H

#include <stdint.h>
#include <vector>


class Resampler {
public:
								Resampler();

			void				Init(int inRate, int outRate);

			bool				Passthrough() const
									{ return fInRate == fOutRate; }

			/* Output frame k uses the input frames
			   [index - Before(), index + After()], see Position(). */
			int					Before() const { return fHalf - 1; }
			int					After() const { return fHalf; }
			int					Taps() const { return fTaps; }

			void				Position(int64_t outFrame, int64_t* index,
									double* fraction) const;
			int64_t				OutputFrames(int64_t inFrames) const;

			/* window: Taps() stereo frames, the first one being input frame
			   index - Before(). Writes one stereo frame. */
			void				Frame(const int16_t* window, double fraction,
									int16_t* out) const;

private:
			int					fInRate;
			int					fOutRate;
			int					fHalf;
			int					fTaps;
			std::vector<float>	fTable;		/* (kPhases + 1) rows of fTaps */
};

#endif
