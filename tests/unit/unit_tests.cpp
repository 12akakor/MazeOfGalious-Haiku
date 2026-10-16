/*
 * Unit tests for the portable platform code (no Be API needed).
 * Built and run by "make test".
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <vector>

#include "../../src/platform/audio_mixer.h"
#include "../../src/platform/platform.h"
#include "../../src/platform/platform_internal.h"
#include "../../src/platform/resampler.h"
#include "../../src/platform/video_scaler.h"
#include "../../src/loadpcx.h"

static int sFailures = 0;
static int sChecks = 0;

#define CHECK(cond) do { sChecks++; if (!(cond)) { sFailures++; \
	printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

struct MogSoundView { int16_t* data; int frames; int volume; };

static const char* sData = "data";


static void
write_wav(const char* path, int channels, int rate, int bits,
	const void* data, int bytes)
{
	FILE* fp = fopen(path, "wb");
	unsigned char h[44];
	memcpy(h, "RIFF", 4);
	int v = 36 + bytes; memcpy(h + 4, &v, 4);
	memcpy(h + 8, "WAVEfmt ", 8);
	v = 16; memcpy(h + 16, &v, 4);
	short s = 1; memcpy(h + 20, &s, 2);
	s = (short)channels; memcpy(h + 22, &s, 2);
	memcpy(h + 24, &rate, 4);
	v = rate * channels * bits / 8; memcpy(h + 28, &v, 4);
	s = (short)(channels * bits / 8); memcpy(h + 32, &s, 2);
	s = (short)bits; memcpy(h + 34, &s, 2);
	memcpy(h + 36, "data", 4);
	memcpy(h + 40, &bytes, 4);
	fwrite(h, 1, 44, fp);
	fwrite(data, 1, (size_t)bytes, fp);
	fclose(fp);
}


static void
test_wav_decoding(void)
{
	/* 16 bit mono 44100: duplicated to stereo, unchanged values. */
	int16_t mono[4] = { 0, 1000, -1000, 32767 };
	write_wav("/tmp/mog_t1.wav", 1, 44100, 16, mono, sizeof(mono));
	MogSoundView* s = (MogSoundView*)snd_load("/tmp/mog_t1.wav");
	CHECK(s != NULL);
	if (s != NULL) {
		CHECK(s->frames == 4);
		CHECK(s->data[2] == 1000 && s->data[3] == 1000);
		CHECK(s->data[6] == 32767 && s->data[7] == 32767);
		snd_free((MogSound*)s);
	}

	/* 8 bit unsigned 22050: twice as many frames, 128 -> 0, a constant
	   level stays constant (away from the start and end). */
	unsigned char u8[64];
	memset(u8, 200, sizeof(u8));
	write_wav("/tmp/mog_t2.wav", 1, 22050, 8, u8, sizeof(u8));
	s = (MogSoundView*)snd_load("/tmp/mog_t2.wav");
	CHECK(s != NULL);
	if (s != NULL) {
		CHECK(s->frames == 128);
		bool flat = true;
		for (int i = 40; i < 88; i++)
			flat = flat && abs(s->data[i * 2] - 72 * 256) <= 1
				&& s->data[i * 2] == s->data[i * 2 + 1];
		CHECK(flat);
		snd_free((MogSound*)s);
	}

	/* Unsupported / missing files fail cleanly. */
	CHECK(snd_load("/tmp/does-not-exist.wav") == NULL);
	unsigned char junk[16] = { 0 };
	FILE* fp = fopen("/tmp/mog_t3.wav", "wb");
	fwrite(junk, 1, sizeof(junk), fp);
	fclose(fp);
	CHECK(snd_load("/tmp/mog_t3.wav") == NULL);
}


static void
test_stream_matches_sound(const char* file)
{
	char path[512];
	snprintf(path, sizeof(path), "%s/sound/%s", sData, file);
	MogSoundView* whole = (MogSoundView*)snd_load(path);
	MogStream* stream = stream_open(path);
	CHECK(whole != NULL && stream != NULL);
	if (whole == NULL || stream == NULL)
		return;
	int16_t* buffer = (int16_t*)malloc(sizeof(int16_t) * 2 * (whole->frames + 5000));
	int total = 0;
	while (!stream_eof(stream)) {
		int got = stream_read(stream, buffer + total * 2, 777);
		total += got;
		if (got == 0)
			break;
	}
	/* Same length (within resampler rounding) and same samples. */
	CHECK(abs(total - whole->frames) <= 2);
	int n = total < whole->frames ? total : whole->frames;
	int maxDiff = 0;
	for (int i = 0; i < n * 2; i++) {
		int d = abs(buffer[i] - whole->data[i]);
		if (d > maxDiff)
			maxDiff = d;
	}
	if (maxDiff > 2)
		printf("  %s: max sample difference %d\n", file, maxDiff);
	CHECK(maxDiff <= 2);

	/* Rewind restarts at the first frame. */
	stream_rewind(stream);
	CHECK(!stream_eof(stream));
	int16_t first[2];
	CHECK(stream_read(stream, first, 1) == 1);
	CHECK(abs(first[0] - whole->data[0]) <= 2);

	free(buffer);
	stream_close(stream);
	snd_free((MogSound*)whole);
}


/* Signal to error ratio (dB) of a resampled sine against the exact sine. */
static double
sine_snr(int inRate, double frequency, bool linear)
{
	const int inFrames = inRate;	/* one second */
	std::vector<int16_t> in((size_t)inFrames * 2);
	for (int i = 0; i < inFrames; i++) {
		double v = 16000.0 * sin(2.0 * M_PI * frequency * i / inRate);
		in[i * 2] = in[i * 2 + 1] = (int16_t)lrint(v);
	}
	Resampler r;
	r.Init(inRate, MIXER_RATE);
	int64_t count = r.OutputFrames(inFrames);
	std::vector<int16_t> padded(((size_t)r.Before() + inFrames + r.After()) * 2);
	memcpy(&padded[(size_t)r.Before() * 2], in.data(), in.size() * 2);
	double signal = 0.0, error = 0.0;
	for (int64_t k = count / 4; k < count * 3 / 4; k++) {
		int64_t index;
		double fraction;
		r.Position(k, &index, &fraction);
		int16_t out[2];
		if (linear) {
			int a = in[index * 2], b = in[(index + 1) * 2];
			out[0] = (int16_t)lrint(a + (b - a) * fraction);
		} else
			r.Frame(&padded[(size_t)index * 2], fraction, out);
		double exact = 16000.0 * sin(2.0 * M_PI * frequency * k / MIXER_RATE);
		signal += exact * exact;
		error += (out[0] - exact) * (out[0] - exact);
	}
	return 10.0 * log10(signal / error);
}


static void
test_resampler(void)
{
	Resampler same;
	same.Init(44100, 44100);
	CHECK(same.Passthrough() && same.Taps() == 1);
	int16_t frame[2] = { 123, -456 }, out[2];
	same.Frame(frame, 0.0, out);
	CHECK(out[0] == 123 && out[1] == -456);

	Resampler up;
	up.Init(11025, 44100);
	CHECK(up.OutputFrames(11025) == 44100);
	int64_t index;
	double fraction;
	up.Position(5, &index, &fraction);
	CHECK(index == 1 && fabs(fraction - 0.25) < 1e-9);

	const struct { int rate; double frequency; double minimum; } cases[] = {
		{ 11025, 1000.0, 70.0 }, { 11025, 4000.0, 60.0 },
		{ 22050, 8000.0, 60.0 }, { 48000, 15000.0, 60.0 }
	};
	for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		double sinc = sine_snr(cases[i].rate, cases[i].frequency, false);
		double linear = sine_snr(cases[i].rate, cases[i].frequency, true);
		printf("  resampler %5d Hz, %5.0f Hz sine: %.1f dB (linear %.1f dB)\n",
			cases[i].rate, cases[i].frequency, sinc, linear);
		CHECK(sinc >= cases[i].minimum);
		CHECK(sinc > linear + 10.0);
	}
}


static int16_t sMusicValue = 0;

static void
music_hook(int16_t* out, int frames)
{
	for (int i = 0; i < frames * 2; i++)
		out[i] = sMusicValue;
}


static void
test_mixer(void)
{
	CHECK(mixer_open(false));
	int16_t loud[200];
	for (int i = 0; i < 200; i++)
		loud[i] = 30000;
	write_wav("/tmp/mog_t4.wav", 2, 44100, 16, loud, sizeof(loud));
	MogSound* s = snd_load("/tmp/mog_t4.wav");
	CHECK(s != NULL);

	int16_t out[200];
	mixer_set_music_hook(music_hook);
	sMusicValue = 1000;
	mixer_render(out, 100);
	CHECK(out[0] == 1000 && out[199] == 1000);

	/* Volume 128 adds the full sample; sum is clipped. */
	snd_play(s);
	mixer_render(out, 100);
	CHECK(out[0] == 31000);
	snd_play(s);
	snd_play(s);
	mixer_render(out, 100);
	CHECK(out[0] == 32767);

	/* Volume 64 halves. */
	snd_set_volume(s, 64);
	sMusicValue = 0;
	snd_play(s);
	mixer_render(out, 100);
	CHECK(out[0] == 15000);
	snd_set_volume(s, 999);
	snd_set_volume(NULL, 10);

	/* At most MIXER_CHANNELS sounds at once (Mix_PlayChannel(-1) behaviour). */
	for (int i = 0; i < MIXER_CHANNELS + 3; i++)
		snd_play(s);
	sMusicValue = 0;
	snd_set_volume(s, 1);	/* 30000 * 1 >> 7 = 234 per channel */
	mixer_render(out, 1);
	CHECK(out[0] == 234 * MIXER_CHANNELS);

	/* Sounds end; silence afterwards. */
	mixer_render(out, 100);
	mixer_render(out, 100);
	CHECK(out[0] == 0);

	/* Freeing a playing sound stops it safely. */
	snd_play(s);
	snd_free(s);
	mixer_render(out, 10);
	CHECK(out[0] == 0);

	mixer_set_music_hook(NULL);
	mixer_render(out, 10);
	CHECK(out[0] == 0 && out[19] == 0);
	mixer_close();
	CHECK(!mixer_is_open());
	snd_play(NULL);
}


static void
test_key_names(void)
{
	/* Names used by the game: cheat words compare single letters. */
	CHECK(strcmp(plat_key_name(MOGK_a), "a") == 0);
	CHECK(strcmp(plat_key_name(MOGK_z), "z") == 0);
	CHECK(strcmp(plat_key_name(MOGK_UP), "up") == 0);
	CHECK(strcmp(plat_key_name(MOGK_SPACE), "space") == 0);
	CHECK(strcmp(plat_key_name(MOGK_F1), "f1") == 0);
	CHECK(strcmp(plat_key_name(-1), "unknown key") == 0);
	CHECK(strcmp(plat_key_name(MOGK_LAST), "unknown key") == 0);
	CHECK(MOGK_LAST == 323 && MOGK_SPACE == 32 && MOGK_F12 == 293);
}


static void
test_input_state(void)
{
	plat_set_input_override(false);
	plat_internal_set_key(MOGK_LALT, true);
	CHECK(plat_key_state()[MOGK_LALT] == 1);
	CHECK((plat_modifiers() & PLAT_MOD_ALT) != 0);
	plat_internal_set_key(MOGK_LALT, false);
	CHECK(plat_modifiers() == 0);
	plat_internal_set_key(MOGK_LAST, true);	/* ignored, no overflow */

	plat_set_input_override(true);
	plat_internal_set_key(MOGK_a, true);
	CHECK(plat_key_state()[MOGK_a] == 0);
	plat_set_input_override(false);

	PlatEvent e;
	while (plat_internal_pop_event(&e)) {}
	for (int i = 0; i < 100; i++)
		plat_internal_push_event(PLAT_EV_KEYDOWN, MOGK_a + (i % 26), 0);
	int count = 0;
	while (plat_internal_pop_event(&e)) {
		if (count == 0)
			CHECK(e.key == MOGK_a);
		count++;
	}
	CHECK(count == 64);	/* queue size, oldest events kept */
}


static void
test_palette_and_scaler(void)
{
	char path[512];
	unsigned char rgb[768];
	snprintf(path, sizeof(path), "%s/graphics/original/konami.pcx", sData);
	CHECK(pcx_read_palette(path, rgb));
	CHECK(!pcx_read_palette("/tmp/does-not-exist.pcx", rgb));

	uint8_t pal[768];
	memset(pal, 0, sizeof(pal));
	pal[3] = 0x12; pal[4] = 0x34; pal[5] = 0x56;
	plat_set_palette(pal);
	uint8_t back[768];
	plat_get_palette(back);
	CHECK(memcmp(pal, back, 768) == 0);
	uint8_t src[2] = { 0, 1 };
	uint32_t dst[2];
	plat_internal_convert(src, dst, 2);
	CHECK(dst[0] == 0xff000000u && dst[1] == 0xff123456u);

	/* 2x1 image into 7x4: factor min(7/2, 4/1) = 3. */
	uint32_t img[2] = { 0xffaaaaaa, 0xffbbbbbb };
	uint32_t out[7 * 4];
	int factor, x, y;
	NearestScaler::Layout(2, 1, 7, 4, &factor, &x, &y);
	CHECK(factor == 3 && x == 0 && y == 0);
	NearestScaler scaler;
	scaler.Process(img, 2, 1, 1, out, 7, 4, 7);
	CHECK(out[0] == 0xffaaaaaa && out[2] == 0xffaaaaaa && out[3] == 0xffbbbbbb);
	CHECK(out[5] == 0xffbbbbbb && out[6] == 0);
	CHECK(out[3 * 7] == 0);	/* row 3 is border */

	/* 640x400 into 1920x1080: factor min(3, 2) = 2, centred. */
	NearestScaler::Layout(640, 400, 1920, 1080, &factor, &x, &y);
	CHECK(factor == 2 && x == 320 && y == 140);
	/* Smaller than the source: factor 1, clipped, no crash. */
	NearestScaler::Layout(640, 400, 320, 200, &factor, &x, &y);
	CHECK(factor == 1);
	static uint32_t big[640 * 400];
	static uint32_t small[320 * 200];
	scaler.Process(big, 640, 400, 1, small, 320, 200, 320);
	CHECK(scaler_for_name("anything") != NULL);
}


static uint64_t
fnv(const uint32_t* data, size_t count)
{
	uint64_t h = 1469598103934665603ULL;
	for (size_t i = 0; i < count; i++)
		h = (h ^ data[i]) * 1099511628211ULL;
	return h;
}


static void
test_xbr(void)
{
	/* Pattern checked against the original GLSL shader (WebGL, see
	   tests/xbr/): identical output at factor 3. */
	const int w = 24, h = 24;
	static uint32_t img[w * h];
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			uint32_t c = 0xff202040;
			if (x + y < 20 && x > y / 2)
				c = 0xffe0c040;
			if ((x - 16) * (x - 16) + (y - 14) * (y - 14) < 30)
				c = 0xff40c0e0;
			if (x == 3 && y > 4 && y < 20)
				c = 0xffffffff;
			img[y * w + x] = c;
		}
	}
	static uint32_t out[w * 3 * h * 3];
	XbrScaler::Scale(img, w, h, 3, out);
	CHECK(fnv(out, w * 3 * h * 3) == 0xf1ebc94a7cb4d3b5ULL);

	/* A flat image stays flat; factor 1 is a copy. */
	static uint32_t flat[8 * 8], flatOut[8 * 4 * 8 * 4];
	for (int i = 0; i < 64; i++)
		flat[i] = 0xff336699;
	XbrScaler::Scale(flat, 8, 8, 4, flatOut);
	bool same = true;
	for (int i = 0; i < 8 * 4 * 8 * 4; i++)
		same = same && flatOut[i] == 0xff336699;
	CHECK(same);
	XbrScaler::Scale(img, w, h, 1, out);
	CHECK(memcmp(img, out, sizeof(img)) == 0);

	/* Whole filters: borders black, picture centred, aspect kept. */
	static uint32_t frame[640 * 400];
	for (int i = 0; i < 640 * 400; i++)
		frame[i] = 0xff808080;
	static uint32_t screen[1920 * 1080];
	int x, y, rw, rh;
	scaler_fit(640, 400, 1920, 1080, &x, &y, &rw, &rh);
	CHECK(rw == 1728 && rh == 1080 && x == 96 && y == 0);
	const char* names[] = { "xbr", "sharp" };
	for (int n = 0; n < 2; n++) {
		for (int factor = 1; factor <= 2; factor++) {
			memset(screen, 0x55, sizeof(screen));
			scaler_for_name(names[n])->Process(frame, 640, 400, factor, screen,
				1920, 1080, 1920);
			CHECK(screen[0] == 0 && screen[1919] == 0);
			CHECK(screen[540 * 1920 + 960] == 0xff808080);
			CHECK(screen[540 * 1920 + 96] == 0xff808080);
			CHECK(screen[540 * 1920 + 95] == 0);
		}
	}
	/* The cache (only changed parts filtered again) gives exactly the same
	   picture as filtering every frame completely. */
	{
		static uint32_t art[640 * 400];
		uint32_t seed = 1;
		for (int i = 0; i < 640 * 400; i++) {
			seed = seed * 1103515245u + 12345u;
			/* Blocks of 8x8 with a few colours, like tiles. */
			int block = ((i % 640) / 8 + (i / 640 / 8) * 80) * 7;
			art[i] = 0xff000000u | (uint32_t)((block * 0x3a5b7) & 0xc0c0c0)
				| ((seed >> 28) == 0 ? 0x202020u : 0u);
		}
		const int sizes[][2] = { { 1728, 1080 }, { 1001, 627 } };
		for (int n = 0; n < 2; n++) {
			int dw = sizes[n][0], dh = sizes[n][1];
			std::vector<uint32_t> a((size_t)dw * dh), b((size_t)dw * dh);
			XbrScaler cached;
			cached.SetTileBudget(0);
			bool equal = true;
			int partial = 0;
			for (int f = 0; f < 12; f++) {
				/* A moving sprite, and a full change in frame 6. */
				for (int y = 0; y < 24; y++) {
					for (int x = 0; x < 16; x++) {
						int px = 37 + f * 9 + x, py = 101 + f * 5 + y;
						art[py * 640 + px] = (x + y + f) % 3 == 0
							? 0xffffffffu : 0xff20a040u;
					}
				}
				if (f == 6) {
					for (int i = 0; i < 640 * 400; i++)
						art[i] ^= 0x00101010u;
				}
				XbrScaler fresh;
				cached.Process(art, 640, 400, 1, a.data(), dw, dh, dw);
				fresh.Process(art, 640, 400, 1, b.data(), dw, dh, dw);
				equal = equal && a == b;
				if (cached.LastDirtyTiles() < cached.TileCount())
					partial++;
			}
			CHECK(equal);
			CHECK(partial == 10);
		}

		/* With a budget a large change is finished over several frames
		   (sharp bilinear meanwhile); then the picture is identical. */
		std::vector<uint32_t> a(1728 * 1080), b(1728 * 1080);
		XbrScaler budgeted;
		budgeted.SetTileBudget(100);
		budgeted.Process(art, 640, 400, 1, a.data(), 1728, 1080, 1728);
		for (int i = 0; i < 640 * 400; i++)
			art[i] ^= 0x00080808u;
		budgeted.Process(art, 640, 400, 1, a.data(), 1728, 1080, 1728);
		CHECK(budgeted.PendingTiles() == budgeted.TileCount() - 100);
		CHECK(budgeted.LastDirtyTiles() == 100);
		int frames = 1;
		while (budgeted.PendingTiles() > 0 && frames < 50) {
			budgeted.Process(art, 640, 400, 1, a.data(), 1728, 1080, 1728);
			frames++;
		}
		CHECK(frames == (budgeted.TileCount() + 99) / 100);
		XbrScaler fresh;
		fresh.Process(art, 640, 400, 1, b.data(), 1728, 1080, 1728);
		CHECK(a == b);
	}

	CHECK(strcmp(scaler_for_name("unknown")->Name(), "xbr") == 0);
	CHECK(strcmp(scaler_next(scaler_for_name("xbr"))->Name(), "nearest") == 0);
	CHECK(strcmp(scaler_next(scaler_for_name("nearest"))->Name(), "sharp") == 0);
	CHECK(strcmp(scaler_next(scaler_for_name("sharp"))->Name(), "xbr") == 0);
}


static void
test_source_factor(void)
{
	char path[512];
	snprintf(path, sizeof(path), "%s/graphics/original/tiles.pcx", sData);
	CHECK(pcx_source_factor(path) == 2);
	CHECK(pcx_source_factor("/tmp/does-not-exist.pcx") == 1);

	/* A 4x2 PCX without doubled pixels. */
	unsigned char header[128];
	memset(header, 0, sizeof(header));
	header[0] = 10; header[1] = 5; header[2] = 1; header[3] = 8;
	header[8] = 3; header[10] = 1;	/* xmax 3, ymax 1 */
	header[65] = 1; header[66] = 4;	/* planes, bytes per line */
	unsigned char data[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	FILE* fp = fopen("/tmp/mog_t5.pcx", "wb");
	fwrite(header, 1, 128, fp);
	fwrite(data, 1, 8, fp);
	fclose(fp);
	CHECK(pcx_source_factor("/tmp/mog_t5.pcx") == 1);
	/* Same size, doubled: 1 1 2 2 / 1 1 2 2 (RLE runs of 2). */
	unsigned char doubled[8] = { 0xC2, 1, 0xC2, 2, 0xC2, 1, 0xC2, 2 };
	fp = fopen("/tmp/mog_t5.pcx", "wb");
	fwrite(header, 1, 128, fp);
	fwrite(doubled, 1, 8, fp);
	fclose(fp);
	CHECK(pcx_source_factor("/tmp/mog_t5.pcx") == 2);

	plat_set_source_factor(2);
	CHECK(plat_internal_source_factor() == 2);
	plat_set_source_factor(7);
	CHECK(plat_internal_source_factor() == 1);
	plat_set_scaler("sharp");
	CHECK(strcmp(plat_scaler_name(), "sharp") == 0);
	plat_next_scaler();
	CHECK(strcmp(plat_scaler_name(), "xbr") == 0);
}


int
main(int argc, char** argv)
{
	if (argc > 1)
		sData = argv[1];
	test_wav_decoding();
	test_resampler();
	test_stream_matches_sound("original/demon2.ogg");	/* 44100 */
	test_stream_matches_sound("original/tomb1.ogg");	/* 11025 */
	test_stream_matches_sound("original/bell.wav");		/* 22050 wav */
	test_mixer();
	test_key_names();
	test_input_state();
	test_palette_and_scaler();
	test_xbr();
	test_source_factor();
	printf("%d checks, %d failures\n", sChecks, sFailures);
	return sFailures == 0 ? 0 : 1;
}

/* Platform stubs (no device). */
bool plat_audio_open(int, PlatAudioCallback, void*) { return false; }
void plat_audio_close(void) {}
void plat_audio_lock(void) {}
void plat_audio_unlock(void) {}
const char* plat_settings_dir(void) { return "/tmp"; }
void plat_backend_scaler_changed(void) {}
