/*
 * Portable software mixer and decoders. See audio_mixer.h.
 */
#include "audio_mixer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "resampler.h"

#define STB_VORBIS_HEADER_ONLY
#include "../third_party/stb_vorbis.c"


struct MogSound {
	int16_t*	data;		/* stereo, MIXER_RATE */
	int			frames;
	int			volume;
};


namespace {

struct Channel {
	MogSound*	sound;
	int			position;
};

Channel sChannels[MIXER_CHANNELS];
MusicHook sMusicHook = NULL;
bool sOpen = false;
bool sDevice = false;


inline int16_t
clamp16(int32_t value)
{
	if (value > 32767)
		return 32767;
	if (value < -32768)
		return -32768;
	return (int16_t)value;
}


/* Converts interleaved int16 (1 or 2 channels, any rate) to stereo at
   MIXER_RATE (windowed sinc, see resampler.h). Returns a malloc'ed buffer. */
int16_t*
convert_to_output(const int16_t* in, int inFrames, int channels, int rate,
	int* outFrames)
{
	if (inFrames <= 0 || channels < 1 || rate <= 0) {
		*outFrames = 0;
		return NULL;
	}

	Resampler resampler;
	resampler.Init(rate, MIXER_RATE);
	int64_t count = resampler.OutputFrames(inFrames);
	int16_t* out = (int16_t*)malloc(sizeof(int16_t) * 2 * (size_t)count);

	/* Stereo copy with silence before and after for the filter. */
	int before = resampler.Before();
	int after = resampler.After();
	size_t paddedFrames = (size_t)before + inFrames + after;
	int16_t* padded = (int16_t*)calloc(paddedFrames * 2, sizeof(int16_t));
	if (out == NULL || padded == NULL) {
		free(out);
		free(padded);
		*outFrames = 0;
		return NULL;
	}
	for (int i = 0; i < inFrames; i++) {
		int16_t* frame = padded + ((size_t)before + i) * 2;
		frame[0] = in[i * channels];
		frame[1] = in[i * channels + (channels == 1 ? 0 : 1)];
	}

	for (int64_t k = 0; k < count; k++) {
		int64_t index;
		double fraction;
		resampler.Position(k, &index, &fraction);
		/* padded frame 'index' is input frame index - before. */
		resampler.Frame(padded + (size_t)index * 2, fraction, out + k * 2);
	}
	free(padded);
	*outFrames = (int)count;
	return out;
}


uint32_t
read_le32(const uint8_t* p)
{
	return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24);
}


uint16_t
read_le16(const uint8_t* p)
{
	return (uint16_t)(p[0] | (p[1] << 8));
}


uint8_t*
read_file(const char* path, size_t* size)
{
	FILE* fp = fopen(path, "rb");
	if (fp == NULL)
		return NULL;
	fseek(fp, 0, SEEK_END);
	long length = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if (length <= 0) {
		fclose(fp);
		return NULL;
	}
	uint8_t* data = (uint8_t*)malloc((size_t)length);
	if (data != NULL && fread(data, 1, (size_t)length, fp) != (size_t)length) {
		free(data);
		data = NULL;
	}
	fclose(fp);
	*size = (size_t)length;
	return data;
}


/* Decodes a PCM WAV file (8 bit unsigned or 16 bit signed, mono/stereo). */
int16_t*
decode_wav(const char* path, int* frames)
{
	size_t size = 0;
	uint8_t* file = read_file(path, &size);
	if (file == NULL)
		return NULL;

	int16_t* result = NULL;
	if (size >= 12 && memcmp(file, "RIFF", 4) == 0
		&& memcmp(file + 8, "WAVE", 4) == 0) {
		int format = 0, channels = 0, rate = 0, bits = 0;
		const uint8_t* data = NULL;
		uint32_t dataSize = 0;
		size_t offset = 12;
		while (offset + 8 <= size) {
			uint32_t chunkSize = read_le32(file + offset + 4);
			const uint8_t* chunk = file + offset + 8;
			if (chunkSize > size - offset - 8)
				chunkSize = (uint32_t)(size - offset - 8);
			if (memcmp(file + offset, "fmt ", 4) == 0 && chunkSize >= 16) {
				format = read_le16(chunk);
				channels = read_le16(chunk + 2);
				rate = (int)read_le32(chunk + 4);
				bits = read_le16(chunk + 14);
			} else if (memcmp(file + offset, "data", 4) == 0) {
				data = chunk;
				dataSize = chunkSize;
			}
			offset += 8 + chunkSize + (chunkSize & 1);
		}

		if (format == 1 && data != NULL && (channels == 1 || channels == 2)
			&& (bits == 8 || bits == 16)) {
			int inFrames = (int)(dataSize / (uint32_t)(channels * bits / 8));
			int16_t* pcm = (int16_t*)malloc(sizeof(int16_t)
				* (size_t)(inFrames * channels + 1));
			if (pcm != NULL) {
				for (int i = 0; i < inFrames * channels; i++) {
					if (bits == 8)
						pcm[i] = (int16_t)((data[i] - 128) << 8);
					else
						pcm[i] = (int16_t)read_le16(data + i * 2);
				}
				result = convert_to_output(pcm, inFrames, channels, rate,
					frames);
				free(pcm);
			}
		} else {
			fprintf(stderr, "audio: unsupported WAV format in %s\n", path);
		}
	}
	free(file);
	return result;
}


int16_t*
decode_ogg(const char* path, int* frames)
{
	int channels = 0, rate = 0;
	short* pcm = NULL;
	int inFrames = stb_vorbis_decode_filename(path, &channels, &rate, &pcm);
	if (inFrames <= 0 || pcm == NULL)
		return NULL;

	int16_t* result = NULL;
	if (channels == 1 || channels == 2) {
		result = convert_to_output(pcm, inFrames, channels, rate, frames);
	} else {
		/* Keep the first two channels. */
		int16_t* stereo = (int16_t*)malloc(sizeof(int16_t) * 2
			* (size_t)inFrames);
		if (stereo != NULL) {
			for (int i = 0; i < inFrames; i++) {
				stereo[i * 2] = pcm[i * channels];
				stereo[i * 2 + 1] = pcm[i * channels + 1];
			}
			result = convert_to_output(stereo, inFrames, 2, rate, frames);
			free(stereo);
		}
	}
	free(pcm);
	return result;
}


bool
is_ogg(const char* path)
{
	FILE* fp = fopen(path, "rb");
	if (fp == NULL)
		return false;
	char magic[4];
	bool ogg = fread(magic, 1, 4, fp) == 4 && memcmp(magic, "OggS", 4) == 0;
	fclose(fp);
	return ogg;
}

}	// namespace


/* Sounds -------------------------------------------------------------- */

MogSound*
snd_load(const char* path)
{
	int frames = 0;
	int16_t* data = is_ogg(path) ? decode_ogg(path, &frames)
		: decode_wav(path, &frames);
	if (data == NULL) {
		fprintf(stderr, "audio: could not decode %s\n", path);
		return NULL;
	}
	MogSound* sound = (MogSound*)malloc(sizeof(MogSound));
	if (sound == NULL) {
		free(data);
		return NULL;
	}
	sound->data = data;
	sound->frames = frames;
	sound->volume = MIXER_MAX_VOLUME;
	return sound;
}


void
snd_free(MogSound* sound)
{
	if (sound == NULL)
		return;
	mixer_lock();
	for (int i = 0; i < MIXER_CHANNELS; i++) {
		if (sChannels[i].sound == sound)
			sChannels[i].sound = NULL;
	}
	mixer_unlock();
	free(sound->data);
	free(sound);
}


void
snd_set_volume(MogSound* sound, int volume)
{
	if (sound == NULL)
		return;
	if (volume < 0)
		volume = 0;
	if (volume > MIXER_MAX_VOLUME)
		volume = MIXER_MAX_VOLUME;
	sound->volume = volume;
}


void
snd_play(MogSound* sound)
{
	if (sound == NULL || !sOpen)
		return;
	mixer_lock();
	for (int i = 0; i < MIXER_CHANNELS; i++) {
		if (sChannels[i].sound == NULL) {
			sChannels[i].sound = sound;
			sChannels[i].position = 0;
			break;
		}
	}
	mixer_unlock();
}


/* Streams ------------------------------------------------------------- */

#define STREAM_BUFFER_FRAMES	4096

struct MogStream {
	/* WAV: whole file converted in memory. */
	int16_t*	pcm;
	int			pcmFrames;
	int			pcmPosition;

	/* Ogg: decoded on demand and resampled to MIXER_RATE. The buffer holds
	   the input frames [bufferStart, bufferStart + bufferFrames). */
	stb_vorbis*	vorbis;
	Resampler*	resampler;
	int16_t*	window;			/* Taps() frames, for the stream edges */
	int16_t		buffer[STREAM_BUFFER_FRAMES * 2];
	int64_t		bufferStart;
	int			bufferFrames;
	int64_t		outPosition;	/* output frames produced */
	bool		sourceDone;

	bool		eof;
};


/* Drops the frames before keepFrom and decodes until the buffer is full. */
static void
stream_fill(MogStream* stream, int64_t keepFrom)
{
	int64_t drop = keepFrom - stream->bufferStart;
	if (drop > stream->bufferFrames)
		drop = stream->bufferFrames;
	if (drop > 0) {
		int keep = stream->bufferFrames - (int)drop;
		memmove(stream->buffer, stream->buffer + drop * 2,
			sizeof(int16_t) * 2 * (size_t)keep);
		stream->bufferFrames = keep;
		stream->bufferStart += drop;
	}

	while (!stream->sourceDone && stream->bufferFrames < STREAM_BUFFER_FRAMES) {
		int space = STREAM_BUFFER_FRAMES - stream->bufferFrames;
		int got = stb_vorbis_get_samples_short_interleaved(stream->vorbis, 2,
			stream->buffer + stream->bufferFrames * 2, space * 2);
		if (got <= 0)
			stream->sourceDone = true;
		else
			stream->bufferFrames += got;
	}
}


MogStream*
stream_open(const char* path)
{
	MogStream* stream = (MogStream*)calloc(1, sizeof(MogStream));
	if (stream == NULL)
		return NULL;

	if (is_ogg(path)) {
		int error = 0;
		stream->vorbis = stb_vorbis_open_filename(path, &error, NULL);
		if (stream->vorbis == NULL) {
			fprintf(stderr, "audio: could not open %s (%d)\n", path, error);
			free(stream);
			return NULL;
		}
		stb_vorbis_info info = stb_vorbis_get_info(stream->vorbis);
		stream->resampler = new Resampler();
		stream->resampler->Init((int)info.sample_rate, MIXER_RATE);
		stream->window = (int16_t*)malloc(sizeof(int16_t) * 2
			* (size_t)stream->resampler->Taps());
		stream_fill(stream, 0);
	} else {
		stream->pcm = decode_wav(path, &stream->pcmFrames);
		if (stream->pcm == NULL) {
			free(stream);
			return NULL;
		}
	}
	return stream;
}


int
stream_read(MogStream* stream, int16_t* out, int frames)
{
	if (stream == NULL || stream->eof)
		return 0;

	if (stream->pcm != NULL) {
		int count = stream->pcmFrames - stream->pcmPosition;
		if (count > frames)
			count = frames;
		memcpy(out, stream->pcm + stream->pcmPosition * 2,
			sizeof(int16_t) * 2 * (size_t)count);
		stream->pcmPosition += count;
		if (stream->pcmPosition >= stream->pcmFrames)
			stream->eof = true;
		return count;
	}

	const Resampler& resampler = *stream->resampler;
	const int before = resampler.Before();
	const int after = resampler.After();
	const int taps = resampler.Taps();
	int written = 0;
	while (written < frames) {
		int64_t index;
		double fraction;
		resampler.Position(stream->outPosition, &index, &fraction);
		if (!stream->sourceDone
			&& index + after >= stream->bufferStart + stream->bufferFrames)
			stream_fill(stream, index - before);
		int64_t end = stream->bufferStart + stream->bufferFrames;
		if (index >= end) {
			stream->eof = true;
			break;
		}

		int64_t first = index - before;
		const int16_t* window;
		if (first >= stream->bufferStart && first + taps <= end)
			window = stream->buffer + (first - stream->bufferStart) * 2;
		else {
			/* Start or end of the stream: silence outside it. */
			for (int j = 0; j < taps; j++) {
				int64_t f = first + j;
				bool inside = f >= stream->bufferStart && f < end;
				const int16_t* src = stream->buffer
					+ (f - stream->bufferStart) * 2;
				stream->window[j * 2] = inside ? src[0] : 0;
				stream->window[j * 2 + 1] = inside ? src[1] : 0;
			}
			window = stream->window;
		}
		resampler.Frame(window, fraction, out + written * 2);
		written++;
		stream->outPosition++;
	}
	return written;
}


bool
stream_eof(MogStream* stream)
{
	return stream == NULL || stream->eof;
}


void
stream_rewind(MogStream* stream)
{
	if (stream == NULL)
		return;
	stream->eof = false;
	if (stream->pcm != NULL) {
		stream->pcmPosition = 0;
		return;
	}
	stb_vorbis_seek_start(stream->vorbis);
	stream->bufferStart = 0;
	stream->bufferFrames = 0;
	stream->outPosition = 0;
	stream->sourceDone = false;
	stream_fill(stream, 0);
}


void
stream_close(MogStream* stream)
{
	if (stream == NULL)
		return;
	if (stream->vorbis != NULL)
		stb_vorbis_close(stream->vorbis);
	delete stream->resampler;
	free(stream->window);
	free(stream->pcm);
	free(stream);
}


/* Mixer --------------------------------------------------------------- */

static void
device_callback(int16_t* out, int frames, void* /*cookie*/)
{
	mixer_render(out, frames);
}


bool
mixer_open(bool openDevice)
{
	if (sOpen)
		return true;
	memset(sChannels, 0, sizeof(sChannels));
	if (openDevice) {
		if (!plat_audio_open(MIXER_RATE, device_callback, NULL))
			return false;
		sDevice = true;
	}
	sOpen = true;
	return true;
}


void
mixer_close(void)
{
	if (sDevice)
		plat_audio_close();
	sDevice = false;
	sOpen = false;
	memset(sChannels, 0, sizeof(sChannels));
}


bool
mixer_is_open(void)
{
	return sOpen;
}


void
mixer_set_music_hook(MusicHook hook)
{
	mixer_lock();
	sMusicHook = hook;
	mixer_unlock();
}


void
mixer_lock(void)
{
	if (sDevice)
		plat_audio_lock();
}


void
mixer_unlock(void)
{
	if (sDevice)
		plat_audio_unlock();
}


/* Called with the audio lock held by the device back end (like SDL), or
   directly by tests. */
void
mixer_render(int16_t* out, int frames)
{
	if (sMusicHook != NULL)
		sMusicHook(out, frames);
	else
		memset(out, 0, sizeof(int16_t) * 2 * (size_t)frames);

	for (int c = 0; c < MIXER_CHANNELS; c++) {
		Channel& channel = sChannels[c];
		MogSound* sound = channel.sound;
		if (sound == NULL)
			continue;
		int count = sound->frames - channel.position;
		if (count > frames)
			count = frames;
		const int16_t* in = sound->data + channel.position * 2;
		int volume = sound->volume;
		for (int i = 0; i < count * 2; i++)
			out[i] = clamp16(out[i] + ((in[i] * volume) >> 7));
		channel.position += count;
		if (channel.position >= sound->frames)
			channel.sound = NULL;
	}
}
