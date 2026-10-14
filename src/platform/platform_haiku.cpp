/*
 * Native Haiku / RenkuOS back end (Be API, no SDL).
 *
 *   window/video  BApplication, BWindow, BView, BBitmap (B_RGB32)
 *   keyboard      get_key_info() polling while the window is active
 *   audio         BSoundPlayer, feeding the portable mixer
 *   timing        system_time(), snooze()
 *   files         find_directory(), app directory via app_info
 *
 * The game runs on its own thread (game_main()); the BApplication message
 * loop runs on the main thread.
 */
#include "platform_internal.h"
#include "video_scaler.h"

#include <Application.h>
#include <Bitmap.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Locker.h>
#include <MediaDefs.h>
#include <OS.h>
#include <Path.h>
#include <Roster.h>
#include <Screen.h>
#include <SoundPlayer.h>
#include <View.h>
#include <Window.h>

#include <atomic>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


static const char* kSignature = "application/x-vnd.BrainGames-MazeOfGalious";
static const char* kSettingsName = "MazeOfGalious";

enum {
	kMsgRedraw = 'mgRd'
};


/* Haiku raw key codes (physical keys) -> MOGK_*. */
static const MOGKey kRawToMog[128] = {
	/* 0x00 */ 0, MOGK_ESCAPE, MOGK_F1, MOGK_F2, MOGK_F3, MOGK_F4, MOGK_F5,
	/* 0x07 */ MOGK_F6, MOGK_F7, MOGK_F8, MOGK_F9, MOGK_F10, MOGK_F11,
	/* 0x0d */ MOGK_F12, MOGK_PRINT, MOGK_SCROLLOCK, MOGK_PAUSE,
	/* 0x11 */ MOGK_BACKQUOTE, MOGK_1, MOGK_2, MOGK_3, MOGK_4, MOGK_5, MOGK_6,
	/* 0x18 */ MOGK_7, MOGK_8, MOGK_9, MOGK_0, MOGK_MINUS, MOGK_EQUALS,
	/* 0x1e */ MOGK_BACKSPACE, MOGK_INSERT, MOGK_HOME, MOGK_PAGEUP,
	/* 0x22 */ MOGK_NUMLOCK, MOGK_KP_DIVIDE, MOGK_KP_MULTIPLY, MOGK_KP_MINUS,
	/* 0x26 */ MOGK_TAB, MOGK_q, MOGK_w, MOGK_e, MOGK_r, MOGK_t, MOGK_y,
	/* 0x2d */ MOGK_u, MOGK_i, MOGK_o, MOGK_p, MOGK_LEFTBRACKET,
	/* 0x32 */ MOGK_RIGHTBRACKET, MOGK_BACKSLASH, MOGK_DELETE, MOGK_END,
	/* 0x36 */ MOGK_PAGEDOWN, MOGK_KP7, MOGK_KP8, MOGK_KP9, MOGK_KP_PLUS,
	/* 0x3b */ MOGK_CAPSLOCK, MOGK_a, MOGK_s, MOGK_d, MOGK_f, MOGK_g, MOGK_h,
	/* 0x42 */ MOGK_j, MOGK_k, MOGK_l, MOGK_SEMICOLON, MOGK_QUOTE,
	/* 0x47 */ MOGK_RETURN, MOGK_KP4, MOGK_KP5, MOGK_KP6, MOGK_LSHIFT,
	/* 0x4c */ MOGK_z, MOGK_x, MOGK_c, MOGK_v, MOGK_b, MOGK_n, MOGK_m,
	/* 0x53 */ MOGK_COMMA, MOGK_PERIOD, MOGK_SLASH, MOGK_RSHIFT, MOGK_UP,
	/* 0x58 */ MOGK_KP1, MOGK_KP2, MOGK_KP3, MOGK_KP_ENTER, MOGK_LCTRL,
	/* 0x5d */ MOGK_LALT, MOGK_SPACE, MOGK_RALT, MOGK_RCTRL, MOGK_LEFT,
	/* 0x62 */ MOGK_DOWN, MOGK_RIGHT, MOGK_KP0, MOGK_KP_PERIOD, MOGK_LSUPER,
	/* 0x67 */ MOGK_RSUPER, MOGK_MENU, MOGK_LESS, MOGK_KP_EQUALS, MOGK_POWER,
	/* 0x6c */ 0, 0, 0, 0,
	/* 0x70 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


class MoGView;
class MoGWindow;

namespace {

MoGWindow* sWindow = NULL;
MoGView* sView = NULL;
std::atomic<bool> sActive(false);
std::atomic<bool> sRedrawPending(false);
std::atomic<int> sViewWidth(0);
std::atomic<int> sViewHeight(0);
std::atomic<bool> sGameDone(false);

int sWidth = 0;
int sHeight = 0;
bool sFullscreen = false;
bool sCursorHidden = false;
BRect sWindowedFrame;
char sTitle[128] = "";
uint32* sConverted = NULL;

/* Front bitmap is drawn by the window thread, back bitmap is written by the
   game thread. sFrameLock protects the swap and the drawing. */
BLocker sFrameLock("mog frame");
BBitmap* sFront = NULL;
BBitmap* sBack = NULL;

uint8 sRawKeys[16];

BLocker sAudioLock("mog audio");
BSoundPlayer* sPlayer = NULL;
PlatAudioCallback sAudioCallback = NULL;
void* sAudioCookie = NULL;
int16_t* sAudioTemp = NULL;
size_t sAudioTempFrames = 0;

char sDataDir[B_PATH_NAME_LENGTH];
char sSettingsDir[B_PATH_NAME_LENGTH];

int sArgc = 0;
char** sArgv = NULL;
thread_id sGameThread = -1;
int sExitCode = 0;

}	// namespace


/* View ------------------------------------------------------------------ */

class MoGView : public BView {
public:
	MoGView(BRect frame)
		:
		BView(frame, "game", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS)
	{
		SetViewColor(B_TRANSPARENT_COLOR);
		sViewWidth = frame.IntegerWidth() + 1;
		sViewHeight = frame.IntegerHeight() + 1;
	}

	virtual void AttachedToWindow()
	{
		MakeFocus(true);
	}

	virtual void Draw(BRect /*updateRect*/)
	{
		BRect bounds = Bounds();
		sFrameLock.Lock();
		if (sFront == NULL) {
			SetHighColor(0, 0, 0);
			FillRect(bounds);
		} else if (sFront->Bounds() == bounds) {
			DrawBitmap(sFront, BPoint(0, 0));
		} else {
			/* Size changed, the next frame will match again. */
			DrawBitmap(sFront, sFront->Bounds(), bounds);
		}
		/* Wait until the app_server has used the bitmap before the game
		   thread may write into it again. */
		Sync();
		sFrameLock.Unlock();
	}

	virtual void FrameResized(float width, float height)
	{
		sViewWidth = (int)width + 1;
		sViewHeight = (int)height + 1;
		Invalidate();
	}

	/* The game reads the keyboard state itself; swallow key messages so
	   they do not trigger anything else. */
	virtual void KeyDown(const char* /*bytes*/, int32 /*numBytes*/) {}
	virtual void KeyUp(const char* /*bytes*/, int32 /*numBytes*/) {}
};


/* Window ---------------------------------------------------------------- */

class MoGWindow : public BWindow {
public:
	MoGWindow(BRect frame, const char* title)
		:
		BWindow(frame, title, B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
	{
		sView = new MoGView(Bounds());
		AddChild(sView);
	}

	virtual bool QuitRequested()
	{
		/* Let the game shut down cleanly; plat_shutdown() closes us. */
		plat_internal_push_event(PLAT_EV_QUIT, 0, 0);
		return false;
	}

	virtual void WindowActivated(bool active)
	{
		sActive = active;
		BWindow::WindowActivated(active);
	}

	virtual void MessageReceived(BMessage* message)
	{
		if (message->what == kMsgRedraw) {
			sRedrawPending = false;
			sView->Invalidate();
			return;
		}
		BWindow::MessageReceived(message);
	}
};


/* Application ----------------------------------------------------------- */

static int32
game_thread(void* /*data*/)
{
	sExitCode = game_main(sArgc, sArgv);
	sGameDone = true;
	be_app->PostMessage(B_QUIT_REQUESTED);
	return sExitCode;
}


class MoGApp : public BApplication {
public:
	MoGApp()
		:
		BApplication(kSignature)
	{
	}

	virtual void ReadyToRun()
	{
		sGameThread = spawn_thread(game_thread, "MoG game", B_DISPLAY_PRIORITY,
			NULL);
		if (sGameThread < B_OK || resume_thread(sGameThread) != B_OK) {
			fprintf(stderr, "Cannot start the game thread\n");
			sGameDone = true;
			Quit();
		}
	}

	virtual bool QuitRequested()
	{
		if (!sGameDone) {
			plat_internal_push_event(PLAT_EV_QUIT, 0, 0);
			return false;
		}
		return true;
	}
};


/* Paths ----------------------------------------------------------------- */

static bool
has_game_data(const char* dir)
{
	char path[B_PATH_NAME_LENGTH];
	struct stat st;
	snprintf(path, sizeof(path), "%s/graphics", dir);
	return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}


static bool
find_data_dir(void)
{
	const char* env = getenv("MOG_DATA");
	if (env != NULL && has_game_data(env)) {
		snprintf(sDataDir, sizeof(sDataDir), "%s", env);
		return true;
	}

	/* Next to the executable (development, and the package layout). */
	app_info info;
	BPath appDir;
	if (be_app->GetAppInfo(&info) == B_OK) {
		BEntry entry(&info.ref);
		if (entry.GetPath(&appDir) == B_OK && appDir.GetParent(&appDir) == B_OK) {
			if (has_game_data(appDir.Path())) {
				snprintf(sDataDir, sizeof(sDataDir), "%s", appDir.Path());
				return true;
			}
			BPath data(appDir.Path(), "data");
			if (has_game_data(data.Path())) {
				snprintf(sDataDir, sizeof(sDataDir), "%s", data.Path());
				return true;
			}
		}
	}

	/* Shared data directories. */
	const directory_which kDirs[] = {
		B_USER_NONPACKAGED_DATA_DIRECTORY,
		B_SYSTEM_NONPACKAGED_DATA_DIRECTORY,
		B_SYSTEM_DATA_DIRECTORY
	};
	for (size_t i = 0; i < sizeof(kDirs) / sizeof(kDirs[0]); i++) {
		BPath path;
		if (find_directory(kDirs[i], &path) != B_OK)
			continue;
		path.Append(kSettingsName);
		if (has_game_data(path.Path())) {
			snprintf(sDataDir, sizeof(sDataDir), "%s", path.Path());
			return true;
		}
	}
	return false;
}


static void
find_settings_dir(void)
{
	const char* env = getenv("MOG_SETTINGS");
	if (env != NULL) {
		create_directory(env, 0755);
		snprintf(sSettingsDir, sizeof(sSettingsDir), "%s", env);
		return;
	}
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) == B_OK) {
		path.Append(kSettingsName);
		create_directory(path.Path(), 0755);
		snprintf(sSettingsDir, sizeof(sSettingsDir), "%s", path.Path());
	} else {
		snprintf(sSettingsDir, sizeof(sSettingsDir), "/tmp");
	}
}


int
main(int argc, char** argv)
{
	sArgc = argc;
	sArgv = argv;

	MoGApp* app = new MoGApp();

	find_settings_dir();
	if (!find_data_dir()) {
		fprintf(stderr, "Maze of Galious: game data (graphics, sound, rooms) "
			"not found.\n");
		delete app;
		return 1;
	}
	if (chdir(sDataDir) != 0) {
		fprintf(stderr, "Cannot use data directory %s: %s\n", sDataDir,
			strerror(errno));
		delete app;
		return 1;
	}

	app->Run();
	delete app;

	if (sGameThread >= B_OK) {
		status_t result;
		wait_for_thread(sGameThread, &result);
	}
	return sExitCode;
}


/* Video ----------------------------------------------------------------- */

static BRect
windowed_frame(void)
{
	/* As large as fits comfortably on the screen (90% of the width, 85% of
	   the height), keeping 16:10; at least the game's own size. The
	   scaler fills any window size. */
	BScreen screen;
	BRect screenFrame = screen.Frame();
	float maxWidth = (screenFrame.Width() + 1) * 0.90f;
	float maxHeight = (screenFrame.Height() + 1) * 0.85f;
	float width = maxWidth;
	float height = width * sHeight / sWidth;
	if (height > maxHeight) {
		height = maxHeight;
		width = height * sWidth / sHeight;
	}
	if (width < sWidth || height < sHeight) {
		width = sWidth;
		height = sHeight;
	}
	BRect frame(0, 0, (int)width - 1, (int)height - 1);
	frame.OffsetTo(screenFrame.left + (int)((screenFrame.Width() - frame.Width()) / 2),
		screenFrame.top + (int)((screenFrame.Height() - frame.Height()) / 2));
	return frame;
}


bool
plat_init(int width, int height, bool fullscreen, const char* title)
{
	sWidth = width;
	sHeight = height;
	sConverted = new uint32[(size_t)width * height];
	memset(sConverted, 0, sizeof(uint32) * (size_t)width * height);

	snprintf(sTitle, sizeof(sTitle), "%s", title);
	sWindowedFrame = windowed_frame();
	sWindow = new MoGWindow(sWindowedFrame, title);
	plat_backend_scaler_changed();
	sWindow->SetSizeLimits(width / 2, 32768, height / 2, 32768);
	sWindow->Show();

	if (fullscreen)
		plat_set_fullscreen(true);
	return true;
}


void
plat_shutdown(void)
{
	plat_audio_close();

	if (sCursorHidden) {
		be_app->ShowCursor();
		sCursorHidden = false;
	}
	if (sWindow != NULL && sWindow->Lock()) {
		sWindow->Quit();
		sWindow = NULL;
		sView = NULL;
	}

	sFrameLock.Lock();
	delete sFront;
	delete sBack;
	sFront = sBack = NULL;
	sFrameLock.Unlock();

	delete[] sConverted;
	sConverted = NULL;
}


const char*
plat_name(void)
{
	return "Haiku/RenkuOS native (Be API)";
}


void
plat_present(const uint8_t* pixels)
{
	if (sWindow == NULL)
		return;

	plat_internal_convert(pixels, sConverted, sWidth * sHeight);

	int width = sViewWidth;
	int height = sViewHeight;
	if (width < 1 || height < 1)
		return;

	if (sBack == NULL || sBack->Bounds().IntegerWidth() + 1 != width
		|| sBack->Bounds().IntegerHeight() + 1 != height) {
		delete sBack;
		sBack = new BBitmap(BRect(0, 0, width - 1, height - 1), B_RGB32);
		if (sBack->InitCheck() != B_OK) {
			delete sBack;
			sBack = NULL;
			return;
		}
	}

	plat_internal_scaler()->Process(sConverted, sWidth, sHeight,
		plat_internal_source_factor(), (uint32_t*)sBack->Bits(),
		width, height, (int)(sBack->BytesPerRow() / 4));

	sFrameLock.Lock();
	BBitmap* swap = sFront;
	sFront = sBack;
	sBack = swap;
	sFrameLock.Unlock();

	if (!sRedrawPending.exchange(true))
		sWindow->PostMessage(kMsgRedraw);
}


bool
plat_set_fullscreen(bool fullscreen)
{
	if (sWindow == NULL || fullscreen == sFullscreen)
		return sFullscreen;
	if (!sWindow->Lock())
		return sFullscreen;

	if (fullscreen) {
		sWindowedFrame = sWindow->Frame();
		BScreen screen(sWindow);
		BRect frame = screen.Frame();
		sWindow->SetLook(B_NO_BORDER_WINDOW_LOOK);
		sWindow->SetFeel(B_FLOATING_ALL_WINDOW_FEEL);
		sWindow->MoveTo(frame.LeftTop());
		sWindow->ResizeTo(frame.Width(), frame.Height());
		sWindow->Activate(true);
		if (!sCursorHidden) {
			be_app->HideCursor();
			sCursorHidden = true;
		}
	} else {
		sWindow->SetFeel(B_NORMAL_WINDOW_FEEL);
		sWindow->SetLook(B_TITLED_WINDOW_LOOK);
		sWindow->MoveTo(sWindowedFrame.LeftTop());
		sWindow->ResizeTo(sWindowedFrame.Width(), sWindowedFrame.Height());
		if (sCursorHidden) {
			be_app->ShowCursor();
			sCursorHidden = false;
		}
	}
	sFullscreen = fullscreen;
	sWindow->Unlock();
	return sFullscreen;
}


void
plat_backend_scaler_changed(void)
{
	if (sWindow == NULL || !sWindow->Lock())
		return;
	char title[160];
	snprintf(title, sizeof(title), "%s [%s]", sTitle, plat_scaler_name());
	sWindow->SetTitle(title);
	sWindow->Unlock();
}


bool
plat_is_fullscreen(void)
{
	return sFullscreen;
}


/* Time ------------------------------------------------------------------ */

uint32_t
plat_ticks_ms(void)
{
	static bigtime_t sStart = system_time();
	return (uint32_t)((system_time() - sStart) / 1000);
}


void
plat_sleep_ms(uint32_t ms)
{
	snooze((bigtime_t)ms * 1000);
}


/* Input ----------------------------------------------------------------- */

static void
poll_keyboard(void)
{
	if (plat_internal_input_override())
		return;

	key_info info;
	if (!sActive || get_key_info(&info) != B_OK) {
		/* Window inactive: release everything. */
		if (memcmp(sRawKeys, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16) != 0) {
			memset(sRawKeys, 0, sizeof(sRawKeys));
			plat_internal_clear_keys();
		}
		return;
	}

	MOGKey pressed[128];
	int pressedCount = 0;
	for (int raw = 1; raw < 128; raw++) {
		uint8 mask = (uint8)(1 << (7 - (raw & 7)));
		bool down = (info.key_states[raw >> 3] & mask) != 0;
		bool was = (sRawKeys[raw >> 3] & mask) != 0;
		if (down == was)
			continue;
		MOGKey key = kRawToMog[raw];
		if (key == 0)
			continue;
		plat_internal_set_key(key, down);
		if (down)
			pressed[pressedCount++] = key;
	}
	memcpy(sRawKeys, info.key_states, sizeof(sRawKeys));

	/* Events after all keys are updated, so modifiers are current. */
	int modifiers = plat_modifiers();
	for (int i = 0; i < pressedCount; i++)
		plat_internal_push_event(PLAT_EV_KEYDOWN, pressed[i], modifiers);
}


bool
plat_poll_event(PlatEvent* event)
{
	poll_keyboard();
	return plat_internal_pop_event(event);
}


void
plat_pump_events(void)
{
	poll_keyboard();
}


/* Files ----------------------------------------------------------------- */

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


/* Audio ----------------------------------------------------------------- */

static void
player_callback(void* /*cookie*/, void* buffer, size_t size,
	const media_raw_audio_format& format)
{
	sAudioLock.Lock();
	if (sAudioCallback == NULL || format.channel_count != 2) {
		memset(buffer, 0, size);
	} else if (format.format == media_raw_audio_format::B_AUDIO_SHORT) {
		sAudioCallback((int16_t*)buffer, (int)(size / 4), sAudioCookie);
	} else if (format.format == media_raw_audio_format::B_AUDIO_FLOAT) {
		size_t frames = size / 8;
		if (frames > sAudioTempFrames) {
			delete[] sAudioTemp;
			sAudioTemp = new int16_t[frames * 2];
			sAudioTempFrames = frames;
		}
		sAudioCallback(sAudioTemp, (int)frames, sAudioCookie);
		float* out = (float*)buffer;
		for (size_t i = 0; i < frames * 2; i++)
			out[i] = sAudioTemp[i] / 32768.0f;
	} else {
		memset(buffer, 0, size);
	}
	sAudioLock.Unlock();
}


bool
plat_audio_open(int rate, PlatAudioCallback callback, void* cookie)
{
	media_raw_audio_format format;
	memset(&format, 0, sizeof(format));
	format.frame_rate = (float)rate;
	format.channel_count = 2;
	format.format = media_raw_audio_format::B_AUDIO_SHORT;
	format.byte_order = B_MEDIA_HOST_ENDIAN;
	format.buffer_size = 2048 * 2 * sizeof(int16_t);

	sAudioLock.Lock();
	sAudioCallback = callback;
	sAudioCookie = cookie;
	sAudioLock.Unlock();

	sPlayer = new BSoundPlayer(&format, "Maze of Galious", player_callback);
	if (sPlayer->InitCheck() != B_OK || sPlayer->Start() != B_OK) {
		fprintf(stderr, "BSoundPlayer: %s\n", strerror(sPlayer->InitCheck()));
		delete sPlayer;
		sPlayer = NULL;
		sAudioLock.Lock();
		sAudioCallback = NULL;
		sAudioLock.Unlock();
		return false;
	}
	media_raw_audio_format actual = sPlayer->Format();
	if ((int)actual.frame_rate != rate || actual.channel_count != 2) {
		fprintf(stderr, "Audio: device format %.0f Hz, %u channels is not "
			"supported\n", actual.frame_rate, (unsigned)actual.channel_count);
	}
	sPlayer->SetHasData(true);
	return true;
}


void
plat_audio_close(void)
{
	if (sPlayer == NULL)
		return;
	sPlayer->Stop();
	delete sPlayer;
	sPlayer = NULL;
	sAudioLock.Lock();
	sAudioCallback = NULL;
	delete[] sAudioTemp;
	sAudioTemp = NULL;
	sAudioTempFrames = 0;
	sAudioLock.Unlock();
}


void
plat_audio_lock(void)
{
	sAudioLock.Lock();
}


void
plat_audio_unlock(void)
{
	sAudioLock.Unlock();
}
