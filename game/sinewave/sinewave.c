#include "sinewave.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define GOOD_HIT 50/3.f
#define PASSING_HIT 65/3.f
#define REPLAY_VERSION 1.00

static kpqueue* keypresses = NULL;
static int key_states[] = {0,0,0,0};
static int reset_time;
static FILE* fp;

static const char *vertexShader = "#version 130\n"
	"#extension GL_ARB_explicit_attrib_location : enable\n"
	"layout (location = 0) in vec3 vertex;"
	"uniform float trans;"
	"uniform float time;"
	"uniform vec4 color;"
	"out vec4 ourColor;"
	"void main()"
	"{"
	"	gl_Position = vec4(vertex.x - trans, vertex.y, 0.0, 1.0);"	
	"	if(abs(vertex.z) <= time)"
	"	{"
	"		float col = (vertex.x - trans+1.0)/(1.0/3.0);"
	"		ourColor = vec4(color.x*col,color.y*col,color.z*col,color.w);"
	"	}"
	"	else"
	"	{"
	"		if(vertex.z < 0)\n"
	"			ourColor = vec4(0,0,0,0);\n"
	"		else\n"
	"			ourColor = vec4(color.x*0.25,color.y*0.25,color.z*0.25,color.w);"
	"	}"
	"}\0";
	
static const char *fragmentShader = "#version 130\n"
	"out vec4 FragColor;"
	"in vec4 ourColor;"
	"void main()"
	"{"
		"FragColor = ourColor;"
	"}\0";

static int process_noinput()
{
	MSG msg;
	int result_ = 0;
	while(PeekMessage(&msg, video_->hWnd, 0, 0, PM_NOREMOVE))
	{
		if(GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg); 
			
			switch(msg.message) {
				case WM_KEYDOWN:
					switch (msg.wParam) {
						case VK_ESCAPE:
							play_soundeffect(MENU_BACK);
							result_ = -1;
							break;
						case VK_SPACE:
							result_ = 1;
							break;
					}
				break;
			}
			DispatchMessage(&msg); 
		}
		else
			result_ = -1;
	}
						
	return result_;
}

static int process_practice(int elapsed)
{
	MSG msg;
	int result_ = 0;
	
	while(PeekMessage(&msg, video_->hWnd, 0, 0, PM_NOREMOVE))
	{
		if(GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg); 
			
			switch(msg.message) {
				case WM_KEYDOWN:
					
					if(msg.wParam == KEYS_[0])
					{
						if(!key_states[0])
						{
							key_states[0] = 1;
							insert_keypress(0,elapsed/audio_->pitch_);
							update_keygraphic(0,elapsed/audio_->pitch_);
						}
						break;
					}
					else if(msg.wParam == KEYS_[1])
					{
						if(!key_states[1])
						{
							key_states[1] = 1;
							insert_keypress(1,elapsed/audio_->pitch_);
							update_keygraphic(1,elapsed/audio_->pitch_);
						}
						break;
					}
					else if(msg.wParam == KEYS_[2])
					{
						if(!key_states[2])
						{
							key_states[2] = 1;
							insert_keypress(2,elapsed/audio_->pitch_);
							update_keygraphic(2,elapsed/audio_->pitch_);
						}
						break;
					}
					else if(msg.wParam == KEYS_[3])
					{
						if(!key_states[3])
						{
							key_states[3] = 1;
							insert_keypress(3,elapsed/audio_->pitch_);
							update_keygraphic(3,elapsed/audio_->pitch_);
						}
						break;
					}
					
					switch (msg.wParam) {
						case VK_ESCAPE:
							result_ = -1;
							break;
						
						case VK_OEM_3:
							result_ = -2;
							break;
					}
					break;
				case WM_KEYUP:
					if(msg.wParam == KEYS_[0])
						key_states[0] = 0;
					else if(msg.wParam == KEYS_[1])
						key_states[1] = 0;
					else if(msg.wParam == KEYS_[2])
						key_states[2] = 0;
					else if(msg.wParam == KEYS_[3])
						key_states[3] = 0;
					
					break;
			}
			DispatchMessage(&msg); 
		}
		else
			result_ = -1;
	}
						
	return result_;
}

static int process_messages(int elapsed)
{
	MSG msg;
	int result_ = 0;
	int t = elapsed;
	
	while(PeekMessage(&msg, video_->hWnd, 0, 0, PM_NOREMOVE))
	{
		if(GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg); 
			
			switch(msg.message) {
				case WM_KEYDOWN:
					
					if(msg.wParam == KEYS_[0])
					{
						if(!key_states[0])
						{
							key_states[0] = 1;
							insert_keypress(0,elapsed/audio_->pitch_);
							update_keygraphic(0,elapsed/audio_->pitch_);
							fputc(0,fp);
							fwrite(&t,sizeof(int),1,fp);
						}
						break;
					}
					else if(msg.wParam == KEYS_[1])
					{
						if(!key_states[1])
						{
							key_states[1] = 1;
							insert_keypress(1,elapsed/audio_->pitch_);
							update_keygraphic(1,elapsed/audio_->pitch_);
							fputc(1,fp);
							fwrite(&t,sizeof(int),1,fp);
						}
						break;
					}
					else if(msg.wParam == KEYS_[2])
					{
						if(!key_states[2])
						{
							key_states[2] = 1;
							insert_keypress(2,elapsed/audio_->pitch_);
							update_keygraphic(2,elapsed/audio_->pitch_);
							fputc(2,fp);
							fwrite(&t,sizeof(int),1,fp);
						}
						break;
					}
					else if(msg.wParam == KEYS_[3])
					{
						if(!key_states[3])
						{
							key_states[3] = 1;
							insert_keypress(3,elapsed/audio_->pitch_);
							update_keygraphic(3,elapsed/audio_->pitch_);
							fputc(3,fp);
							fwrite(&t,sizeof(int),1,fp);
						}
						break;
					}
					
					switch (msg.wParam) {
						case VK_ESCAPE:
							play_soundeffect(MENU_BACK);
							result_ = -1;
							break;
						case VK_SPACE:
							result_ = 1;
							break;
						case VK_OEM_3:
							play_soundeffect(MENU_CONFIRM);
							if(reset_time == -1)
								reset_time = elapsed;
							else
								result_ = 2;
							break;
						}
					break;
				case WM_KEYUP:
					if(msg.wParam == KEYS_[0])
						key_states[0] = 0;
					else if(msg.wParam == KEYS_[1])
						key_states[1] = 0;
					else if(msg.wParam == KEYS_[2])
						key_states[2] = 0;
					else if(msg.wParam == KEYS_[3])
						key_states[3] = 0;
					
					break;
			}
			DispatchMessage(&msg); 
		}
		else
			result_ = -1;
	}
						
	return result_;
}

static int vertexProgram = -1;
static int NUM_LINES = 0;
static int songLength = 0;
static int songLengthSV = 0;
static int songOffset = 0;
static int firstNote = 0;
static int lastNote = 0;

int sinewave(chart_data* cd, score_struct* score)
{
	Beat** lines = init_lines(cd,0,0);
	
	reset_time = -1;
	
	float spd, rate, bpm;
	
	bpm = cd->svs->bpm;
	spd = 60000.f / bpm;
	rate = spd * 2;
	
	int vertexLocation = glGetUniformLocation(vertexProgram, "trans");
	int vertexTime = glGetUniformLocation(vertexProgram, "time");
	int vertexColor = glGetUniformLocation(vertexProgram, "color");
	
	//Strings for displaying info
	char combo_string[16],time_string[16],miss_string[16],pace_string[16],grade_string[16];
	
	memset(combo_string,0,sizeof(combo_string));
	memset(time_string,0,sizeof(time_string));
	memset(miss_string,0,sizeof(miss_string));
	memset(pace_string,0,sizeof(pace_string));
	memset(grade_string,0,sizeof(grade_string));
	
	sprintf(combo_string,"COMBO\n00000");
	sprintf(time_string,"TIME\n00:00");
	sprintf(miss_string,"MISS\n0000");
	sprintf(pace_string,"RP\n0");
	sprintf(grade_string,"GRADE\n");
	
	float deviation = 0, pace = 0, hiterr_neg = 0, hiterr_pos = 0, xp = 0, disp_xp = 0;
	int early = 0, late = 0, combo = 0, miss = 0, oops = 0, total_notes = 0;
	for(int i = 0; i < NUM_LINES; i++)
	{
		pace += cd->nNotes[i]*HIT_WINDOW;
		total_notes += cd->nNotes[i];
	}
	
	float difficulty = run_diffcalc(cd,audio_->pitch_);
	
	reset_keygraphic();
	
	int return_code = 0, offset_passed = 0, finished_track = 0, quitting = 0;
	double elapsed = 0, lastdraw = 0, fade_start = 0;
	
	struct sv_data* currsv = cd->svs->tail;
	
	float s=spd, r=rate;
	int32_t lastsv = 0;
	double lastsvtime = 0.0;
	
	LARGE_INTEGER freq,start,end;
	QueryPerformanceFrequency(&freq); 
	QueryPerformanceCounter(&start);
	
	char replayfile[256];
	float version = REPLAY_VERSION;
	int keymap[4], replay_key, replay_time = 999999999;
	
	memcpy(keymap,KEYMAP_,sizeof(int)*4);
	
	if(CreateDirectory("replays",NULL))
		SetFileAttributes("replays",FILE_ATTRIBUTE_HIDDEN);
	
	if(score->result == 0)
	{
		time_t time_now = time(NULL);
		
		memset(replayfile,0,sizeof(replayfile));
		
		score->date = *localtime(&time_now);
		score->deviation = 0;
		score->miss = 0;
		
		sprintf(replayfile,"replays/%d-%d%02d%02d%02d%02d%02d.bin",
					score->id,score->date.tm_year+1900,score->date.tm_mon+1,
						score->date.tm_mday,score->date.tm_hour,score->date.tm_min,
							score->date.tm_sec);
		
		fp = fopen(replayfile,"wb");
		
		if(!fp)
		{
			write_error("ERROR::UNABLE TO CREATE REPLAY FILE",NULL,0);
			write_error(strerror(errno),NULL,1);
		}
		else
		{
			fwrite(&version,sizeof(float),1,fp);
			fwrite(KEYMAP_,sizeof(int),4,fp);
			fwrite(&songOffset,sizeof(int),1,fp);
			fwrite(&audio_->pitch_,sizeof(float),1,fp);
		}
	}
	else
	{
		sprintf(replayfile,"replays/%d-%d%02d%02d%02d%02d%02d.bin",
					score->id,score->date.tm_year+1900,score->date.tm_mon+1,
						score->date.tm_mday,score->date.tm_hour,score->date.tm_min,
							score->date.tm_sec);
		
		fp = fopen(replayfile,"rb");
		if(!fp)
		{
			write_error("ERROR::UNABLE TO OPEN REPLAY FILE",replayfile,0);
			write_error(strerror(errno),NULL,1);
			return_code = -2;
		}
		else
		{
			fread(&version,sizeof(float),1,fp);
			fread(KEYMAP_,sizeof(int),4,fp);
			fread(&songOffset,sizeof(int),1,fp);
			fseek(fp,sizeof(float),SEEK_CUR);
			
			if(!feof(fp))
			{
				replay_key = fgetc(fp);
				fread(&replay_time,sizeof(int),1,fp);
			}
		}
		video_->settings_[OFFSET].value_ = 0;
	}
	
	int offset_check = 0, nbeeps = 0;
	
	while(!return_code)
	{
		if(get_frameready())
		{
			if(!offset_passed)
			{
				QueryPerformanceCounter(&end);
				
				elapsed = (end.QuadPart - start.QuadPart)*1000/freq.QuadPart * audio_->pitch_;
				if(elapsed >= songOffset-video_->settings_[OFFSET].value_-10)
				{
					play_track(0,1.0);
					offset_passed = 1;
				}
			}
			else if(finished_track)
			{
				QueryPerformanceCounter(&end);
				elapsed += (end.QuadPart-start.QuadPart)*1000.0/freq.QuadPart*0.1;
				start = end;
			}
			else if(elapsed >= songLength)
			{
				if(!offset_check)
				{
					QueryPerformanceCounter(&start);
					offset_check = 1;
				}
				QueryPerformanceCounter(&end);
				
				elapsed = songLength + (end.QuadPart-start.QuadPart)*1000.0/freq.QuadPart;
			}
			else
			{
				elapsed = get_trackpos(NULL)*1000.0+songOffset-video_->settings_[OFFSET].value_;
			}
			
			while(currsv != NULL && currsv->time <= elapsed)
			{
				if(currsv->type == 0)
					s = spd / currsv->bpm;
				else
					s = spd * (cd->svs->bpm/currsv->bpm);
				r = s*2;
				lastsvtime = currsv->svtime;
				lastsv = currsv->time;
				currsv = currsv->tail;
			}
			
			//Get replay hits
			if(score->result != 0)
			{
				while(elapsed/audio_->pitch_ >= replay_time/audio_->pitch_)
				{
					insert_keypress(replay_key,replay_time/audio_->pitch_);
					update_keygraphic(replay_key,replay_time/audio_->pitch_);
					
					if(!feof(fp))
					{
						replay_key = fgetc(fp);
						fread(&replay_time,sizeof(int),1,fp);
					}
					else
						replay_time = 999999999;
				}
			}
			
			//Check for hits
			while(keypresses != NULL)
			{
				int32_t nextnote = 999999999, nextnote_ind = 0, nextnote_i = -1;
				for(int i = 0; i < NUM_LINES; i++)
				{
					if(lines[i]->currNote != NULL)
					{
						if((int)round(lines[i]->currNote->time/audio_->pitch_) < nextnote)
						{
							nextnote = (int)round(lines[i]->currNote->time/audio_->pitch_);
							nextnote_ind = (1 << i);
							nextnote_i = i;
						}
						else if((int)round(lines[i]->currNote->time/audio_->pitch_) == nextnote)
							nextnote_ind |= (1 << i);
					}
				}
				
				if(video_->settings_[HITSOUNDS].value_)
					play_soundeffect(HITSOUND1+KEYMAP_[keypresses->key]);
				
				if(abs(keypresses->time-nextnote)<HIT_WINDOW)
				{
					int hit_time = (float)(keypresses->time-nextnote);
					
					//Update deviation
					if(hit_time < 0)
					{
						hiterr_neg += hit_time;
						early++;
					}
					else 
					{
						hiterr_pos += hit_time;
						late++;
					}
					
					//See if the correct key was pressed
					if(nextnote_ind & (1<<KEYMAP_[keypresses->key]))
						nextnote_ind = KEYMAP_[keypresses->key];
					else
						nextnote_ind = nextnote_i;
					
					
					if(KEYMAP_[keypresses->key] == nextnote_ind)
					{
						deviation += abs(hit_time);
						pace -= (HIT_WINDOW-abs(hit_time));
						lines[nextnote_ind]->currNote->hittime = hit_time+HIT_WINDOW;
						
						
						if(abs(hit_time) <= GOOD_HIT)
							add_hitanimation(lines[nextnote_ind]->currNote->svtime,(nextnote_ind==0?LRED_:LBLUE_));
						
						if(video_->settings_[HESTYLE].value_)
						{
							float hity;
							if(video_->settings_[HESTYLE].value_ == 1)
								hity = (float)hit_time/HIT_WINDOW*1/3.f;
							else
								hity = lines[nextnote_ind]->point.y;
							
							if(hit_time < 0)
								add_hiterrorpoint(hity,(float)keypresses->time,(abs(hit_time)<=PASSING_HIT?GREEN_:TEAL_));
							else
								add_hiterrorpoint(hity,(float)keypresses->time,(abs(hit_time)<=PASSING_HIT?GREEN_:YELLOW_));
						}
						
						combo = -abs(combo)-1;
					}
					else
					{	
						deviation += HIT_WINDOW;
						lines[nextnote_ind]->currNote->hittime = hit_time-HIT_WINDOW;
						
						if(video_->settings_[HESTYLE].value_)
						{
							float hity;
							if(video_->settings_[HESTYLE].value_ == 1)
								hity = fabs(lines[nextnote_ind]->point.y)*(hit_time<0?-1:1);
							else
								hity = lines[nextnote_ind]->point.y;
							
							add_hiterrorpoint(hity,keypresses->time,RED_);
						}
						
						oops = -abs(oops)-1;
						combo = 0;
						sprintf(combo_string,"COMBO\n%05d",combo);
					}
					
					
					lines[nextnote_ind]->currNote = lines[nextnote_ind]->currNote->tail;
					pop_keypress();
				}
				else
					clear_keypresses();
			}
			
			//Check for misses
			if(!finished_track && !quitting)
			{
				for(int i = 0; i < NUM_LINES; i++)
				{
					if(lines[i]->currNote != NULL)
					{
						if(elapsed/audio_->pitch_ >= lines[i]->currNote->time/audio_->pitch_ + HIT_WINDOW)
						{
							if(video_->settings_[HESTYLE].value_)
								add_hiterrorpoint(0,elapsed/audio_->pitch_,RED_);
							
							deviation += HIT_WINDOW;
							miss = -abs(miss)-1;
							combo = 0;
							sprintf(combo_string,"COMBO\n%05d",combo);
							lines[i]->currNote->hittime = HIT_WINDOW*2+1;
							lines[i]->currNote = lines[i]->currNote->tail;
						}
					}
					else
						finished_track++;
				}
				if(finished_track == NUM_LINES && elapsed >= songLength)
				{
					QueryPerformanceCounter(&start);
					fade_start = elapsed;
					finished_track = 1;
				}
				else
					finished_track = 0;
			}
			
			glClear(GL_COLOR_BUFFER_BIT);
			
			double total_time = (elapsed-lastsv)*(rate/r)+lastsvtime;
			
			for(int i = 0; i < NUM_LINES; i++)
			{	
				update_line(lines[i],cd->svs,cd->notes[i],total_time);
				if(video_->lastProgram_ != vertexProgram)
				{
					glUseProgram(vertexProgram);
					video_->lastProgram_ = vertexProgram;
				}
				
				lines[i]->point.x = total_time/rate;
				
				glUniform1f(vertexLocation, total_time/rate+1/3.0);
				glUniform1f(vertexTime,total_time);
				glUniform4f(vertexColor, (i == 0 ? 1 : 0), (i == 2 ? 1 : 0), (i == 1 ? 1 : 0), 1.0);
				
				glBindVertexArray(lines[i]->VAO[lines[i]->vindex%BUFFER_BUFFER]);  
				glDrawArrays(GL_TRIANGLE_STRIP, 0, lines[i]->nvertices);
				
				glBindBuffer(GL_ARRAY_BUFFER, lines[i]->VBO[lines[i]->vindex%BUFFER_BUFFER]);
				glBufferData(GL_ARRAY_BUFFER, lines[i]->nvertices*sizeof(struct vertex), NULL, GL_DYNAMIC_DRAW);
				
				lines[i]->vindex++;
				
				switch(i)
				{
					case 0:
						draw_circle(-1/3.f,lines[i]->point.y,RED_,0.0375,0);
					break;
					case 1:
						draw_circle(-1/3.f,lines[i]->point.y,BLUE_,0.0375,0);
					break;
					case 2:
						draw_circle(-1/3.f,lines[i]->point.y,GREEN_,0.0375,0);
					break;
				}
			}
			
			draw_hitanimations(total_time,spd);
			
			//Draw lines at start and end of map
			if(firstNote-spd*6 >= total_time-spd*8/6 && firstNote-spd*6 < total_time+spd*8/3)
			{
				if(total_time >= firstNote-spd*8)
					render_simpletext("GET READY",0.0,0.75,WHITE_,2,TXT_CENTERED|TXT_TOPALIGNED,NULL);
				draw_line((firstNote-spd*6 - total_time)/(spd*4/3)*4/3.f-1/3.f,0.0,WHITE_,0.5,1);
				
				if(elapsed+video_->settings_[OFFSET].value_ >= firstNote-spd*8+spd*nbeeps)
				{
					play_soundeffect(MENU_BEEP);
					nbeeps++;
				}
			}
			else if(firstNote >= total_time-spd*8/6 && firstNote < total_time+spd*8/3)
				draw_line((firstNote - total_time)/(spd*8/3)*4/3.f-1/3.f,0.0,WHITE_,0.5,1);
			else if(lastNote >= total_time-spd*8/6 && lastNote < total_time+spd*8/3)
				draw_line((lastNote - total_time)/(spd*8/3)*4/3.f-1/3.f,0.0,WHITE_,0.5,1);
			
			/*for(int i = 0; i < nmeasures; i++)
			{
				if(measure_lines[i] >= total_time-spd*8/6 && measure_lines[i] < total_time+spd*8/3)
					draw_line((measure_lines[i] - total_time)/(spd*8/3)*4/3.f-1/3.f,0.0,WHITE_,0.5,1);
				else if(measure_lines[i] > total_time+spd*8/3)
					break;
			}*/
			
			//Draw Game Text
			if(elapsed < songLength)
				sprintf(time_string, "TIME\n%-2.2d:%-2.2d", (int)((songLength-elapsed)/audio_->pitch_/60000),(int)(fmod((songLength-elapsed)/audio_->pitch_,60000)/1000));
			else
				sprintf(time_string, "TIME\n00:00");
			
			if(combo < 0)
			{
				combo = abs(combo);
				sprintf(combo_string,"COMBO\n%05d",combo);
			}
			if(miss < 0 || oops < 0)
			{
				miss = abs(miss);
				oops = abs(oops);
				sprintf(miss_string,"MISS\n%04d",miss+oops);
			}
			if(early+late+miss >= 10)
			{
				float d = deviation/(early+late+miss);
				sprintf(grade_string, "GRADE\n%s", get_gradetext(update_gradebar(d,elapsed-lastdraw)));
			
				if(pace/total_notes < HIT_WINDOW)
					xp = ((1/pow((pace/total_notes)/10,2)-1/pow(HIT_WINDOW/10,2))*pow(difficulty,2));
			}
			
			if(disp_xp < xp)
			{
				disp_xp += (elapsed-lastdraw)/10.f;
				if(disp_xp > xp)
					disp_xp = xp;
				sprintf(pace_string, "RP\n%d", (int)disp_xp);
			}
			
			float gradepos;
			render_simpletext(combo_string,-1.0,1.0,WHITE_,3.25,TXT_TOPALIGNED,NULL);
			render_simpletext(miss_string,-0.5,1.0,WHITE_,3.25,TXT_CENTERED|TXT_TOPALIGNED,NULL);
			render_simpletext(time_string,0.0,1.0,WHITE_,3.25,TXT_CENTERED|TXT_TOPALIGNED,NULL);
			render_simpletext(pace_string,0.5,1.0,WHITE_,3.25,TXT_CENTERED|TXT_TOPALIGNED,NULL);
			render_simpletext(grade_string,1.0,1.0,WHITE_,3.25,TXT_RGHTALIGNED|TXT_TOPALIGNED,&gradepos);
			
			//Draw Scrolling Text
			render_scrolltext((elapsed-lastdraw)/audio_->pitch_,WHITE_,2.5,TXT_BOTALIGNED);
			
			//Draw Grade Bar
			draw_gradebar(1.0,gradepos,3.25,3);
			
			//Draw Center Line
			draw_linea(0.0,0.0,WHITE_,0.5,1.0,0);
			
			//Draw Hit Error
			if(video_->settings_[HESTYLE].value_)
				draw_hiterror(0.0,0.0,elapsed/audio_->pitch_);
			
			//Draw Keyboard Graphic
			draw_keygraphic(0.f,-0.965,elapsed/audio_->pitch_);
			
			//Draw Background Image
			draw_image(-0.8,-0.9625,0.2,0.15);
			
			if(elapsed < firstNote - spd*12 && offset_passed && !finished_track)
			{
				render_simpletext("PRESS SPACE TO SKIP INTRO",0.0,0.75,WHITE_,2,TXT_CENTERED|TXT_TOPALIGNED,NULL);
			}
			
			if(finished_track || quitting)
			{
				if(elapsed-fade_start >= (quitting==1?500:200))
				{
					return_code = (quitting==1?-1:1);
					break;
				}
				else
				{
					float doll = (elapsed-fade_start)/(quitting==1?500:200);
					
					glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
					draw_rectangle(0.f,0.f,1.f,1.f,BLACK_,doll,0);
					glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
					
					set_trackvolume(1.0-2/3.f*doll);
				}
				
			}
			else if(elapsed < 1000)
			{
				glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
				draw_rectangle(0.f,0.f,1.f,1.f,BLACK_,(1000-elapsed)/1000,0);
				glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
			}
		
			if(reset_time != -1)
				if(elapsed - reset_time >= 250*audio_->pitch_)
					reset_time = -1;
			
			draw_frametimer(1.0,-1.0,TXT_BOTALIGNED|TXT_RGHTALIGNED);
			if(video_->settings_[SCREEN_BUFFER].value_)
				SwapBuffers(video_->hDC);
			else
				glFlush();
			
			update_frametimer();
			
			lastdraw = elapsed;
			
		}
		
		if(score->result == 0)
			return_code = process_messages(round(elapsed));
		else
			return_code = process_noinput();
		
		if(score->result == 0 && return_code == -1 && !finished_track && !quitting && offset_passed)
		{
			set_trackvolume(1/3.f);
			QueryPerformanceCounter(&start);
			fade_start = elapsed;
			quitting = 1;
			return_code = 0;
		}
		else if(return_code == 1)
		{
			if(elapsed < firstNote - spd*12 && offset_passed && !finished_track)
			{
				if(score->result != 0)
				{
					while((firstNote - spd*12) >= replay_time)
					{
						if(!feof(fp))
						{
							replay_key = fgetc(fp);
							fread(&replay_time,sizeof(int),1,fp);
						}
						else
							replay_time = 999999999;
					}
				}
				set_trackpos(NULL,(int)(firstNote-spd*12-songOffset));
			}
			return_code = 0;
		}
		
		update_fps();
		cpu_wait();
	}
	
	if(fp)
		fclose(fp);
	
	if(return_code == -1 && finished_track == 1)
		return_code = 1;
	
	if(score->result == 0)
	{
		score->deviation = (deviation+(total_notes-(early+late+miss))*HIT_WINDOW)/total_notes;
		score->miss = miss + oops + (total_notes-(early+late+miss));
		
		dbupdate_stat(early+late,1);
	}
	else
	{
		memcpy(KEYMAP_,keymap,sizeof(int)*4);
		score->miss = miss + oops;
		video_->settings_[OFFSET].value_ = video_->settings_[OFFSET].update_;
	}
	
	if(early+late == 0)
		if(return_code != 2)
			return_code = -2;
	
	if(score->result == 0 && (return_code == 2 || return_code == -2))
		remove(replayfile);
	
	clear_keypresses();
	destroy_hitanimation();
	
	destroy_hiterror();
	destroy_lines(lines);
	
	return return_code;
}

Beat** init_lines(chart_data* cd, double time, int flag)
{
	Beat** lines;
	
	NUM_LINES = atoi(get_datavalue(cd->infodata,"LINES"));
	lines = (Beat**)malloc(sizeof(Beat*)*NUM_LINES);
	
	for(int i = 0; i < NUM_LINES; i++)
		lines[i] = NULL;
	
	songOffset = 0;
	songLength = 0;
	songLengthSV = 0;
	lastNote = 0;
	
	float spd = 60000.f / cd->svs->bpm;
	int firstnote = 999999999;
	
	for(int i = 0; i < NUM_LINES; i++)
		if(cd->notes[i]->time < firstnote)
			firstnote = cd->notes[i]->time;
	if(firstnote == 999999999)
		firstnote = 0;
	
	firstNote = firstnote;
	
	for(int i = 0; i < NUM_LINES; i++)
	{
		struct note_data* note_iter = cd->notes[i];
		while(note_iter != NULL)
		{
			if(note_iter->time > songLength)
				songLength = note_iter->time;
			note_iter = note_iter->tail;
		}
	}
	
	if(flag == 0)
	{
		//Generate three measures of silence if the audio intro isn't long enough.
		if(firstnote < spd*12)
			songOffset = spd*12 - firstnote;
		firstNote = firstnote+songOffset;
		
		//Adjust note & sv timings to the offset
		struct sv_data* sv_iter = cd->svs->tail;
		while(sv_iter != NULL)
		{
			sv_iter->time += songOffset;
			sv_iter = sv_iter->tail;
		}
		for(int i = 0; i < NUM_LINES; i++)
		{
			struct note_data* note_iter = cd->notes[i];
			while(note_iter != NULL)
			{
				note_iter->time += songOffset;
				if(note_iter->time > songLength)
					songLength = note_iter->time;
				note_iter = note_iter->tail;
			}
		}
	}
	
	/*if(measure_lines != NULL)
		free(measure_lines);
	nmeasures = (songLength-firstNote)/(spd*4) + 1;
	measure_lines = (float*)malloc(sizeof(float)*nmeasures);
	memset(measure_lines,0,sizeof(float)*nmeasures);*/

	for(int i = 0; i < NUM_LINES; i++)
	{
		int32_t length = calculate_svtime(cd->svs,cd->notes[i],flag);
		lines[i] = allocate_line(i,spd);
		lines[i]->currNote = cd->notes[i];
		if(length > songLengthSV)
			songLengthSV = length;
		
		/*lines[i] = allocate_full(i,spd,cd->notes[i]);
		load_line(lines[i],cd->svs,cd->notes[i]);*/
		
		if(flag == 1)
		{
			while(lines[i]->currNote != NULL)
			{
				if(time < lines[i]->currNote->time)
					break;
				lines[i]->currNote = lines[i]->currNote->tail;	
			}
			lines[i]->bufferpos = 999999999;
		}
	}
	
	if(vertexProgram == -1)
		vertexProgram = create_program(vertexShader, fragmentShader);
	reset_gradebar();
	
	return lines;
}

double preview_sinewave(Beat** lines, chart_data* cd, double elapsed, int flag)
{
	float spd = 60000.0/cd->svs->bpm, rate = spd*2, s = spd, r = rate;
	int32_t lastsv = 0;
	double lastsvtime = 0.0;
	
	struct sv_data* currsv = cd->svs->tail;

	while(currsv != NULL && currsv->time <= elapsed)
	{
		if(currsv->type == 0 && flag == 0)
			s = spd / currsv->bpm;
		else if(currsv->type == 1)
			s = spd * (cd->svs->bpm/currsv->bpm);
		r = s*2;
		lastsvtime = currsv->svtime;
		lastsv = currsv->time;
		currsv = currsv->tail;
	}
	
	double total_time = (elapsed-lastsv)*(rate/r)+lastsvtime;
	
	glUseProgram(vertexProgram);
			
	int vertexLocation = glGetUniformLocation(vertexProgram, "trans");
	int vertexTime = glGetUniformLocation(vertexProgram, "time");
	int vertexColor = glGetUniformLocation(vertexProgram, "color");
	
	/*for(int i = 0; i < NUM_LINES; i++)
	{	

		//update_line(lines[i],cd->svs,cd->notes[i],total_time,&point);
		glUseProgram(vertexProgram);
		if(elapsed < spd*8/3)
			glUniform1f(vertexLocation,total_time/rate+1/3.0);
		else
			glUniform1f(vertexLocation,total_time/rate+1/3.0);
		glUniform1f(vertexTime,total_time);

		glUniform4f(vertexColor, (i == 0 ? 1 : 0), (i == 2 ? 1 : 0), (i == 1 ? 1 : 0), 1.0);
		glBindVertexArray(lines[i]->VAO[lines[i]->vindex%BUFFER_BUFFER]); 
		glDrawArrays(GL_TRIANGLE_STRIP, (int)(total_time-spd*4/3)*2, (int)spd*8);
		//lines[i]->vindex++;
		
		draw_circle(-1/3.f,(lines[i]->vertices[(int)(total_time)*2].y+lines[i]->vertices[(int)(total_time)*2+1].y)/2,(float[]){(i==0?1:0),(i==2?1:0),(i==1?1:0),1.0},0.0375,0);
	}*/
	
	for(int i = 0; i < NUM_LINES; i++)
	{	
		update_line(lines[i],cd->svs,cd->notes[i],total_time);
		if(video_->lastProgram_ != vertexProgram)
		{
			glUseProgram(vertexProgram);
			video_->lastProgram_ = vertexProgram;
		}
		
		glUniform1f(vertexLocation, total_time/rate+1/3.0);
		glUniform1f(vertexTime,total_time);
		glUniform4f(vertexColor, (i == 0 ? 1 : 0), (i == 2 ? 1 : 0), (i == 1 ? 1 : 0), 1.0);
		
		glBindVertexArray(lines[i]->VAO[lines[i]->vindex%BUFFER_BUFFER]);  
		glDrawArrays(GL_TRIANGLE_STRIP, 0, lines[i]->nvertices);
		
		glBindBuffer(GL_ARRAY_BUFFER, lines[i]->VBO[lines[i]->vindex%BUFFER_BUFFER]);
		glBufferData(GL_ARRAY_BUFFER, lines[i]->nvertices*sizeof(struct vertex), NULL, GL_DYNAMIC_DRAW);
		
		lines[i]->vindex++;
		
		switch(i)
		{
			case 0:
				draw_circle(-1/3.f,lines[i]->point.y,RED_,0.0375,0);
			break;
			case 1:
				draw_circle(-1/3.f,lines[i]->point.y,BLUE_,0.0375,0);
			break;
			case 2:
				draw_circle(-1/3.f,lines[i]->point.y,GREEN_,0.0375,0);
			break;
		}
	}

	/*int measure = (int)((elapsed-lastsv)/(s*4));
	for(int i = measure; i < measure+2; i++)
		draw_line(((lastsv==0?firstNote:lastsv)+i*(s*4)-elapsed)/(s*4)*2.f-1.f+2/3.f,0.0,WHITE_,0.5,1);*/
	
	draw_line(0.0,0.0,WHITE_,1.0,0);
	
	return total_time;
}

int practice_sinewave(float bpm, char* pattern, int flag, int swap)
{
	chart_data* cd = create_chartdata("1");
	NUM_LINES = 2;
	
	update_datavalue(cd->infodata,"CREATOR","rstdev");
	update_datavalue(cd->infodata,"LINES","2");
	add_sv(cd,-1,1,bpm);
	
	cd->notes = (struct note_data**)malloc(sizeof(struct note_data*)*NUM_LINES);
	cd->nNotes = (int*)malloc(sizeof(int)*NUM_LINES);
	for(int i = 0; i < NUM_LINES; i++)
	{
		cd->notes[i] = NULL;
		cd->nNotes[i] = 0;
	}
	
	float spd = 60000.0/cd->svs->bpm, rate = spd*2;
	int t = 100, lastnote = 0, early = 0, late = 0, miss = 0, patlength = strlen(pattern);
	
	songOffset = 2000;
	
	for(int i = 0; i < t; i++)
	{
		for(int j = 0; j < patlength; j++)
		{
			int ind = pattern[j]-'0';
			
			for(int k = 0; k < NUM_LINES; k++)
			{
				if((ind >> k) & 1)
				{
					lastnote = (int)((float)j/16.f*spd*4+i*((float)patlength/16.f)*spd*4);
					add_note(cd,k,lastnote+songOffset);
				}
			}	
		}
	}
	
	for(int i = 0; i < NUM_LINES; i++)
	{
		if(cd->nNotes[i] == 0)
			add_note(cd,i,lastnote+songOffset);
	}
	
	audio_->length_ = lastnote+10000;
	
	Beat** lines = init_lines(cd,0,1);
	
	songOffset = 2000;
	
	LARGE_INTEGER freq,start,end;
	QueryPerformanceFrequency(&freq); 
	QueryPerformanceCounter(&start);
	
	glUseProgram(vertexProgram);
			
	int vertexLocation = glGetUniformLocation(vertexProgram, "trans");
	int vertexTime = glGetUniformLocation(vertexProgram, "time");
	int vertexColor = glGetUniformLocation(vertexProgram, "color");
	
	double elapsed = 0, lastdraw = 0, fade_start = 0, pause_offset = 0;
	float deviation = 0;
	int return_code = 0, finished_track = 0, paused = 0;
	int grade_arr[9];
	
	memset(grade_arr,0,sizeof(grade_arr));
	
	struct note_data* curr_note[NUM_LINES];
	int pass_note = 0, patterncount = 0;
	
	for(int i = 0; i < NUM_LINES; i++)
		curr_note[i] = cd->notes[i];

	//Strings for displaying info
	char time_string[16], grade_string[16],grade_out[128];
	memset(time_string,0,sizeof(time_string));
	memset(grade_string,0,sizeof(grade_string));
	memset(grade_out,0,sizeof(grade_out));
	
	sprintf(time_string,"TIME\n00:00");
	sprintf(grade_string,"GRADE\n");
	
	reset_keygraphic();
	
	char buffer[32];
	for(int i = 0; i < 8; i++)
	{
		sprintf(buffer,"%s: %d\n",get_gradetext_index(i),grade_arr[i]);
		strcat(grade_out,buffer);
	}
	
	set_pitch(1.0);
	
	while(!return_code)
	{
		if(get_frameready())
		{
			if(!paused)
			{
				QueryPerformanceCounter(&end);
				elapsed = (end.QuadPart-start.QuadPart)*1000/freq.QuadPart+pause_offset;
			}
			
			if(!flag && elapsed-songOffset >= patterncount*((float)patlength/16.f)*spd*4)
			{
				if(early+late+miss >= 10 && patterncount > 0)
				{
					int g = get_grade(deviation/(early+late+miss));
					grade_arr[g]++;
					
					memset(grade_out,0,sizeof(grade_out));
					for(int i = 0; i < 8; i++)
					{
						sprintf(buffer,"%s: %d\n",get_gradetext_index(i),grade_arr[i]);
						strcat(grade_out,buffer);
					}
				}
				patterncount++;
				early = 0;
				late = 0;
				miss = 0;
				deviation = 0;
				
				sprintf(grade_string,"GRADE\n");
				reset_gradebar();
			}
			
			pass_note = 0;
			for(int i = 0; i < NUM_LINES; i++)
			{
				if(curr_note[i] != NULL && (elapsed-video_->settings_[SEOFFSET].value_) >= curr_note[i]->time)
				{
					pass_note |= (1<<i);
					curr_note[i] = curr_note[i]->tail;
				}
			}
			if(swap)
			{
				switch(pass_note)
				{	
					case 1:
					play_soundeffect(SNARE_DRUM);
					play_soundeffect(BASS_KICK);
					break;
					case 2:
					play_soundeffect(BASS_KICK);
					break;
					case 3:
					play_soundeffect(HI_HAT);
					play_soundeffect(BASS_KICK);
					break;
				}
			}
			else
			{
				switch(pass_note)
				{	
					case 1:
					play_soundeffect(BASS_KICK);
					break;
					case 2:
					play_soundeffect(SNARE_DRUM);
					play_soundeffect(BASS_KICK);
					break;
					case 3:
					play_soundeffect(HI_HAT);
					play_soundeffect(BASS_KICK);
					break;
				}
			}
			
			//Check for hits
			while(keypresses != NULL)
			{
				if(paused)
				{
					paused = 0;
					QueryPerformanceCounter(&start);
					pop_keypress();
					continue;
				}
				
				int32_t nextnote = 999999999, nextnote_ind = 0, nextnote_i = -1;
				for(int i = 0; i < NUM_LINES; i++)
				{
					if(lines[i]->currNote != NULL)
					{
						if(lines[i]->currNote->time < nextnote)
						{
							nextnote = lines[i]->currNote->time;
							nextnote_ind = (1 << i);
							nextnote_i = i;
						}
						else if(lines[i]->currNote->time == nextnote)
							nextnote_ind |= (1 << i);
					}
				}
				
				if(video_->settings_[HITSOUNDS].value_)
					play_soundeffect(HITSOUND1+KEYMAP_[keypresses->key]);
				
				if(abs(keypresses->time-nextnote)<HIT_WINDOW)
				{
					int hit_time = (float)(keypresses->time-nextnote);
					
					//Update deviation
					if(hit_time < 0)
						early++;
					else 
						late++;
					
					//See if the correct key was pressed
					if(nextnote_ind & (1<<KEYMAP_[keypresses->key]))
						nextnote_ind = KEYMAP_[keypresses->key];
					else
						nextnote_ind = nextnote_i;
					
					
					if(KEYMAP_[keypresses->key] == nextnote_ind)
					{
						lines[nextnote_ind]->currNote->hittime = hit_time+HIT_WINDOW;
						deviation += abs(hit_time);
						
						if(abs(hit_time) <= GOOD_HIT)
							add_hitanimation(lines[nextnote_ind]->currNote->svtime,(nextnote_ind==0?LRED_:LBLUE_));
						
						if(video_->settings_[HESTYLE].value_)
						{
							float hity;
							if(video_->settings_[HESTYLE].value_ == 1)
								hity = (float)hit_time/HIT_WINDOW*1/3.f;
							else
								hity = lines[nextnote_ind]->point.y;
							
							if(hit_time < 0)
								add_hiterrorpoint(hity,(float)keypresses->time,(abs(hit_time)<=PASSING_HIT?GREEN_:TEAL_));
							else
								add_hiterrorpoint(hity,(float)keypresses->time,(abs(hit_time)<=PASSING_HIT?GREEN_:YELLOW_));
						}
					}
					else
					{	
						lines[nextnote_ind]->currNote->hittime = hit_time-HIT_WINDOW;
						deviation += HIT_WINDOW;
						
						if(video_->settings_[HESTYLE].value_)
						{
							float hity;
							if(video_->settings_[HESTYLE].value_ == 1)
								hity = fabs(lines[nextnote_ind]->point.y)*(hit_time<0?-1:1);
							else
								hity = lines[nextnote_ind]->point.y;
							
							add_hiterrorpoint(hity,(float)keypresses->time,RED_);
						}
					}
					lines[nextnote_ind]->currNote = lines[nextnote_ind]->currNote->tail;
					pop_keypress();
				}
				else
					clear_keypresses();
			}
			
			if(!finished_track)
			{
				//Check for misses
				for(int i = 0; i < NUM_LINES; i++)
				{
					if(lines[i]->currNote != NULL)
					{
						if(elapsed >= lines[i]->currNote->time + HIT_WINDOW)
						{
							if(video_->settings_[HESTYLE].value_)
								add_hiterrorpoint(0,elapsed,RED_);
							
							deviation += HIT_WINDOW;
							miss++;
							
							lines[i]->currNote->hittime = HIT_WINDOW*2+1;
							lines[i]->currNote = lines[i]->currNote->tail;
						}
					}
					else
						finished_track++;
				}
				
				if(finished_track != NUM_LINES || elapsed < lastnote)
					finished_track = 0;
				else
					fade_start = elapsed;
			}
			
			glClear(GL_COLOR_BUFFER_BIT);
			
			double total_time = elapsed;
			
			for(int i = 0; i < NUM_LINES; i++)
			{	
				update_line(lines[i],cd->svs,cd->notes[i],total_time);
				if(video_->lastProgram_ != vertexProgram)
				{
					glUseProgram(vertexProgram);
					video_->lastProgram_ = vertexProgram;
				}
				
				lines[i]->point.x = total_time/rate;
				
				glUniform1f(vertexLocation, total_time/rate+1/3.0);
				glUniform1f(vertexTime,total_time);
				glUniform4f(vertexColor, (i == 0 ? 1 : 0), (i == 2 ? 1 : 0), (i == 1 ? 1 : 0), 1.0);
				
				glBindVertexArray(lines[i]->VAO[lines[i]->vindex%BUFFER_BUFFER]);  
				glDrawArrays(GL_TRIANGLE_STRIP, 0, lines[i]->nvertices);
				
				glBindBuffer(GL_ARRAY_BUFFER, lines[i]->VBO[lines[i]->vindex%BUFFER_BUFFER]);
				glBufferData(GL_ARRAY_BUFFER, lines[i]->nvertices*sizeof(struct vertex), NULL, GL_DYNAMIC_DRAW);
				
				lines[i]->vindex++;
				
				switch(i)
				{
					case 0:
						draw_circle(-1/3.f,lines[i]->point.y,RED_,0.0375,0);
					break;
					case 1:
						draw_circle(-1/3.f,lines[i]->point.y,BLUE_,0.0375,0);
					break;
					case 2:
						draw_circle(-1/3.f,lines[i]->point.y,GREEN_,0.0375,0);
					break;
				}
			}
			
			draw_hitanimations(total_time,spd);
			
			if(firstNote >= total_time-spd*8/6 && firstNote < total_time+spd*8/3)
				draw_line((firstNote - total_time)/(spd*8/3)*4/3.f-1/3.f,0.0,WHITE_,0.5,1);
			else if(lastNote >= total_time-spd*8/6 && lastNote < total_time+spd*8/3)
				draw_line((lastNote - total_time)/(spd*8/3)*4/3.f-1/3.f,0.0,WHITE_,0.5,1);
			
			if(early+late+miss >= 10)
			{
				float d = deviation/(early+late+miss);
				sprintf(grade_string, "GRADE\n%s", get_gradetext(update_gradebar(d,elapsed-lastdraw)));
			}
			
			if(!paused)
				sprintf(time_string,"TIME\n%-2.2d:%-2.2d",(int)(elapsed/60000),(int)(fmod(elapsed,60000)/1000));
			
			float gradepos;
			render_simpletext(time_string,0.0,0.99,WHITE_,3.25,TXT_CENTERED|TXT_TOPALIGNED,NULL);
			render_simpletext(grade_string,1.0,0.99,WHITE_,3.25,TXT_RGHTALIGNED|TXT_TOPALIGNED,&gradepos);
			
			if(flag)
				render_simpletext("ENDLESS MODE",-0.99,0.99,WHITE_,2.75,TXT_TOPALIGNED,NULL);
			else
				render_simpletext(grade_out,-0.99,0.99,WHITE_,2.75,TXT_TOPALIGNED,NULL);
			
			//Draw Grade Bar
			draw_gradebar(1.0,gradepos,3.25,3);
			
			//Draw Center Line
			draw_linea(0.0,0.0,WHITE_,0.5,1.0,0);
			
			//Draw Hit Error
			if(video_->settings_[HESTYLE].value_)
				draw_hiterror(0.0,0.0,elapsed);
			
			//Draw Keyboard Graphic
			draw_keygraphic(0.f,-0.965,elapsed);
			
			if(paused)
				render_simpletext("PRESS ANY KEYBIND TO UNPAUSE\nPRESS ESC TO EXIT",0.f,0.75,WHITE_,2,TXT_CENTERED,NULL);
			else
				render_simpletext("PRESS ESC TO PAUSE",-0.99,-1.0,WHITE_,2.5,TXT_BOTALIGNED,NULL);
			//render_simpletext("PRACTICE MODE",-0.99,1.0,WHITE_,2.5,TXT_TOPALIGNED,NULL);
			if(finished_track)
			{
				if(elapsed-fade_start >= 500)
				{
					return_code = 1;
					break;
				}
				else
				{
					float doll = (elapsed-fade_start)/500;
					
					glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
					draw_rectangle(0.f,0.f,1.f,1.f,BLACK_,doll,0);
					glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
				}
				
			}
			else if(elapsed < 250)
			{
				glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
				draw_rectangle(0.f,0.f,1.f,1.f,BLACK_,(250-elapsed)/250,0);
				glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
			}
			
			draw_frametimer(1.0,-1.0,TXT_BOTALIGNED|TXT_RGHTALIGNED);
			if(video_->settings_[SCREEN_BUFFER].value_)
				SwapBuffers(video_->hDC);
			else
				glFlush();
			
			update_frametimer();
			
			lastdraw = elapsed;
		}
		
		return_code = process_practice(round(elapsed));
		
		if(return_code == -1)
		{
			if(!paused && !finished_track)
			{
				play_soundeffect(MENU_CONFIRM);
				paused = 1;
				return_code = 0;
				pause_offset = elapsed;
			}
			else
				play_soundeffect(MENU_BACK);
		}
		else if(return_code == -2)
			return_code = 0;
		
		update_fps();
		cpu_wait();
	}

	clear_keypresses();
	destroy_hitanimation();
	
	destroy_hiterror();
	destroy_lines(lines);
	
	destroy_chartdata(cd);
	
	end_preview();
	
	return return_code;
}

int editor_sinewave(chart_data* cd, int flag)
{
	Beat** lines = init_lines(cd,(int)get_trackpos(NULL)*1000-video_->settings_[OFFSET].value_,1);
	
	float spd, rate, bpm, ot = get_trackpos(NULL)*1000;
	
	bpm = cd->svs->bpm;
	spd = 60000.f / bpm;
	rate = spd * 2;
	
	struct sv_data* currsv = cd->svs->tail;
	
	float s=spd, r=rate;
	int32_t lastsv = 0;
	double lastsvtime = 0.0;
	
	glUseProgram(vertexProgram);
			
	int vertexLocation = glGetUniformLocation(vertexProgram, "trans");
	int vertexTime = glGetUniformLocation(vertexProgram, "time");
	int vertexColor = glGetUniformLocation(vertexProgram, "color");
	
	double elapsed = 0, lastdraw = 0;
	int return_code = 0, finished_track = 0;
	
	reset_keygraphic();
	
	play_track(get_trackpos(NULL)*1000,1.0);
	
	while(!return_code)
	{
		if(get_frameready())
		{
			elapsed = get_trackpos(NULL)*1000.0-video_->settings_[OFFSET].value_;
			
			while(currsv != NULL && currsv->time <= elapsed)
			{
				if(currsv->type == 0)
					s = spd / currsv->bpm;
				else
					s = spd * (cd->svs->bpm/currsv->bpm);
				r = s*2;
				lastsvtime = currsv->svtime;
				lastsv = currsv->time;
				currsv = currsv->tail;
			}
			
			//Check for hits
			while(keypresses != NULL)
			{
				int32_t nextnote = 999999999, nextnote_ind = 0, nextnote_i = -1;
				for(int i = 0; i < NUM_LINES; i++)
				{
					if(lines[i]->currNote != NULL)
					{
						if((int)round(lines[i]->currNote->time/audio_->pitch_) < nextnote)
						{
							nextnote = (int)round(lines[i]->currNote->time/audio_->pitch_);
							nextnote_ind = (1 << i);
							nextnote_i = i;
						}
						else if((int)round(lines[i]->currNote->time/audio_->pitch_) == nextnote)
							nextnote_ind |= (1 << i);
					}
				}
				
				if(video_->settings_[HITSOUNDS].value_)
					play_soundeffect(HITSOUND1+KEYMAP_[keypresses->key]);
				
				if(abs(keypresses->time-nextnote)<HIT_WINDOW)
				{
					int hit_time = (float)(keypresses->time-nextnote);
					
					//See if the correct key was pressed
					if(nextnote_ind & (1<<KEYMAP_[keypresses->key]))
						nextnote_ind = KEYMAP_[keypresses->key];
					else
						nextnote_ind = nextnote_i;
					
					
					if(KEYMAP_[keypresses->key] == nextnote_ind)
					{
						lines[nextnote_ind]->currNote->hittime = hit_time+HIT_WINDOW;
						
						if(abs(hit_time) <= GOOD_HIT)
							add_hitanimation(lines[nextnote_ind]->currNote->svtime,(nextnote_ind==0?LRED_:LBLUE_));
						
						if(video_->settings_[HESTYLE].value_)
						{
							float hity;
							if(video_->settings_[HESTYLE].value_ == 1)
								hity = (float)hit_time/HIT_WINDOW*1/3.f;
							else
								hity = lines[nextnote_ind]->point.y;
							
							if(hit_time < 0)
								add_hiterrorpoint(hity,(float)keypresses->time,(abs(hit_time)<=PASSING_HIT?GREEN_:TEAL_));
							else
								add_hiterrorpoint(hity,(float)keypresses->time,(abs(hit_time)<=PASSING_HIT?GREEN_:YELLOW_));
						}
					}
					else
					{	
						lines[nextnote_ind]->currNote->hittime = hit_time-HIT_WINDOW;
						
						if(video_->settings_[HESTYLE].value_)
						{
							float hity;
							if(video_->settings_[HESTYLE].value_ == 1)
								hity = fabs(lines[nextnote_ind]->point.y)*(hit_time<0?-1:1);
							else
								hity = lines[nextnote_ind]->point.y;
							
							add_hiterrorpoint(hity,(float)keypresses->time,RED_);
						}
					}
					lines[nextnote_ind]->currNote = lines[nextnote_ind]->currNote->tail;
					pop_keypress();
				}
				else
					clear_keypresses();
			}
			

			//Check for misses
			for(int i = 0; i < NUM_LINES; i++)
			{
				if(lines[i]->currNote != NULL)
				{
					if(elapsed/audio_->pitch_ >= lines[i]->currNote->time/audio_->pitch_ + HIT_WINDOW)
					{
						if(video_->settings_[HESTYLE].value_)
							add_hiterrorpoint(0,elapsed/audio_->pitch_,RED_);
						
						lines[i]->currNote->hittime = HIT_WINDOW*2+1;
						lines[i]->currNote = lines[i]->currNote->tail;
					}
				}
				else
					finished_track++;
			}
			
			if(finished_track != NUM_LINES)
				finished_track = 0;
			else
				return_code = 1;
			
			glClear(GL_COLOR_BUFFER_BIT);
			
			double total_time = (elapsed-lastsv)*(rate/r)+lastsvtime;
			
			if(total_time > lastNote)
				break;
			
			for(int i = 0; i < NUM_LINES; i++)
			{	
				update_line(lines[i],cd->svs,cd->notes[i],total_time);
				if(video_->lastProgram_ != vertexProgram)
				{
					glUseProgram(vertexProgram);
					video_->lastProgram_ = vertexProgram;
				}
				
				lines[i]->point.x = total_time/rate;
				
				glUniform1f(vertexLocation, total_time/rate+1/3.0);
				glUniform1f(vertexTime,total_time);
				glUniform4f(vertexColor, (i == 0 ? 1 : 0), (i == 2 ? 1 : 0), (i == 1 ? 1 : 0), 1.0);
				
				glBindVertexArray(lines[i]->VAO[lines[i]->vindex%BUFFER_BUFFER]);  
				glDrawArrays(GL_TRIANGLE_STRIP, 0, lines[i]->nvertices);
				
				glBindBuffer(GL_ARRAY_BUFFER, lines[i]->VBO[lines[i]->vindex%BUFFER_BUFFER]);
				glBufferData(GL_ARRAY_BUFFER, lines[i]->nvertices*sizeof(struct vertex), NULL, GL_DYNAMIC_DRAW);
				
				lines[i]->vindex++;
				
				switch(i)
				{
					case 0:
						draw_circle(-1/3.f,lines[i]->point.y,RED_,0.0375,0);
					break;
					case 1:
						draw_circle(-1/3.f,lines[i]->point.y,BLUE_,0.0375,0);
					break;
					case 2:
						draw_circle(-1/3.f,lines[i]->point.y,GREEN_,0.0375,0);
					break;
				}
			}
			
			draw_hitanimations(total_time,spd);
			
			if(firstNote >= total_time-spd*8/6 && firstNote < total_time+spd*8/3)
				draw_line((firstNote - total_time)/(spd*8/3)*4/3.f-1/3.f,0.0,WHITE_,0.5,1);
			else if(lastNote >= total_time-spd*8/6 && lastNote < total_time+spd*8/3)
				draw_line((lastNote - total_time)/(spd*8/3)*4/3.f-1/3.f,0.0,WHITE_,0.5,1);
			
			//Draw Center Line
			draw_linea(0.0,0.0,WHITE_,0.5,1.0,0);
			
			//Draw Hit Error
			if(video_->settings_[HESTYLE].value_)
				draw_hiterror(0.0,0.0,elapsed/audio_->pitch_);
			
			//Draw Keyboard Graphic
			draw_keygraphic(0.f,-0.965,elapsed/audio_->pitch_);
			
			render_simpletext("TEST PLAY",-1.0,0.99,WHITE_,2.5,TXT_TOPALIGNED,NULL);
			
			draw_frametimer(1.0,-1.0,TXT_BOTALIGNED|TXT_RGHTALIGNED);
			if(video_->settings_[SCREEN_BUFFER].value_)
				SwapBuffers(video_->hDC);
			else
				glFlush();
			
			update_frametimer();
			
			lastdraw = elapsed;
		}
		
		return_code = process_practice(round(elapsed));
		
		if(return_code == -2)
		{
			set_trackpos(NULL,ot);
			
			reset_keygraphic();
			destroy_hitanimation();
			destroy_hiterror();
			
			currsv = cd->svs->tail;
			s=spd, r=rate;
			lastsv = 0;
			lastsvtime = 0.0;
			
			for(int i = 0; i < NUM_LINES; i++)
			{
				lines[i]->currNote = cd->notes[i];

				while(lines[i]->currNote != NULL)
				{
					if(ot < lines[i]->currNote->time)
						break;
					lines[i]->currNote = lines[i]->currNote->tail;	
				}
				
				lines[i]->bufferpos = 999999999;
			}
			
			return_code = 0;
		}
		
		update_fps();
		cpu_wait();
	}

	clear_keypresses();
	destroy_hitanimation();
	
	destroy_hiterror();
	destroy_lines(lines);
	
	return return_code;
}

void end_preview()
{
	glDeleteProgram(vertexProgram);
	vertexProgram = -1;
}

void load_line(Beat *line, struct sv_data* svs, struct note_data* notes)
{
	struct note_data* currnote = notes;
	int32_t nextnote = currnote->svtime, lastnote = 0;
	int noteCount = 0;
	int dir = (line->index % 2) == 0 ? 1 : -1;
	
	float thickness = 0.0275, spd, rate, spdval;
	
	spd = 60000.f / svs->bpm;
	rate = spd*2;
	spdval = spd / (nextnote - lastnote);
	
	int color = (spdval >= 0.45 ? 1 : -1);
	
	for(int i = 0; i < line->nvertices; i++)
	{
		if (i == nextnote)
		{
			noteCount++;
			if(currnote != NULL)
			{
				lastnote = currnote->svtime;
				currnote = currnote->tail;
				
				if(currnote == NULL)
					nextnote = lastnote + 10000;
				else
					nextnote = currnote->svtime;
			}
			
			spdval = spd / (nextnote - lastnote);
			color = (spdval >= 0.45 ? 1 : -1);
		}
		
		double x = cos(M_PI / (nextnote - lastnote) * (i - nextnote));
		double y = sin(M_PI / (nextnote - lastnote) * (i - nextnote));
		
		if(noteCount % 2 == 0)
		{
			x = 90+x*45*-dir;
			y = fabs(y)*dir;
		}
		else
		{
			x = 90-x*45*-dir;
			y = -fabs(y)*dir;
		}
		
		line->vertices[i*2].x = ((float)i/rate) - cos(x*M_PI/180)*thickness/video_->aspectRatio_;
		line->vertices[i*2].y = y/spdval/2.25/1.05 - sin(x*M_PI/180)*thickness;
		line->vertices[i*2].time = i*color;
		
		line->vertices[i*2+1].x = ((float)i/rate) + cos(x*M_PI/180)*thickness/video_->aspectRatio_;
		line->vertices[i*2+1].y = y/spdval/2.25/1.05 + sin(x*M_PI/180)*thickness;
		line->vertices[i*2+1].time = i*color;
	}
	
	glBindBuffer(GL_ARRAY_BUFFER,line->VBO[line->vindex%BUFFER_BUFFER]);
	glBufferSubData(GL_ARRAY_BUFFER,0,line->nvertices*2*sizeof(struct vertex),line->vertices);
}

void update_line(Beat *line, struct sv_data* svs, struct note_data* notes, double time)
{
	if(time+line->nvertices/3+1 < line->bufferpos)
	{
		int32_t t_ = time - line->nvertices*2/3;
		memset(line->vertices,0,line->nvertices*2*sizeof(struct vertex));
		if(t_ > 0)
			line->bufferpos = t_;
		else
			line->bufferpos = 0;
	}
	
	int32_t i = (line->bufferpos%line->stride), t = line->bufferpos;
	struct note_data* currnote = notes;
	int32_t nextnote = currnote->svtime, lastnote = 0;
	int noteCount = 0;
	
	int dir = (line->index % 2) == 0 ? 1 : -1;
	
	while(currnote != NULL && currnote->svtime <= t)
	{
		lastnote = currnote->svtime;
		currnote = currnote->tail;
		noteCount++;
	}
	
	if(currnote == NULL)
		nextnote = songLengthSV + 10000;
	else
		nextnote = currnote->svtime;
	
	float thickness = 0.0275, spd, rate, spdval;
	
	spd = 60000.f / svs->bpm;
	rate = spd*2;
	spdval = spd / (nextnote - lastnote);
	
	int color = (spdval >= 0.45 ? 1 : -1);

	while(t <= time+line->nvertices/3)
	{
		if (t == nextnote)
		{
			noteCount++;
			if(currnote != NULL)
			{
				lastnote = currnote->svtime;
				currnote = currnote->tail;
				
				if(currnote == NULL)
					nextnote = songLengthSV + 10000;
				else
					nextnote = currnote->svtime;
			}
			
			spdval = spd / (nextnote - lastnote);
			color = (spdval >= 0.45 ? 1 : -1);
		}
		
		double x = cos(M_PI / (nextnote - lastnote) * (t - nextnote));
		double y = sin(M_PI / (nextnote - lastnote) * (t - nextnote));
		
		if(noteCount % 2 == 0)
		{
			x = 90+x*45*-dir;
			y = fabs(y)*dir;
		}
		else
		{
			x = 90-x*45*-dir;
			y = -fabs(y)*dir;
		}
		
		line->vertices[i*2].x = ((float)t/rate) - cos(x*M_PI/180)*thickness/video_->aspectRatio_;
		line->vertices[i*2].y = y/spdval/2.25/1.05 - sin(x*M_PI/180)*thickness;
		line->vertices[i*2].time = t*color;
		
		line->vertices[i*2+1].x = ((float)t/rate) + cos(x*M_PI/180)*thickness/video_->aspectRatio_;
		line->vertices[i*2+1].y = y/spdval/2.25/1.05 + sin(x*M_PI/180)*thickness;
		line->vertices[i*2+1].time = t*color;
		
		memcpy(&line->vertices[(i+line->stride)*2],&line->vertices[i*2],sizeof(struct vertex)*2);
		
		t++;
		i = (i+1)%line->stride;
	}
	
	line->bufferpos = t;
	
	glBindBuffer(GL_ARRAY_BUFFER,line->VBO[line->vindex%BUFFER_BUFFER]);
	glBufferSubData(GL_ARRAY_BUFFER,0,line->nvertices*sizeof(struct vertex),line->vertices+i*2);
	
	int ind = (((int)time)%line->stride)*2;
	
	line->point.y = (line->vertices[ind].y+line->vertices[ind+1].y)/2;
	
	/*struct sv_data* currsv = svs->tail;
	double lastsv = svs->bpm;
	while(currsv != NULL)
	{
		if(currsv->svtime >= time-line->nvertices*8/6 && currsv->svtime < t)
		{
			float pos = (currsv->svtime - time)/(line->nvertices/3)*4/3.f-1/3.f;
			char svtext[16];
			sprintf(svtext,"%.2f",currsv->bpm/svs->bpm);
			if(currsv->bpm < lastsv)
			{
				render_simpletext(svtext,pos,0.525,WHITE_,2.25,TXT_CENTERED|TXT_BOTALIGNED,NULL);
				draw_line(pos,0.0,GREEN_,0.5,1);
			}
			else if(currsv->bpm > lastsv)
			{
				render_simpletext(svtext,pos,0.525,WHITE_,2.25,TXT_CENTERED|TXT_BOTALIGNED,NULL);
				draw_line(pos,0.0,YELLOW_,0.5,1);
			}
		}
		lastsv = currsv->bpm;
		currsv = currsv->tail;
	}*/
}

int32_t calculate_svtime(struct sv_data* svs, struct note_data* notes, int flag)
{
	struct note_data* currnote = notes;
	struct sv_data* currsv = svs->tail;
	
	int32_t nextnote = currnote->time;
	double svtime = 0.0;
	float speed = 1.f;
	int type = 0;
	
	for(int i = 0; i < audio_->length_+songOffset; i++)
	{
		if(currsv != NULL)
		{
			if(i == currsv->time)
			{
				type = currsv->type;
				if(type == 0)
					speed = currsv->bpm;
				else
					speed = currsv->bpm/svs->bpm;
				currsv->svtime = svtime;
				currsv = currsv->tail;
			}
		}
		if (i == nextnote)
		{
			currnote->svtime = svtime;
			if(svtime > lastNote)
				lastNote = svtime;
			if(currnote->tail != NULL)
			{
				currnote = currnote->tail;
				nextnote = currnote->time;
			}
		}
		if(flag != 2)
		{
			svtime += speed;
		}
		else
		{
			if(type == 0)
				svtime += 1;
			else
				svtime += speed;
		}
	}
	return (int)svtime;
}

Beat* allocate_line(int index, float spd)
{
	Beat *line = (Beat *)malloc(sizeof(Beat));
	memset(line, 0, sizeof(Beat));
	
	line->currNote = NULL;
	line->nvertices = (int)(spd*8);
	if(line->nvertices % 2 != 0)
		line->nvertices++;
	line->bufferpos = 0;
	line->stride = line->nvertices/2;
	line->index = index;
	line->vindex = 0;
	
	line->vertices = (struct vertex*)malloc(line->nvertices*2*sizeof(struct vertex));
	memset(line->vertices,0,line->nvertices*2*sizeof(struct vertex));
	
	glGenVertexArrays(BUFFER_BUFFER, line->VAO);
	glGenBuffers(BUFFER_BUFFER, line->VBO);
	
	for(int i = 0; i < BUFFER_BUFFER; i++)
	{
		glBindVertexArray(line->VAO[i]);

		glBindBuffer(GL_ARRAY_BUFFER, line->VBO[i]);
		glBufferData(GL_ARRAY_BUFFER, line->nvertices*sizeof(struct vertex), NULL, GL_DYNAMIC_DRAW);
	 
		glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void*)0);
		glEnableVertexAttribArray(0);
		
		glBindVertexArray(0);
	}
	
	return line;
}

Beat* allocate_full(int index, float spd, struct note_data* notes)
{
	Beat *line = (Beat *)malloc(sizeof(Beat));
	memset(line, 0, sizeof(Beat));
	
	struct note_data* currnote = notes;
	while(currnote->tail != NULL)
	{
		currnote = currnote->tail;
	}
	
	line->currNote = NULL;
	line->nvertices = (int)currnote->svtime+10000;
	if(line->nvertices % 2 != 0)
		line->nvertices++;
	line->bufferpos = 0;
	line->stride = line->nvertices/2;
	line->index = index;
	line->vindex = 0;
	
	line->vertices = (struct vertex*)malloc(line->nvertices*2*sizeof(struct vertex));
	memset(line->vertices,0,line->nvertices*2*sizeof(struct vertex));
	
	glGenVertexArrays(BUFFER_BUFFER, line->VAO);
	glGenBuffers(BUFFER_BUFFER, line->VBO);
	
	for(int i = 0; i < BUFFER_BUFFER; i++)
	{
		glBindVertexArray(line->VAO[i]);

		glBindBuffer(GL_ARRAY_BUFFER, line->VBO[i]);
		glBufferData(GL_ARRAY_BUFFER, line->nvertices*2*sizeof(struct vertex), NULL, GL_DYNAMIC_DRAW);
	 
		glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void*)0);
		glEnableVertexAttribArray(0);
		
		glBindVertexArray(0);
	}
	
	return line;
}

void insert_keypress(int key,int32_t time)
{
	if(keypresses == NULL)
	{
		keypresses = (kpqueue*)malloc(sizeof(kpqueue));
		keypresses->key = key;
		keypresses->time = time;
		keypresses->tail = NULL;
		
		return;
	}
	
	kpqueue* kp = keypresses;
	while(kp != NULL)
	{
		if(kp->key == key && kp->time == time)
			return;
		if(kp->tail == NULL)
		{
			kp->tail = (kpqueue*)malloc(sizeof(kpqueue));
			kp->tail->key = key;
			kp->tail->time = time;
			kp->tail->tail = NULL;
			
			return;
		}
		kp = kp->tail;
	}
}

void pop_keypress()
{
	if(keypresses != NULL)
	{
		kpqueue* kp = keypresses;
		keypresses = keypresses->tail;
		free(kp);
	}
}

void clear_keypresses()
{
	while(keypresses != NULL)
		pop_keypress();
}

void destroy_lines(Beat** lines)
{
	for(int i = 0; i < NUM_LINES; i++)
	{
		if(lines[i] != NULL)
		{
			glDeleteVertexArrays(BUFFER_BUFFER, lines[i]->VAO);
			glDeleteBuffers(BUFFER_BUFFER, lines[i]->VBO);
			
			free(lines[i]->vertices);
			free(lines[i]);
		}
	}
	free(lines);
	
	/*if(measure_lines != NULL)
	{
		free(measure_lines);
		measure_lines = NULL;
		nmeasures = 0;
	}*/
}