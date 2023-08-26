#ifndef AUDIO_H
#define AUDIO_H

#include "../video/video.h"
#include <bass/bass.h>
#include <bass/bass_fx.h>


enum SOUNDEFFECTS_ { MENU_BEEP, MENU_FASTBEEP, MENU_CONFIRM, MENU_BACK, MENU_INVALID, TRACK_CLEAR, TRACK_FAIL, HITSOUND1, HITSOUND2, HITSOUND3, SCREENSHOT, BASS_KICK, HI_HAT, SNARE_DRUM, N_SOUNDEFFECTS };

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Structs

typedef struct audio_struct
{
	DWORD 				effects_[N_SOUNDEFFECTS];
	HSTREAM 			track_,fadetrack_,hittrack_[2];
	BASS_CHANNELINFO	info_;
	unsigned long		length_;
	float				volume_,mvolume_,sevolume_,pitch_,freq_,hitfreq_,tfadetime_,ftfadetime_,ftfadevol_;
	char*				hitdata_[2];
} audio_struct;
////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Functions
int 			init_audio();
int				load_track(char*);
int 			bass_loop();
//int			decode_track(int);
void			play_track(int,float);
void 			play_hittrack(int);
void			set_trackvolume(float);
void 			change_audiovolume();
void 			set_pitch(float);
void 			switch_tracks();
void 			queue_track();
void 			pause_track();
int 			get_trackstatus(HSTREAM*);
float			get_tracklength();
void 			set_trackpos(HSTREAM*,float);
double 			get_trackpos(HSTREAM*);
float 			track_fadein(float);
float 			track_fadeout(float);
void 			stop_track(HSTREAM*);
void 			stop_tracks();
int 			load_soundeffect(char*,int);
void 			play_soundeffect(int);
void 			stop_soundeffect(int);
void 			destroy_audio();
void 			generate_audio(int,int,void*,float);
void 			generate_calibration();
void 			set_tracklink(HSTREAM*,HSTREAM*);
void 			set_trackloop(HSTREAM*,int);
void 			mute_hittrack(int);
void 			destroy_hittrack();
////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Globals
extern audio_struct* audio_;
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif //AUDIO_H