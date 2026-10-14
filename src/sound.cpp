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


void Sound_create_music(char *f1,char *f2,char *f3)
{
	mixer_lock();
	char tmp[128];
	int seq_len=0;

	if (sound_enabled) {
		if (f1!=0) {
			Sound_obtain_file_name(f1,tmp);
			strcpy(music_files[0],f1);
			strcpy(music_realfiles[0],tmp);
			music_loaded[0]=true;
			music_sound[0]=Sound_create_stream(f1);
			seq_len=1;
		} else {
			music_files[0][0]=0;
			music_loaded[0]=false;
		} /* if */ 
		if (f2!=0) {
			Sound_obtain_file_name(f2,tmp);
			strcpy(music_files[1],f2);
			strcpy(music_realfiles[1],tmp);
			music_loaded[1]=true;
			music_sound[1]=Sound_create_stream(f2);
			seq_len=2;
		} else {
			music_files[1][0]=0;
			music_loaded[1]=false;
		} /* if */ 
		if (f3!=0) {
			Sound_obtain_file_name(f3,tmp);
			strcpy(music_files[2],f3);
			strcpy(music_realfiles[2],tmp);
			music_loaded[2]=true;
			music_sound[2]=Sound_create_stream(f3);
			seq_len=3;
		} else {
			music_files[2][0]=0;
			music_loaded[2]=false;
		} /* if */ 

		music_position=0;
		playing_music=true;
	} /* if */ 
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
	mixer_lock();
	int i;

	if (sound_enabled) {
		playing_music=false;
		for(i=0;i<3;i++) {
			if (music_loaded[i]) { stream_close(music_sound[i]); music_sound[i]=0; }
			music_loaded[i]=false;
		} /* if */ 
	} /* if */ 
	mixer_unlock();
} /* Sound_release_music */ 


void Sound_temporary_release_music(void)
{
	mixer_lock();
	int i;

	if (sound_enabled) {
		playing_music=false;
		for(i=0;i<3;i++) {
			if (music_loaded[i]) { stream_close(music_sound[i]); music_sound[i]=0; }
		} /* if */ 
	} /* if */ 
	mixer_unlock();
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
	mixer_lock();	
	char tmp[128];
	int seq_len=0;
//	FILE *fp;

	if (sound_enabled) {
//		fp=fopen("report.txt","w");
//		fprintf(fp,"%s\n%s\n%s\n",music_files[0],music_files[1],music_files[2]);
//		fclose(fp);

		if (music_loaded[0]) { 
			music_loaded[0]=true;
			Sound_obtain_file_name(music_files[0],tmp);
			strcpy(music_realfiles[0],tmp);
			music_sound[0]=Sound_create_stream(music_files[0]);
			seq_len=1;
		} /* if */ 
		if (music_loaded[1]) { 
			music_loaded[1]=true;
			Sound_obtain_file_name(music_files[1],tmp);
			strcpy(music_realfiles[1],tmp);
			music_sound[1]=Sound_create_stream(music_files[1]);
			seq_len=2;
		} /* if */ 
		if (music_loaded[2]) { 
			music_loaded[2]=true;
			Sound_obtain_file_name(music_files[2],tmp);
			strcpy(music_realfiles[2],tmp);
			music_sound[2]=Sound_create_stream(music_files[2]);
			seq_len=3;
		} /* if */ 

		playing_music=true;
	} /* if */ 

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










