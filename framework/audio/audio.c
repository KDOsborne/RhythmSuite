#include "audio.h"
#include "../error/error.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct note_data
{
	int32_t 			time;
	float				hittime;
	double				svtime;
	struct note_data*	tail;
};

struct WAV_header
{
	char 	riff[4];
	int32_t size;
	char 	wave[4];
	char 	fmt[4];
	int32_t wavchunk;
	int16_t wavformat;
	int16_t channels;
	int32_t freq;
	int32_t byterate;
	int16_t balign;
	int16_t bitspersample;
	char 	data[4];
	int32_t datasize;
};

audio_struct* audio_;

int init_audio()
{
	if (HIWORD(BASS_GetVersion()) != BASSVERSION) {
		write_error("ERROR::An incorrect version of BASS.DLL was loaded",NULL,0);
		return -1;
	}
	if (HIWORD(BASS_FX_GetVersion())!= BASSVERSION) {
		write_error("ERROR::An incorrect version of BASS_FX.DLL was loaded (2.4 is required)",NULL,0);
		return -1;
	}
	if (!BASS_Init(-1,44100,0,video_->hWnd,NULL)) {
		write_error_int("ERROR::Can't initialize audio device\nCODE: ",BASS_ErrorGetCode(),0);
		return -1;
	}
	
	audio_ = (audio_struct*)malloc(sizeof(audio_struct));
	memset(audio_,0,sizeof(audio_struct));
	
	audio_->volume_ = (float)video_->settings_[VOLUME].value_/100.f;
	if(audio_->volume_ < 0)
		audio_->volume_ = 0.f;
	else if(audio_->volume_ > 1)
		audio_->volume_ = 1.f;
	
	audio_->mvolume_ = (float)video_->settings_[MVOLUME].value_/100.f;
	audio_->sevolume_ = (float)video_->settings_[SEVOLUME].value_/100.f;
	audio_->pitch_ = 1.0;
	audio_->freq_ = 44100;
	audio_->tfadetime_ = -1.0;
	audio_->ftfadetime_ = -1.0;
	audio_->hitdata_[0] = NULL;
	audio_->hitdata_[1] = NULL;
	
	if(audio_->mvolume_ < 0)
		audio_->mvolume_ = 0.f;
	else if(audio_->mvolume_ > 1)
		audio_->mvolume_ = 1.f;
	
	if(audio_->sevolume_ < 0)
		audio_->sevolume_ = 0.f;
	else if(audio_->sevolume_ > 1)
		audio_->sevolume_ = 1.f;
	
	load_soundeffect("sound_effects/menu_beep.mp3",MENU_BEEP);
	load_soundeffect("sound_effects/menu_beep.mp3",MENU_FASTBEEP);
	load_soundeffect("sound_effects/menu_select.mp3",MENU_CONFIRM);
	load_soundeffect("sound_effects/menu_back.mp3",MENU_BACK);
	load_soundeffect("sound_effects/menu_invalid.mp3",MENU_INVALID);
	load_soundeffect("sound_effects/track_clear.mp3",TRACK_CLEAR);
	load_soundeffect("sound_effects/track_fail.mp3",TRACK_FAIL);
	load_soundeffect("sound_effects/hitsound.wav",HITSOUND1);
	load_soundeffect("sound_effects/hitsound.wav",HITSOUND2);
	load_soundeffect("sound_effects/hitsound.wav",HITSOUND3);
	load_soundeffect("sound_effects/screenshot.mp3",SCREENSHOT);
	load_soundeffect("sound_effects/bass_kick.wav",BASS_KICK);
	load_soundeffect("sound_effects/hi-hat.wav",HI_HAT);
	load_soundeffect("sound_effects/snare_drum.wav",SNARE_DRUM);
	
	change_audiovolume();
	
	return 0;
}

int load_track(char* filename)
{
	if(audio_->track_)
		stop_track(&audio_->track_);
	
	HSTREAM stream = BASS_StreamCreateFile(FALSE,filename,0,0,BASS_STREAM_PRESCAN);
	if(!stream)
	{
		write_error("WARNING::Can't load mp3 file: ",filename,0);
		write_error_int("CODE: ",BASS_ErrorGetCode(),1);
		return -1;
	}
	
	BASS_ChannelGetInfo(stream,&(audio_->info_));
	
	audio_->track_ = stream;
	audio_->tfadetime_ = -1.0;
	audio_->length_ = (int)(BASS_ChannelBytes2Seconds(audio_->track_,BASS_ChannelGetLength(audio_->track_,BASS_POS_BYTE))*1000+1);
	
	return 0;
}

int bass_loop()
{
	if(audio_->track_)
		stop_track(&audio_->track_);
	
	HSTREAM stream = BASS_StreamCreateFile(FALSE,"sound_effects/bass_loop.wav",0,0,BASS_SAMPLE_LOOP);
	if(!stream)
	{
		write_error("WARNING::Can't load file: ","sound_effects/bass_loop.wav",0);
		write_error_int("CODE: ",BASS_ErrorGetCode(),1);
		return -1;
	}
	
	BASS_ChannelGetInfo(stream,&(audio_->info_));
	
	audio_->track_ = stream;
	audio_->tfadetime_ = -1.0;
	audio_->length_ = (int)(BASS_ChannelBytes2Seconds(audio_->track_,BASS_ChannelGetLength(audio_->track_,BASS_POS_BYTE))*1000+1);
	
	play_track(0,1.0);
	
	return 0;
}

/*int decode_music(int ms)
{
	stop_music();
	
	//Load the audio file.
	HSTREAM stream = BASS_StreamCreateFile(FALSE,audio_->info_.filename,0,0,BASS_STREAM_DECODE);
	if(!stream)
	{
		printf("Can't create stream: %s\n", audio_->info_.filename);
		return -1;
	}
	
	//Get the length of the file in bytes and seconds.
	BASS_ChannelGetInfo(stream,&(audio_->info_));
	
	QWORD len = BASS_ChannelGetLength(stream, BASS_POS_BYTE);
	double dur = BASS_ChannelBytes2Seconds(stream, len);
	
	//Calculate the sample rate.
	double samplerate = len / dur;
	int ratio = 1;
	
	//We're converting to 32bit float samples, so increase the buffer size if the audio data is 16 or 8bit.
	if(audio_->info_.flags & BASS_SAMPLE_8BITS)
		ratio = 4;
	else if(!(audio_->info_.flags & BASS_SAMPLE_FLOAT))
		ratio = 2;
	len *= ratio;
	
	//Calculate how many bytes to allocate for the inserted silence.
	int silencebytes = samplerate*ratio/1000.0 * ms;
	
	//Audio data must be provided with total bytes divisible by 8.
	while(silencebytes % 8 != 0)
		silencebytes++;
	
	//Allocate memory for the audio.
	float* buffer = (float*)malloc(len+silencebytes);
	memset(buffer,0,len+silencebytes);
	
	//Get the mp3 data.
	BASS_ChannelGetData(stream,buffer+silencebytes/sizeof(float),len|BASS_DATA_FLOAT);
	
	//Create a stream to load the silence and mp3 audio into.
	HSTREAM str = BASS_StreamCreate(samplerate/4,2,BASS_SAMPLE_FLOAT,STREAMPROC_PUSH,NULL);
	if(!str)
	{
		free(buffer);
		printf("Couldn't create stream %d\n",BASS_ErrorGetCode());
		return -1;
	}
	
	//Queue the audio.
	if(BASS_StreamPutData(str,buffer,len+silencebytes) == -1)
	{
		free(buffer);
		printf("Couldn't put data: %d\n",BASS_ErrorGetCode());
		return -1;
	}
	
	audio_->track_ = str;
	audio_->buffer_ = buffer;
	
	return 0;
}*/

void play_track(int pos, float volume)
{
	if(!BASS_ChannelSetAttribute(audio_->track_,BASS_ATTRIB_VOL,volume*audio_->mvolume_*audio_->volume_))
		write_error_int("WARNING::Can't set volume\nCODE: ",BASS_ErrorGetCode(),0);
	if(!BASS_ChannelSetPosition(audio_->track_,BASS_ChannelSeconds2Bytes(audio_->track_,pos/1000.0),BASS_POS_BYTE))
		write_error_int("WARNING::Failed to set channel position\nCODE: ",BASS_ErrorGetCode(),0);
	if(!BASS_ChannelPlay(audio_->track_,FALSE))
		write_error_int("WARNING::Can't play music\nCODE: ",BASS_ErrorGetCode(),0);
	
	BASS_ChannelGetAttribute(audio_->track_,BASS_ATTRIB_FREQ,&audio_->freq_);
	BASS_ChannelSetAttribute(audio_->track_,BASS_ATTRIB_FREQ,audio_->freq_*audio_->pitch_);
	audio_->tfadetime_ = -1.0;
}

void play_hittrack(int index)
{
	if(!BASS_ChannelPlay(audio_->hittrack_[index],FALSE))
		write_error_int("WARNING::Can't play hitsounds\nCODE: ",BASS_ErrorGetCode(),0);
}

void change_audiovolume()
{
	audio_->volume_ = (float)video_->settings_[VOLUME].value_/100.f;
	if(audio_->volume_ < 0)
		audio_->volume_ = 0.f;
	else if(audio_->volume_ > 1)
		audio_->volume_ = 1.f;
	
	audio_->mvolume_ = (float)video_->settings_[MVOLUME].value_/100.f;
	audio_->sevolume_ = (float)video_->settings_[SEVOLUME].value_/100.f;
	
	if(audio_->mvolume_ < 0)
		audio_->mvolume_ = 0.f;
	else if(audio_->mvolume_ > 1)
		audio_->mvolume_ = 1.f;
	
	if(audio_->sevolume_ < 0)
		audio_->sevolume_ = 0.f;
	else if(audio_->sevolume_ > 1)
		audio_->sevolume_ = 1.f;
	
	for(int i = 0; i < N_SOUNDEFFECTS; i++)
		BASS_ChannelSetAttribute(audio_->effects_[i],BASS_ATTRIB_VOL,audio_->sevolume_*audio_->volume_);
}

void set_trackvolume(float volume)
{
	BASS_ChannelSetAttribute(audio_->track_,BASS_ATTRIB_VOL,volume*audio_->mvolume_*audio_->volume_);
}

void set_pitch(float pitch)
{
	if(pitch > 2.0)
		pitch = 2.0;
	else if(pitch < 0.5)
		pitch = 0.5;
	audio_->pitch_ = pitch;
	
	if(audio_->track_)
		BASS_ChannelSetAttribute(audio_->track_,BASS_ATTRIB_FREQ,audio_->freq_*pitch);
	if(audio_->fadetrack_)
		BASS_ChannelSetAttribute(audio_->fadetrack_,BASS_ATTRIB_FREQ,audio_->freq_*pitch);
	
	for(int i = 0; i < 2; i++)
	{
		if(audio_->hitdata_[i] != NULL)
			BASS_ChannelSetAttribute(audio_->hittrack_[i],BASS_ATTRIB_FREQ,audio_->hitfreq_*pitch);
	}
}

void switch_tracks()
{
	if(audio_->fadetrack_)
		stop_track(&audio_->fadetrack_);
	
	audio_->fadetrack_ = audio_->track_;
	audio_->track_ = 0;
	audio_->tfadetime_ = -1.0;
	audio_->ftfadetime_ = -1.0;
}

float track_fadein(float secs)
{
	float volume;
	BASS_ChannelGetAttribute(audio_->track_,BASS_ATTRIB_VOL,&volume);
	
	if(audio_->mvolume_*audio_->volume_ > 0.f)
		volume /= audio_->mvolume_*audio_->volume_;
	
	if(audio_->tfadetime_ == -1.0)
		audio_->tfadetime_ = get_trackpos(&audio_->track_);
	
	if(volume < 1.0)
	{
		volume = (get_trackpos(&audio_->track_) - audio_->tfadetime_) / secs;
		if(volume > 1.0)
			volume = 1.0;
		BASS_ChannelSetAttribute(audio_->track_,BASS_ATTRIB_VOL,volume*audio_->mvolume_*audio_->volume_);
	}
	
	return volume;
}

float track_fadeout(float secs)
{
	float volume = -1.0;
	
	if(get_trackstatus(&audio_->fadetrack_) == BASS_ACTIVE_PLAYING)
	{
		BASS_ChannelGetAttribute(audio_->fadetrack_,BASS_ATTRIB_VOL,&volume);
		
		if(audio_->mvolume_*audio_->volume_ > 0.f)
			volume /= audio_->mvolume_*audio_->volume_;
		else
			volume = 1.0;
		
		if(volume > 0.0)
		{
			if(audio_->ftfadetime_ == -1.0)
			{
				audio_->ftfadetime_ = get_trackpos(&audio_->fadetrack_);
				audio_->ftfadevol_ = volume;
			}
			
			volume = audio_->ftfadevol_-(get_trackpos(&audio_->fadetrack_)-audio_->ftfadetime_)/secs;
			if(volume <= 0.0)
			{
				stop_track(&audio_->fadetrack_);
				audio_->ftfadetime_ = -1.0;
				return -1.0;
			}
			else
				BASS_ChannelSetAttribute(audio_->fadetrack_,BASS_ATTRIB_VOL,volume*audio_->mvolume_*audio_->volume_);
		}
		else
		{
			stop_track(&audio_->fadetrack_);
			audio_->ftfadetime_ = -1.0;
			return -1.0;
		}
	}
	
	return volume;
}

void pause_track()
{
	BASS_ChannelSetAttribute(audio_->track_,BASS_ATTRIB_FREQ,0);
	if(!BASS_ChannelPause(audio_->track_))
		write_error_int("WARNING::Can't pause music\nCODE: ",BASS_ErrorGetCode(),0);
}

void queue_track()
{
	pause_track();
	if(!BASS_ChannelSetPosition(audio_->track_,0,BASS_POS_BYTE))
		write_error_int("WARNING::Failed to set channel position\nCODE: ",BASS_ErrorGetCode(),0);
	if(!BASS_ChannelUpdate(audio_->track_,0))
		write_error_int("WARNING::Failed to update channel\nCODE: ",BASS_ErrorGetCode(),0);
}

int get_trackstatus(HSTREAM* track)
{
	if(track == NULL)
		track = &audio_->track_;

	return BASS_ChannelIsActive(*track);
}

float get_tracklength()
{
	return BASS_ChannelBytes2Seconds(audio_->track_,BASS_ChannelGetLength(audio_->track_, BASS_POS_BYTE));
}

void set_trackpos(HSTREAM* track, float ms)
{
	if(track == NULL)
	{
		track = &audio_->track_;
		if(audio_->hitdata_[0] != NULL)
			BASS_ChannelSetPosition(audio_->hittrack_[0],BASS_ChannelSeconds2Bytes(audio_->hittrack_[0],ms/1000.0),BASS_POS_BYTE|BASS_POS_SCAN);
		if(audio_->hitdata_[1] != NULL)
			BASS_ChannelSetPosition(audio_->hittrack_[1],BASS_ChannelSeconds2Bytes(audio_->hittrack_[1],ms/1000.0),BASS_POS_BYTE|BASS_POS_SCAN);
	}
	
	BASS_ChannelSetPosition(*track,BASS_ChannelSeconds2Bytes(*track,ms/1000.0),BASS_POS_BYTE|BASS_POS_SCAN);
}

double get_trackpos(HSTREAM* track)
{
	if(track == NULL)
		track = &audio_->track_;
		
	return BASS_ChannelBytes2Seconds(*track,BASS_ChannelGetPosition(*track, BASS_POS_BYTE));
}

void stop_track(HSTREAM* track)
{
	if(track == NULL)
		track = &audio_->track_;

	BASS_ChannelStop(*track);
	BASS_StreamFree(*track);
	
	*track = 0;
}

void stop_tracks()
{
	stop_track(&audio_->track_);
	stop_track(&audio_->fadetrack_);
	stop_track(&audio_->hittrack_[0]);
	stop_track(&audio_->hittrack_[1]);
}

int load_soundeffect(char* filename, int effect)
{
	HSAMPLE samp = BASS_SampleLoad(FALSE,filename,0,0,10,0);
	if(!samp)
	{
		write_error("ERROR::Can't load sample: ", filename,0);
		write_error_int("CODE: ",BASS_ErrorGetCode(),1);
		return -1;
	}
	
	DWORD handle = BASS_SampleGetChannel(samp,0);
	if(!handle)
	{
		write_error_int("ERROR::Can't create playback channel\nCODE: ",BASS_ErrorGetCode(),0);
		return -1;
	}
	
	audio_->effects_[effect] = handle;
	
	return 0;
}

void play_soundeffect(int effect)
{
	if((effect == MENU_BEEP))
	{
		if(get_trackstatus(&audio_->effects_[effect]) != BASS_ACTIVE_PLAYING || get_trackpos(&audio_->effects_[effect]) > 2/30.f)
			if(!BASS_ChannelPlay(audio_->effects_[effect],TRUE))
				write_error_int("WARNING::Can't play sound effect\nCODE: ",BASS_ErrorGetCode(),0);
	}
	else
		if(!BASS_ChannelPlay(audio_->effects_[effect],TRUE))
			write_error_int("WARNING::Can't play sound effect\nCODE: ",BASS_ErrorGetCode(),0);
}

void stop_soundeffect(int effect)
{
	BASS_ChannelPause(audio_->effects_[effect]);
}

void destroy_audio()
{
	stop_track(&audio_->track_);
	stop_track(&audio_->fadetrack_);
	destroy_hittrack();
	
	BASS_Free();
	free(audio_);
}

void generate_audio(int effect, int index, void* notes, float dur)
{
	HSTREAM stream;
	
	switch(effect)
	{
		case HITSOUND1:
		case HITSOUND2:
		case HITSOUND3:
			stream = BASS_StreamCreateFile(FALSE,"sound_effects/hitsound.wav",0,0,BASS_STREAM_DECODE);
		break;
		case BASS_KICK:
			stream = BASS_StreamCreateFile(FALSE,"sound_effects/bass_kick.wav",0,0,BASS_STREAM_DECODE);
		break;
		default:
			return;
		break;
	}
	
	if(!stream)
	{
		printf("CANNOT MAKE STREAM: %d\n", BASS_ErrorGetCode());
		return;
	}
	
	BASS_CHANNELINFO info_;
	BASS_ChannelGetInfo(stream,&info_);
	
	QWORD bytes = BASS_ChannelGetLength(stream,BASS_POS_BYTE);
	char* data = (char*)malloc(bytes);
	if(BASS_ChannelGetData(stream,data,bytes) == -1)
	{
		printf("CANNOT GET DATA: %d\n", BASS_ErrorGetCode());
		free(data);
		return;
	}
	
	int samplerate = info_.freq;
	int channels = info_.chans;
	int resolution;
	if(info_.flags & BASS_SAMPLE_8BITS)
		resolution = 1;
	else if(info_.flags & BASS_SAMPLE_FLOAT)
		resolution = 4;
	else
		resolution = 2;
	
	int bps = resolution*8;
	int bytespersamp = bps*channels/8;
	int byteLength = (int)(samplerate*channels*resolution*dur);
	
	while(byteLength % 8 != 0)
		byteLength++;
	
	struct WAV_header header = {
		"RIFF",
		sizeof(struct WAV_header)+byteLength-8,
		"WAVE",
		"fmt ",
		16,
		1,
		channels,
		samplerate,
		(samplerate*bps*channels)/8,
		bytespersamp,
		bps,
		"data",
		byteLength
	};

	char* newdat = (char*)malloc(byteLength+sizeof(struct WAV_header));
	memset(newdat,0,byteLength+sizeof(struct WAV_header));
	memcpy(newdat,&header,sizeof(struct WAV_header));
	
	struct note_data* note = (struct note_data*)notes;
	char* dat = newdat+sizeof(struct WAV_header);
	float time;
	
	for(int i = 0; i < byteLength; i+=bytespersamp)
	{
		time = (float)i/bytespersamp/samplerate*1000;
		if(round(time) >= note->time)
		{
			if(byteLength - i >= bytes)
				memcpy(dat+i,data,bytes);
			else
				memcpy(dat+i,data,(byteLength-i));
			
			note = note->tail;
			if(note == NULL)
				break;
		}
	}

	HSTREAM newstream = BASS_StreamCreateFile(TRUE,newdat,0,sizeof(struct WAV_header)+byteLength,BASS_STREAM_PRESCAN);
	if(!newstream)
	{
		free(data);
		free(newdat);
		return;
	}
	
	BASS_ChannelSetAttribute(newstream,BASS_ATTRIB_VOL,audio_->sevolume_*audio_->volume_);
	BASS_ChannelSetAttribute(newstream,BASS_ATTRIB_FREQ,samplerate*audio_->pitch_);
	
	if(audio_->hitdata_[index] != NULL)
	{
		stop_track(&audio_->hittrack_[index]);
		BASS_StreamFree(audio_->hittrack_[index]);
		free(audio_->hitdata_[index]);
	}
	
	audio_->hittrack_[index] = newstream;
	audio_->hitdata_[index] = newdat;
	audio_->hitfreq_ = samplerate;
	
	BASS_StreamFree(stream);
	free(data);
}

void generate_calibration()
{
	HSTREAM stream = BASS_StreamCreateFile(FALSE,"sound_effects/bass_kick.wav",0,0,BASS_STREAM_DECODE);
	
	if(!stream)
	{
		printf("CANNOT MAKE STREAM: %d\n", BASS_ErrorGetCode());
		return;
	}
	
	BASS_CHANNELINFO info_;
	BASS_ChannelGetInfo(stream,&info_);
	
	QWORD bytes = BASS_ChannelGetLength(stream,BASS_POS_BYTE);
	char* data = (char*)malloc(bytes);
	if(BASS_ChannelGetData(stream,data,bytes) == -1)
	{
		printf("CANNOT GET DATA: %d\n", BASS_ErrorGetCode());
		free(data);
		return;
	}
	
	int samplerate = info_.freq;
	int channels = info_.chans;
	int resolution;
	if(info_.flags & BASS_SAMPLE_8BITS)
		resolution = 1;
	else if(info_.flags & BASS_SAMPLE_FLOAT)
		resolution = 4;
	else
		resolution = 2;
	
	int bps = resolution*8;
	int bytespersamp = bps*channels/8;
	int byteLength = (int)(samplerate*channels*resolution*60);
	
	while(byteLength % 8 != 0)
		byteLength++;
	
	struct WAV_header header = {
		"RIFF",
		sizeof(struct WAV_header)+byteLength-8,
		"WAVE",
		"fmt ",
		16,
		1,
		channels,
		samplerate,
		(samplerate*bps*channels)/8,
		bytespersamp,
		bps,
		"data",
		byteLength
	};

	char* newdat = (char*)malloc(byteLength+sizeof(struct WAV_header));
	memset(newdat,0,byteLength+sizeof(struct WAV_header));
	memcpy(newdat,&header,sizeof(struct WAV_header));
	
	char* dat = newdat+sizeof(struct WAV_header);
	float time;
	int curr = 0;
	
	for(int i = 0; i < byteLength; i++)
	{
		time = (float)i/bytespersamp/samplerate*1000;
		if(round(time) >= curr*500)
		{
			curr++;
			if(byteLength - i >= bytes)
				memcpy(dat+i,data,bytes);
			else
				memcpy(dat+i,data,(byteLength-i));
		}
	}

	HSTREAM newstream = BASS_StreamCreateFile(TRUE,newdat,0,sizeof(struct WAV_header)+byteLength,BASS_STREAM_PRESCAN);
	if(!newstream)
	{
		free(data);
		free(newdat);
		return;
	}
	
	BASS_ChannelSetAttribute(newstream,BASS_ATTRIB_VOL,audio_->sevolume_*audio_->volume_);
	BASS_ChannelSetAttribute(newstream,BASS_ATTRIB_FREQ,samplerate*audio_->pitch_);
	
	if(audio_->hitdata_[0] != NULL)
	{
		stop_track(&audio_->hittrack_[0]);
		BASS_StreamFree(audio_->hittrack_[0]);
		free(audio_->hitdata_[0]);
	}
	
	audio_->hittrack_[0] = newstream;
	audio_->hitdata_[0] = newdat;
	audio_->hitfreq_ = samplerate;
	
	BASS_StreamFree(stream);
	free(data);
}

void mute_hittrack(int m)
{
	for(int i = 0; i < 2; i++)
	{
		if(audio_->hitdata_[i] != NULL)
			BASS_ChannelSetAttribute(audio_->hittrack_[i],BASS_ATTRIB_VOL,m==0?audio_->sevolume_*audio_->volume_:0);
	}
}

void destroy_hittrack()
{
	for(int i = 0; i < 2; i++)
	{
		if(audio_->hitdata_[i] != NULL)
		{
			stop_track(&audio_->hittrack_[i]);
			BASS_StreamFree(audio_->hittrack_[i]);
			free(audio_->hitdata_[i]);
			
			audio_->hitdata_[i] = NULL;
			audio_->hittrack_[i] = 0;
		}
	}
}

void set_tracklink(HSTREAM* track1, HSTREAM* track2)
{
	BASS_ChannelSetLink(*track1,*track2);
}

void set_trackloop(HSTREAM* track, int loop)
{
	BASS_ChannelFlags(*track,loop*BASS_SAMPLE_LOOP,BASS_SAMPLE_LOOP);
}
