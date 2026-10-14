#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform/platform.h"
#include "sound.h"

#include "bitmaps.h"
#include "sprites.h"
#include "tiles.h"

#include "mog.h"

#include "debug.h"

extern void DebugReport(void);

#ifdef MOG_TESTHOOKS
#include "test/testhooks.h"
extern unsigned char* buffer_screen;
#endif

int SCREEN_X = 640;
int SCREEN_Y = 400;
int COLOUR_DEPTH = 8;

#define TRANSPARANT_COLOR (0)

// Port: start in a window (upstream started full screen); Alt+Enter or
// --fullscreen switches.
bool fullscreen = false;

// Redrawing constant
int REDRAWING_PERIOD = 40;

// Frames per second counter
float frames_per_sec = 0.0;
int frames_per_sec_tmp = 0;
int init_time = 0;

// Paths
char *default_g_path = "graphics/original/";
char *default_s_path = "sound/original/";
char **g_paths = 0;
int n_g_paths = 0, act_g_path = 0;
char *g_path = 0;
char **s_paths = 0;
int n_s_paths = 0, act_s_path = 0;
char *s_path = 0;

extern int music_volume, sfx_volume;
extern int fighting_demon;


void pause(unsigned int time)
{
	/* Port: sleep instead of busy waiting. */
	plat_sleep_ms(time);
} 


/* Pantalla: */ 

void Render(void);
static bool initialize_platform(void);
static void finalize_platform(void);


/* Port: the platform back end owns main() and calls game_main() on the game
   thread. The window, input and timing go through platform.h. */
int game_main(int argc, char** argv)
{
	int time, act_time;
	PlatEvent event;
	bool quit = false;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--window") == 0 || strcmp(argv[i], "-w") == 0)
			fullscreen = false;
		else if (strcmp(argv[i], "--fullscreen") == 0 || strcmp(argv[i], "-f") == 0)
			fullscreen = true;
	}

	if (!initialize_platform()) {
		return 1;
	}

	time = init_time = plat_ticks_ms();

#ifdef MOG_TESTHOOKS
	if (th_active())
		plat_set_input_override(true);
#endif

	GameInit(SCREEN_X, SCREEN_Y);

	while (!quit) {
	
		while (plat_poll_event(&event)) {
			if (event.type == PLAT_EV_QUIT) {
				quit = true;
				continue;
			}
			if (event.type != PLAT_EV_KEYDOWN)
				continue;

			// default quit: F12
			if (event.key == MOGK_F12) {
				quit = true;
			} /* if */

			// Alt+Enter: toggle full screen (upstream: also Apple+F on OS X)
			if (event.key == MOGK_RETURN && (event.modifiers & PLAT_MOD_ALT) != 0) {
				fullscreen = plat_set_fullscreen(!fullscreen);
			}

			// Change graphic set with either F10 or 9
			if (event.key == MOGK_F10 || event.key == MOGK_9) {

				act_g_path++;
				if (act_g_path >= n_g_paths) {
					act_g_path = 0;
				}
				
				g_path = g_paths[act_g_path];
				ReleaseGraphics();
				ReloadGraphics(SCREEN_X, SCREEN_Y);
				guardar_configuracion("MoG.cfg");
				
				if (fighting_demon != 0) {
					redo_demonintro(fighting_demon, 0, SCREEN_X, SCREEN_Y);
				}
			} 

			// Change sound set with either F11 or 0
			if (event.key == MOGK_F11 || event.key == MOGK_0) {

				act_s_path++;
				if (act_s_path >= n_s_paths) {
					act_s_path = 0;
				}
				
				s_path = s_paths[act_s_path];
				ReleaseSound(false);
				ReloadSound();
				music_recovery();
				guardar_configuracion("MoG.cfg");
				SetSFXVolume(sfx_volume);
			}

			if (event.key == MOGK_d) {
				write_debug_report("debug-report.txt");
			} 
		}

#ifdef MOG_TESTHOOKS
		if (th_active()) {
			// Deterministic run: exactly one game cycle per iteration.
			th_apply_input(plat_key_state(), MOGK_LAST);
			Render();
			unsigned char pal[768];
			plat_get_palette(pal);
			th_audio_frame();
			if (th_end_frame(buffer_screen, SCREEN_X, SCREEN_Y, pal))
				quit = true;
			continue;
		}
#endif
		// game main loop
		act_time = plat_ticks_ms();
		if (!quit && act_time - time >= REDRAWING_PERIOD) {
			int max_frame_step = 10;
			do {
				time += REDRAWING_PERIOD;
				if ((act_time - time) > 10 * REDRAWING_PERIOD) {
					time = act_time;
				}

				// render graphics
				Render();

				act_time = plat_ticks_ms();
				max_frame_step--;
			} while (act_time - time >= REDRAWING_PERIOD && max_frame_step > 0);

		}
		if ((act_time - init_time) >= 1000) {
			frames_per_sec = frames_per_sec_tmp;
			frames_per_sec_tmp = 0;
			init_time = act_time;
		}
		
		plat_sleep_ms(1);
	}

	finalize_platform();

	return 0;
}


static bool initialize_platform(void)
{
	output_debug_message("Platform: %s\n", plat_name());

	if (!plat_init(SCREEN_X, SCREEN_Y, fullscreen, "Maze of Galious v0.63")) {
		output_debug_message("Couldn't open a %ix%i window\n", SCREEN_X, SCREEN_Y);
		return false;
	}

	Sound_initialization();

	pause(1000);

	return true;
}


static void finalize_platform(void)
{
	ReleaseSound(true);
	ReleaseGraphics();
	Sound_release();
	plat_shutdown();
}
