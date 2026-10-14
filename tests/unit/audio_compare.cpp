/*
 * Compares the portable decoders with SDL_mixer (Mix_LoadWAV) for every sound
 * file of a sound set. Development tool, Linux only.
 * usage: audio_compare <file>...
 * Prints per file: frames (ours / SDL_mixer) and the error relative to the
 * signal level (RMS of difference / RMS of SDL_mixer output).
 */
#include <SDL.h>
#include <SDL_mixer.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../../src/platform/audio_mixer.h"
#include "../../src/platform/platform.h"

struct MogSoundView { int16_t* data; int frames; int volume; };

int
main(int argc, char** argv)
{
	SDL_Init(SDL_INIT_AUDIO);
	if (Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 1024) != 0) {
		fprintf(stderr, "Mix_OpenAudio: %s\n", Mix_GetError());
		return 1;
	}
	int worst = 0;
	for (int i = 1; i < argc; i++) {
		Mix_Chunk* chunk = Mix_LoadWAV(argv[i]);
		MogSound* ours = snd_load(argv[i]);
		if (chunk == NULL || ours == NULL) {
			printf("%-40s LOAD FAILED (sdl=%p ours=%p)\n", argv[i], (void*)chunk,
				(void*)ours);
			worst = 2;
			continue;
		}
		MogSoundView* view = (MogSoundView*)ours;
		int sdlFrames = (int)(chunk->alen / 4);
		const int16_t* ref = (const int16_t*)chunk->abuf;
		int n = sdlFrames < view->frames ? sdlFrames : view->frames;
		double se = 0, ss = 0;
		for (int j = 0; j < n * 2; j++) {
			double d = (double)view->data[j] - ref[j];
			se += d * d;
			ss += (double)ref[j] * ref[j];
		}
		double rel = ss > 0 ? sqrt(se / ss) : 0;
		printf("%-40s frames %7d / %7d  rel.err %.4f\n", argv[i], view->frames,
			sdlFrames, rel);
		if (rel > 0.25 || abs(view->frames - sdlFrames) > 64)
			worst = worst < 1 ? 1 : worst;
		Mix_FreeChunk(chunk);
		snd_free(ours);
	}
	Mix_CloseAudio();
	return worst;
}

/* Stubs for the platform functions the mixer references. */
bool plat_audio_open(int, PlatAudioCallback, void*) { return false; }
void plat_audio_close(void) {}
void plat_audio_lock(void) {}
void plat_audio_unlock(void) {}
