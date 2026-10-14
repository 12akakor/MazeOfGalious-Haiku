/*
 * Portable software mixer and decoders (WAV, Ogg Vorbis).
 *
 * Replaces SDL_mixer and SDL_sound. Output is always 44100 Hz stereo int16.
 * Behaviour follows what the game used from SDL_mixer: 8 sound channels,
 * Mix_PlayChannel(-1, ...) semantics, per-sound volume 0..128, and one music
 * hook (Mix_HookMusic) that writes the music before the sounds are added.
 */
#ifndef MOG_AUDIO_MIXER_H
#define MOG_AUDIO_MIXER_H

#include <stdint.h>

#define MIXER_RATE		44100
#define MIXER_CHANNELS	8
#define MIXER_MAX_VOLUME	128


/* Sound effects (fully decoded into memory). */
struct MogSound;

MogSound* snd_load(const char* path);
void snd_free(MogSound* sound);
void snd_set_volume(MogSound* sound, int volume);
/* Plays on the first free channel; does nothing if all channels are busy. */
void snd_play(MogSound* sound);


/* Streams (music). Not thread safe: use from the music hook or with the
   audio lock held. */
struct MogStream;

MogStream* stream_open(const char* path);
/* Returns the number of stereo frames written; less than 'frames' at the end. */
int stream_read(MogStream* stream, int16_t* out, int frames);
bool stream_eof(MogStream* stream);
void stream_rewind(MogStream* stream);
void stream_close(MogStream* stream);


/* Mixer. */
typedef void (*MusicHook)(int16_t* out, int frames);

/* Opens the audio device through the platform layer. With openDevice false
   the mixer works without a device (tests call mixer_render() directly). */
bool mixer_open(bool openDevice);
void mixer_close(void);
bool mixer_is_open(void);
void mixer_set_music_hook(MusicHook hook);
void mixer_lock(void);
void mixer_unlock(void);

/* Produces 'frames' stereo frames (normally called by the audio device). */
void mixer_render(int16_t* out, int frames);

#endif	/* MOG_AUDIO_MIXER_H */
