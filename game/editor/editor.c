#include "editor.h"
#include "../sinewave/sinewave.h"

#include <math.h>
#include <string.h>

static void init_editor();
static void destroy_editor();
static void destroy_chartlist();
static int 	load_songs();
static void insert_chart(int,float,char*,char*,char*);
static void sort_charts();
static void update_songtext();
static void run_editor();

struct chart
{
	char*			songname_;
	char*			filename_;
	char*			directory_;
	float			difficulty_;
	int				id_;
};

static char* helptext = "\3EDITOR HOTKEYS:\1\n"
	"F1 - SET OFFSET TO CURRENT POSITION\n"
	"F2 - SET PREVIEW TO CURRENT POSITION\n"
	"F5 - APPLY CHANGES\n"
	"F6 - TEST PLAY\n"
	"F8 - SAVE CHANGES\n"
	"F9 - MUTE HITSOUNDS\n"
	"TAB - TOGGLE NOTE/SV PLACEMENT\n"
	"KEYBINDS - PLACE NOTE\n"
	"SHIFT+KEYBINDS(ABOVE CENTER) - PLACE NOTES CONTINUOUSLY\n"
	"LEFT CLICK(BELOW CENTER) - PLACE SV\n"
	"RIGHT CLICK(BELOW CENTER) - REMOVE NOTE/SV\n"
	"SCROLL WHEEL - CHANGE TIME\n"
	"SHIFT+SCROLL WHEEL - CHANGE BEAT DIVISOR\n"
	"CTRL+SCROLL WHEEL - CHANGE SV RATE\n"
	"UP/DOWN ARROWS - INCREASE/DECREASE OFFSET\n"
	"0-9 - JUMP TO SONG POSITION\n"
	"SPACEBAR - PLAY/PAUSE\n\n"
	"\3TEST PLAY HOTKEYS:\1\n"
	"TILDE - RESTART TEST PLAY\n"
	"\0";

static options_struct* songList_,*songDetails_,*songCreate_,*songEdit_;
static chart_struct* chartStruct_;
static chart_struct* backChart_;
static struct chart* chartList_;
static int newtrack = 0, edittrack = 0, redraw  = 0, nCharts = 0, denom = 5, offset, firstnote, resound = 0, placesv = -1, removenotes = -1, removesv = -1, changesv = -1, placemode = 0, setpreview = 0, changed = 0, saved = 0, ctrl = 0, shift = 0, mute = 0;
static char dtext[16], offtext[64], savetext[32], previewtext[64], timetext[64], difftext[32], ratetext[16];
static char ntrackname[65], ntrackbpm[9], ntrackmp3[MAX_PATH], ntrackbg[MAX_PATH];
static int denominations[] = { 3, 4, 6, 8, 12, 16, 24, 32 };
static int sizeorder[] = { 3,4,3,4,3,4,3,4 };
static int key_states[] = {0,0,0,0};
static int click_states[] = {0,0};

static void start_audio()
{
	char* audiofile = get_datavalue(chartStruct_->headerdata,"AUDIO");
	
	if(audiofile == NULL)
		return;
	
	char filebuffer[256];
	memset(filebuffer,0,sizeof(filebuffer));
	sprintf(filebuffer,"%s/%s",chartList_[songList_->currOption_-1].directory_,audiofile);
	
	if(load_track(filebuffer) != -1)
		play_track(0,0.0);
}

static void update_songlist()
{
	int ndisp = 27, halfdisp = (int)(ndisp/2)+1;
	
	songList_->total_ = (songList_->nOptions_ >= ndisp ? ndisp : songList_->nOptions_);
	
	if(songList_->total_ < ndisp || songList_->currOption_ <= ndisp/2)
	{
		songList_->from_ = -1;
	}
	else
	{
		if(songList_->nOptions_-songList_->currOption_ < halfdisp)
		{
			songList_->from_ = -songList_->nOptions_-halfdisp-(halfdisp-1-(songList_->nOptions_-songList_->currOption_));
		}
		else
		{
			songList_->from_ = -ndisp/2;
		}
	}
	
	int currSong = songList_->currOption_-1;
	if(currSong >= 0)
	{
		char filebuffer[256];
		memset(filebuffer,0,sizeof(filebuffer));
		sprintf(filebuffer,"%s/%s",chartList_[currSong].directory_,chartList_[currSong].filename_);

		parse_rst(filebuffer,chartStruct_,0);
		parse_rst(filebuffer,backChart_,0);
		
		start_audio();
	}
	else
		stop_tracks();
	update_songtext();
}

static void update_trackcreate()
{
	char buffer[512];
	float y = 0.925;
	
	clear_options(songCreate_);
	
	sprintf(buffer,"TRACK NAME: %s\n",ntrackname);
	add_option(songCreate_,buffer,-1.0,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	
	sprintf(buffer,"TRACK BPM: %s\n",ntrackbpm);
	add_option(songCreate_,buffer,-1.0,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	
	if(ntrackmp3[0] == '\0')
		add_option(songCreate_,"SELECT MP3 FILE[X]\n",-1.0,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	else
		add_option(songCreate_,"SELECT MP3 FILE[O]\n",-1.0,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	
	if(ntrackbg[0] == '\0')
		add_option(songCreate_,"SELECT BACKGROUND IMAGE[X]\n",-1.0,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	else
		add_option(songCreate_,"SELECT BACKGROUND IMAGE[O]\n",-1.0,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	add_option(songCreate_,"CREATE",-1.0,y,WHITE_,0,TXT_TOPALIGNED,NULL);
}

static void update_trackedit()
{
	char buffer[512];
	float y = 0.925;
	clear_options(songEdit_);

	sprintf(buffer,"TRACK BPM: %s\n",ntrackbpm);
	add_option(songEdit_,buffer,-1.0,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	add_option(songEdit_,"UPDATE",-1.0,y,WHITE_,0,TXT_TOPALIGNED,NULL);
}

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
					if(newtrack)
					{
						if(songCreate_->currOption_ == 0)
						{
							if((isalnum(msg.wParam) || msg.wParam == ' ' || msg.wParam == '(' || msg.wParam == ')') && strlen(ntrackname)+1 < sizeof(ntrackname))
							{
								sprintf(ntrackname,"%s%c",ntrackname,toupper(msg.wParam));
								update_trackcreate();
								play_soundeffect(MENU_FASTBEEP);
							}
						}
						else if(songCreate_->currOption_ == 1)
						{
							if((isdigit(msg.wParam) || (msg.wParam == '.' && !strstr(ntrackbpm,"."))) && strlen(ntrackbpm)+1 < sizeof(ntrackbpm))
							{
								sprintf(ntrackbpm,"%s%c",ntrackbpm,msg.wParam);
								update_trackcreate();
								play_soundeffect(MENU_FASTBEEP);
							}
						}
					}
					else if(edittrack)
					{
						if(songEdit_->currOption_ == 0)
						{
							if((isdigit(msg.wParam) || (msg.wParam == '.' && !strstr(ntrackbpm,"."))) && strlen(ntrackbpm)+1 < sizeof(ntrackbpm))
							{
								sprintf(ntrackbpm,"%s%c",ntrackbpm,msg.wParam);
								update_trackedit();
								play_soundeffect(MENU_FASTBEEP);
							}
						}
					}
				break;
				case WM_KEYDOWN:
				redraw = 1;
				switch (msg.wParam) {
					case VK_F1:
						if(songList_->currOption_ > 0)
						{
							edittrack = 1;
							sprintf(ntrackbpm,"%4.4f",chartStruct_->charts[0]->svs->bpm);
							update_trackedit();
						}
					break;
					case VK_BACK:
						if(newtrack)
						{
							play_soundeffect(MENU_BEEP);
							if(songCreate_->currOption_ == 0 && strlen(ntrackname) > 0)
							{
								ntrackname[strlen(ntrackname)-1] = '\0';
								update_trackcreate();
							}
							else if(songCreate_->currOption_ == 1 && strlen(ntrackbpm) > 0)
							{
								ntrackbpm[strlen(ntrackbpm)-1] = '\0';
								update_trackcreate();
							}
						}
						else if(edittrack)
						{
							play_soundeffect(MENU_BEEP);
							if(songEdit_->currOption_ == 0 && strlen(ntrackbpm) > 0)
							{
								ntrackbpm[strlen(ntrackbpm)-1] = '\0';
								update_trackedit();
							}
						}
					break;
					case VK_ESCAPE:		
						play_soundeffect(MENU_BACK);
						if(newtrack)
						{
							newtrack = 0;
						}
						else if(edittrack)
						{
							edittrack = 0;
							memset(ntrackbpm,0,sizeof(ntrackbpm));
						}
						else
							result_ = -1;
						break;
					case VK_RETURN:
						if(songList_->currOption_ > 0)
						{
							if(edittrack)
							{
								if(songEdit_->currOption_ == 1)
								{
									struct sv_data* svs = chartStruct_->charts[0]->svs;
									double ratio = svs->bpm/atof(ntrackbpm);
									
									svs->bpm = atof(ntrackbpm);
									
									int nLines = atoi(get_datavalue(chartStruct_->charts[0]->infodata,"LINES"));
									
									for(int i = 0; i < nLines; i++)
									{
										struct note_data* note = chartStruct_->charts[0]->notes[i]->tail;
										int firstnote = chartStruct_->charts[0]->notes[i]->time;
										
										while(note != NULL)
										{
											note->time = firstnote + (int)round(((double)note->time-firstnote)*ratio);
											note = note->tail;
										}
									}
									update_rstfile(chartStruct_,NULL);
									update_songlist();
									edittrack = 0;
								}
							}
							else
							{
								run_editor();
								set_pitch(1.f);
							}
						}
						else
						{
							if(!newtrack)
							{
								play_soundeffect(MENU_CONFIRM);
								newtrack = 1;
								
							}
							else
							{
								if(songCreate_->currOption_ == 2)
								{
									play_soundeffect(MENU_CONFIRM);
									
									OPENFILENAME ofn;       // common dialog box structure
 
									// Initialize OPENFILENAME
									memset(&ofn,0,sizeof(ofn));
									ofn.lStructSize = sizeof(ofn);
									ofn.hwndOwner = 0;
									ofn.lpstrFile = ntrackmp3;
									ofn.lpstrFile[0] = '\0';
									ofn.nMaxFile = sizeof(ntrackmp3);
									ofn.lpstrFilter = "MP3\0*.mp3\0";
									ofn.nFilterIndex = 0;
									ofn.lpstrTitle = "Select an mp3 file";
									ofn.lpstrFileTitle = NULL;
									ofn.nMaxFileTitle = 0;
									ofn.lpstrInitialDir = NULL;
									ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
								 
									SendMessage(video_->hWnd,WM_KILLFOCUS,0,0);
									// Display the Open dialog box. 
									GetOpenFileName(&ofn);

									SendMessage(video_->hWnd,WM_SETFOCUS,0,0);
									
									update_trackcreate();
								}
								else if(songCreate_->currOption_ == 3)
								{
									play_soundeffect(MENU_CONFIRM);
									
									OPENFILENAME ofn;       // common dialog box structure
 
									// Initialize OPENFILENAME
									memset(&ofn,0,sizeof(ofn));
									ofn.lStructSize = sizeof(ofn);
									ofn.hwndOwner = 0;
									ofn.lpstrFile = ntrackbg;
									ofn.lpstrFile[0] = '\0';
									ofn.nMaxFile = sizeof(ntrackbg);
									ofn.lpstrFilter = "JPG\0*.jpg\0";
									ofn.nFilterIndex = 0;
									ofn.lpstrTitle = "Select a JPG file";
									ofn.lpstrFileTitle = NULL;
									ofn.nMaxFileTitle = 0;
									ofn.lpstrInitialDir = NULL;
									ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
								 
									SendMessage(video_->hWnd,WM_KILLFOCUS,0,0);
									// Display the Open dialog box. 
									GetOpenFileName(&ofn);
									
									SendMessage(video_->hWnd,WM_SETFOCUS,0,0);
									
									update_trackcreate();
								}
								else if(songCreate_->currOption_ == songCreate_->nOptions_-1)
								{
									if(ntrackmp3[0] == '\0' || ntrackbg[0] == '\0')
									{
										play_soundeffect(MENU_INVALID);
									}
									else
									{
										char filename[MAX_PATH];
										int trackid = songdb_getcount()+1;
										float bpm = atof(ntrackbpm);
	
										sprintf(filename,"./Songs/%d %s/",trackid,ntrackname);
										if(!CreateDirectory(filename,NULL))
										{
											write_error(strerror(errno),NULL,0);
											break;
										}
										
										sprintf(filename,"./Songs/%d %s/audio.mp3",trackid,ntrackname);
										CopyFile(ntrackmp3,filename,FALSE);
										sprintf(filename,"./Songs/%d %s/bg.jpg",trackid,ntrackname);
										CopyFile(ntrackbg,filename,FALSE);
										
										if(create_rstfile(ntrackname, trackid, bpm) == 0)
										{
											songdb_create();
											destroy_editor();
											init_editor();
											
											for(int i = 0; i < nCharts; i++)
											{
												if(trackid == chartList_[i].id_)
												{
													songList_->currOption_ = i+1;
													update_songlist();
													break;
												}
											}
										}
									}
								}
							}
						}
						break;
					case VK_DOWN:
						if(newtrack)
						{
							increment_option(songCreate_,1);
						}
						else if(edittrack)
						{
							increment_option(songEdit_,1);
						}
						else
						{
							if(songList_->currOption_ == songList_->nOptions_-1)
								break;
							increment_option(songList_,1);
							update_songlist();
						}
						play_soundeffect(MENU_BEEP);
						break;
					case VK_UP:
						if(newtrack)
						{
							increment_option(songCreate_,-1);
						}
						else if(edittrack)
						{
							increment_option(songEdit_,-1);
						}
						else
						{
							if(songList_->currOption_ == 0)
								break;
							increment_option(songList_,-1);
							update_songlist();
						}
						play_soundeffect(MENU_BEEP);
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

int editor()
{
	int return_code = 0;
	
	init_editor();
	
	redraw = 1;
	
	while (!return_code)
	{	
		if(get_trackstatus(NULL) != BASS_ACTIVE_PLAYING && songList_->currOption_ != 0)
		{
			if(get_trackstatus(NULL) == BASS_ACTIVE_PAUSED)
				play_track(get_trackpos(NULL)*1000,0.0);
			else
				start_audio();
		}
		else
		{
			track_fadein(2.0);
		}
		
		if(get_frameready())
		{
			if(redraw || video_->shouldReload_)
			{
				glClear(GL_COLOR_BUFFER_BIT);
				
				if(newtrack)
				{
					draw_options(songCreate_);
				}
				else if(edittrack)
				{
					draw_options(songEdit_);
				}
				else
				{
					draw_options(songList_);
					draw_options(songDetails_);
				}
				if(video_->settings_[SCREEN_BUFFER].value_)
					SwapBuffers(video_->hDC);
				else
					glFlush();
				
				if(video_->shouldReload_)
				{
					redraw = 1;
					video_->shouldReload_--;
				}
				else
					redraw = 0;
				
			}
			update_frametimer();
		}
		
		return_code = process_messages();
		
		update_fps();
		cpu_wait();
	}
	
	destroy_editor();
	
	return return_code;
}

static void init_editor()
{
	float y = 0.925;
	
	chartList_ = NULL;
	backChart_ = NULL;
	nCharts = 0;
	newtrack = 0;
	chartStruct_ = create_chartstruct();
	backChart_ = create_chartstruct();
	
	set_pitch(1.0);
	
	load_songs();
	sort_charts();
	
	songList_ = create_options("CHOOSE A TRACK TO EDIT\n",LPURPLE_,2.75,0,0,27,PURPLE_,TXT_NORESET);
	songList_->highlighted_ = 1;
	
	add_option(songList_,"+NEW TRACK+\n",-1.0,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	for(int i = 0; i < nCharts; i++)
		add_option(songList_,chartList_[i].songname_,-1.0,y,WHITE_,0,TXT_TOPALIGNED,NULL);

	songDetails_ = create_options("TRACK DETAILS\n",LPURPLE_,2.75,-1,-1,-1,WHITE_,TXT_NORESET|TXT_TOPALIGNED|TXT_CENTERED);

	songCreate_ = create_options("CREATE NEW TRACK\n",LPURPLE_,2.75,0,-1,-1,PURPLE_,TXT_NORESET);
	add_option(songCreate_,"TRACK NAME:\n",-1.0,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	add_option(songCreate_,"TRACK BPM:\n",-1.0,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	add_option(songCreate_,"SELECT MP3 FILE[X]\n",-1.0,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	add_option(songCreate_,"SELECT BACKGROUND IMAGE[X]\n",-1.0,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	add_option(songCreate_,"CREATE",-1.0,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	songCreate_->highlighted_ = 1;
	
	songEdit_ = create_options("EDIT TRACK\n",LPURPLE_,2.75,0,-1,-1,PURPLE_,TXT_NORESET);
	add_option(songEdit_,"TRACK BPM:\n",-1.0,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	add_option(songEdit_,"UPDATE",-1.0,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	songEdit_->highlighted_ = 1;
	
	memset(ntrackname,0,sizeof(ntrackname));
	memset(ntrackbpm,0,sizeof(ntrackbpm));
	memset(ntrackmp3,0,sizeof(ntrackmp3));
	memset(ntrackbg,0,sizeof(ntrackbg));
}

static void destroy_editor()
{
	delete_options(songList_);
	delete_options(songDetails_);
	delete_options(songCreate_);
	destroy_chartlist();

	destroy_chartstruct(chartStruct_);
	destroy_chartstruct(backChart_);
	
	stop_tracks();
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
			if(strcmp(chartList_[j].songname_,chartList_[i].songname_) < 0)
			{
				struct chart temp;
				memcpy(&temp,&chartList_[i],sizeof(struct chart));
				memcpy(&chartList_[i],&chartList_[j],sizeof(struct chart));
				memcpy(&chartList_[j],&temp,sizeof(struct chart));
			}
		}
	}
}

static void update_songtext()
{
	char buffer[256];
	float y = 0.925;
	
	memset(buffer,0,sizeof(buffer));
	
	clear_options(songDetails_);
	
	if(songList_->currOption_ == 0)
		return;
	
	//Update track creator string
	char* str = get_datavalue(chartStruct_->charts[0]->infodata,"CREATOR");
	for(int i = 0; i < strlen(str); i++)
		str[i] = toupper(str[i]);
	
	sprintf(buffer,"CREATOR: %s\n",str);
	add_option(songDetails_,buffer,0.75,y,WHITE_,0,TXT_TOPALIGNED|TXT_CENTERED,NULL);
	
	//Update track bpm string
	sprintf(buffer,"BPM: %d\n",(int)round(chartStruct_->charts[0]->svs->bpm));
	add_option(songDetails_,buffer,0.75,y,WHITE_,0,TXT_TOPALIGNED|TXT_CENTERED,NULL);
	
	int nLines = atoi(get_datavalue(chartStruct_->charts[0]->infodata,"LINES"));
	int firstnote = 999999999, songDur_ = 0, songN_ = 0;
	for(int i = 0; i < nLines; i++)
	{
		struct note_data* note = chartStruct_->charts[0]->notes[i];
		songN_ += chartStruct_->charts[0]->nNotes[i];
		if(chartStruct_->charts[0]->nNotes[i] == 0)
			continue;
		
		if(note != NULL && note->time < firstnote)
			firstnote = note->time;
		
		while(note->tail != NULL)
			note = note->tail;
		
		if(note->time > songDur_)
			songDur_ = note->time;
	}
	if(firstnote == 999999999)
		firstnote = 0;
	
	songDur_ -= firstnote;
	
	//Update track length string
	sprintf(buffer,"LENGTH: %-2.2d:%-2.2d\n", (int)(songDur_/60000),(int)(fmod(songDur_,60000)/1000));
	add_option(songDetails_,buffer,0.75,y,WHITE_,0,TXT_TOPALIGNED|TXT_CENTERED,NULL);
	
	//Update track notecount string
	sprintf(buffer,"NOTES: %d\n",songN_);
	add_option(songDetails_,buffer,0.75,y,WHITE_,0,TXT_TOPALIGNED|TXT_CENTERED,NULL);
	
	//Update track svcount string
	sprintf(buffer,"SV CHANGES: %d\n",chartStruct_->charts[0]->nSvs-1);
	add_option(songDetails_,buffer,0.75,y,WHITE_,0,TXT_TOPALIGNED|TXT_CENTERED,NULL);
	
	//Update track level string
	sprintf(buffer,"LEVEL: %d\n", (int)round(chartList_[songList_->currOption_-1].difficulty_));
	add_option(songDetails_,buffer,0.75,y,WHITE_,0,TXT_TOPALIGNED|TXT_CENTERED,NULL);
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

static void update_offset(chart_struct* cs, int off, int currtime)
{
	int nLines = atoi(get_datavalue(cs->charts[0]->infodata,"LINES"));
	for(int i = 0; i < nLines; i++)
	{
		struct note_data* note = cs->charts[0]->notes[i];
		while(note != NULL)
		{
			if(currtime == -1 || note->time >= currtime)
				note->time += off;
			note->svtime = 0;
			note = note->tail;
		}
	}
	struct sv_data* svs = cs->charts[0]->svs->tail;
	while(svs != NULL)
	{
		if(currtime == -1 || svs->time >= currtime)
			svs->time += off;
		svs->svtime = 0;
		svs = svs->tail;
	}
	
	int preview = atoi(get_datavalue(cs->headerdata,"PREVIEW"));
	if(currtime == -1 || preview >= currtime)
	{
		char prev[32];

		preview += off;
		if(preview < offset)
			preview = offset;
		
		itoa(preview,prev,10);
		
		update_datavalue(cs->headerdata,"PREVIEW",prev);
		sprintf(previewtext,"\nPREVIEW: %d",preview);
	}
}

static void load_hitsounds()
{
	int nLines = atoi(get_datavalue(chartStruct_->charts[0]->infodata,"LINES"));
	if(nLines > 2)
		nLines = 2;
	
	int stat = 0;
	if(get_trackstatus(NULL) == BASS_ACTIVE_PLAYING)
		stat = 1;
	if(stat)
		pause_track();
	
	float tpos = get_trackpos(NULL)*1000.f;
	
	for(int i = 0; i < nLines; i++)
	{
		generate_audio(HITSOUND1,i,chartStruct_->charts[0]->notes[i],(float)audio_->length_/1000);
		set_tracklink(&audio_->track_,&audio_->hittrack_[i]);
		set_trackpos(&audio_->hittrack_[i],tpos);
	}
	
	if(stat)
		play_track(tpos,1.0);
}

static int process_editmessages(int mouse, int* drag, int currtime)
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
					case VK_F1:
						offset = (int)(get_trackpos(NULL)*1000);
						update_offset(chartStruct_,offset-firstnote,-1);
						update_offset(backChart_,offset-firstnote,-1);
						load_hitsounds();
						
						firstnote = offset;
						
						sprintf(offtext,"OFFSET: %d",offset);
						sprintf(savetext,"\n\n\nUNSAVED CHANGES");
						
						result_ = 2;
					break;
					case VK_F2:
						setpreview = 1;
					break;
					case VK_F5:
						if(offset != firstnote)
						{
							update_offset(chartStruct_,offset-firstnote,-1);
							update_offset(backChart_,offset-firstnote,-1);
							
							if(!changed)
								load_hitsounds();
							
							firstnote = offset;
							
							sprintf(savetext,"\n\n\nUNSAVED CHANGES");
							result_ = 2;
						}
						if(changed)
						{
							destroy_chartstruct(chartStruct_);
							chartStruct_ = copy_chartstruct(backChart_);
							load_hitsounds();

							sprintf(savetext,"\n\n\nUNSAVED CHANGES");
							sprintf(difftext,"\n\nLEVEL: %.2f",run_diffcalc(chartStruct_->charts[0],1.0));
							result_ = 2;
						}
						break;
					case VK_F6:
						pause_track();
						
						float ot = get_trackpos(NULL)*1000;
						struct chart_struct* temp = copy_chartstruct(chartStruct_);
						mute_hittrack(1);
						
						editor_sinewave(temp->charts[0],0);
						destroy_chartstruct(temp);
						
						set_trackpos(NULL,ot);
						pause_track();
						mute_hittrack(0);
						
						break;
					case VK_F8:
						update_rstfile(chartStruct_,NULL);
						sprintf(savetext,"\n\n\nUP TO DATE");
						saved = 1;
						break;
					case VK_F9:
						mute = (!mute);
						mute_hittrack(mute);
						break;
					case VK_ESCAPE:		
						play_soundeffect(MENU_BACK);					
						result_ = -1;
						break;
					case VK_RETURN:
						break;
					case VK_DOWN:
						if(ctrl)
						{
							update_offset(backChart_,-1,currtime);
							changed = 1;
						}
						else
						{
							if(offset > 0)
							{
								play_soundeffect(MENU_BEEP);
								offset--;
								sprintf(offtext,"OFFSET: %d",offset);
							}
						}
						break;
					case VK_UP:
						if(ctrl)
						{
							update_offset(backChart_,1,currtime);
							changed = 1;
						}
						else
						{
							play_soundeffect(MENU_BEEP);
							offset++;
							sprintf(offtext,"OFFSET: %d",offset);
						}
						break;
					case VK_TAB:
						placemode = !placemode;
						break;
					case VK_SPACE:
						int status = get_trackstatus(NULL);
						switch(status)
						{
							case BASS_ACTIVE_PLAYING:
								pause_track();
								break;
							case BASS_ACTIVE_PAUSED:
								play_track(get_trackpos(NULL)*1000,1.0);
								resound = 1;
								break;
							case BASS_ACTIVE_STOPPED:
								play_track(0,0.0);
								resound = 1;
								break;
						}	
						break;
					case VK_OEM_PLUS:
						if(get_trackstatus(NULL) == BASS_ACTIVE_PAUSED)
						{
							float p = get_trackpos(NULL)*1000;
							play_track(get_trackpos(NULL)*1000,1.0);
							set_pitch(audio_->pitch_+0.05);
							pause_track();
							set_trackpos(NULL,p);
						}
						else
							set_pitch(audio_->pitch_+0.05);
						
						sprintf(ratetext,"\n\nRATE: %.2f",audio_->pitch_);
						break;
					case VK_OEM_MINUS:
						if(get_trackstatus(NULL) == BASS_ACTIVE_PAUSED)
						{
							float p = get_trackpos(NULL)*1000;
							play_track(get_trackpos(NULL)*1000,1.0);
							set_pitch(audio_->pitch_-0.05);
							pause_track();
							set_trackpos(NULL,p);
						}
						else
							set_pitch(audio_->pitch_-0.05);
						sprintf(ratetext,"\n\nRATE: %.2f",audio_->pitch_);
						break;
					case VK_SHIFT:
						shift = 1;
					break;
					case VK_CONTROL:
						ctrl = 1;
					break;
				}
				break;
				
				case WM_KEYUP:
				switch (msg.wParam) {
					case VK_SHIFT:
						shift = 0;
					break;
					case VK_CONTROL:
						ctrl = 0;
					break;
				}
				break;
				
				case WM_LBUTTONDOWN:
					if(!click_states[0])
					{
						if(mouse)
						{
							pause_track();
							*drag = 1;
						}
						else if(placemode == 1)
						{
							placesv = 0;
						}
						click_states[0] = 1;
					}
				break;
				
				case WM_LBUTTONUP:
					if(mouse)
						*drag = -1;
					placesv = -1;
					click_states[0] = 0;
				break;
				
				case WM_RBUTTONDOWN:
					if(!click_states[1])
					{
						if(placemode == 0)
							removenotes = 1;
						else if(placemode == 1)
							removesv = 1;
						click_states[1] = 1;
					}
				break;
				
				case WM_RBUTTONUP:
					removenotes = -1;
					removesv = -1;
					click_states[1] = 0;
				break;
				
				case WM_CHAR:
					if(*drag == 0)
					{
						if(msg.wParam >= '0' && msg.wParam <= '9')
						{
							set_trackpos(NULL,(msg.wParam-'0')/10.f*(get_tracklength()*1000.f));
						}
					}
				break;
				case WM_MOUSEWHEEL:
					int delta = GET_WHEEL_DELTA_WPARAM(msg.wParam);
					
					if(delta < 0)
					{
						if(shift)
						{
							if(denom > 0)
							{
								play_soundeffect(MENU_BEEP);
								denom--;
							}
						}
						else if(ctrl)
						{
							if(placemode == 1)
								changesv = -1;
						}
						else
						{
							if(get_trackstatus(NULL) == BASS_ACTIVE_PLAYING)
								pause_track();
							
							if(get_trackpos(NULL)*1000.f > 100)
							{
								set_trackpos(NULL,get_trackpos(NULL)*1000.f-100);
							}
							else
							{
								set_trackpos(NULL,0);
							}
						}
					}
					else
					{
						if(shift)
						{
							if(denom < sizeof(denominations)/sizeof(int)-1)
							{
								play_soundeffect(MENU_BEEP);
								denom++;
							}
						}
						else if(ctrl)
						{
							if(placemode == 1)
								changesv = 1;
						}
						else
						{
							if(get_trackstatus(NULL) == BASS_ACTIVE_PLAYING)
								pause_track();
							
							set_trackpos(NULL,get_trackpos(NULL)*1000.f+100);
							
							resound = 1;
						}
					}
					sprintf(dtext,"DIVISOR: 1/%d",denominations[denom]);
				break;
			}
			
			DispatchMessage(&msg); 
		}
		else
			result_ = -1;
	}
	return result_;
}

static void run_editor()
{
	Beat** lines = init_lines(chartStruct_->charts[0],0,2);

	float trackpos = 0.f, total_time = 0.f, tracklength = get_tracklength()*1000.f;
	int w = video_->settings_[SCREEN_W].value_;
	int h = video_->settings_[SCREEN_H].value_;
	int return_code = 0, mouse_on = 0, mouse_drag = 0;
	
	POINT mousepos;
	
	set_cursor(1);
	offset = 0;
	resound = 1;
	
	int nLines = atoi(get_datavalue(chartStruct_->charts[0]->infodata,"LINES"));
	int preview = atoi(get_datavalue(chartStruct_->headerdata,"PREVIEW"));

	firstnote = 999999999;
	for(int i = 0; i < nLines; i++)
	{
		if(chartStruct_->charts[0]->notes[i]->time < firstnote)
		{
			firstnote = chartStruct_->charts[0]->notes[i]->time;
			offset = firstnote;
		}
	}
	firstnote = offset;
	
	float scr = 60000/chartStruct_->charts[0]->svs->bpm*4, bpm = chartStruct_->charts[0]->svs->bpm;
	int measure, currtime = -1, starttime, placenote[nLines];
	
	saved = 0;
	placemode = 0;
	mute = 0;
	denom = 5;
	
	memset(placenote,0,sizeof(placenote));
	
	sprintf(dtext,"DIVISOR: 1/%d",denominations[denom]);
	sprintf(timetext,"\nTIME: 0");
	sprintf(ratetext,"\n\nRATE: %.2f",audio_->pitch_);
	sprintf(offtext,"OFFSET: %d",offset);
	
	sprintf(previewtext,"\nPREVIEW: %d",preview);
	sprintf(difftext,"\n\nLEVEL: %.2f",run_diffcalc(chartStruct_->charts[0],1.0));
	sprintf(savetext,"\n\n\nUP TO DATE");
	
	struct note_data* note[nLines];
	struct sv_data* svs;
	
	calculate_svtime(backChart_->charts[0]->svs,backChart_->charts[0]->notes[0],2);
	
	load_hitsounds();
	
	while(!return_code)
	{
		track_fadein(2.0);
		
		GetCursorPos(&mousepos);
		ScreenToClient(video_->hWnd,&mousepos);
		
		if(placemode == 0)
		{
			for(int i = 0; i < 4; i++)
			{
				if(GetKeyState(KEYS_[i]) & 0x8000)
				{
					if(!key_states[i] || shift)
					{
						placenote[KEYMAP_[i]] = 1;
						key_states[i] = 1;
					}
				}
				else
					key_states[i] = 0;
			}
		}
		
		if(get_frameready())
		{
			if(GetKeyState(VK_OEM_3) & 0x8000)
			{
				int was_paused = 1;
				if(get_trackstatus(NULL) == BASS_ACTIVE_PLAYING)
				{
					pause_track();
					was_paused = 0;
				}
				
				glClear(GL_COLOR_BUFFER_BIT);
				
				render_simpletext(helptext,-1.0,0.99,WHITE_,3.25,TXT_TOPALIGNED,NULL);
				
				glBlendFunc(GL_SRC_ALPHA, GL_SRC_ALPHA);
				draw_rectangle(0.f,0.f,1.f,1.f,BLACK_,0.925,0);
				glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
				
				if(video_->settings_[SCREEN_BUFFER].value_)
					SwapBuffers(video_->hDC);
				else
					glFlush();
			
				while(GetKeyState(VK_OEM_3) & 0x8000)
				{
					get_frameready();
					
					update_frametimer();
					
					process_editmessages(mouse_on, &mouse_drag, currtime);
					
					update_fps();
					cpu_wait();
				}
				resound = 1;
				if(!was_paused)
					play_track(get_trackpos(NULL)*1000,1.0);
			}
		
			glClear(GL_COLOR_BUFFER_BIT);
			
			trackpos = get_trackpos(NULL)*1000.f;
			
			total_time = preview_sinewave(lines,chartStruct_->charts[0],trackpos,1);
			
			draw_rectangle(trackpos/tracklength*2.f-1.f,0.f,0.01,0.04,WHITE_,1.0,1);
			
			for(int i = 0; i < nLines; i++)
				note[i] = backChart_->charts[0]->notes[i];
			svs = backChart_->charts[0]->svs->tail;
			bpm = chartStruct_->charts[0]->svs->bpm;
			starttime = firstnote;
			while(svs != NULL && svs->time < trackpos)
			{
				if(svs->type == 1)
				{
					starttime = svs->time;
					bpm = svs->bpm;
					
				}
				svs = svs->tail;
			}
			
			scr = 60000/bpm*4;
			
			measure = (int)((trackpos-starttime)/scr);
			
			for(int i = measure-1; i < measure+2; i++)
			{
				float x,y;
				int col;
				for(int j = 0; j < denominations[denom]; j++)
				{
					int t = round(scr*i+(float)j/denominations[denom]*scr)+starttime;
					if(t-starttime < 0 || t > audio_->length_)
						continue;
					
					switch(sizeorder[denom])
					{
						case 4:
							if((j*4)%denominations[denom] == 0)
							{
								y = 1/4.f;
								col = RED_;
							}
							else if((j*8)%denominations[denom] == 0)
							{
								y = 1/6.f;
								col = YELLOW_;
							}
							else if((j*16)%denominations[denom] == 0)
							{
								y = 1/10.f;
								col = GREEN_;
							}
							else
							{
								y = 1/12.f;
								col = BLUE_;
							}
						break;
						
						case 3:
							if((j*3)%denominations[denom] == 0)
							{
								y = 1/4.f;
								col = PURPLE_;
							}
							else if((j*6)%denominations[denom] == 0)
							{
								y = 1/6.f;
								col = TEAL_;
							}
							else if((j*12)%denominations[denom] == 0)
							{
								y = 1/10.f;
								col = WHITE_;
							}
							else
							{
								y = 1/12.f;
								col = BLUE_;
							}
						break;
					}
					
					x = (((float)starttime+(i+(float)j/(denominations[denom]))*scr-trackpos)/scr*2.f-1.f+2/3.f);
					draw_line(x,-1.0,col,y,1);
					
					if(mousepos.y > h/2)
					{
						if(fabs((mousepos.x/(float)w*2.f-1.f)-x) < 1.f/denominations[denom])
						{
							if(placemode == 0)
								draw_circle(x,-1.f+y+0.0625,WHITE_,0.0125,0);
							else if(placemode == 1)
								draw_line(x,0.f,WHITE_,0.5,1);
							currtime = t;
						}
					}
					else
					{
						if(fabs(trackpos-video_->settings_[OFFSET].value_*2-t) < 1.f/denominations[denom]*scr)
							currtime = t;
					}
					
					for(int k = 0; k < nLines; k++)
					{
						while(note[k] != NULL && ((int)note[k]->time - t < -1))
							note[k] = note[k]->tail;
						if(note[k] != NULL && abs(note[k]->time - t) < 2)
						{
							draw_circle(x,-1.f+y+0.025,(k==0?RED_:BLUE_),0.0125,0);
						}
					}
				}
			}
		
			float lastsv = 1.f;
			svs = backChart_->charts[0]->svs->tail;
			while(svs != NULL)
			{
				if(svs->svtime >= total_time-lines[0]->nvertices*8/6 && svs->svtime < total_time+lines[0]->nvertices/3)
				{
					float pos = (svs->svtime - total_time)/(lines[0]->nvertices/3)*4/3.f-1/3.f;
					char svtext[16];
					sprintf(svtext,"%.3f",svs->bpm);
					if(svs->bpm < lastsv)
					{
						render_simpletext(svtext,pos,0.525,WHITE_,2.25,TXT_CENTERED|TXT_BOTALIGNED,NULL);
						draw_line(pos,0.0,GREEN_,0.5,1);
					}
					else
					{
						render_simpletext(svtext,pos,0.525,WHITE_,2.25,TXT_CENTERED|TXT_BOTALIGNED,NULL);
						draw_line(pos,0.0,YELLOW_,0.5,1);
					}
				}
				lastsv = svs->bpm;
				svs = svs->tail;
			}
			
			for(int i = 0; i < nLines; i++)
			{
				if(placenote[i] == 0)
					continue;
				
				struct note_data* n = backChart_->charts[0]->notes[i];
				while(n != NULL && n->tail != NULL && n->tail->time < currtime-1)
					n = n->tail;

				if(n != NULL && abs(n->time - currtime) < 2)
					currtime = n->time;
				else if(n != NULL && n->tail != NULL && abs(n->tail->time - currtime) < 2)
					currtime = n->tail->time;
				
				if(n == NULL || n->time > currtime)
				{
					struct note_data* newnote = (struct note_data*)malloc(sizeof(struct note_data));
					newnote->time = currtime;
					newnote->hittime = -999999999;
					newnote->svtime = -1;
					newnote->tail = n;
					
					backChart_->charts[0]->notes[i] = newnote;
					backChart_->charts[0]->nNotes[i]++;
					changed = 1;
				}
				else if(n->time != currtime && (n->tail == NULL || n->tail->time > currtime))
				{
					struct note_data* newnote = (struct note_data*)malloc(sizeof(struct note_data));
					newnote->time = currtime;
					newnote->hittime = -999999999;
					newnote->svtime = -1;
					newnote->tail = n->tail;
					
					n->tail = newnote;
					backChart_->charts[0]->nNotes[i]++;
					changed = 1;
				}
				else if(mousepos.y > h/2 && backChart_->charts[0]->nNotes[i] > 1)
				{
					if(currtime == n->time)
					{
						backChart_->charts[0]->notes[i] = n->tail;
						backChart_->charts[0]->nNotes[i]--;
						
						free(n);
						changed = 1;
					}				 
					else if(n->tail != NULL && currtime == n->tail->time)
					{	
						struct note_data* temp = n->tail;
						n->tail = n->tail->tail;
						backChart_->charts[0]->nNotes[i]--;
						
						free(temp);
						changed = 1;
					}
				}
			
				placenote[i] = 0;
			}

			if(removenotes != -1)
			{
				if(mousepos.y > h/2)
				{
					for(int i = 0; i < nLines; i++)
					{
						if(backChart_->charts[0]->nNotes[i] <= 1)
							continue;
						
						struct note_data* n = backChart_->charts[0]->notes[i];
						while(n != NULL && n->tail != NULL && abs(n->tail->time) < currtime)
							n = n->tail;
						
						if(n != NULL && n->time == currtime)
						{
							backChart_->charts[0]->notes[i] = n->tail;
							backChart_->charts[0]->nNotes[i]--;
							
							free(n);
							changed = 1;
						}
						else
						{
							if(n->tail != NULL && n->tail->time == currtime)
							{
								struct note_data* temp = n->tail;
								n->tail = n->tail->tail;
								backChart_->charts[0]->nNotes[i]--;
								
								free(temp);
								changed = 1;
							}
						}
					}
				}
			}
			
			if(placesv != -1)
			{
				if(mousepos.y > h/2)
				{
					struct sv_data* s = backChart_->charts[0]->svs->tail;
					while(s != NULL && s->tail != NULL && s->tail->time < currtime-1)
						s = s->tail;
					
					if(s != NULL && abs(s->time - currtime) < 2)
						currtime = s->time;
					else if(s != NULL && s->tail != NULL && abs(s->tail->time - currtime) < 2)
						currtime = s->tail->time;
					
					if(s == NULL || s->time > currtime)
					{
						struct sv_data* newsv = (struct sv_data*)malloc(sizeof(struct sv_data));
						newsv->time = currtime;
						newsv->svtime = -1;
						newsv->bpm = 1.0;
						newsv->type = 0;
						newsv->tail = s;
						
						backChart_->charts[0]->svs->tail = newsv;
						backChart_->charts[0]->nSvs++;
						
						calculate_svtime(backChart_->charts[0]->svs,backChart_->charts[0]->notes[0],2);
						
						changed = 1;
					}
					else
					{
						if(s->tail == NULL || s->tail->time > currtime)
						{
							struct sv_data* newsv = (struct sv_data*)malloc(sizeof(struct sv_data));
							newsv->time = currtime;
							newsv->svtime = -1;
							newsv->bpm = 1.0;
							newsv->type = 0;
							newsv->tail = s->tail;
							
							s->tail = newsv;
							backChart_->charts[0]->nSvs++;
							
							calculate_svtime(backChart_->charts[0]->svs,backChart_->charts[0]->notes[0],2);
							
							changed = 1;
						}
					}
				}
				
				placesv = -1;
			}
			
			if(removesv != -1)
			{
				if(mousepos.y > h/2)
				{
					struct sv_data* s = backChart_->charts[0]->svs->tail;
					while(s != NULL && s->tail != NULL && s->tail->time < currtime-1)
						s = s->tail;
				
					if(s != NULL && abs(s->time - currtime) < 2)
						currtime = s->time;
					else if(s != NULL && s->tail != NULL && abs(s->tail->time - currtime) < 2)
						currtime = s->tail->time;
					
					if(s != NULL && s->time == currtime)
					{
						backChart_->charts[0]->svs->tail = s->tail;
						backChart_->charts[0]->nSvs--;
						
						calculate_svtime(backChart_->charts[0]->svs,backChart_->charts[0]->notes[0],2);
						
						free(s);
						changed = 1;
					}
					else
					{
						if(s->tail != NULL && s->tail->time == currtime)
						{
							struct sv_data* temp = s->tail;
							s->tail = s->tail->tail;
							backChart_->charts[0]->nSvs--;
							
							calculate_svtime(backChart_->charts[0]->svs,backChart_->charts[0]->notes[0],2);
							
							free(temp);
							changed = 1;
						}
					}
				}
				removesv = -1;
			}
			
			if(changesv != 0)
			{
				if(mousepos.y > h/2)
				{
					struct sv_data* s = backChart_->charts[0]->svs->tail;
					while(s != NULL && s->time < currtime-1)
						s = s->tail;
				
					if(s != NULL && abs(s->time - currtime) < 2)
						currtime = s->time;
					
					if(s != NULL && s->time == currtime)
					{
						if(changesv > 0)
							s->bpm += 0.125;
						else
							s->bpm -= 0.125;
						if(s->bpm < 0.125)
							s->bpm = 0.125;
					
						changed = 1;
					}
				}
				changesv = 0;
			}
			
			if(setpreview != 0)
			{
				char prev[32];
				preview = currtime;
				
				itoa(preview,prev,10);
				
				sprintf(previewtext,"\nPREVIEW: %d",preview);
				sprintf(savetext,"\n\n\nUNSAVED CHANGES");
				update_datavalue(chartStruct_->headerdata,"PREVIEW",prev);
				
				setpreview = 0;
				changed = 1;
			}
			
			sprintf(timetext,"\nTIME: %d",(int)trackpos);
			
			render_simpletext(dtext,0.99,0.99,WHITE_,2.25,TXT_TOPALIGNED|TXT_RGHTALIGNED,NULL);
			render_simpletext(timetext,0.99,0.99,WHITE_,2.25,TXT_TOPALIGNED|TXT_RGHTALIGNED,NULL);
			render_simpletext(ratetext,0.99,0.99,WHITE_,2.25,TXT_TOPALIGNED|TXT_RGHTALIGNED,NULL);
			
			if(offset == firstnote)
				render_simpletext(offtext,-1.f,0.99,WHITE_,2.25,TXT_TOPALIGNED,NULL);
			else
				render_simpletext(offtext,-1.f,0.99,offset<firstnote?BLUE_:RED_,2.25,TXT_TOPALIGNED,NULL);
			if(strstr(savetext,"UNSAVED"))
				render_simpletext(savetext,-1.f,0.99,RED_,2.25,TXT_TOPALIGNED,NULL);
			else
				render_simpletext(savetext,-1.f,0.99,WHITE_,2.25,TXT_TOPALIGNED,NULL);
			
			render_simpletext(previewtext,-1.f,0.99,WHITE_,2.25,TXT_TOPALIGNED,NULL);
			render_simpletext(difftext,-1.f,0.99,WHITE_,2.25,TXT_TOPALIGNED,NULL);
			
			draw_frametimer(1.0,-1.0,TXT_BOTALIGNED|TXT_RGHTALIGNED);
			if(video_->settings_[SCREEN_BUFFER].value_)
				SwapBuffers(video_->hDC);
			else
				glFlush();
			
			update_frametimer();
		}
		
		if(mouse_drag == 1)
		{
			set_trackpos(NULL,(float)mousepos.x/w*tracklength);
		}
		else if(mouse_drag == -1)
		{
			//play_track(get_trackpos(NULL)*1000,1.0);
			mouse_drag = 0;
		}
		
		if(fabs((float)mousepos.x/w-trackpos/tracklength) <= 0.005 && abs(mousepos.y-h/2) < (float)h*0.02)
		{	
			if(!mouse_on)
			{
				set_cursor(2);
				mouse_on = 1;
			}
		}
		else
		{
			if(mouse_on && !mouse_drag)
			{
				set_cursor(1);
				mouse_on = 0;
			}
		}
		
		return_code = process_editmessages(mouse_on, &mouse_drag, currtime);
		
		if(return_code == 2)
		{
			destroy_lines(lines);
			lines = init_lines(chartStruct_->charts[0],0,2);
			calculate_svtime(backChart_->charts[0]->svs,backChart_->charts[0]->notes[0],2);
			return_code = 0;
		}
		
		update_fps();
		cpu_wait();
	}

	destroy_lines(lines);
	end_preview();
	
	set_cursor(0);
	placemode = 0;
	
	destroy_hittrack();
	if(saved)
		songdb_create();
}