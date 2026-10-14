#ifndef __BRAIN_SOUND
#define __BRAIN_SOUND

#include <stdint.h>

struct MogSound;
typedef MogSound * SOUNDT;

bool Sound_initialization(void);
void Sound_release(void);

SOUNDT Sound_create_sound(char *file,int flags);
void Delete_sound(SOUNDT s);
void Sound_play(SOUNDT s);

void Sound_create_music(char *f1,char *f2,char *f3);
void Sound_subst_music(char *f);
void Sound_subst_music_now(char *f);
void Sound_temporary_release_music(void);
void Sound_release_music(void);
void Sound_pause_music(void);
void Sound_unpause_music(void);

void music_recovery(void);

void myMusicPlayer(int16_t *stream, int frames);
void PlayAudioQueue(void); 

#endif

