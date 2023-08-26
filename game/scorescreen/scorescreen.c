#include "scorescreen.h"
#include "../sinewave/sinewave.h"

#include <math.h>

static options_struct* scoreList;
static score_struct* scores;
static int first_note, duration, useoffset, drawgraph;
static char datetext[256];

static void remove_score(score_struct* score)
{
	char textbuf[256];
	memset(textbuf,0,sizeof(textbuf));
	
	sprintf(textbuf,"replays/%d-%d%02d%02d%02d%02d%02d.bin",
					score->id,score->date.tm_year+1900,score->date.tm_mon+1,
						score->date.tm_mday,score->date.tm_hour,score->date.tm_min,
							score->date.tm_sec);
						
	remove(textbuf);
	
	if(score->result == 1)
		database_removescore(score);
}

static void update_scorelist()
{
	int ndisp = 10;
	
	scoreList->total_ = (scoreList->nOptions_ >= ndisp ? ndisp : scoreList->nOptions_);
	
	if(scoreList->total_ < ndisp || scoreList->currOption_ <= ndisp/2)
	{
		scoreList->from_ = -1;
	}
	else
	{
		if(scoreList->nOptions_-scoreList->currOption_ < ndisp/2)
		{
			scoreList->from_ = -scoreList->nOptions_-ndisp/2-(ndisp/2-(scoreList->nOptions_-scoreList->currOption_));
		}
		else
		{
			scoreList->from_ = -ndisp/2;
		}
	}
	
}

static void load_scrolltext(chart_struct* cs)
{
	char filebuffer[256];
	memset(filebuffer,0,sizeof(filebuffer));
	sprintf(filebuffer,"REPLAYING: %s",get_datavalue(cs->headerdata,"TITLE"));
	
	if(strchr(filebuffer,'\n'))
		strchr(filebuffer,'\n')[0] = '\0';
	
	for(int i = 0; i < strlen(filebuffer); i++)
		filebuffer[i] = toupper(filebuffer[i]);
	
	char backtext[64];
	memset(backtext,0,sizeof(backtext));
	
	sprintf(backtext,"LEVEL:<%d>RATE:<%.2f>BPM:<%d>",(int)round(run_diffcalc(cs->charts[0],audio_->pitch_)),audio_->pitch_,(int)(cs->charts[0]->svs->bpm*audio_->pitch_));
	
	char* creator = get_datavalue(cs->charts[0]->infodata,"CREATOR");
	sprintf(filebuffer,"%s<%s>",filebuffer,creator==NULL?"???":creator);
	
	init_scrolltext(filebuffer,backtext,-1.0,-1.0,-1.0,-0.6);
}

static int process_messages(chart_data* cd)
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
					case 'R':
						play_soundeffect(MENU_CONFIRM);
						result_ = 1;
						break;
					case VK_DOWN:
						if(scoreList->currOption_ == scoreList->nOptions_-1)
							break;
						increment_option(scoreList,1);
						update_scorelist();
						play_soundeffect(MENU_BEEP);
						replay_score(cd,&scores[scoreList->currOption_]);
						drawgraph = 2;
						break;
					case VK_UP:
						if(scoreList->currOption_ == 0)
							break;
						increment_option(scoreList,-1);
						update_scorelist();
						play_soundeffect(MENU_BEEP);
						replay_score(cd,&scores[scoreList->currOption_]);
						drawgraph = 2;
						break;
					case VK_LEFT:
						break;
					case VK_RIGHT:
						break;
					case VK_DELETE:
						result_ = 2;
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

int scorescreen(chart_struct* cs, score_struct* score)
{
	int rank = -1, newrank = -1;
	if(score != NULL && score->result == 1)
	{
		rank = db_playerrank();
		database_addscore(score);
	}
		
	chart_data* cd = cs->charts[0];
	
	int id = atoi(get_datavalue(cs->headerdata,"TRACKID"));
	int nLines = atoi(get_datavalue(cd->infodata,"LINES"));
	int nscores = 0, total_notes = 0;
	
	/*if(score != NULL && score->result == -1)
		scores = database_getscores(id,&nscores,score);
	else
		scores = database_getscores(id,&nscores,NULL);*/
	
	scores = database_getscores(id,&nscores,NULL);
	
	if(score != NULL && score->result == -1)
	{
		scores = (score_struct*)realloc(scores,sizeof(score_struct)*(nscores+1));
		memcpy(scores+nscores,score,sizeof(score_struct));
		nscores++;
	}
	
	if(scores == NULL)
	{
		printf("NO SCORES\n");
		return -1;
	}
	
	struct note_data* notes[nLines];
	for(int i = 0; i < nLines; i++)
	{
		notes[i] = cd->notes[i];
		total_notes += cd->nNotes[i];
	}
	
	char textbuf[256];
	memset(textbuf,0,sizeof(textbuf));
	
	int scoreid = 1;
	
	scoreList = create_options("SCORE HISTORY\n",WHITE_,3.5,0,0,(nscores>10?10:nscores),PURPLE_,TXT_NORESET);
	scoreList->highlighted_ = 1;
	for(int i = 0; i < nscores; i++)
	{
		int col = WHITE_;
		if(scores[i].deviation > FAIL_ACC)
			col = RED_;
		
		if(score != NULL && compare_identifier(score,scores+i))
		{
			if(score->result == -1)
			{
				sprintf(textbuf,"--- %s %dRP %d MISS<%.2f>   \n",get_gradetext(scores[i].deviation),scores[i].points,scores[i].miss,scores[i].rate);
				scoreid--;
			}
			else
			{
				if(i == 0)
					newrank = db_playerrank();
				if(scores[i].miss == 0)
					sprintf(textbuf,"%03d %s %dRP FULL COMBO<%.2f>%s   \n",scoreid,get_gradetext(scores[i].deviation),scores[i].points,scores[i].rate,(i==0?"(NEW BEST!)":"(NEW)"));
				else
					sprintf(textbuf,"%03d %s %dRP %d MISS<%.2f>%s   \n",scoreid,get_gradetext(scores[i].deviation),scores[i].points,scores[i].miss,scores[i].rate,(i==0?"(NEW BEST!)":"(NEW)"));
			}
			scoreList->currOption_ = i;
		}
		else
		{
			if(scores[i].miss == 0)
				sprintf(textbuf,"%03d %s %dRP FULL COMBO<%.2f>   \n",scoreid,get_gradetext(scores[i].deviation),scores[i].points,scores[i].rate);
			else
				sprintf(textbuf,"%03d %s %dRP %d MISS<%.2f>   \n",scoreid,get_gradetext(scores[i].deviation),scores[i].points,scores[i].miss,scores[i].rate);
		}
		add_option(scoreList,textbuf,-0.95,0.0,col,0,TXT_TOPALIGNED,NULL);
		scoreid++;
	}
	
	update_scorelist();
	
	set_pitch(1.0);
	set_trackvolume(1/3.f);
	
	if(score != NULL)
	{
		if(score->deviation > FAIL_ACC || score->result == -1)
		{
			set_trackvolume(1/4.f);
			if(scores[scoreList->currOption_].result == 1)
				play_soundeffect(TRACK_FAIL);
		}
		else
			play_soundeffect(TRACK_CLEAR);
	}
	
	if(score == NULL)
		useoffset = 1;
	else
		useoffset = 0;

	int return_code = 0;
	float r = 0.01, w = 0.95, h = 0.375;
	
	char titletext[256];
	memset(titletext,0,sizeof(titletext));
	
	sprintf(titletext,"%s(%s)<LEVEL %d>",get_datavalue(cs->headerdata,"TITLE"),get_datavalue(cd->infodata,"CREATOR"),(int)round(run_diffcalc(cs->charts[0],1.0)));
	
	memset(datetext,0,sizeof(titletext));
	
	for(int i = 0; i < strlen(titletext); i++)
		titletext[i] = toupper(titletext[i]);
	
	replay_score(cd,&scores[scoreList->currOption_]);
	
	drawgraph = 2;
	
	while(!return_code)
	{
		if(get_trackstatus(NULL) != BASS_ACTIVE_PLAYING)
			play_track(0,1/3.f);
		
		if(get_frameready())
		{
			if(drawgraph || video_->shouldReload_)
			{
				glClear(GL_COLOR_BUFFER_BIT);
				
				draw_rectangle(0.0,0.5,w,h,WHITE_,0.25,0);
				
				draw_line(0.0,0.5,WHITE_,w,0);
				
				for(int i = 1; i < 3; i++)
				{
					draw_line(0.0,h*i/2+0.5,WHITE_,w,0);
					draw_line(0.0,-h*i/2+0.5,WHITE_,w,0);
				}
				
				for(int i = 0; i < nLines; i++)
				{
					struct note_data* note = notes[i];
					while(note != NULL)
					{
						if(note->hittime == -999999999)
							break;
						
						if(note->hittime < 0)
						{
							glBlendFunc(GL_ONE,GL_ZERO);
							draw_circle((float)(note->time-first_note)/duration*((w-r/video_->aspectRatio_)*2)-w+r/video_->aspectRatio_,0.5+(note->hittime+HIT_WINDOW)/HIT_WINDOW*(h-r),YELLOW_,r,0);
							glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
						}
						else if(note->hittime >= (HIT_WINDOW*2))
						{
							glBlendFunc(GL_ONE,GL_ZERO);
							draw_circle((float)(note->time-first_note)/duration*((w-r/video_->aspectRatio_)*2)-w+r/video_->aspectRatio_,0.5+(h-r),YELLOW_,r,0);
							glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
						}
						else
							draw_circle((float)(note->time-first_note)/duration*((w-r/video_->aspectRatio_)*2)-w+r/video_->aspectRatio_,0.5+(note->hittime-HIT_WINDOW)/HIT_WINDOW*(h-r),(i==0?RED_:BLUE_),r,0);
						note = note->tail;
					}
				}
				
				render_simpletext("+42MS",-0.95,h/2+0.5,WHITE_,2.25,TXT_TOPALIGNED,NULL);
				render_simpletext("-42MS",-0.95,-h/2+0.5,WHITE_,2.25,TXT_BOTALIGNED,NULL);
				render_simpletext("+83MS",-0.95,h+0.5,WHITE_,2.25,TXT_TOPALIGNED,NULL);
				render_simpletext("-83MS",-0.95,-h+0.5,WHITE_,2.25,TXT_BOTALIGNED,NULL);
				
				if(scores[scoreList->currOption_].result == -1)
					render_simpletext("TRACK ABORTED",0.0,h/4,WHITE_,3.5,TXT_TOPALIGNED|TXT_CENTERED,NULL);
				else if(scores[scoreList->currOption_].deviation > FAIL_ACC)
					render_simpletext("TRACK FAILED",0.0,h/4,WHITE_,3.5,TXT_TOPALIGNED|TXT_CENTERED,NULL);
				else
					render_simpletext("TRACK CLEARED",0.0,h/4,WHITE_,3.5,TXT_TOPALIGNED|TXT_CENTERED,NULL);
				
				render_simpletext(titletext,-0.95,h+0.575,WHITE_,2.5,0,NULL);
				render_simpletext(datetext,0.95,h+0.575,WHITE_,2.5,TXT_RGHTALIGNED,NULL);
				
				if(newrank != -1 && newrank != rank)
				{
					char ranktext[64];
					memset(ranktext,0,sizeof(ranktext));
					
					sprintf(ranktext,"+PLAYER RATING+\n%d<+%d>",newrank,newrank-rank);
					render_simpletext(ranktext,0.95,0.0,WHITE_,3.5,TXT_TOPALIGNED|TXT_RGHTALIGNED,NULL);
				}

				draw_options(scoreList);
				
				render_simpletext("R TO WATCH REPLAY",-1.0,-1.0,WHITE_,3.5,TXT_BOTALIGNED,NULL);
				if(score == NULL)
					render_simpletext("DEL TO REMOVE REPLAY",1.0,-1.0,WHITE_,3.5,TXT_RGHTALIGNED|TXT_BOTALIGNED,NULL);
				
				if(video_->settings_[SCREEN_BUFFER].value_)
					SwapBuffers(video_->hDC);
				else
					glFlush();
				
				if(video_->shouldReload_)
				{
					drawgraph = 2;
					video_->shouldReload_ = 0;
				}
				if(drawgraph > 0)
					drawgraph--;
			}
			update_frametimer();
		}
		
		return_code = process_messages(cd);
		
		if(return_code == 1)
		{
			set_pitch(scores[scoreList->currOption_].rate);
			load_scrolltext(cs);
			queue_track();
			
			sinewave(cd,&scores[scoreList->currOption_]);
			
			set_pitch(1.0);
			set_trackvolume(1/3.f);
			
			destroy_scrolltext();
			
			useoffset = 0;
			replay_score(cd,&scores[scoreList->currOption_]);
			
			drawgraph = 2;
			return_code = 0;
		}
		else if(return_code == 2)
		{
			if(score == NULL)
			{
				char *txt = get_option_text(scoreList);
				char* index = txt+(strlen(txt)-4);
				if(strstr(txt,"[X]"))
				{
					index[0] = ' ';
					index[1] = ' ';
					index[2] = ' ';
				}
				else
				{
					
					index[0] = '[';
					index[1] = 'X';
					index[2] = ']';
				}
				drawgraph = 2;
			}
			return_code = 0;
		}
		update_fps();
		cpu_wait();
	}
	
	if(score != NULL && score->result == -1)
		remove_score(score);
	else if(score == NULL)
	{
		char *txt;
		for(int i = 0; i < nscores; i++)
		{
			scoreList->currOption_ = i;
			txt = get_option_text(scoreList);
			if(strstr(txt,"[X]"))
				remove_score(&scores[i]);
		}
	}
	
	stop_soundeffect(TRACK_CLEAR);
	stop_soundeffect(TRACK_FAIL);
	
	delete_options(scoreList);
	free(scores);
	scores = NULL;
	
	return return_code;
}

int replay_score(chart_data* cd, score_struct* score)
{
	//Get note pointers
	int nLines = atoi(get_datavalue(cd->infodata,"LINES"));
	int finished = 0, ms = 0;
	first_note = 999999999;
	duration = 0;
	
	struct note_data* notes[nLines];
	for(int i = 0; i < nLines; i++)
	{
		notes[i] = cd->notes[i];
		if(notes[i]->time < first_note)
			first_note = notes[i]->time;
		while(notes[i] != NULL)
		{
			notes[i]->hittime = -999999999;
			if(notes[i]->tail == NULL)
				if(notes[i]->time > duration)
					duration = notes[i]->time;
			
			notes[i] = notes[i]->tail;
		}
		notes[i] = cd->notes[i];
	}
	
	duration -= first_note;
	
	sprintf(datetext,"PLAYED ON %d/%02d/%02d %02d:%02d:%02d",score->date.tm_year+1900,score->date.tm_mon+1,
						score->date.tm_mday,score->date.tm_hour,score->date.tm_min,
							score->date.tm_sec);
	
	char replayfile[256];
	memset(replayfile,0,sizeof(replayfile));
	
	sprintf(replayfile,"replays/%d-%d%02d%02d%02d%02d%02d.bin",
				score->id,score->date.tm_year+1900,score->date.tm_mon+1,
					score->date.tm_mday,score->date.tm_hour,score->date.tm_min,
						score->date.tm_sec);
	
	FILE* fp = fopen(replayfile,"rb");
	if(!fp)
		return -1;
	
	//Load header data
	float version;
	int keymap[4], offset;
	
	fread(&version,sizeof(float),1,fp);
	fread(keymap,sizeof(keymap),1,fp);
	fread(&offset,sizeof(int),1,fp);
	fseek(fp,sizeof(float),SEEK_CUR);
	
	if(!useoffset)
	{
		offset = 0;
	}
	else
	{
		//first_note += offset;
		//duration -= offset;
	}
	
	//Load next keypress
	int replay_key = -1, replay_time = -1;
	if(!feof(fp))
	{
		replay_key = fgetc(fp);
		fread(&replay_time,sizeof(int),1,fp);
	}
	
	while(!finished)
	{
		while(ms == replay_time)
		{
			int32_t nextnote = 999999999, nextnote_ind = 0, nextnote_i = -1;
			for(int i = 0; i < nLines; i++)
			{
				if(notes[i] != NULL)
				{
					if((int)round((notes[i]->time+offset)/score->rate) < nextnote)
					{
						nextnote = (int)round((notes[i]->time+offset)/score->rate);
						nextnote_ind = (1 << i);
						nextnote_i = i;
					}
					else if((int)round((notes[i]->time+offset)/score->rate) == nextnote)
						nextnote_ind |= (1 << i);
				}
			}
		
			if(abs(replay_time/score->rate-nextnote)<HIT_WINDOW)
			{
				int hit_time = (float)replay_time/score->rate-nextnote;
				
				//See if the correct key was pressed
				if(nextnote_ind & (1<<keymap[replay_key]))
					nextnote_ind = keymap[replay_key];
				else
					nextnote_ind = nextnote_i;
				
				
				if(keymap[replay_key] == nextnote_ind)
					notes[nextnote_ind]->hittime = hit_time+HIT_WINDOW;
				else	
					notes[nextnote_ind]->hittime = hit_time-HIT_WINDOW;
				
				notes[nextnote_ind] = notes[nextnote_ind]->tail;
				
			}
			
			if(!feof(fp))
			{
				replay_key = fgetc(fp);
				fread(&replay_time,sizeof(int),1,fp);
			}
			else 
			{
				replay_key = -1;
				replay_time = -1;
			}
		}
		
		for(int i = 0; i < nLines; i++)
		{
			if(notes[i] != NULL)
			{
				if((float)ms/score->rate >= (float)(notes[i]->time+offset)/score->rate+HIT_WINDOW)
				{
					notes[i]->hittime = HIT_WINDOW*2+1;
					notes[i] = notes[i]->tail;
				}
			}
			else
				finished++;
		}
		if(finished != nLines)
			finished = 0;
		
		ms++;
	}
	
	fclose(fp);
	
	return 0;
}
