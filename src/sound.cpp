#include "platform/platform.h"
#include "platform/audio_mixer.h"
#include "sound.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "MOGtypes.h"



/* Paths: */ 
extern char *default_g_path;
extern char *default_s_path;
extern char **g_paths;
extern int n_g_paths,act_g_path;
extern char *g_path;
extern char **s_paths;
extern int n_s_paths,act_s_path;
extern char *s_path;
extern int music_volume;

bool sound_enabled=false;


int music_position=0;
bool playing_music=false;
bool music_loaded[8]={false,false,false,false,false,false,false,false};			
MogStream *music_sound[8]={0,0,0,0,0,0,0,0};
char music_files[8][128];		/* File names */ 
char music_realfiles[8][128];	/* File names including music directory */ 


bool Sound_initialization(void)
{
	/* Port: SDL_mixer/SDL_sound replaced by the portable mixer. In test mode
	   no device is opened; the test driver renders audio itself. */
	bool openDevice=getenv("MOG_TEST_FRAMES")==NULL;

	sound_enabled=true;
	fprintf (stderr, "Initializing audio (%s).\n", plat_name());
	if (!mixer_open(openDevice))  {
	  fprintf (stderr, "Unable to open audio.\n");
	  sound_enabled=false;
	  fprintf (stderr, "Running the game without audio.\n");
	  return false;	
	} /* if */ 

	fprintf (stderr, "    opened at %d Hz 16 bit stereo\n", MIXER_RATE);
	mixer_set_music_hook(myMusicPlayer);

	return true;
} /* Sound_init */ 

void Sound_release(void)
{
	if (sound_enabled) {
		mixer_set_music_hook(0);
		mixer_close();
	} /* if */ 
	sound_enabled=false;
} /* Sound_release */ 


/* a check to see if file is readable and greater than zero */
int file_check(char *fname)
{
	FILE *fp;

	if ((fp=fopen(fname,"r"))!=NULL) {
		if (fseek(fp,0L, SEEK_END)==0 && ftell(fp)>0) {
  			fclose(fp);
			return true;
		} /* if */ 
		/* either the file could not be read (==-1) or size was zero (==0) */ 
		fprintf(stderr,"ERROR in file_check(): the file %s is corrupted.\n", fname);
		fclose(fp);
		exit(1);
	} /* if */ 
	return false;
} /* file_check */ 



SOUNDT Sound_create_sound(char *file,int flags)
{
	int n_ext=6;
	char *ext[6]={".WAV",".OGG",".MP3",".wav",".ogg",".mp3"};
	char name[256],name2[256];
	int i;

	if (sound_enabled) {
		for(i=0;i<n_ext;i++) {
			strcpy(name,file);
			strcat(name,ext[i]);
			sprintf(name2,"%s%s",s_path,name);
			if (file_check(name2)) return snd_load(name2);
		} /* for */ 
		for(i=0;i<n_ext;i++) {
			strcpy(name,file);
			strcat(name,ext[i]);
			sprintf(name2,"%s%s",default_s_path,name);
			if (file_check(name2)) return snd_load(name2);
		} /* for */ 

		fprintf(stderr,"ERROR in Sound_create_sound(): Could not load sound file: %s%s.(wav|ogg|mp3)\n",s_path, file);
		exit(1);
	} else {
		return 0;
	} /* if */ 
} /* Sound_create_sound */ 


void Delete_sound(SOUNDT s)
{
	if (sound_enabled) snd_free(s);
} /* Delete_sound */ 


void Sound_play(SOUNDT s)
{
	if (sound_enabled) snd_play(s);
} /* Sound_play */ 


void Sound_obtain_file_name(char *file,char *fullname)
{
	int n_ext=6;
	char *ext[6]={".WAV",".OGG",".MP3",".wav",".ogg",".mp3"};
	char name[256],name2[256];
	int i;

	for(i=0;i<n_ext;i++) {
		strcpy(name,file);
		strcat(name,ext[i]);
		sprintf(name2,"%s%s",s_path,name);
		if (file_check(name2)) {
			strcpy(fullname,name2);
			return;
		} /* if */ 
		strcpy(name,file);
		strcat(name,ext[i]);
		sprintf(name2,"%s%s",default_s_path,name);
		if (file_check(name2)) {
			strcpy(fullname,name2);
			return;
		} /* if */ 

	} /* for */ 

	fprintf(stderr,"ERROR in Sound_obtain_file_name(): Could not find sound file: %s%s.(wav|ogg|mp3)\n",s_path, file);
 	fullname[0]=0;
	exit(1);
} /* Sound_obtain_file_name */ 


MogStream *Sound_create_stream(char *file)
{
	int n_ext=6;
	char *ext[6]={".WAV",".OGG",".MP3",".wav",".ogg",".mp3"};
	char name[256],name2[256];
	int i;

	if (sound_enabled) {
		for(i=0;i<n_ext;i++) {
			strcpy(name,file);
			strcat(name,ext[i]);
			sprintf(name2,"%s%s",s_path,name);
			if (file_check(name2)) return stream_open(name2);
		} /* for */ 
		for(i=0;i<n_ext;i++) {
			strcpy(name,file);
			strcat(name,ext[i]);
			sprintf(name2,"%s%s",default_s_path,name);
			if (file_check(name2)) return stream_open(name2);
		} /* for */ 
		
		fprintf(stderr,"ERROR in Sound_create_stream(): Could not load sound file: %s%s.(wav|ogg|mp3)\n",s_path, file);
		exit(1);
	} else {
		return 0;
	} /* if */ 
} /* Sound_create_stream */ 


/* Port: open (and start decoding) the music files before taking the audio
   lock. Opening an Ogg file can take tens of milliseconds on a slow or
   single-CPU machine; with the lock held the audio thread had to wait, which
   made the sound stutter at every music change. */
void Sound_create_music(char *f1,char *f2,char *f3)
{
	char tmp[128];
	char *files[3]={f1,f2,f3};
	MogStream *streams[3]={0,0,0};
	char realfiles[3][128];
	int i;

	if (!sound_enabled) return;
	for(i=0;i<3;i++) {
		if (files[i]!=0) {
			Sound_obtain_file_name(files[i],tmp);
			strcpy(realfiles[i],tmp);
			streams[i]=Sound_create_stream(files[i]);
		} /* if */ 
	} /* for */ 

	mixer_lock();
	for(i=0;i<3;i++) {
		if (files[i]!=0) {
			strcpy(music_files[i],files[i]);
			strcpy(music_realfiles[i],realfiles[i]);
			music_loaded[i]=true;
			music_sound[i]=streams[i];
		} else {
			music_files[i][0]=0;
			music_loaded[i]=false;
		} /* if */ 
	} /* for */ 
	music_position=0;
	playing_music=true;
	mixer_unlock();
} /* Sound_create_music */ 

void Sound_subst_music(char *f)
{
	char tmp[128];

	Sound_obtain_file_name(f,tmp);
//	my_SubstMusic(tmp);
} /* Sound_sust_music */ 


void Sound_subst_music_now(char *f)
{
	Sound_release_music();
	Sound_create_music(f,0,0);
} /* Sound_sust_music_now */ 



void Sound_release_music(void)
{
	MogStream *closing[3]={0,0,0};
	int i;

	mixer_lock();
	if (sound_enabled) {
		playing_music=false;
		for(i=0;i<3;i++) {
			if (music_loaded[i]) { closing[i]=music_sound[i]; music_sound[i]=0; }
			music_loaded[i]=false;
		} /* if */ 
	} /* if */ 
	mixer_unlock();
	/* Port: close outside the audio lock. */
	for(i=0;i<3;i++) stream_close(closing[i]);
} /* Sound_release_music */ 


void Sound_temporary_release_music(void)
{
	MogStream *closing[3]={0,0,0};
	int i;

	mixer_lock();
	if (sound_enabled) {
		playing_music=false;
		for(i=0;i<3;i++) {
			if (music_loaded[i]) { closing[i]=music_sound[i]; music_sound[i]=0; }
		} /* if */ 
	} /* if */ 
	mixer_unlock();
	/* Port: close outside the audio lock. */
	for(i=0;i<3;i++) stream_close(closing[i]);
} /* Sound_release_music */ 


void Sound_pause_music(void)
{
	mixer_lock();
	playing_music=false;
	mixer_unlock();
} /* Sound_pause_music */ 


void Sound_unpause_music(void)
{
	mixer_lock();
	playing_music=true;
	mixer_unlock();
} /* Sound_unpause_music */ 


void music_recovery(void)
{
	/* Port: reopen the streams of the current music (after a sound set
	   change) before taking the audio lock, see Sound_create_music(). */
	char tmp[128];
	MogStream *streams[3]={0,0,0};
	int i;

	if (!sound_enabled) return;
	for(i=0;i<3;i++) {
		if (music_loaded[i]) {
			Sound_obtain_file_name(music_files[i],tmp);
			strcpy(music_realfiles[i],tmp);
			streams[i]=Sound_create_stream(music_files[i]);
		} /* if */ 
	} /* for */ 

	mixer_lock();
	for(i=0;i<3;i++) {
		if (music_loaded[i]) music_sound[i]=streams[i];
	} /* for */ 
	playing_music=true;
	mixer_unlock();
} /* music_recovery */ 


/* Music hook, called by the mixer from the audio thread with the audio lock
   held. Plays music_sound[0..2] in sequence; the last loaded part loops. */
void myMusicPlayer(int16_t *stream, int frames)
{
	int i,act=0;

	if (!playing_music) {
		/* No music to play: */ 
		memset(stream,0,sizeof(int16_t)*2*frames);
		return;
	} /* if */ 

	while(act<frames) {
		if (music_loaded[music_position] && music_sound[music_position]!=0) {
			/* Play a music file: */ 
			if (stream_eof(music_sound[music_position])) {
				/* End of file: */ 
				if (music_loaded[music_position+1] && music_sound[music_position+1]!=0) {
					music_position++;
				} /* if */ 
				stream_rewind(music_sound[music_position]);
				if (stream_eof(music_sound[music_position])) break;
			} else {
				/* In the middle of the file: */ 
				int16_t *ptr=stream+act*2;
				int decoded=stream_read(music_sound[music_position],ptr,frames-act);
				for(i=0;i<decoded*2;i++) {
					ptr[i]=(int16_t)((int32_t(ptr[i])*int32_t(music_volume))/127);
				} /* for */ 
				act+=decoded;
			} /* if */ 
		} else {
			break;
		} /* if */ 
	} /* while */ 

	/* No (more) music: */ 
	if (act<frames) memset(stream+act*2,0,sizeof(int16_t)*2*(frames-act));
} /* myMusicPlayer */ 










