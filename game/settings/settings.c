#include "settings.h"
#include <math.h>
#include <stdio.h>

static settings** setList_;
static settings*  settings_;
static options_struct* setType_;
static struct monitor_mode* modeList_;
static int beats, changeKey = -1, redraw, calibrating, secalibrating, seoffset;
static float deviation, stdev, devlist[20];

static const int nOpt[] = { 5, 8, 6 };

static void change_volume(int n)
{
	int vol = atoi(get_option_text(setList_[1]->options_[settings_->currSetting_])) + n;
	char buf[4];
	
	clear_options(setList_[1]->options_[settings_->currSetting_]);
	
	if(vol < 0)
		vol = 0;
	else if(vol > 100)
		vol = 100;
	
	sprintf(buf,"%d",vol);
	
	add_option(setList_[1]->options_[settings_->currSetting_],buf,-1.0,0.5,WHITE_,0,0,NULL);
	
	video_->settings_[VOLUME+settings_->currSetting_].value_ = vol;
	video_->settings_[VOLUME+settings_->currSetting_].update_ = vol;
	
	change_audiovolume();
}

static void change_offset(int n)
{
	int offset = video_->settings_[OFFSET].value_ + n;
	char buf[16];
	
	clear_options(setList_[1]->options_[4]);
	
	if(offset > 500)
		offset = 500;
	else if(offset < -500)
		offset = -500;
	
	sprintf(buf,"%dMS",offset);
	
	add_option(setList_[1]->options_[4],buf,-1.0,0.5,WHITE_,0,0,NULL);
	
	video_->settings_[OFFSET].value_ = offset;
	video_->settings_[OFFSET].update_ = offset;
}

static void change_seoffset(int n)
{
	int offset = video_->settings_[SEOFFSET].value_ + n;
	
	if(offset > 0)
		offset = 0;
	else if(offset < -500)
		offset = -500;
	
	video_->settings_[SEOFFSET].value_ = offset;
	video_->settings_[SEOFFSET].update_ = offset;
	seoffset = offset;
}

static void change_hiterror(int type, int n)
{
	int newvalue;
	char buf[16];
	
	destroy_hiterror();
	
	switch(type)
	{
		case 2:
			newvalue = video_->settings_[HESTYLE].value_+n;
			if(newvalue < 0)
				newvalue = 2;
			else if(newvalue > 2)
				newvalue = 0;
			video_->settings_[HESTYLE].value_ = newvalue;
			video_->settings_[HESTYLE].update_ = newvalue;
		break;
		case 3:
			newvalue = video_->settings_[HESIZE].value_+n;
			
			if(newvalue < 0)
				newvalue = 0;
			else if(newvalue > 10)
				newvalue = 10;
			
			clear_options(settings_->options_[type]);
			sprintf(buf,"%d",newvalue);
			add_option(settings_->options_[type],buf,-1.0,0.5,WHITE_,0,0,NULL);
			video_->settings_[HESIZE].value_ = newvalue;
			video_->settings_[HESIZE].update_ = newvalue;
		break;
		case 4:
			newvalue = video_->settings_[HEALPHA].value_+n;
			
			if(newvalue < 1)
				newvalue = 1;
			else if(newvalue > 100)
				newvalue = 100;
			
			clear_options(settings_->options_[type]);
			sprintf(buf,"%d",newvalue);
			add_option(settings_->options_[type],buf,-1.0,0.5,WHITE_,0,0,NULL);
			video_->settings_[HEALPHA].value_ = newvalue;
			video_->settings_[HEALPHA].update_ = newvalue;
		break;
	}
	
	add_hiterrorpoint(0,0,GREEN_);
}

static void change_playstyle(int n)
{
	int style = video_->settings_[PLAYSTYLE].value_ + n;
	char buf[16];
	
	clear_options(setList_[2]->options_[0]);
	clear_options(setList_[2]->options_[1]);
		
	add_option(setList_[2]->options_[0],"PLAYSTYLE:",-1.0,0.5,WHITE_,0,0,NULL);
	add_option(setList_[2]->options_[1],"\nKEYBINDS:",-1.0,0.5,WHITE_,0,0,NULL);
	
	if(style < 0)
		style = 5;
	else if(style > 5)
		style = 0;
	
	int k[4];
	
	switch (style)
	{
		case 1:
			k[0] = RED_;
			k[1] = BLUE_;
			k[2] = BLUE_;
			k[3] = RED_;
			break;
		case 2:
			k[0] = RED_;
			k[1] = RED_;
			k[2] = BLUE_;
			k[3] = BLUE_;
			break;	
		case 3:
			k[0] = BLUE_;
			k[1] = BLUE_;
			k[2] = RED_;
			k[3] = RED_;
			break;
		case 4:
			k[0] = RED_;
			k[1] = BLUE_;
			k[2] = RED_;
			k[3] = BLUE_;
			break;
		case 5:
			k[0] = BLUE_;
			k[1] = RED_;
			k[2] = BLUE_;
			k[3] = RED_;
			break;
		default:
			k[0] = BLUE_;
			k[1] = RED_;
			k[2] = RED_;
			k[3] = BLUE_;
			break;
	}
	
	
	for(int i = 0; i < 4; i++)
	{
		add_option(setList_[2]->options_[0],"|",-1.0,0.5,k[i],0,0,NULL);
		
		if(GetKeyNameTextA(KEYSL_[i] << 16,buf,sizeof(buf)))
		{
			for(int j = 0; j < strlen(buf); j++)
			{
				if(buf[j] == ',')
					sprintf(buf,"COMMA");
				if(buf[j] == ' ')
					buf[j] = '_';
				else
					buf[j] = toupper(buf[j]);
			}
			add_option(setList_[2]->options_[1],buf,-1.0,0.5,k[i],0,0,NULL);
		}
		KEYMAP_[i] = (k[i]==RED_?0:1);
	}
	
	video_->settings_[PLAYSTYLE].value_ = style;
	video_->settings_[PLAYSTYLE].update_ = style;
}

static int change_key(MSG msg)
{
	char buf[16];
	
	if(GetKeyNameTextA(msg.lParam,buf,sizeof(buf)))
	{
		for(int i = 0; i < 4; i++)
		{
			if(i != changeKey && KEYSL_[i] == ((msg.lParam >> 16) & 511))
			{
				KEYS_[i] = KEYS_[changeKey];
				KEYSL_[i] = KEYSL_[changeKey];
				
				video_->settings_[KEY1+i].value_ = KEYSL_[changeKey];
				video_->settings_[KEY1+i].update_ = KEYSL_[changeKey];
			}
		}
		KEYS_[changeKey] = msg.wParam;
		KEYSL_[changeKey] = (msg.lParam >> 16) & 511;
		
		video_->settings_[KEY1+changeKey].value_ = KEYSL_[changeKey];
		video_->settings_[KEY1+changeKey].update_ = KEYSL_[changeKey];
	}
	else 
		return 0;
	
	change_playstyle(0);
	
	return 1;
}

static void change_hitsounds(int n)
{
	video_->settings_[HITSOUNDS].value_ = n;
	video_->settings_[HITSOUNDS].update_ = n;
}

static int calibrate_messages()
{
	MSG msg;
	int result_ = 0;
	
	float elapsed = get_trackpos(&audio_->hittrack_[0])*1000.f;
	
	elapsed = fmod(elapsed,60000/120.f);
	
	while(PeekMessage(&msg, video_->hWnd, 0, 0, PM_NOREMOVE))
	{
		if(GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg); 
			
			switch(msg.message) {
				case WM_KEYDOWN:
				redraw = 2;
				if(msg.wParam == VK_ESCAPE)
				{
					play_soundeffect(MENU_BACK);
					calibrating = 0;
					destroy_hittrack();
				}
				else
				{
					float hit_time;
					
					if(video_->settings_[HITSOUNDS].value_)
						play_soundeffect(HITSOUND1);
					
					if(elapsed < 60000/120.f/2)
						hit_time = elapsed;
					else
						hit_time = elapsed-60000/120.f;
					
					deviation += hit_time;
					devlist[calibrating-1] = hit_time;
					
					calibrating++;
					if(calibrating > 20)
					{
						destroy_hittrack();
						play_soundeffect(HI_HAT);
						play_soundeffect(BASS_KICK);
						calibrating = -1;
						
						float s = 0;
						for(int i = 0; i < 20; i++)
							s += pow((devlist[i]-(deviation/20)),2);
						
						stdev = sqrt(s/20);
					}
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

static int align_messages()
{
	MSG msg;
	int result_ = 0;
	
	float elapsed = get_trackpos(&audio_->hittrack_[0])*1000.f;
	if(elapsed < 60000/120.f && beats > 2)
		beats = 0;
	if(elapsed >= 60000/120.f*beats+seoffset)
	{
		beats++;
		play_soundeffect(BASS_KICK);
	}
	
	elapsed = fmod(elapsed,60000/120.f);
	
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
						play_soundeffect(MENU_BACK);
						secalibrating = 0;
						destroy_hittrack();
						break;
					case VK_LEFT:
						change_seoffset(-1);
						play_soundeffect(MENU_BEEP);
						break;
					case VK_RIGHT:
						if(seoffset == 0)
							play_soundeffect(MENU_INVALID);
						else
							play_soundeffect(MENU_BEEP);
						change_seoffset(1);
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
				
				if(changeKey != -1)
				{
					if(msg.wParam == VK_ESCAPE || msg.wParam == VK_OEM_3 || msg.wParam == VK_SPACE)
						changeKey = -1;
					else
					{
						result_ = change_key(msg);
						if(result_)
						{
							changeKey++;
							if(changeKey > 3)
								changeKey = -1;
							play_soundeffect(MENU_BEEP);
						}
						result_ = 0;
					}
					settings_->options_[settings_->currSetting_]->currOption_ = changeKey+1;
					break;
				}
				switch (msg.wParam) {
				
					case VK_ESCAPE:
						play_soundeffect(MENU_BACK);
						calibrating = 0;
						result_ = -1;
						break;
					case VK_RETURN:
						if(setType_->currOption_ == 1)
						{
							if(settings_->currSetting_ == 5)
							{
								generate_calibration();
								set_trackloop(&audio_->hittrack_[0],1);
								play_hittrack(0);
								calibrating = 1;
								beats = 0;
								deviation = 0;
								stdev = 0;
								memset(devlist,0,sizeof(devlist));
							}
							else if(settings_->currSetting_ == 6)
							{
								generate_calibration();
								set_trackloop(&audio_->hittrack_[0],1);
								play_hittrack(0);
								secalibrating = 1;
								beats = 0;
							}
							
						}
						else if(setType_->currOption_ == 2 && settings_->currSetting_ == 1)
						{
							changeKey = 0;
							settings_->options_[settings_->currSetting_]->currOption_ = changeKey+1;
							play_soundeffect(MENU_CONFIRM);
						}
						if(settings_->currSetting_ == nOpt[setType_->currOption_]-1)
						{
							play_soundeffect(MENU_CONFIRM);
							if(settings_->options_[settings_->currSetting_]->currOption_ == 0)
								return -1;
							else
							{
								for(int i = 0; i < nOpt[setType_->currOption_]-1; i++)
								{
									if(i == 0)
									{
										int w_,h_;
										sscanf(get_option_text(settings_->options_[i]),"%dX%d",&w_, &h_);
										update_videosettings("WIDTH",w_);
										update_videosettings("HEIGHT",h_);
									}
									else if(i == 1)
									{
										update_videosettings("FULLSCREEN",settings_->options_[i]->currOption_);
									}
									else if(i == 2)
									{
										update_videosettings("FPSLIMIT",settings_->options_[i]->currOption_);
									}
								}
								
								update_video();
							}
						}
						break;
					case VK_DOWN:
						if(settings_->currSetting_ == -1)
						{
							setType_->highlighted_ = 0;
							settings_->currSetting_ = 0;
						}
						else
						{
							settings_->options_[settings_->currSetting_]->highlighted_ = 0;
							settings_->currSetting_++;
						}
						if(settings_->currSetting_ == nOpt[setType_->currOption_])
						{
							setType_->highlighted_ = 1;
							settings_->currSetting_ = -1;
						}
						else
							settings_->options_[settings_->currSetting_]->highlighted_ = 1;
						play_soundeffect(MENU_BEEP);
						break;
						
					case VK_UP:
						if(settings_->currSetting_ == -1)
						{
							setType_->highlighted_ = 0;
							settings_->currSetting_ = nOpt[setType_->currOption_]-1;
						}
						else
						{
							settings_->options_[settings_->currSetting_]->highlighted_ = 0;
							settings_->currSetting_--;
						}
						if(settings_->currSetting_ == -1)
							setType_->highlighted_ = 1;
						else
							settings_->options_[settings_->currSetting_]->highlighted_ = 1;
						play_soundeffect(MENU_BEEP);
						break;
					case VK_LEFT:
						if(settings_->currSetting_ == -1)
						{
							increment_option(setType_,-1);
							settings_ = setList_[setType_->currOption_];
						}
						else
						{
							if(setType_->currOption_ == 1)
							{
								if(settings_->currSetting_ >= 0 && settings_->currSetting_ <= 2)
									change_volume(-1);
								else if(settings_->currSetting_ == 3)
									change_hitsounds(!settings_->options_[settings_->currSetting_]->currOption_);
								else if(settings_->currSetting_ == 4)
									change_offset(-1);
							}
							else if(setType_->currOption_ == 2)
							{
								if(settings_->currSetting_ == 0)
								{
									change_playstyle(-1);
									play_soundeffect(MENU_BEEP);
									break;
								}
								else if(settings_->currSetting_ >= 2 && settings_->currSetting_ <= 4)
									change_hiterror(settings_->currSetting_,-1);
								
							}
							increment_option(settings_->options_[settings_->currSetting_],-1);
						}
						play_soundeffect(MENU_BEEP);
						break;
					case VK_RIGHT:
						if(settings_->currSetting_ == -1)
						{
							increment_option(setType_,1);
							settings_ = setList_[setType_->currOption_];
						}
						else
						{
							if(setType_->currOption_ == 1)
							{
								if(settings_->currSetting_ >= 0 && settings_->currSetting_ <= 2)
									change_volume(1);
								else if(settings_->currSetting_ == 3)
									change_hitsounds(!settings_->options_[settings_->currSetting_]->currOption_);
								else if(settings_->currSetting_ == 4)
									change_offset(1);
							}
							else if(setType_->currOption_ == 2)
							{
								if(settings_->currSetting_ == 0)
								{
									change_playstyle(1);
									play_soundeffect(MENU_BEEP);
									break;
								}
								else if(settings_->currSetting_ >= 2 && settings_->currSetting_ <= 4)
									change_hiterror(settings_->currSetting_,1);
							}
							increment_option(settings_->options_[settings_->currSetting_],1);
						}
						play_soundeffect(MENU_BEEP);
						break;
					case VK_F11:
						video_->settings_[SCREEN_BUFFER].update_ = !video_->settings_[SCREEN_BUFFER].update_;
						update_video();
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

int settings_menu()
{
	int return_code = 0;
	if(video_->shouldReload_)
		video_->shouldReload_ = 0;

	init_settings();
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	
	redraw = 2;
	calibrating = 0;
	secalibrating = 0;
	
	while (!return_code)
	{	
		if(get_frameready())
		{
			if(redraw || video_->shouldReload_)
			{
				glClear(GL_COLOR_BUFFER_BIT);
				
				if(video_->shouldReload_)
				{
					reload_modes();
					video_->shouldReload_ = 0;
				}
				for(int i = 0; i < nOpt[setType_->currOption_]; i++)
					draw_options(settings_->options_[i]);
				draw_options(setType_);
				
				if(setType_->currOption_ == 0 && settings_->currSetting_ == 3)
				{
					if(video_->settings_[SCREEN_BUFFER].value_)
					{
						render_simpletext("DOUBLE BUFFERING IS \7ON\1\nPRESS \3F11\1 TO TURN IT \2OFF",0.f,0.f,WHITE_,2.25,TXT_CENTERED,NULL);
					}
					else
					{
						render_simpletext("DOUBLE BUFFERING IS \2OFF\1\nPRESS \3F11\1 TO TURN IT \7ON",0.f,0.f,WHITE_,2.25,TXT_CENTERED,NULL);
						render_simpletext("\n\n\n\5WARNING: IF YOUR SYSTEM DOESNT SUPPORT DOUBLE BUFFERING THE GAME WINDOW WILL GO BLACK\n\1IF THIS HAPPENS PRESS \3F11\1 TO TURN DOUBLE BUFFERING \2OFF",0.f,0.f,WHITE_,2.25,TXT_CENTERED,NULL);
						render_simpletext("\n\n\n\n\n\nIF DOUBLE BUFFERING IS \2OFF\1 YOU WILL NOT BE ABLE TO USE GAME CAPTURE IN OBS",0.f,0.f,WHITE_,2.25,TXT_CENTERED,NULL);
					}
				}
				else if(setType_->currOption_ == 1 && 
						(settings_->currSetting_ == 4 || settings_->currSetting_ == 5))
				{
					if(calibrating > 0)
					{
						render_simpletext("TAP ANY KEY TO THE BEAT",0.f,0.f,WHITE_,2.25,TXT_CENTERED,NULL);

						char buf[4];
						sprintf(buf,"\n%d",21-calibrating);
						render_simpletext(buf,0.f,0.f,WHITE_,2.25,TXT_CENTERED,NULL);
					}
					else if(calibrating < 0)
					{
						int grade = get_grade(stdev);
						
						char buf[128];
						
						if(grade == 8)
						{
							render_simpletext("CALIBRATION FAILED!\n\n\2YOU WERE TOO INACCURATE. TRY TAPPING MORE CONSISTENTLY",0.f,0.f,WHITE_,2.25,TXT_CENTERED,NULL);
						}
						else if(grade == 7)
						{
							render_simpletext("CALIBRATION FAILED!\n\n\5YOU WERE A BIT TOO INACCURATE. TRY TAPPING MORE CONSISTENTLY",0.f,0.f,WHITE_,2.25,TXT_CENTERED,NULL);
						}
						else
						{
							render_simpletext("CALIBRATION COMPLETE!",0.f,0.f,WHITE_,2.25,TXT_CENTERED,NULL);
							sprintf(buf,"\nGRADE:\3 %s\1\nSET YOUR OFFSET TO:\3 %d",get_gradetext(stdev),(int)round(deviation/20));
							render_simpletext(buf,0.f,0.f,WHITE_,2.25,TXT_CENTERED,NULL);
							if(grade >= 4)
							{
								render_simpletext("\n\n\n\n\4YOU SHOULD CALIBRATE MULTIPLE TIMES TO CONFIRM THE RESULTS",0.f,0.f,WHITE_,2.25,TXT_CENTERED,NULL);
							}
							else if(grade >= 1)
							{
								render_simpletext("\n\n\n\n\4YOU KILLED IT!",0.f,0.f,WHITE_,2.25,TXT_CENTERED,NULL);
							}
							else
							{
								render_simpletext("\n\n\n\n\6W\6T\6F\6? \6Y\6O\6U\6R\6E \6I\6N\6S\6A\6N\6E\6!",0.f,0.f,WHITE_,2.25,TXT_CENTERED,NULL);
							}
						}
					}
					else
					{
						if(settings_->currSetting_ == 5)
							render_simpletext("\3TIP:\1 CALIBRATION IS BEST WITH HEADPHONES ON",0.f,0.f,WHITE_,2.25,TXT_CENTERED,NULL);
					}
				}
				else if(setType_->currOption_ == 1 && settings_->currSetting_ == 6)
				{
					if(secalibrating)
					{
						char buffer[128];
						sprintf(buffer,"ADJUST THE OFFSET BACKWARDS UNTIL THE BEATS LINE UP\nCURRENT OFFSET:\3 %d",seoffset);
						render_simpletext(buffer,0.f,0.f,WHITE_,2.25,TXT_CENTERED,NULL);
					}
					else
						render_simpletext("THIS WILL DETERMINE YOUR AUDIO DRIVER DELAY FOR PRACTICE+DEMO MODE",0.f,0.f,WHITE_,2.25,TXT_CENTERED,NULL);
				}
				else if(setType_->currOption_ == 2)
				{
					switch(settings_->currSetting_)
					{
						case 0:
							switch(video_->settings_[PLAYSTYLE].value_)
							{
								case 0:
									render_simpletext("THE DEFAULT PLAYSTYLE\nTHIS IS HOW YOU SHOULD PLAY THE GAME",0.f,0.f,WHITE_,2.25,TXT_CENTERED,NULL);
								break;
								case 1:
									render_simpletext("THE DEFAULT PLAYSTYLE BUT FLIPPED\nTHIS IS FINE TOO",0.f,0.f,WHITE_,2.25,TXT_CENTERED,NULL);
								break;
								case 2:
									render_simpletext("THIS PLAYSTYLE WILL MAKE CERTAIN PATTERNS VERY DIFFICULT\n\5NOT RECOMMENED",0.f,0.f,WHITE_,2.25,TXT_CENTERED,NULL);
								break;
								case 3:
									render_simpletext("THIS PLAYSTYLE WILL MAKE CERTAIN PATTERNS VERY DIFFICULT\n\5NOT RECOMMENED",0.f,0.f,WHITE_,2.25,TXT_CENTERED,NULL);
								break;
								case 4:
									render_simpletext("THE DEFAULT PLAYSTYLE BUT SHUFFLED\nTHIS IS EFFECTIVELY THE SAME AS THE DEFAULT PLAYSTYLE",0.f,0.f,WHITE_,2.25,TXT_CENTERED,NULL);
								break;
								case 5:
									render_simpletext("THE DEFAULT PLAYSTYLE BUT SHUFFLED\nTHIS IS EFFECTIVELY THE SAME AS THE DEFAULT PLAYSTYLE",0.f,0.f,WHITE_,2.25,TXT_CENTERED,NULL);
								break;
							}		
						break;
						case 1:
							if(changeKey == -1)
								render_simpletext("PRESS ENTER TO TYPE NEW KEYBINDS",0.f,0.f,WHITE_,2.25,TXT_CENTERED,NULL);
						break;
						case 2:
							if(video_->settings_[HESTYLE].value_ == 0)
								render_simpletext("THIS WILL MAKE ACCURACY \2VERY\1 DIFFICULT\n\5NOT RECOMMENED",0.f,0.f,WHITE_,2.25,TXT_CENTERED,NULL);
							else if(video_->settings_[HESTYLE].value_ == 1)
								render_simpletext("HIT ERROR APPEARS AT THE ACCURACY OF YOUR NOTES(\4EARLY=LOWER \5LATE=HIGHER\1)",0.f,0.f,WHITE_,2.25,TXT_CENTERED,NULL);
							else if(video_->settings_[HESTYLE].value_ == 2)
							{
								render_simpletext("HIT ERROR APPEARS AT THE Y VALUE OF THE WAVE WHEN THE NOTE WAS PRESSED",0.f,0.f,WHITE_,2.25,TXT_CENTERED,NULL);
								render_simpletext("\nGREEN=GOOD \4TEAL=EARLY \5YELLOW=LATE",0.f,0.f,GREEN_,2.25,TXT_CENTERED,NULL);
							}
						case 3:
						case 4:
							if(video_->settings_[HESTYLE].value_)
								draw_hiterror(0.0,-0.1,0);
						break;
					}
				}

				//draw_frametimer(1.0,1.0,TXT_TOPALIGNED|TXT_RGHTALIGNED);
				if(video_->settings_[SCREEN_BUFFER].value_)
					SwapBuffers(video_->hDC);
				else
					glFlush();
				
				if(video_->shouldReload_)
				{
					video_->shouldReload_ = 0;
					redraw = 2;
				}
				if(redraw > 0)
					redraw--;
			}
			
			update_frametimer();
		}
			
		if(calibrating > 0)
			return_code = calibrate_messages();
		else if(secalibrating)
			return_code = align_messages();
		else
			return_code = process_messages();
		
		update_fps();
		cpu_wait();
	}
	
	destroy_settings();
	
	update_settingsfile();
	
	return return_code;
}

void init_settings()
{
	setType_ = create_options(NULL,WHITE_,4,0,-1,-1,PURPLE_,0);
	add_option(setType_,"VIDEO",-0.5,0.75,WHITE_,0,TXT_CENTERED,NULL);
	add_option(setType_,"AUDIO",0.0,0.75,WHITE_,0,TXT_CENTERED,NULL);
	add_option(setType_,"GAMEPLAY",0.5,0.75,WHITE_,0,TXT_CENTERED,NULL);
	setType_->highlighted_ = 1;
	
	setList_ = (settings**)malloc(sizeof(settings*)*setType_->nOptions_);
	for(int i = 0; i < setType_->nOptions_; i++)
	{
		setList_[i] = (settings*)malloc(sizeof(settings));
		memset(setList_[i],0,sizeof(settings));
		
		setList_[i]->currSetting_ = -1;
		setList_[i]->options_ = (options_struct**)malloc(sizeof(options_struct*)*nOpt[i]);
		for(int j = 0; j < nOpt[i]; j++)
			setList_[i]->options_[j] = NULL;
	}
	
	//VIDEO SETTINGS
	settings_ = setList_[0];
	
	//Resolutions
	modeList_ = NULL;
	get_resolutions();
	settings_->options_[0] = create_options("RESOLUTION:",WHITE_,4,0,0,1,PURPLE_,0);

	struct monitor_mode* currMode_ = modeList_;
	char modeBuffer_[50];
	
	while(currMode_ != NULL)
	{
		sprintf(modeBuffer_,"%uX%u",currMode_->width_, currMode_->height_);
		add_option(settings_->options_[0],modeBuffer_,-1.0,0.5,WHITE_,0,0,NULL);
		if(currMode_->width_ == video_->settings_[SCREEN_W].value_ && currMode_->height_ == video_->settings_[SCREEN_H].value_)
			settings_->options_[0]->currOption_ = settings_->options_[0]->nOptions_-1;
		currMode_ = currMode_->tail_;
	}
	
	//Fullscreen Check
	settings_->options_[1] = create_options("\nFULLSCREEN:",WHITE_,4,video_->settings_[SCREEN_FULL].value_,0,1,PURPLE_,0);
	add_option(settings_->options_[1],"OFF",-1.0,0.5,WHITE_,0,0,NULL);
	add_option(settings_->options_[1],"ON",-1.0,0.5,WHITE_,0,0,NULL);
	
	//FPS Limits
	char fpsbuf[64];

	settings_->options_[2] = create_options("\n\nFPSLIMIT:",WHITE_,4,video_->settings_[SCREEN_FPS].value_,0,1,PURPLE_,0);
	sprintf(fpsbuf,"POWERSAVER(%ld)",video_->dm.dmDisplayFrequency*2);
	add_option(settings_->options_[2],fpsbuf,-1.0,0.5,WHITE_,0,0,NULL);
	sprintf(fpsbuf,"BALANCED(%ld)",video_->dm.dmDisplayFrequency*4);
	add_option(settings_->options_[2],fpsbuf,-1.0,0.5,WHITE_,0,0,NULL);
	sprintf(fpsbuf,"PERFORMANCE(%ld)",video_->dm.dmDisplayFrequency*8);
	add_option(settings_->options_[2],fpsbuf,-1.0,0.5,WHITE_,0,0,NULL);
	
	settings_->options_[3] = create_options(NULL,WHITE_,4,0,0,1,PURPLE_,0);
	add_option(settings_->options_[3],"\n\n\nDOUBLE BUFFERING",-1.0,0.5,WHITE_,0,0,NULL);
	
	//Double Buffering
	/*settings_->options_[3] = create_options("\n\n\nDOUBLEBUFFER:",WHITE_,4,video_->settings_[SCREEN_BUFFER].value_,-1,1,PURPLE_,0);
	add_option(settings_->options_[3],"OFF",-1.0,0.0,WHITE_,0,0,NULL);
	add_option(settings_->options_[3],"ON",-1.0,0.0,WHITE_,0,0,NULL);*/
	
	//Back/Apply
	settings_->options_[nOpt[0]-1] = create_options(NULL,WHITE_,4,0,0,-1,PURPLE_,0);
	add_option(settings_->options_[nOpt[0]-1],"BACK",-1.0,-1.0,WHITE_,0,TXT_BOTALIGNED,NULL);
	add_option(settings_->options_[nOpt[0]-1],"APPLY",1.0,-1.0,WHITE_,0,TXT_RGHTALIGNED|TXT_BOTALIGNED,NULL);

	//AUDIO SETTINGS
	settings_ = setList_[1];
	char audiobuf[4];
	
	sprintf(audiobuf,"%d",(int)round((audio_->volume_*100.f)));
	
	settings_->options_[0] = create_options("MASTER VOLUME:",WHITE_,4,0,0,1,PURPLE_,0);
	add_option(settings_->options_[0],audiobuf,-1.0,0.5,WHITE_,0,0,NULL);
	
	sprintf(audiobuf,"%d",(int)round((audio_->mvolume_*100.f)));
	
	settings_->options_[1] = create_options("\nMUSIC VOLUME:",WHITE_,4,0,0,1,PURPLE_,0);
	add_option(settings_->options_[1],audiobuf,-1.0,0.5,WHITE_,0,0,NULL);
	
	sprintf(audiobuf,"%d",(int)round((audio_->sevolume_*100.f)));
	
	settings_->options_[2] = create_options("\n\nEFFECT VOLUME:",WHITE_,4,0,0,1,PURPLE_,0);
	add_option(settings_->options_[2],audiobuf,-1.0,0.5,WHITE_,0,0,NULL);
	
	settings_->options_[3] = create_options("\n\n\nHITSOUNDS:",WHITE_,4,video_->settings_[HITSOUNDS].value_,0,1,PURPLE_,0);
	add_option(settings_->options_[3],"OFF",-1.0,0.5,WHITE_,0,0,NULL);
	add_option(settings_->options_[3],"ON",-1.0,0.5,WHITE_,0,0,NULL);
	
	settings_->options_[4] = create_options("\n\n\n\nOFFSET:",WHITE_,4,0,0,1,PURPLE_,0);
	change_offset(0);
	
	settings_->options_[5] = create_options(NULL,WHITE_,4,0,0,1,PURPLE_,0);
	add_option(settings_->options_[5],"\n\n\n\n\nCALIBRATE",-1.0,0.5,TEAL_,0,0,NULL);
	
	settings_->options_[6] = create_options(NULL,WHITE_,4,0,0,1,PURPLE_,0);
	add_option(settings_->options_[6],"\n\n\n\n\n\nALIGN EFFECTS",-1.0,0.5,TEAL_,0,0,NULL);
	change_seoffset(0);
	
	settings_->options_[nOpt[1]-1] = create_options(NULL,WHITE_,4,0,0,-1,PURPLE_,0);
	add_option(settings_->options_[nOpt[1]-1],"BACK",-1.0,-1.0,WHITE_,0,TXT_BOTALIGNED,NULL);
	
	//GAMEPLAY SETTINGS
	settings_ = setList_[2];
	
	settings_->options_[0] = create_options(NULL,WHITE_,4,0,-1,-1,PURPLE_,TXT_NORESET);
	settings_->options_[1] = create_options(NULL,WHITE_,4,0,-1,-1,PURPLE_,TXT_NORESET);
	change_playstyle(0);
	
	settings_->options_[2] = create_options("\n\nHIT ERROR:",WHITE_,4,video_->settings_[HESTYLE].value_,0,1,PURPLE_,0);
	add_option(settings_->options_[2],"OFF",-1.0,0.5,WHITE_,0,0,NULL);
	add_option(settings_->options_[2],"TYPE 1",-1.0,0.5,WHITE_,0,0,NULL);
	add_option(settings_->options_[2],"TYPE 2",-1.0,0.5,WHITE_,0,0,NULL);
	
	settings_->options_[3] = create_options("\n\n\nHIT ERROR SIZE:",WHITE_,4,0,0,1,PURPLE_,0);
	settings_->options_[4] = create_options("\n\n\n\nHIT ERROR INTENSITY:",WHITE_,4,0,0,1,PURPLE_,0);
	
	change_hiterror(2,0);
	change_hiterror(3,0);
	change_hiterror(4,0);
	
	settings_->options_[nOpt[2]-1] = create_options(NULL,WHITE_,4,0,0,-1,PURPLE_,0);
	add_option(settings_->options_[nOpt[2]-1],"BACK",-1.0,-1.0,WHITE_,0,TXT_BOTALIGNED,NULL);
	
	settings_ = setList_[0];
}

void reload_modes()
{
	destroy_modelist(modeList_);
	modeList_ = NULL;
	
	clear_options(setList_[0]->options_[0]);
	
	setList_[0]->options_[0]->currOption_ = 0;
	
	get_resolutions();
	
	struct monitor_mode* currMode_ = modeList_;
	char modeBuffer_[50];
	
	while(currMode_ != NULL)
	{
		sprintf(modeBuffer_,"%uX%u",currMode_->width_, currMode_->height_);
		add_option(setList_[0]->options_[0],modeBuffer_,-1.0,0.5,WHITE_,0,0,NULL);
		
		if(currMode_->width_ == video_->settings_[SCREEN_W].value_ && currMode_->height_ == video_->settings_[SCREEN_H].value_)
			setList_[0]->options_[0]->currOption_ = setList_[0]->options_[0]->nOptions_-1;
		currMode_ = currMode_->tail_;
	}
}

void get_resolutions()
{
	HMONITOR monitor_ = MonitorFromWindow(video_->hWnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO minfo_;
	memset(&minfo_,0,sizeof(MONITORINFO));
	minfo_.cbSize = sizeof(MONITORINFO);
	
	GetMonitorInfo(monitor_,&minfo_);
	
	video_->x = minfo_.rcMonitor.left;
	video_->y = minfo_.rcMonitor.top;
	//printf("%ld %ld %ld %ld", minfo_.rcMonitor.left, minfo_.rcMonitor.top, minfo_.rcMonitor.right, minfo_.rcMonitor.bottom);
	
	int iDevNum = 0;
	DISPLAY_DEVICE dd;
	memset(&dd, 0, sizeof(DISPLAY_DEVICE));
	dd.cb = sizeof(DISPLAY_DEVICE);
	
	DEVMODE dm;
	memset(&dm, 0, sizeof(DEVMODE));
	dm.dmSize = sizeof(DEVMODE);
	//printf("%ld %ld\n", minfo_.rcMonitor.left, minfo_.rcMonitor.top);
	while(EnumDisplayDevices(NULL, iDevNum, &dd, 0))
	{
		int iModeNum = 0;
		EnumDisplaySettingsEx(dd.DeviceName,ENUM_CURRENT_SETTINGS,&dm,0);
		//printf("%ld %ld %ld %ld\n", dm.dmPosition.x, dm.dmPosition.y, dm.dmPelsWidth, dm.dmPelsHeight);
		if(dm.dmPosition.x != minfo_.rcMonitor.left || dm.dmPosition.y != minfo_.rcMonitor.top)
		{
			iDevNum++;
			continue;
		}
		
		update_videosettings("MONITOR",iDevNum);
		video_->dd = dd;
		video_->dm = dm;
		while(EnumDisplaySettingsEx(dd.DeviceName,iModeNum,&dm,0))
		{
			insert_mode(dm.dmPelsWidth, dm.dmPelsHeight);
			//printf("%ld %ld %ld %ld\n", dm.dmPosition.x, dm.dmPosition.y, dm.dmPelsWidth, dm.dmPelsHeight);
			iModeNum++;
		}
		break;
	}
}

void insert_mode(unsigned int w, unsigned int h)
{
	if(modeList_ == NULL)
	{
		modeList_ = (struct monitor_mode*)malloc(sizeof(struct monitor_mode));
		modeList_->width_ = w;
		modeList_->height_ = h;
		modeList_->head_ = NULL;
		modeList_->tail_ = NULL;
		
		return;
	}
	
	struct monitor_mode* currMode_ = modeList_;
	struct monitor_mode* m_ = (struct monitor_mode*)malloc(sizeof(struct monitor_mode));
	m_->width_ = w;
	m_->height_ = h;
			
	while(currMode_ != NULL)
	{
		if(w < currMode_->width_)
		{
			m_->head_ = currMode_->head_;
			m_->tail_ = currMode_;
			if(currMode_->head_ != NULL)
				currMode_->head_->tail_ = m_;
			else
				modeList_ = m_;
			currMode_->head_ = m_;
			
			return;
		}
		else if(w == currMode_->width_)
		{
			if(h < currMode_->height_)
			{
				m_->head_ = currMode_->head_;
				m_->tail_ = currMode_;
				
				if(currMode_->head_ != NULL)
					currMode_->head_->tail_ = m_;
				else
					modeList_ = m_;
				
				currMode_->head_ = m_;
				return;
			}
			else if(h == currMode_->height_)
			{
				free(m_);
				return;
			}
		}
		if(currMode_->tail_ == NULL)
		{
			m_->head_ = currMode_;
			m_->tail_ = NULL;
			
			currMode_->tail_ = m_;
			return;
		}
		currMode_ = currMode_->tail_;
	}
}

void destroy_modelist(struct monitor_mode* mode)
{
	if(mode != NULL)
	{
		destroy_modelist(mode->tail_);
		free(mode);
	}
}

void destroy_settings()
{
	for(int i = 0; i < setType_->nOptions_; i++)
	{
		for(int j = 0; j < nOpt[i]; j++)
			delete_options(setList_[i]->options_[j]);
		free(setList_[i]->options_);
		free(setList_[i]);
	}
	free(setList_);
	
	delete_options(setType_);
	destroy_modelist(modeList_);
	
	modeList_ = NULL;
	setList_ = NULL;
	
}