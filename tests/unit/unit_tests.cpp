/*
 * Unit tests for the portable platform code (no Be API needed).
 * Built and run by "make test".
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/platform/audio_mixer.h"
#include "../../src/platform/platform.h"
#include "../../src/platform/platform_internal.h"
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

	/* 8 bit unsigned 22050: twice as many frames, 128 -> 0. */
	unsigned char u8[3] = { 128, 255, 0 };
	write_wav("/tmp/mog_t2.wav", 1, 22050, 8, u8, 3);
	s = (MogSoundView*)snd_load("/tmp/mog_t2.wav");
	CHECK(s != NULL);
	if (s != NULL) {
		CHECK(s->frames == 6);
		CHECK(s->data[0] == 0);
		CHECK(s->data[4] == 127 * 256);
		CHECK(s->data[8] == -128 * 256);
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


int
main(int argc, char** argv)
{
	if (argc > 1)
		sData = argv[1];
	test_wav_decoding();
	test_stream_matches_sound("original/demon2.ogg");	/* 44100 */
	test_stream_matches_sound("original/tomb1.ogg");	/* 11025 */
	test_stream_matches_sound("original/bell.wav");		/* 22050 wav */
	test_mixer();
	test_key_names();
	test_input_state();
	test_palette_and_scaler();
	printf("%d checks, %d failures\n", sChecks, sFailures);
	return sFailures == 0 ? 0 : 1;
}

/* Platform stubs (no device). */
bool plat_audio_open(int, PlatAudioCallback, void*) { return false; }
void plat_audio_close(void) {}
void plat_audio_lock(void) {}
void plat_audio_unlock(void) {}
const char* plat_settings_dir(void) { return "/tmp"; }
