/*
 * SDL 1.2 back end, for development and testing on Linux only.
 *
 * Uses SDL for the window, keyboard and audio device. Decoding, mixing,
 * palette conversion and scaling are the same portable code the native
 * Haiku/RenkuOS back end uses. SDL_mixer, SDL_sound and SDL_image are not
 * used.
 */
#include "platform_internal.h"
#include "video_scaler.h"

#include <SDL.h>

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


namespace {

SDL_Surface* sScreen = NULL;
int sWidth = 0;
int sHeight = 0;
bool sFullscreen = false;
const char* sTitle = "";
uint32_t* sConverted = NULL;
Scaler* sScaler = NULL;

PlatAudioCallback sAudioCallback = NULL;
void* sAudioCookie = NULL;
bool sAudioOpen = false;

char sDataDir[PATH_MAX];
char sSettingsDir[PATH_MAX];


bool
set_video_mode(bool fullscreen)
{
	int width = sWidth * 2;
	int height = sHeight * 2;
	Uint32 flags = SDL_SWSURFACE;
	if (fullscreen) {
		const SDL_VideoInfo* info = SDL_GetVideoInfo();
		if (info != NULL && info->current_w > 0) {
			width = info->current_w;
			height = info->current_h;
		}
		flags |= SDL_FULLSCREEN;
	}
	SDL_Surface* screen = SDL_SetVideoMode(width, height, 32, flags);
	if (screen == NULL) {
		fprintf(stderr, "SDL_SetVideoMode: %s\n", SDL_GetError());
		return false;
	}
	sScreen = screen;
	sFullscreen = fullscreen;
	SDL_WM_SetCaption(sTitle, NULL);
	SDL_ShowCursor(fullscreen ? SDL_DISABLE : SDL_ENABLE);
	return true;
}


int
modifiers_from_sdl(SDLMod mod)
{
	int modifiers = 0;
	if (mod & KMOD_SHIFT)
		modifiers |= PLAT_MOD_SHIFT;
	if (mod & KMOD_CTRL)
		modifiers |= PLAT_MOD_CTRL;
	if (mod & KMOD_ALT)
		modifiers |= PLAT_MOD_ALT;
	if (mod & KMOD_META)
		modifiers |= PLAT_MOD_META;
	return modifiers;
}


void
process_sdl_events(void)
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_KEYDOWN:
				/* SDL 1.2 key symbols have the same values as MOGK_*. */
				plat_internal_set_key((MOGKey)event.key.keysym.sym, true);
				plat_internal_push_event(PLAT_EV_KEYDOWN,
					(MOGKey)event.key.keysym.sym,
					modifiers_from_sdl(event.key.keysym.mod));
				break;
			case SDL_KEYUP:
				plat_internal_set_key((MOGKey)event.key.keysym.sym, false);
				break;
			case SDL_ACTIVEEVENT:
				if ((event.active.state & SDL_APPINPUTFOCUS) != 0
					&& event.active.gain == 0)
					plat_internal_clear_keys();
				break;
			case SDL_QUIT:
				plat_internal_push_event(PLAT_EV_QUIT, 0, 0);
				break;
		}
	}
}


void
audio_trampoline(void* /*userdata*/, Uint8* stream, int length)
{
	/* SDL holds the audio lock while calling this. */
	if (sAudioCallback != NULL)
		sAudioCallback((int16_t*)stream, length / 4, sAudioCookie);
	else
		memset(stream, 0, (size_t)length);
}


bool
has_game_data(const char* dir)
{
	char path[PATH_MAX];
	struct stat st;
	snprintf(path, sizeof(path), "%s/graphics", dir);
	return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}


void
find_data_dir(const char* argv0)
{
	const char* env = getenv("MOG_DATA");
	if (env != NULL && has_game_data(env)) {
		snprintf(sDataDir, sizeof(sDataDir), "%s", env);
		return;
	}
	if (has_game_data(".")) {
		snprintf(sDataDir, sizeof(sDataDir), ".");
		return;
	}
	char exe[PATH_MAX];
	ssize_t length = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
	if (length <= 0) {
		snprintf(exe, sizeof(exe), "%s", argv0);
		length = (ssize_t)strlen(exe);
	}
	exe[length] = '\0';
	char* slash = strrchr(exe, '/');
	if (slash != NULL)
		*slash = '\0';
	snprintf(sDataDir, sizeof(sDataDir), "%s/data", exe);
	if (!has_game_data(sDataDir))
		snprintf(sDataDir, sizeof(sDataDir), "%s", exe);
}

}	// namespace


int
main(int argc, char** argv)
{
	find_data_dir(argv[0]);
	if (chdir(sDataDir) != 0) {
		fprintf(stderr, "Cannot use data directory %s: %s\n", sDataDir,
			strerror(errno));
		return 1;
	}
	const char* home = getenv("HOME");
	snprintf(sSettingsDir, sizeof(sSettingsDir), "%s/.mog",
		home != NULL ? home : ".");
	mkdir(sSettingsDir, 0755);

	return game_main(argc, argv);
}


bool
plat_init(int width, int height, bool fullscreen, const char* title)
{
	sWidth = width;
	sHeight = height;
	sTitle = title;
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		return false;
	}
	SDL_EnableUNICODE(1);
	sConverted = new uint32_t[(size_t)width * height];
	sScaler = scaler_for_name("nearest");
	return set_video_mode(fullscreen);
}


void
plat_shutdown(void)
{
	plat_audio_close();
	delete[] sConverted;
	sConverted = NULL;
	SDL_Quit();
}


const char*
plat_name(void)
{
	return "SDL 1.2 (Linux development back end)";
}


void
plat_present(const uint8_t* pixels)
{
	if (sScreen == NULL)
		return;
	plat_internal_convert(pixels, sConverted, sWidth * sHeight);
	if (SDL_MUSTLOCK(sScreen))
		SDL_LockSurface(sScreen);
	if (sScreen->format->BytesPerPixel == 4) {
		sScaler->Process(sConverted, sWidth, sHeight, 1,
			(uint32_t*)sScreen->pixels, sScreen->w, sScreen->h,
			sScreen->pitch / 4);
	}
	if (SDL_MUSTLOCK(sScreen))
		SDL_UnlockSurface(sScreen);
	SDL_Flip(sScreen);
}


bool
plat_set_fullscreen(bool fullscreen)
{
	if (!set_video_mode(fullscreen))
		set_video_mode(sFullscreen);
	return sFullscreen;
}


bool
plat_is_fullscreen(void)
{
	return sFullscreen;
}


uint32_t
plat_ticks_ms(void)
{
	return SDL_GetTicks();
}


void
plat_sleep_ms(uint32_t ms)
{
	SDL_Delay(ms);
}


bool
plat_poll_event(PlatEvent* event)
{
	process_sdl_events();
	return plat_internal_pop_event(event);
}


void
plat_pump_events(void)
{
	process_sdl_events();
}


const char*
plat_data_dir(void)
{
	return sDataDir;
}


const char*
plat_settings_dir(void)
{
	return sSettingsDir;
}


bool
plat_audio_open(int rate, PlatAudioCallback callback, void* cookie)
{
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
		return false;
	SDL_AudioSpec wanted;
	memset(&wanted, 0, sizeof(wanted));
	wanted.freq = rate;
	wanted.format = AUDIO_S16SYS;
	wanted.channels = 2;
	wanted.samples = 1024;
	wanted.callback = audio_trampoline;
	sAudioCallback = callback;
	sAudioCookie = cookie;
	/* NULL 'obtained': SDL converts to the device format if needed. */
	if (SDL_OpenAudio(&wanted, NULL) < 0) {
		fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
		sAudioCallback = NULL;
		return false;
	}
	sAudioOpen = true;
	SDL_PauseAudio(0);
	return true;
}


void
plat_audio_close(void)
{
	if (!sAudioOpen)
		return;
	SDL_CloseAudio();
	sAudioOpen = false;
	sAudioCallback = NULL;
}


void
plat_audio_lock(void)
{
	if (sAudioOpen)
		SDL_LockAudio();
}


void
plat_audio_unlock(void)
{
	if (sAudioOpen)
		SDL_UnlockAudio();
}
