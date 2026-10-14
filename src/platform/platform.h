/*
 * Platform layer for the Maze of Galious remake.
 *
 * The game code only talks to this interface. There are two back ends:
 *   platform_sdl.cpp    Linux development build (SDL 1.2, video/input/audio out only)
 *   platform_haiku.cpp  native Haiku / RenkuOS build (Be API, no SDL)
 *
 * The back end owns main(). It initialises itself and then calls game_main()
 * on the thread that runs the game.
 */
#ifndef MOG_PLATFORM_H
#define MOG_PLATFORM_H

#include <stdint.h>

#include "mogkeys.h"


/* Implemented by the game (main.cpp). */
int game_main(int argc, char** argv);


/* Lifecycle ------------------------------------------------------------- */

/* Creates the window (width x height logical pixels, 8-bit indexed). */
bool plat_init(int width, int height, bool fullscreen, const char* title);
void plat_shutdown(void);
const char* plat_name(void);


/* Video ----------------------------------------------------------------- */

/* 256 entries, 3 bytes each (R, G, B). */
void plat_set_palette(const uint8_t* rgb);
void plat_get_palette(uint8_t* rgb);

/* Shows an 8-bit indexed frame of the size given to plat_init(). */
void plat_present(const uint8_t* pixels);

bool plat_set_fullscreen(bool fullscreen);
bool plat_is_fullscreen(void);


/* Time ------------------------------------------------------------------ */

uint32_t plat_ticks_ms(void);
void plat_sleep_ms(uint32_t ms);


/* Input ----------------------------------------------------------------- */

enum {
	PLAT_EV_NONE = 0,
	PLAT_EV_KEYDOWN,
	PLAT_EV_QUIT
};

enum {
	PLAT_MOD_SHIFT = 1,
	PLAT_MOD_CTRL = 2,
	PLAT_MOD_ALT = 4,
	PLAT_MOD_META = 8
};

struct PlatEvent {
	int		type;
	MOGKey	key;
	int		modifiers;
};

/* Returns false when there are no more events. */
bool plat_poll_event(PlatEvent* event);

/* Refreshes the key state array returned by plat_key_state(). */
void plat_pump_events(void);

/* MOGK_LAST entries, nonzero when the key is held. */
unsigned char* plat_key_state(void);
int plat_modifiers(void);

/* While enabled, the back end leaves the key state array alone (used by the
   deterministic test driver, which writes the key state itself). */
void plat_set_input_override(bool enabled);

/* Same strings as SDL 1.2 SDL_GetKeyName(). */
const char* plat_key_name(MOGKey key);


/* Files ----------------------------------------------------------------- */

/* Directory that contains graphics/, sound/ and rooms/. The back end makes it
   the current directory before game_main() runs. */
const char* plat_data_dir(void);

/* Writable directory for MoG.cfg and other user files (created if needed). */
const char* plat_settings_dir(void);


/* Audio ----------------------------------------------------------------- */

/* Called from the audio thread: fill 'frames' stereo frames (interleaved
   int16, 44100 Hz). */
typedef void (*PlatAudioCallback)(int16_t* out, int frames, void* cookie);

bool plat_audio_open(int rate, PlatAudioCallback callback, void* cookie);
void plat_audio_close(void);

/* Protects data shared with the audio callback. */
void plat_audio_lock(void);
void plat_audio_unlock(void);

#endif	/* MOG_PLATFORM_H */
