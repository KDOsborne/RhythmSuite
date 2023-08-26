#include "practice.h"

struct pattern {
	char* str;
	char* ending;
	char* name;
};
struct pattern_struct {
	struct pattern** patterns;
	int n_patterns[3];
};

static options_struct *patSections_,*patSettings_;
static int redraw,toggle,state[3],stateloc[3],bpm=60,repeat=4,rest=1,swap=0,currState_,currPattern_,changed;
static int backbeat[32];
static char* backpat;
static struct pattern_struct* patternList_ = NULL;

static void init_practice();
static void destroy_practice();
static void update_patList_();
static void update_patorder();
static void update_settext();
static void update_backbeat();
static void add_pattern(char*,char*,char*,int);
static void start_practice();

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
				case WM_KEYDOWN:
				redraw = 2;
				switch (msg.wParam) {
					case VK_ESCAPE:
						result_ = -1;
						play_soundeffect(MENU_BACK);
					break;
					case VK_TAB:
						toggle = !toggle;
						play_soundeffect(MENU_BEEP);
						
					break;
					case VK_RETURN:
						if(!toggle)
						{
							if(patSections_->currOption_ != -1)
							{
								int found = 0;
								for(int i = 0; i < 3; i++)
								{
									if(patSections_->currOption_ == stateloc[i])
									{
										state[i] = !state[i];
										update_patList_();
										found = 1;
									}
								}
								play_soundeffect(MENU_CONFIRM);
								
								if(!found)
									start_practice();
							}
						}
						else
						{
							if(patSettings_->currOption_ == patSettings_->nOptions_-1)
							{
								play_soundeffect(MENU_CONFIRM);
								start_practice();
							}
						}
					break;
					case VK_DOWN:
						if(!toggle)
						{
							increment_option(patSections_,1);
							update_patorder();
							changed = 1;
							play_soundeffect(MENU_BEEP);
						}
						else
						{
							increment_option(patSettings_,1);
							if(rest == 0 && patSettings_->currOption_ == 1)
								increment_option(patSettings_,1);
							play_soundeffect(MENU_BEEP);
						}
					break;
					case VK_UP:
						if(!toggle)
						{
							increment_option(patSections_,-1);
							update_patorder();
							changed = 1;
							play_soundeffect(MENU_BEEP);
						}
						else
						{
							increment_option(patSettings_,-1);
							if(rest == 0 && patSettings_->currOption_ == 1)
								increment_option(patSettings_,-1);
							play_soundeffect(MENU_BEEP);
						}
					break;
					case VK_RIGHT:
						if(toggle)
						{
							switch(patSettings_->currOption_)
							{
								case 0:
									if(bpm < 500)
									{
										bpm += 10;
										changed = 1;
									}
								break;
								case 1:
									if(repeat < 16)
										repeat++;
								break;
								case 2:
									if(rest < 4)
										rest++;
								break;
								case 3:
									swap = !swap;
								break;
							}
							update_settext();
							play_soundeffect(MENU_BEEP);
						}
					break;
					case VK_LEFT:
						if(toggle)
						{
							switch(patSettings_->currOption_)
							{
								case 0:
									if(bpm > 30)
									{
										bpm -= 10;
										changed = 1;
									}
								break;
								case 1:
									if(repeat > 4)
										repeat--;
								break;
								case 2:
									if(rest > 0)
										rest--;
								break;
								case 3:
									swap = !swap;
								break;
							}
							update_settext();
							play_soundeffect(MENU_BEEP);
						}
					break;
					case VK_OEM_PLUS:
						if(bpm < 500)
						{
							bpm += 10;
							update_settext();
							changed = 1;
							play_soundeffect(MENU_BEEP);
						}
					break;
					case VK_OEM_MINUS:
						if(bpm > 30)
						{
							bpm -= 10;
							update_settext();
							changed = 1;
							play_soundeffect(MENU_BEEP);
						}
					break;
				}
				patSettings_->highlighted_ = toggle;
				patSections_->highlighted_ = !toggle;
				break;
			}
			DispatchMessage(&msg); 
		}
		else
			result_ = -1;
	}
	return result_;
}

int practice()
{
	int return_code = 0;
	
	init_practice();

	video_->shouldReload_ = 2;
	redraw = 2;
	
	LARGE_INTEGER freq,start,end;
	float elapsed = 0;
	int beat = 0;
	
	QueryPerformanceFrequency(&freq); 
	QueryPerformanceCounter(&start);

	while (!return_code)
	{	
		if(get_frameready())
		{
			QueryPerformanceCounter(&end);
				
			elapsed = (end.QuadPart-start.QuadPart)*1000/freq.QuadPart;
			
			if(elapsed >= backbeat[beat])
			{
				switch(backpat[beat])
				{
					case '1':
					play_soundeffect(BASS_KICK);
					break;
					case '2':
					play_soundeffect(SNARE_DRUM);
					play_soundeffect(BASS_KICK);
					break;
					case '3':
					play_soundeffect(HI_HAT);
					play_soundeffect(BASS_KICK);
					break;
				}
				beat++;
				if(beat > 31 || backbeat[beat] == -1)
				{
					beat = 0;
					start = end;
					if(changed)
					{
						update_backbeat();
						changed = 0;
					}
				}
			}
			
			if(redraw || video_->shouldReload_)
			{
				glClear(GL_COLOR_BUFFER_BIT);
				
				draw_options(patSettings_);
				draw_options(patSections_);
				
				if(toggle)
					render_simpletext("PRESS \3TAB\1 TO SELECT PATTERNS",0.99f,-0.99f,WHITE_,2.5,TXT_BOTALIGNED|TXT_RGHTALIGNED,NULL);
				else
					render_simpletext("PRESS \3TAB\1 TO SELECT SETTINGS",0.99f,-0.99f,WHITE_,2.5,TXT_BOTALIGNED|TXT_RGHTALIGNED,NULL);
				
				if(video_->settings_[SCREEN_BUFFER].value_)
					SwapBuffers(video_->hDC);
				else
					glFlush();
				
				if(video_->shouldReload_)
				{
					redraw = 2;
					video_->shouldReload_ = 0;
				}
				if(redraw > 0)
					redraw--;
				
			}
			update_frametimer();
		}
		
		return_code = process_messages();
		
		if(changed == 2)
		{
			QueryPerformanceCounter(&start);
			changed = 0;
			beat = 0;
		}
		
		update_fps();
		cpu_wait();
	}
	
	video_->shouldReload_ = 0;
	
	destroy_practice();
	
	stop_tracks();
	
	return return_code;
}

static void init_practice()
{
	float x = -1.0, y = 0.925;
	
	toggle = 0;
	changed = 0;
	currState_ = 0;
	currPattern_ = 0;
	
	if(patternList_ == NULL)
	{
		patternList_ = (struct pattern_struct*)malloc(sizeof(struct pattern_struct));
		memset(patternList_,0,sizeof(struct pattern_struct));
		
		patternList_->patterns = (struct pattern**)malloc(sizeof(struct pattern*)*3);
		
		patternList_->patterns[0] = NULL;
		patternList_->patterns[1] = NULL;
		patternList_->patterns[2] = NULL;
		
		//Basic Patterns
		add_pattern("10101010","1010101010","1-1-1-1(THE CLASSIC)\n",0);
		add_pattern("30303030","3030303030","3-3-3-3(THE MASHER)\n",0);
		add_pattern("30103010","3010301030","3-1-3-1(THE ROCKER)\n",0);
		add_pattern("30101010","3010101030","3-1-1-1-3-1-1-1(THE RUNNING MAN)\n",0);
		add_pattern("301010","30101030","3-1-1-3-1-1(THE SWAPPER)\n",0);
		add_pattern("3010103010301010","301010301030101030","3-1-1-3-1-3-1-1(THE DETOUR)\n",0);
		add_pattern("30303010","3030301030","3-3-3-1(THE ONE-OFF)\n",0);
		add_pattern("303010303010","30301030301030","3-3-1-3-3-1(THE BIG SPLIT)\n",0);
		
		//Intermediate Patterns
		add_pattern("11101110","1110111010","111(THE TRIPLE)\n",1);
		add_pattern("31103110","3110311030","311(THE OPENER)\n",1);
		add_pattern("11301130","1130113030","113(THE CLOSER)\n",1);
		add_pattern("31101010","3110101030","311-1-1(THE OPEN GALLOP)\n",1);
		add_pattern("11301010","1130101030","113-1-1(THE CLOSED GALLOP)\n",1);
		add_pattern("31013010","3101301030","31-13-1(THE SPLITS)\n",1);
		add_pattern("113130","113130113","11313(THE DELAYED ROCKER)\n",1);
		add_pattern("311130113010","31113011301030","31113-113-1(THE RUNNING GALLOP)\n",1);
		
		//Advanced Patterns
		add_pattern("112110","1121103","11211(THE IMPOSTER)\n",2);
		add_pattern("21212121","2121212130","2121(THE ROLLER)\n",2);
		add_pattern("21122112","2112211230","2112(THE WATERFALL)\n",2);
		add_pattern("212212","21221230","212(THE HOT ROLLER)\n",2);
		add_pattern("113223","113223","11322(THE BOOMERANG)\n",2);
		add_pattern("312312","3123123","312(THE HEAVY ROLLER)\n",2);
		add_pattern("31213121","3121312130","3121(THE TWO STEP)\n",2);
		add_pattern("31323132","3132313230","3132(THE ROCK STEP)\n",2);
	}
	
	patSections_ = create_options("SELECT A PATTERN TO PRACTICE\n",LPURPLE_,2.75,0,-1,-1,PURPLE_,TXT_NORESET);
	memset(state,0,sizeof(state));
	
	stateloc[0] = 0;
	stateloc[1] = 1;
	stateloc[2] = 2;
	
	add_option(patSections_,"BASIC\n",x,y,LTEAL_,0,TXT_TOPALIGNED,NULL);
	add_option(patSections_,"INTERMEDIATE\n",x,y,LTEAL_,0,TXT_TOPALIGNED,NULL);
	add_option(patSections_,"ADVANCED\n",x,y,LTEAL_,0,TXT_TOPALIGNED,NULL);
	
	patSections_->highlighted_ = 1;
	
	patSettings_ = create_options("PRACTICE SETTINGS\n",LPURPLE_,2.75,0,-1,-1,PURPLE_,TXT_NORESET|TXT_RGHTALIGNED); 
	update_settext();
	
	update_backbeat();
}

static void destroy_practice()
{
	delete_options(patSections_);
	delete_options(patSettings_);
	patSections_ = NULL;
	patSettings_ = NULL;
}

static void update_patList_()
{
	float x = -1.0, y = 0.925;
	
	clear_options(patSections_);
	for(int i = 0; i < 3; i++)
	{
		switch(i)
		{
			case 0:
				add_option(patSections_,"BASIC\n",x,y,LTEAL_,0,TXT_TOPALIGNED,NULL);
				stateloc[0] = patSections_->nOptions_-1;
			break;
			case 1:
				add_option(patSections_,"INTERMEDIATE\n",x,y,LTEAL_,0,TXT_TOPALIGNED,NULL);
				stateloc[1] = patSections_->nOptions_-1;
			break;
			case 2:
				add_option(patSections_,"ADVANCED\n",x,y,LTEAL_,0,TXT_TOPALIGNED,NULL);
				stateloc[2] = patSections_->nOptions_-1;
			break;
		}
		if(state[i])
			for(int j = 0; j < patternList_->n_patterns[i]; j++)
				add_option(patSections_,patternList_->patterns[i][j].name,x,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	}
	
	update_patorder();
}

static void update_patorder()
{
	int ndisp = 27, halfdisp = (int)(ndisp/2)+1;
	
	patSections_->total_ = (patSections_->nOptions_>=ndisp?ndisp:patSections_->nOptions_);
	
	if(patSections_->total_ < ndisp || patSections_->currOption_ <= ndisp/2)
	{
		patSections_->from_ = -1;
	}
	else
	{
		if(patSections_->nOptions_-patSections_->currOption_ < halfdisp)
		{
			patSections_->from_ = -patSections_->nOptions_-halfdisp-(halfdisp-1-(patSections_->nOptions_-patSections_->currOption_));
		}
		else
		{
			patSections_->from_ = -ndisp/2;
		}
	}
}

static void update_settext()
{
	char buffer[32];
	clear_options(patSettings_);
	
	sprintf(buffer,"BPM: %d\n",bpm);
	add_option(patSettings_,buffer,0.99,0.925,WHITE_,0,TXT_RGHTALIGNED|TXT_TOPALIGNED,NULL);
	
	sprintf(buffer,"REPEAT MEASURES: %d\n",repeat);
	if(rest != 0)
		add_option(patSettings_,buffer,0.99,0.925,WHITE_,0,TXT_RGHTALIGNED|TXT_TOPALIGNED,NULL);
	else
		add_option(patSettings_,"REPEAT MEASURES: INF\n",0.99,0.925,GRAY_,0,TXT_RGHTALIGNED|TXT_TOPALIGNED,NULL);
	
	sprintf(buffer,"REST MEASURES: %d\n",rest);
	add_option(patSettings_,buffer,0.99,0.925,WHITE_,0,TXT_RGHTALIGNED|TXT_TOPALIGNED,NULL);
	
	sprintf(buffer,"SWAP COLORS: %s\n",swap==1?"ON":"OFF");
	add_option(patSettings_,buffer,0.99,0.925,WHITE_,0,TXT_RGHTALIGNED|TXT_TOPALIGNED,NULL);
	
	add_option(patSettings_,"START",0.99,0.925,WHITE_,0,TXT_RGHTALIGNED|TXT_TOPALIGNED,NULL);
}

static void update_backbeat()
{
	for(int i = 0; i < 3; i++)
	{
		if(patSections_->currOption_ == stateloc[i])
			break;
		else if(patSections_->currOption_ > stateloc[i])
		{
			if(i == 2 || patSections_->currOption_ < stateloc[i+1])
			{
				currPattern_ = patSections_->currOption_-stateloc[i]-1;
				currState_ = i;
			}
		}
	}
	backpat = patternList_->patterns[currState_][currPattern_].str;
	
	memset(backbeat,-1,sizeof(backbeat));
	for(int i = 0; i < strlen(backpat)+1; i++)
	{
		backbeat[i] = (int)((float)i/16.f*(60000/bpm)*4);
	}
}

static void add_pattern(char* str, char* ending, char* name, int type)
{
	int n = patternList_->n_patterns[type];
	
	patternList_->patterns[type] = (struct pattern*)realloc(patternList_->patterns[type],sizeof(struct pattern)*(n+1));
	
	patternList_->patterns[type][n].str = malloc(sizeof(char)*(strlen(str)+1));
	patternList_->patterns[type][n].ending = malloc(sizeof(char)*(strlen(ending)+1));
	patternList_->patterns[type][n].name = malloc(sizeof(char)*(strlen(name)+1));
	
	memcpy(patternList_->patterns[type][n].str,str,sizeof(char)*(strlen(str)+1));
	memcpy(patternList_->patterns[type][n].ending,ending,sizeof(char)*(strlen(ending)+1));
	memcpy(patternList_->patterns[type][n].name,name,sizeof(char)*(strlen(name)+1));
	
	patternList_->n_patterns[type]++;
}

/*static void free_patterns()
{
	for(int i = 0; i < 3; i++)
	{
		for(int j = 0; j < patternList_->n_patterns[i]; j++)
		{
			free(patternList_->patterns[i][j].name);
			free(patternList_->patterns[i][j].ending);
			free(patternList_->patterns[i][j].str);
		}
		free(patternList_->patterns[i]);
	}
	free(patternList_->patterns);
	
	patternList_ = NULL;
}*/

static void start_practice()
{
	update_backbeat();
	
	char buffer[1024];
	char* str = patternList_->patterns[currState_][currPattern_].str;
	
	memset(buffer,0,sizeof(buffer));
	
	if(rest == 0)
	{
		sprintf(buffer,"%s%s%s%s",str,str,str,str);
	}
	else
	{
		for(int i = 0; i < repeat-1; i++)
			strcat(buffer,str);
		
		strcat(buffer,patternList_->patterns[currState_][currPattern_].ending);
		
		for(int i = 0; i < rest; i++)
			strcat(buffer,"00000000");
	}
	
	if(swap)
	{
		for(int i = 0; i < strlen(buffer); i++)
		{
			if(buffer[i] == '1')
				buffer[i] = '2';
			else if(buffer[i] == '2')
				buffer[i] = '1';
		}
	}
	
	play_soundeffect(HI_HAT);

	practice_sinewave(bpm,buffer,rest==0,swap);
	
	changed = 2;
	toggle = 0;
}