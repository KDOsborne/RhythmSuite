#include "account.h"

static options_struct* account_options[3];
static int redraw,account_opt;
static char username[16],password[32],password_conf[32];

static void init_account();
static void update_options();
static void destroy_account();

static void clear_data()
{
	if(server_->sdata_[MESSAGE_CREATE-1].status == 1 || server_->sdata_[MESSAGE_CREATE-1].status == -1)
		clear_serverdata(MESSAGE_CREATE-1);
	if(server_->sdata_[MESSAGE_LOGIN-1].status == 1 || server_->sdata_[MESSAGE_LOGIN-1].status == -1)
		clear_serverdata(MESSAGE_LOGIN-1);
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
					if(account_opt > 0)
					{
						if(account_options[account_opt]->currOption_ == 0)
						{
							if((isalnum(msg.wParam) || msg.wParam == '_' || msg.wParam == '-' || msg.wParam == '[' || msg.wParam == ']')
								&& strlen(username)+1 < sizeof(username))
							{
								sprintf(username,"%s%c",username,toupper(msg.wParam));
								update_options();
								play_soundeffect(MENU_FASTBEEP);
							}
						}
						else if(account_options[account_opt]->currOption_ == 1)
						{
							if((isgraph(msg.wParam) && (msg.wParam != '\\' && msg.wParam != '/')) 
								&& strlen(password)+1 < sizeof(password))
							{
								sprintf(password,"%s%c",password,msg.wParam);
								update_options();
								play_soundeffect(MENU_FASTBEEP);
							}
						}
						else if(account_opt == 1 && account_options[account_opt]->currOption_ == 2)
						{
							if((isgraph(msg.wParam) && (msg.wParam != '\\' && msg.wParam != '/')) 
								&& strlen(password_conf)+1 < sizeof(password_conf))
							{
								sprintf(password_conf,"%s%c",password_conf,msg.wParam);
								update_options();
								play_soundeffect(MENU_FASTBEEP);
							}
						}
					}
				break;
				
				case WM_KEYDOWN:
				redraw = 2;
				switch (msg.wParam) {
					case VK_BACK:
						if(account_opt > 0)
						{
							if(account_options[account_opt]->currOption_ == 0 && strlen(username) > 0)
							{
								username[strlen(username)-1] = '\0';
								update_options();
							}
							else if(account_options[account_opt]->currOption_ == 1 && strlen(password) > 0)
							{
								password[strlen(password)-1] = '\0';
								update_options();
							}
							else if(account_opt == 1 && account_options[account_opt]->currOption_ == 2
									&& strlen(password_conf) > 0)
							{
								password_conf[strlen(password_conf)-1] = '\0';
								update_options();
							}
							play_soundeffect(MENU_BEEP);
						}
					break;
					case VK_ESCAPE:
						if(account_opt != 0)
						{
							memset(username,0,sizeof(username));
							memset(password,0,sizeof(password));
							memset(password_conf,0,sizeof(password_conf));
							update_options();
							
							clear_data();
							
							account_opt = 0;
						}
						else
							result_ = -1;
						
						play_soundeffect(MENU_BACK);
						break;
					case VK_RETURN:
						if(account_opt == 0 && account_options[0]->currOption_ != -1)
						{
							if(account_options[0]->currOption_ == account_options[0]->nOptions_-1)
								logout();
							else
								account_opt = account_options[0]->currOption_+1;
							play_soundeffect(MENU_CONFIRM);
						}
						else if(account_opt > 0)
						{
							if(account_opt == 1 && account_options[account_opt]->currOption_ == account_options[account_opt]->nOptions_-1)
							{
								if(strlen(username) >= 3 && strlen(password) >= 8 && strcmp(password,password_conf) == 0)
								{
									create_account(username,password);
									play_soundeffect(MENU_CONFIRM);
								}
								else
									play_soundeffect(MENU_INVALID);
							}
							if(account_opt == 2 && account_options[account_opt]->currOption_ == account_options[account_opt]->nOptions_-1)
							{
								if(strlen(username) >= 3 && strlen(password) >= 8)
								{
									login(username,password);
									play_soundeffect(MENU_CONFIRM);
								}
								else
									play_soundeffect(MENU_INVALID);
							}
						}
						break;
					case VK_DOWN:
						increment_option(account_options[account_opt],1);
						clear_data();
						
						play_soundeffect(MENU_BEEP);
						break;
					case VK_UP:
						increment_option(account_options[account_opt],-1);
						clear_data();
						
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

int account()
{
	int return_code = 0;
	
	init_account();

	video_->shouldReload_ = 2;
	redraw = 2;

	while (!return_code)
	{	
		if(get_frameready())
		{
			if(redraw || video_->shouldReload_ || server_->update_)
			{
				glClear(GL_COLOR_BUFFER_BIT);

				draw_options(account_options[account_opt]);
				if(account_opt == 1)
				{
					if(account_options[account_opt]->currOption_ == 0)
					{
						render_simpletext("USERNAMES MUST BE \4AT LEAST THREE\1 CHARACTERS",0.f,0.f,WHITE_,2.5,TXT_CENTERED,NULL);
					}
					if(account_options[account_opt]->currOption_ == 1 || account_options[account_opt]->currOption_ == 2)
					{
						render_simpletext("PASSWORDS ARE \3CASE SENSITIVE\1\nMUST BE \4AT LEAST EIGHT\1 CHARACTERS",0.f,0.f,WHITE_,2.5,TXT_CENTERED,NULL);
					}
					else if(account_options[account_opt]->currOption_ == 3)
					{
						if(strlen(username) < 3)
							render_simpletext("\r\rUSERNAME IS \5NOT LONG ENOUGH",0.f,0.f,WHITE_,2.5,TXT_CENTERED,NULL);
						if(strcmp(password,password_conf) != 0)
						{
							render_simpletext("PASSWORDS \2DO NOT MATCH",0.f,0.f,WHITE_,2.5,TXT_CENTERED,NULL);
							if(strlen(password) < 8)
							{
								render_simpletext("\nPASSWORD IS \5NOT LONG ENOUGH",0.f,0.f,WHITE_,2.5,TXT_CENTERED,NULL);
							}
						}
						else if(strlen(password) < 8)
						{
							render_simpletext("PASSWORD IS \5NOT LONG ENOUGH",0.f,0.f,WHITE_,2.5,TXT_CENTERED,NULL);
						}
						else
						{
							switch(server_->sdata_[MESSAGE_CREATE-1].status)
							{
								case 10:
									render_simpletext("CONNECTING TO SERVER...",0.f,0.f,WHITE_,2.5,TXT_CENTERED,NULL);
								break;
								case 1:
									if(server_->sdata_[MESSAGE_CREATE-1].result[0] == 1)
										render_simpletext("ACCOUNT SUCCESSFULLY CREATED!",0.f,0.f,WHITE_,2.5,TXT_CENTERED,NULL);
									else if(server_->sdata_[MESSAGE_CREATE-1].result[0] == -1)
										render_simpletext("USERNAME UNAVAILABLE...",0.f,0.f,WHITE_,2.5,TXT_CENTERED,NULL);
									else
										render_simpletext("SERVER ERROR...",0.f,0.f,WHITE_,2.5,TXT_CENTERED,NULL);
								break;
								case -1:
									render_simpletext("COULDNT CONNECT TO SERVER...",0.f,0.f,WHITE_,2.5,TXT_CENTERED,NULL);
								break;
							}
						}
					}
				}
				else if(account_opt == 2)
				{
					if(account_options[account_opt]->currOption_ == 2)
					{
						if(strlen(username) < 3)
							render_simpletext("\rUSERNAME IS \5NOT LONG ENOUGH",0.f,0.f,WHITE_,2.5,TXT_CENTERED,NULL);
						if(strlen(password) < 8)
							render_simpletext("PASSWORD IS \5NOT LONG ENOUGH",0.f,0.f,WHITE_,2.5,TXT_CENTERED,NULL);
						
						switch(server_->sdata_[MESSAGE_LOGIN-1].status)
						{
							case 10:
								render_simpletext("CONNECTING TO SERVER...",0.f,0.f,WHITE_,2.5,TXT_CENTERED,NULL);
							break;
							case 1:
								if(server_->sdata_[MESSAGE_LOGIN-1].result[0] == 1)
									render_simpletext("LOGIN SUCCESSFUL!",0.f,0.f,WHITE_,2.5,TXT_CENTERED,NULL);
								else if(server_->sdata_[MESSAGE_LOGIN-1].result[0] == -1)
									render_simpletext("INCORRECT USERNAME OR PASSWORD...",0.f,0.f,WHITE_,2.5,TXT_CENTERED,NULL);
								else if(server_->sdata_[MESSAGE_LOGIN-1].result[0] == -2)
									render_simpletext("ACCOUNT SUSPENDED...",0.f,0.f,WHITE_,2.5,TXT_CENTERED,NULL);
							break;
							case -1:
								render_simpletext("COULDNT CONNECT TO SERVER...",0.f,0.f,WHITE_,2.5,TXT_CENTERED,NULL);
							break;
						}
					}
				}
				
				render_serverinfo();
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
		
		process_server();
		update_fps();
		cpu_wait();
	}
	
	video_->shouldReload_ = 0;
	
	destroy_account();
	
	return return_code;
}

static void init_account()
{
	float x = -1.f, y = 0.925;
	account_opt = 0;
	
	memset(username,0,sizeof(username));
	memset(password,0,sizeof(password));
	memset(password_conf,0,sizeof(password_conf));
	
	account_options[0] = create_options("SELECT AN OPTION\n",LPURPLE_,3,0,-1,-1,PURPLE_,TXT_TOPALIGNED|TXT_NORESET);
	add_option(account_options[0],"CREATE ACCOUNT\n",x,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	add_option(account_options[0],"LOGIN\n",x,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	add_option(account_options[0],"LOGOUT\n",x,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	account_options[0]->highlighted_ = 1;
	
	account_options[1] = create_options("CREATE ACCOUNT\n",LPURPLE_,3,0,-1,-1,PURPLE_,TXT_TOPALIGNED|TXT_NORESET);
	add_option(account_options[1],"USERNAME: \n",x,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	add_option(account_options[1],"PASSWORD: \n",x,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	add_option(account_options[1],"CONFIRM PASSWORD: \n",x,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	add_option(account_options[1],"SUBMIT",x,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	account_options[1]->highlighted_ = 1;
	
	account_options[2] = create_options("LOGIN\n",LPURPLE_,3,0,-1,-1,PURPLE_,TXT_TOPALIGNED|TXT_NORESET);
	add_option(account_options[2],"USERNAME: \n",x,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	add_option(account_options[2],"PASSWORD: \n",x,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	add_option(account_options[2],"SUBMIT",x,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	account_options[2]->highlighted_ = 1;
}

static void update_options()
{
	char buffer[128],pbuf[32];
	float y = 0.925;
	
	clear_options(account_options[account_opt]);
	
	sprintf(buffer,"USERNAME: %s\n",username);
	add_option(account_options[account_opt],buffer,-1.0,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	
	memset(pbuf,0,sizeof(pbuf));
	for(int i = 0; i < strlen(password); i++)
		pbuf[i] = '~';
	
	sprintf(buffer,"PASSWORD: %s\n",pbuf);
	add_option(account_options[account_opt],buffer,-1.0,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	
	if(account_opt == 1)
	{
		memset(pbuf,0,sizeof(pbuf));
		for(int i = 0; i < strlen(password_conf); i++)
			pbuf[i] = '~';
		
		sprintf(buffer,"CONFIRM PASSWORD: %s\n",pbuf);
		add_option(account_options[account_opt],buffer,-1.0,y,WHITE_,0,TXT_TOPALIGNED,NULL);
	}
	
	add_option(account_options[account_opt],"SUBMIT",-1.0,y,WHITE_,0,TXT_TOPALIGNED,NULL);
}

static void destroy_account()
{
	for(int i = 0; i < 3; i++)
		delete_options(account_options[i]);
}
