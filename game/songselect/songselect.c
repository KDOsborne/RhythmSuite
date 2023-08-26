#include "songselect.h"
#include "../sinewave/sinewave.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define nSections 5

struct chart
{
	char*			songname_;
	char*			filename_;
	char*			directory_;
	float			difficulty_;
	int				id_;
};

static void init_songselect();
static int load_songs();
static void insert_chart(int,float,char*,char*,char*);
static void sort_charts();
static void load_songinfo(int);
static void update_songtext();
static void update_scoretext();
static void start_game();
static void load_bg();
static void load_scrolltext();
static void start_audio();
static void update_searchlist();
static void update_aftersearch();
static void update_chart();
static void destroy_chartlist();
static void destroy_songselect();

static options_struct 	**songList_,*songSearch_,*songSections_,*songCreator_,*songBPM_,*songLength_,*songNotes_,*songLevel_,*songScores_;
static struct chart*	chartList_;
static chart_struct		*chartStruct_;
static int 				currSong,currSection,nCharts,needData;
static char				filebuffer[256],hintText[24],searchstring[16];
static Beat**			songPreview_;
static int				songDur_,songN_,searching,searchsize,found,validated,request,reload;

static int process_messages()
{
	MSG msg;
	int result_ = 0;
	while(PeekMessage(&msg, video_->hWnd, 0, 0, PM_NOREMOVE))
	{
		if(GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg); 
			
			switch(msg.message) {
				case WM_CHAR:
					if(isalnum(msg.wParam) || (msg.wParam == ' ' && searching))
					{
						searching = 1;
						
						if(searchsize < sizeof(searchstring)-1)
						{
							searchstring[searchsize] = toupper(msg.wParam);
							searchstring[searchsize+1] = '\0';
							searchsize++;
							play_soundeffect(MENU_FASTBEEP);
							update_searchlist();
						}
					}
				break;
				case WM_KEYDOWN:
				switch (msg.wParam) {
					case VK_ESCAPE:
						if(searching)
						{
							memset(searchstring,0,sizeof(searchstring));
							searchsize = 0;
							searching = 0;
							if(songSearch_->highlighted_)
								update_aftersearch();
						}
						else if(currSection != -1)
							currSection = -1;
						else
							result_ = -1;
						play_soundeffect(MENU_BACK);
						break;
					case VK_BACK:
						if(searchsize > 0)
						{
							searchstring[searchsize-1] = '\0';
							searchsize--;
							play_soundeffect(MENU_BEEP);
							
							if(searchsize == 0)
							{
								searching = 0;
								if(songSearch_->highlighted_)
									update_aftersearch();
							}
							else
								update_searchlist();
						}
						
						break;
					case VK_F5:
						if(validated == 2)
							update_chart();
					break;
					case VK_F9:
						if(!needData && songScores_->nOptions_ > 1)
						{
							if(songPreview_ != NULL)
								destroy_lines(songPreview_);
							songPreview_ = NULL;
							
							float rate = audio_->pitch_;
							
							load_scrolltext();
							load_bg();
							
							scorescreen(chartStruct_,NULL);
							update_scoretext();
							
							destroy_scrolltext();
							
							set_pitch(rate);
							
							switch_tracks();
						}
					break;
					case VK_TAB:
						/*if(!needData && songScores_->nOptions_ > 1)
						{
							if(songPreview_ != NULL)
								destroy_lines(songPreview_);
							songPreview_ = NULL;
							
							float rate = audio_->pitch_;
							
							load_scrolltext();
							load_bg();
							
							scorescreen(chartStruct_,NULL);
							update_scoretext();
							
							destroy_scrolltext();
							
							set_pitch(rate);
							
							switch_tracks();
						}*/
						if(!needData)
						{
							if(request)
								request = 0;
							else
								request = 1;
							update_scoretext();
						}
						
						break;
					case VK_RETURN:
						if(searching)
						{
							if(found)
							{
								if(!songSearch_->highlighted_)
								{
									play_soundeffect(MENU_BEEP);
									songSearch_->highlighted_ = 1;

									if(track_fadeout(0.5) == -1.0)
										switch_tracks();
								}
								else
								{
									play_soundeffect(MENU_CONFIRM);
									load_songinfo(2);
									start_game();
									switch_tracks();
								}
							}
							else
								play_soundeffect(MENU_INVALID);
						}
						else if(currSection == -1)
						{
							if(songList_[songSections_->currOption_]->nOptions_ == 0)
							{
								play_soundeffect(MENU_INVALID);
								break;
							}
							
							play_soundeffect(MENU_CONFIRM);
							
							currSection = songSections_->currOption_;
							
							int n = 0;
							for(int i = 0; i < currSection; i++)
								n += songList_[i]->nOptions_;
							
							
							if(currSong-n >= 0 && currSong-n < songList_[currSection]->nOptions_)
							{
								songList_[currSection]->currOption_ = currSong-n;
							}
							else
							{
								if(track_fadeout(0.5) == -1.0)
									switch_tracks();
							}
						}
						else
						{
							play_soundeffect(MENU_CONFIRM);
							load_songinfo(2);
							start_game();
							switch_tracks();
						}
						break;
					case VK_DOWN:
						play_soundeffect(MENU_BEEP);
						
						if(searching)
						{
							if(found)
							{
								if(songSearch_->highlighted_)
								{
									increment_option(songSearch_, 1);
								}
								else
								{
									songSearch_->highlighted_ = 1;
								}
								
								if(track_fadeout(0.5) == -1.0)
									switch_tracks();
							}
						}
						else
						{
							if(currSection == -1)
								increment_option(songSections_, 1);
							else
							{
								increment_option(songList_[currSection], 1);
								if(track_fadeout(0.5) == -1.0)
									switch_tracks();
							}
						}
						break;
					case VK_UP:
						play_soundeffect(MENU_BEEP);
						if(searching)
						{
							if(found)
							{
								if(songSearch_->highlighted_)
								{
									increment_option(songSearch_, -1);
								}
								else
								{
									songSearch_->highlighted_ = 1;
								}
								
								if(track_fadeout(0.5) == -1.0)
									switch_tracks();
							}
						}
						else
						{
							if(currSection == -1)
								increment_option(songSections_, -1);
							else
							{
								increment_option(songList_[currSection], -1);
								if(track_fadeout(0.5) == -1.0)
									switch_tracks();
							}
						}
						
						break;
					case VK_LEFT:
						break;
					case VK_RIGHT:
						break;
					case VK_OEM_PLUS:
						set_pitch(audio_->pitch_ + 0.05);
						sprintf(hintText,"+/- CHANGE RATE<\3%.2f\1>",audio_->pitch_);
						if(!needData)
							update_songtext();
						
						break;
					case VK_OEM_MINUS:
						set_pitch(audio_->pitch_ - 0.05);
						sprintf(hintText,"+/- CHANGE RATE<\3%.2f\1>",audio_->pitch_);;
						if(!needData)
							update_songtext();
						
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

int songselect()
{
	int return_code = 0, fading = 0;
	double songpos = 0.0, fadein = 0.0,fadeout = 0.0;
	float preview_start = 0.0, preview_fade = 1.0;

	init_songselect();
	
	load_songinfo(0);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	
	while (!return_code)
	{	
		if(get_trackstatus(NULL) != BASS_ACTIVE_PLAYING && track_fadeout(0.5) == -1.0)
		{
			load_songinfo(0);
			/*start_audio();
			if(songPreview_ != NULL)
			{
				preview_fade = 1.0;
				preview_start = get_trackpos(NULL)*1000.0;
				songpos = preview_start;
			}*/
		}
		else
		{
			songpos = get_trackpos(NULL)*1000.0;
			fadein = track_fadein(1.0);	
			if(songpos/1000 - audio_->tfadetime_ >= 22.5)
			{
				switch_tracks();
			}
			
			if(fadein >= 0.5)
			{
				if(needData)
				{
					load_songinfo(1);
				}

				if(fadein == 1.0)
				{
					if(songPreview_ == NULL)
					{
						songPreview_ = init_lines(chartStruct_->charts[0],songpos,1);
						preview_fade = 1.0;
						preview_start = songpos;
					}
				}
			}
		}
		
		if(request == 1)
		{
			if(validated == 0)
			{
				if(server_->scount_[MESSAGE_VALIDATE-1] == 0)
				{
					if(server_->sdata_[MESSAGE_VALIDATE-1].status == 1)
					{
						validated = server_->sdata_[MESSAGE_VALIDATE-1].result[0];
						update_scoretext();
					}
					else if(server_->sdata_[MESSAGE_VALIDATE-1].status == -1)
					{
						validated = -2;
						update_scoretext();
					}
				}
			}
			else if(validated == 1)
			{
				if(server_->scount_[MESSAGE_REQUEST-1] == 0)
				{
					if(server_->sdata_[MESSAGE_REQUEST-1].status == 1)
					{
						request = 2;
						update_scoretext();
					}
					else if(server_->sdata_[MESSAGE_REQUEST-1].status == -1)
					{
						request = -1;
						update_scoretext();
					}
				}
			}
		}
			
		if(get_frameready())
		{
			glClear(GL_COLOR_BUFFER_BIT);
			
			if(songPreview_ != NULL)
			{
				fadeout = track_fadeout(0.5);
				if(!fading)
					fading = (fadeout != -1.0);
				if(fading)
				{
					if(fadeout != -1.0)
					{
						preview_sinewave(songPreview_,chartStruct_->charts[0],get_trackpos(&audio_->fadetrack_)*1000.0,0);
						//draw_rectangle(0,0.95+0.01,2/3.f,0.01*2.75/2*9+0.01,WHITE_,0.125,2);
						draw_rectangle(-2/3.f,0.f,1/4.f,0.925,WHITE_,0.125,0);
						
						draw_options(songCreator_);
						draw_options(songBPM_);
						draw_options(songLength_);
						draw_options(songNotes_);
						draw_options(songLevel_);
						draw_options(songScores_);
						if(request == 0)
							render_simpletext("PRESS TAB TO VIEW GLOBAL SCORES",-2/3.f-1/4.f,-0.925,TEAL_,2,TXT_BOTALIGNED,NULL);
						
						glBlendFunc(GL_SRC_ALPHA, GL_SRC_ALPHA);
						draw_rectangle(0.f,0.f,1.f,1.f,BLACK_,fadeout*0.75*((1-preview_fade)/0.75),0);
						glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
					}
					else
					{
						destroy_lines(songPreview_);
						songPreview_ = NULL;
						
						fading = 0;
					}
				}
				else
				{
					preview_sinewave(songPreview_,chartStruct_->charts[0],songpos,0);
					//draw_rectangle(0,0.95+0.01,2/3.f,0.01*2.75/2*9+0.01,WHITE_,0.125,2);
					draw_rectangle(-2/3.f,0.f,1/4.f,0.925,WHITE_,0.125,0);
					
					draw_options(songCreator_);
					draw_options(songBPM_);
					draw_options(songLength_);
					draw_options(songNotes_);
					draw_options(songLevel_);
					draw_options(songScores_);
					if(request == 0)
							render_simpletext("PRESS TAB TO VIEW GLOBAL SCORES",-2/3.f-1/4.f,-0.925,TEAL_,2,TXT_BOTALIGNED,NULL);
					
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					draw_rectangle(0.f,0.f,1.f,1.f,BLACK_,preview_fade,0);
					glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
		
					if(preview_fade > 0.25f)
					{
						preview_fade = 1.0 - (songpos - preview_start) / 500.0;
						
						if(preview_fade < 0.25f)
							preview_fade = 0.25f;
					}
				}
			}
			
			if(searching)
			{
				draw_options(songSearch_);
				render_simpletext(searchstring,0.0,0.5,TEAL_,3.5,TXT_CENTERED,NULL);
			}
			else
			{
				if(currSection == -1)
					draw_options(songSections_);
				else
					draw_options(songList_[currSection]);
				
				render_simpletext("TYPE TO SEARCH TRACKS",-1,0.99,TEAL_,2,TXT_TOPALIGNED,NULL);
			}
			render_simpletext(hintText,-1.0,-1.0,WHITE_,3.5,TXT_BOTALIGNED,NULL);
			draw_frametimer(1.0,-1.0,TXT_BOTALIGNED|TXT_RGHTALIGNED);
			render_serverinfo();
			if(video_->settings_[SCREEN_BUFFER].value_)
				SwapBuffers(video_->hDC);
			else
				glFlush();

			update_frametimer();
		}
		
		return_code = process_messages();
		
		process_server();
		update_fps();
		cpu_wait();
	}
	
	destroy_songselect();
	
	set_pitch(1.0);
	
	if(reload)
		songdb_create();
	
	return return_code;
}

static void init_songselect()
{
	reload = 0;
	request = 0;
	validated = 0;
	searching = 0;
	searchsize = 0;
	found = 0;
	nCharts = 0;
	currSection = -1;
	songPreview_ = NULL;
	chartList_ = NULL;
	chartStruct_ = create_chartstruct();
	
	memset(filebuffer,0,sizeof(filebuffer));
	memset(searchstring,0,sizeof(searchstring));
	memset(hintText,0,sizeof(hintText));
	
	sprintf(hintText,"+/- CHANGE RATE<\3%.2f\1>",audio_->pitch_);
	
	load_songs();
	sort_charts();
	
	songSections_ = create_options(NULL,WHITE_,4,0,-1,-1,PURPLE_,TXT_NORESET);
	
	add_option(songSections_,"LEVEL 1-5\n",0.0,-0.5,WHITE_,0,TXT_CENTERED|TXT_TOPALIGNED,NULL);
	add_option(songSections_,"LEVEL 6-10\n",0.0,-0.5,WHITE_,0,TXT_CENTERED|TXT_TOPALIGNED,NULL);
	add_option(songSections_,"LEVEL 11-15\n",0.0,-0.5,WHITE_,0,TXT_CENTERED|TXT_TOPALIGNED,NULL);
	add_option(songSections_,"LEVEL 16-19\n",0.0,-0.5,WHITE_,0,TXT_CENTERED|TXT_TOPALIGNED,NULL);
	add_option(songSections_,"LEVEL 20+\n",0.0,-0.5,WHITE_,0,TXT_CENTERED|TXT_TOPALIGNED,NULL);
	
	songSections_->highlighted_ = 1;
	
	currSong = rand()%nCharts;
	
	if(db_playerrank() == 0)
		currSong = 0;
	
	songList_ = (options_struct**)malloc(sizeof(options_struct*)*nSections);
	
	for(int i = 0; i < nSections; i++)
	{
		songList_[i] = create_options(NULL,WHITE_,5,0,-4,9,PURPLE_,TXT_NORESET|TXT_MINIMIZE);
		for(int j = 0; j < nCharts; j++)
		{
			int add = 0;
			switch(i)
			{
				case 0:
					if(round(chartList_[j].difficulty_) <= 5)
						add = 1;
					break;
				case 1:
					if(round(chartList_[j].difficulty_) >= 6 && round(chartList_[j].difficulty_) <= 10)
						add = 1;
					break;
				case 2:
					if(round(chartList_[j].difficulty_) >= 11 && round(chartList_[j].difficulty_) <= 15)
						add = 1;
					break;
				case 3:
					if(round(chartList_[j].difficulty_) >= 16 && round(chartList_[j].difficulty_) <= 19)
						add = 1;
					break;
				case 4:
					if(round(chartList_[j].difficulty_) >= 20)
						add = 1;
					break;
			}
			if(add)
			{
				add_option(songList_[i],chartList_[j].songname_,0.0,-0.5,WHITE_,0,TXT_CENTERED|TXT_TOPALIGNED,NULL);
				if(j == currSong)
					songSections_->currOption_ = i;
			}
		}
		songList_[i]->highlighted_ = 1;
	}
	
	songSearch_ = create_options(NULL,WHITE_,5,0,-4,9,PURPLE_,TXT_NORESET|TXT_MINIMIZE);
	songSearch_->highlighted_ = 1;
	
	//songList_ = create_options(NULL,WHITE_,4,rand()%nCharts,-5,11,PURPLE_,TXT_NORESET|TXT_MINIMIZE);
	
	/*for(int i = 0; i < nCharts; i++)
		add_option(songList_,chartList_[i].songname_,0.0,-0.6,WHITE_,0,TXT_CENTERED|TXT_TOPALIGNED,NULL);*/
	
	songCreator_ = create_options("CREATOR:",WHITE_,2.5,-1,-1,1,WHITE_,TXT_TOPALIGNED);
	songBPM_ = create_options("\nBPM:",WHITE_,2.5,-1,-1,1,WHITE_,TXT_TOPALIGNED);
	songLength_ = create_options("\n\nLENGTH:",WHITE_,2.5,-1,-1,1,WHITE_,TXT_TOPALIGNED);
	songNotes_ = create_options("\n\n\nNOTES:",WHITE_,2.5,-1,-1,1,WHITE_,TXT_TOPALIGNED);
	songLevel_ = create_options("\n\n\n\nLEVEL:",WHITE_,2.5,-1,-1,1,WHITE_,TXT_TOPALIGNED);
	
	songScores_ = create_options("LOCAL SCORES\n",LPURPLE_,2.25,-1,-1,-1,WHITE_,TXT_NORESET|TXT_TOPALIGNED);
	
	//songList_->highlighted_ = 1;
}

static int load_songs()
{
	FILE* fp = fopen(SONGDB_FILE,"rb");
	if(!fp)
		return -1;
	
	int id,tsize,fsize,dsize;
	float difficulty;
	
	while(!feof(fp))
	{
		if(fread(&id,sizeof(int),1,fp) != 1)
			break;
		
		if(fread(&difficulty,sizeof(float),1,fp) != 1)
			break;
		
		//SKIP STRING SECTION
		fseek(fp,sizeof(int),SEEK_CUR);
		
		if(fread(&tsize,sizeof(int),1,fp) != 1)
			break;
		
		char title[tsize];
		if(fread(title,tsize,1,fp) != 1)
			break;
		
		if(fread(&fsize,sizeof(int),1,fp) != 1)
			break;
		
		char fname[fsize];
		if(fread(fname,fsize,1,fp) != 1)
			break;
		
		if(fread(&dsize,sizeof(int),1,fp) != 1)
			break;
		
		char dirname[dsize];
		if(fread(dirname,dsize,1,fp) != 1)
			break;
		
		insert_chart(id,difficulty,title,fname,dirname);
	}
	
	fclose(fp);
	
	return 0;
}

static void insert_chart(int id, float difficulty, char* songname, char* filename, char* directory)
{
	char *name,*fname,*dir;
	
	name = (char*)malloc(sizeof(char)*(strlen(songname)+2));
	memcpy(name,songname,strlen(songname)+1);
	strcat(name,"\n");
	
	fname = (char*)malloc(strlen(filename)+1);
	memcpy(fname,filename,strlen(filename)+1); 
	
	dir = (char*)malloc(strlen(directory)+1);
	memcpy(dir,directory,strlen(directory)+1);
	
	chartList_ = (struct chart*)realloc(chartList_,sizeof(struct chart)*(nCharts+1));
	chartList_[nCharts].songname_ = name;
	chartList_[nCharts].filename_ = fname;
	chartList_[nCharts].directory_ = dir;
	chartList_[nCharts].difficulty_ = difficulty;
	chartList_[nCharts].id_ = id;
	
	nCharts++;
}

static void sort_charts()
{
	for(int i = 0; i < nCharts; i++)
	{
		for(int j = i+1; j < nCharts; j++)
		{
			if(chartList_[j].difficulty_ < chartList_[i].difficulty_)
			{
				struct chart temp;
				memcpy(&temp,&chartList_[i],sizeof(struct chart));
				memcpy(&chartList_[i],&chartList_[j],sizeof(struct chart));
				memcpy(&chartList_[j],&temp,sizeof(struct chart));
			}
		}
	}
}

static void load_songinfo(int flag)
{
	if(searching)
	{
		if(found && songSearch_->highlighted_)
		{
			int index = 0;
			for(int i = 0; i < nCharts; i++)
			{
				if(strstr(chartList_[i].songname_,searchstring))
				{
					if(index == songSearch_->currOption_)
					{
						currSong = i;
						if(!songSearch_->highlighted_)
						{
							play_soundeffect(MENU_BEEP);
							songSearch_->highlighted_ = 1;
							currSection = -1;
						}
						break;
					}
					index++;
				}
			}
		}
	}
	else if(currSection != -1 && songList_[currSection]->nOptions_ > 0)
	{
		currSong = 0;
		for(int i = 0; i < currSection; i++)
			currSong += songList_[i]->nOptions_;
		currSong += songList_[currSection]->currOption_;
	}
	
	sprintf(filebuffer,"%s/%s",chartList_[currSong].directory_,chartList_[currSong].filename_);
			
	switch(flag)
	{
		case 0:
			parse_rst(filebuffer,chartStruct_,HEADER_ONLY);
			start_audio();
			needData = 1;
			break;
		case 1:
			parse_rst(filebuffer,chartStruct_,0);
			
			//Get new track duration
			int nLines = atoi(get_datavalue(chartStruct_->charts[0]->infodata,"LINES"));
			int firstnote = 999999999;
			songDur_ = 0;
			songN_ = 0;
			for(int i = 0; i < nLines; i++)
			{
				struct note_data* note = chartStruct_->charts[0]->notes[i];
				songN_ += chartStruct_->charts[0]->nNotes[i];
				if(note->time < firstnote)
					firstnote = note->time;
				
				while(note->tail != NULL)
					note = note->tail;
				
				if(note->time > songDur_)
					songDur_ = note->time;
			}
			songDur_ -= firstnote;
			
			if(chartList_[currSong].id_ != server_->sdata_[MESSAGE_VALIDATE-1].result[3])
			{
				request = 0;
				validated = 0;
			}
			
			update_songtext();
			update_scoretext();
			
			needData = 0;
			break;
		case 2:
			parse_rst(filebuffer,chartStruct_,0);
			needData = 0;
			if(songPreview_ != NULL)
				destroy_lines(songPreview_);
			songPreview_ = NULL;
			
			start_audio();
			stop_track(&audio_->fadetrack_);
			queue_track();
			break;
	}	
}

static void update_songtext()
{
	//Update song creator string
	char* str = get_datavalue(chartStruct_->charts[0]->infodata,"CREATOR");
	for(int i = 0; i < strlen(str); i++)
		str[i] = toupper(str[i]);
	
	float x = -11/12.f, y = 0.925;
	
	clear_options(songCreator_);
	add_option(songCreator_,str,x,y,WHITE_,0,0,NULL);
	
	//Update song bpm string
	sprintf(filebuffer,"%d",(int)round(chartStruct_->charts[0]->svs->bpm*audio_->pitch_));
	
	clear_options(songBPM_);
	add_option(songBPM_,filebuffer,x,y,WHITE_,0,0,NULL);
	
	//Update song length string
	sprintf(filebuffer,"%-2.2d:%-2.2d", (int)(songDur_/audio_->pitch_/60000),(int)(fmod(songDur_/audio_->pitch_,60000)/1000));
			
	clear_options(songLength_);
	add_option(songLength_,filebuffer,x,y,WHITE_,0,0,NULL);
	
	//Update song notecount string
	sprintf(filebuffer,"%d",songN_);
	
	clear_options(songNotes_);
	add_option(songNotes_,filebuffer,x,y,WHITE_,0,0,NULL);
	
	//Update song level string
	sprintf(filebuffer,"%d", (int)round(run_diffcalc(chartStruct_->charts[0],audio_->pitch_)));
	
	clear_options(songLevel_);
	if(audio_->pitch_ < 0.975)
		add_option(songLevel_,filebuffer,x,y,LBLUE_,0,0,NULL);
	else if(audio_->pitch_ > 1.025)
		add_option(songLevel_,filebuffer,x,y,LRED_,0,0,NULL);
	else
		add_option(songLevel_,filebuffer,x,y,WHITE_,0,0,NULL);
						
}

static void update_scoretext()
{
	//Update song score string
	char* str = get_datavalue(chartStruct_->headerdata,"TRACKID");
	float x = -11/12.f, y = 0.645;
	
	delete_options(songScores_);

	if(!str)
	{
		songScores_ = create_options("LOCAL SCORES\n",LPURPLE_,2.25,-1,-1,-1,WHITE_,TXT_NORESET|TXT_TOPALIGNED);
		add_option(songScores_,"MISSING TRACK ID",x,y,WHITE_,0,TXT_TOPALIGNED,NULL);
		return;
	}
	
	if(request == 0)
	{
		int nscores = 0;
		score_struct* scores = database_getscores(atoi(str),&nscores,NULL);
		
		songScores_ = create_options("LOCAL SCORES\n",LPURPLE_,2.25,-1,-1,-1,WHITE_,TXT_NORESET|TXT_TOPALIGNED);
	
		if(scores == NULL)
		{
			add_option(songScores_,"UNPLAYED",x,y,WHITE_,0,TXT_TOPALIGNED,NULL);
		}
		else
		{
			char buffer[256];
			for(int i = 0; i < nscores; i++)
			{
				if(i >= 25)
					break;
				if(scores[i].miss == 0)
					sprintf(buffer,"%02d. %dRP %s FULL COMBO<%.2f>\n",i+1,scores[i].points,get_gradetext(scores[i].deviation),scores[i].rate);
				else
					sprintf(buffer,"%02d. %dRP %s %d MISS<%.2f>\n",i+1,scores[i].points,get_gradetext(scores[i].deviation),scores[i].miss,scores[i].rate);
				add_option(songScores_,buffer,x,y,WHITE_,0,TXT_TOPALIGNED,NULL);
			}
			free(scores);
		}
	}
	else
	{
		songScores_ = create_options("GLOBAL SCORES\n",LPURPLE_,2.25,-1,-1,-1,WHITE_,TXT_NORESET|TXT_TOPALIGNED);
	
		if(!server_->signedin_)
		{
			add_option(songScores_,"SIGN IN TO VIEW LEADERBOARDS",x,y,WHITE_,0,TXT_TOPALIGNED,NULL);
		}
		else if(request == 1)
		{
			switch(validated) {
				case 0:
					add_option(songScores_,"CHECKING TRACK...",x,y,WHITE_,0,TXT_TOPALIGNED,NULL);
					validate_track(atoi(str),chartStruct_->filename);
				break;
				case 1:
					add_option(songScores_,"FETCHING SCORES...",x,y,WHITE_,0,TXT_TOPALIGNED,NULL);
					request_scores(atoi(str));
				break;
				case 2:
					add_option(songScores_,"PRESS F5 TO UPDATE CHART...",x,y,WHITE_,0,TXT_TOPALIGNED,NULL);
				break;
				case -1:
					add_option(songScores_,"NO LEADERBOARD",x,y,WHITE_,0,TXT_TOPALIGNED,NULL);
				break;
				case -2:
					add_option(songScores_,"ERROR CONNECTING TO SERVER",x,y,WHITE_,0,TXT_TOPALIGNED,NULL);
				break;
			};
		}
		else if(request == 2)
		{
			if(server_->sdata_[MESSAGE_REQUEST-1].result[0] == 1)
			{
				char score[server_->sdata_[MESSAGE_REQUEST-1].bytes],buffer[256];
				int count = 1;
				
				memcpy(score,server_->sdata_[MESSAGE_REQUEST-1].data,server_->sdata_[MESSAGE_REQUEST-1].bytes);
				
				char* tok = strtok(score,"\n");
				while(tok != NULL)
				{
					int points,miss;
					float deviation,rate;
					char name[16],date[32],buf[48];
					
					memcpy(buf,tok,strlen(tok)+1);
					
					for(int i = 0; i < strlen(tok); i++)
					{
						if(tok[i] == ',')
							tok[i] = ' ';
					}
					
					sscanf(tok,"%s %d %f %d %f %s",name,&points,&deviation,&miss,&rate,date);
					
					deviation /= 1000;
					rate = 1.f+rate*0.025;
					
					if(miss == 0)
						sprintf(buffer,"%d. %s %s\n%dRP %s FULL COMBO<%.2f>\n",count,name,date,points,get_gradetext(deviation),rate);
					else
						sprintf(buffer,"%d. %s %s\n%dRP %s %d MISS<%.2f>\n",count,name,date,points,get_gradetext(deviation),miss,rate);
					
					add_option(songScores_,buffer,x,y,WHITE_,0,TXT_TOPALIGNED,NULL);
					tok = strtok(NULL,"\n");
					count++;
				}
			}
			else if(server_->sdata_[MESSAGE_REQUEST-1].result[0] == -1)
			{
				add_option(songScores_,"NO SCORES",x,y,WHITE_,0,TXT_TOPALIGNED,NULL);
			}
		}
		else if(request == -1)
		{
			add_option(songScores_,"FAILED TO CONNECT TO SERVER...",x,y,WHITE_,0,TXT_TOPALIGNED,NULL);
		}
	}
}

static void start_game()
{
	//Load scrolltext
	load_scrolltext(0);
	
	//Load background image
	load_bg();
	
	reset_keygraphic();
	
	score_struct score;
	score.id = atoi(get_datavalue(chartStruct_->headerdata,"TRACKID"));
	score.rate = audio_->pitch_;
	score.result = 0;
	
	int result_ = sinewave(chartStruct_->charts[0],&score);
	
	while(result_ == 2)
	{
		load_songinfo(2);
		result_ = sinewave(chartStruct_->charts[0],&score);
	}

	destroy_scrolltext();
	
	if(result_ == 1 || result_ == -1)
	{
		float difficulty = run_diffcalc(chartStruct_->charts[0],audio_->pitch_);
		float rate = audio_->pitch_;
		if(score.deviation > FAIL_ACC)
			score.points = 0;
		else
		{
			char buffer[48];
			score.points = (1/pow(score.deviation/10,2)*pow(difficulty,2));
			
			sprintf(buffer,"%d,%d,%d,%d,%d",(int)score.id,(int)score.points,
				(int)(score.deviation*1000),score.miss,(int)round(((score.rate-1.f)/.025)));
			submit_score(buffer,chartStruct_->filename);
			process_server();
		}
		
		score.result = result_;
		
		scorescreen(chartStruct_,&score);
		
		set_pitch(rate);
	}
}

static void load_bg()
{
	char* imgfile = get_datavalue(chartStruct_->headerdata,"IMAGE");
	
	if(imgfile == NULL)
		return;
	
	sprintf(filebuffer,"%s/%s",chartList_[currSong].directory_,imgfile);
	
	load_image(filebuffer);
}

static void load_scrolltext()
{
	char* str = get_datavalue(chartStruct_->charts[0]->infodata,"CREATOR");
	for(int i = 0; i < strlen(str); i++)
		str[i] = toupper(str[i]);
	
	sprintf(filebuffer,"PLAYING: %s<%s>",chartList_[currSong].songname_,str==NULL?"???":str);
	
	if(strchr(filebuffer,'\n'))
		strchr(filebuffer,'\n')[0] = ' ';
	
	char backtext[64];
	memset(backtext,0,sizeof(backtext));
	
	sprintf(backtext,"LEVEL:<%d>RATE:<%.2f>BPM:<%d>",(int)round(run_diffcalc(chartStruct_->charts[0],audio_->pitch_)),audio_->pitch_,(int)(chartStruct_->charts[0]->svs->bpm*audio_->pitch_));
	
	//sprintf(filebuffer,"%s<%s>",filebuffer,chartStruct_->charts[0]->name);
	init_scrolltext(filebuffer,backtext,-1.0,-1.0,-1.0,-0.6);
}

static void start_audio()
{
	char* preview = get_datavalue(chartStruct_->headerdata,"PREVIEW");
	char* audiofile = get_datavalue(chartStruct_->headerdata,"AUDIO");
	
	if(preview == NULL || audiofile == NULL)
		return;
	
	sprintf(filebuffer,"%s/%s",chartList_[currSong].directory_,audiofile);
	
	/*if(track_fadeout(0.5) == -1.0 && track_fadein(1.0) == 1.0)
		switch_tracks();*/
	
	if(load_track(filebuffer) != -1)
		play_track(atoi(preview),0.0);
}

static void update_searchlist()
{
	clear_options(songSearch_);
	
	songSearch_->currOption_ = 0;
	songSearch_->highlighted_ = 0;
	
	for(int i = 0; i < nCharts; i++)
	{
		if(strstr(chartList_[i].songname_,searchstring))
		{
			add_option(songSearch_,chartList_[i].songname_,0.0,-0.5,WHITE_,0,TXT_CENTERED|TXT_TOPALIGNED,NULL);
			if(currSong == i)
			{
				songSearch_->currOption_ = songSearch_->nOptions_-1;
				songSearch_->highlighted_ = 1;
			}
		}
	}
	
	if(songSearch_->nOptions_ == 0)
	{
		add_option(songSearch_,"NO SEARCH RESULTS\n",0.0,-0.5,WHITE_,0,TXT_CENTERED|TXT_TOPALIGNED,NULL);
		found = 0;
	}
	else
		found = 1;
}

static void update_aftersearch()
{
	int n = 0;
	for(int i = 0; i < nSections; i++)
	{
		n += songList_[i]->nOptions_;
		if(currSong < n)
		{
			songSections_->currOption_ = i;

			if(currSong == songList_[i]->currOption_+(n-songList_[i]->nOptions_) && currSection == i)
				songList_[i]->currOption_ = (currSong-(n-songList_[i]->nOptions_));
			else
				currSection = -1;
			
			break;
		}
	}
}

static void update_chart()
{
	if(server_->sdata_[MESSAGE_VALIDATE-1].bytes <= 0)
		return;
	
	FILE* fp = fopen(chartStruct_->filename,"wb");
	if(!fp)
		return;
	
	fwrite(server_->sdata_[MESSAGE_VALIDATE-1].data,1,server_->sdata_[MESSAGE_VALIDATE-1].bytes,fp);
	fclose(fp);
	
	validated = 0;
	request = 1;
	reload = 1;
	
	update_scoretext();
	switch_tracks();
}

static void destroy_chartlist()
{
	for(int i = 0; i < nCharts; i++)
	{
		free(chartList_[i].songname_);
		free(chartList_[i].filename_);
		free(chartList_[i].directory_);
	}
	free(chartList_);
	
	chartList_ = NULL;
	nCharts = 0;
}

void destroy_songselect()
{
	stop_tracks();
	destroy_chartlist();
	
	for(int i = 0; i < nSections; i++)
		delete_options(songList_[i]);
	free(songList_);
	delete_options(songSearch_);
	delete_options(songCreator_);
	delete_options(songBPM_);
	delete_options(songLength_);
	delete_options(songNotes_);
	delete_options(songLevel_);
	delete_options(songScores_);
	
	if(chartStruct_ != NULL)
		destroy_chartstruct(chartStruct_);
	if(songPreview_ != NULL)
		destroy_lines(songPreview_);
	
	end_preview();
}