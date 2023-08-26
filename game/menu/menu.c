#include "menu.h"

static options_struct* menu_options;
static int redraw, menu_opt = -1;

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
						break;
					case VK_RETURN:
						if(menu_options->currOption_ != -1)
						{
							play_soundeffect(MENU_CONFIRM);
							result_ = menu_options->currOption_+1;
						}
						break;
					case VK_DOWN:
						increment_option(menu_options,1);
						play_soundeffect(MENU_BEEP);
						break;
					case VK_UP:
						increment_option(menu_options,-1);
						play_soundeffect(MENU_BEEP);
						break;
					case VK_F5:
						songdb_create();
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

int menu_loop()
{
	int return_code = 0;
	
	init_menu();

	video_->shouldReload_ = 2;
	redraw = 2;

	while (!return_code)
	{	
		if(get_frameready())
		{
			if(redraw || video_->shouldReload_ || server_->update_)
			{
				glClear(GL_COLOR_BUFFER_BIT);

				draw_options(menu_options);
				render_simpletext("ARROW KEYS TO CHANGE SELECTION",-1.0,-1.0,WHITE_,3.25,TXT_BOTALIGNED,NULL);
				render_simpletext("ESC TO RETURN",1.0,-1.0,WHITE_,3.25,TXT_BOTALIGNED|TXT_RGHTALIGNED,NULL);
				render_simpletext("RHYTHM SUITE",0.0,0.75,WHITE_,5,TXT_CENTERED,NULL);
				render_simpletext(BUILD_VERSION,0.0,0.65,WHITE_,2.5,TXT_CENTERED,NULL);
				
				//draw_frametimer(1.0,-1.0,TXT_BOTALIGNED|TXT_RGHTALIGNED);
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
	
	menu_opt = menu_options->currOption_;
	
	destroy_menu();
	
	return return_code;
}

void init_menu()
{
	menu_options = create_options(NULL,WHITE_,4,menu_opt,-1,-1,PURPLE_,TXT_NORESET);
	add_option(menu_options,"\r\r\rPLAY\n",0.0,0.0,WHITE_,0,TXT_CENTERED,NULL);
	add_option(menu_options,"ACCOUNT\n",0.0,0.0,WHITE_,0,TXT_CENTERED,NULL);
	add_option(menu_options,"PRACTICE\n",0.0,0.0,WHITE_,0,TXT_CENTERED,NULL);
	add_option(menu_options,"EDITOR\n",0.0,0.0,WHITE_,0,TXT_CENTERED,NULL);
	add_option(menu_options,"INFO\n",0.0,0.0,WHITE_,0,TXT_CENTERED,NULL);
	add_option(menu_options,"SETTINGS\n",0.0,0.0,WHITE_,0,TXT_CENTERED,NULL);
	add_option(menu_options,"STATS\n",0.0,0.0,WHITE_,0,TXT_CENTERED,NULL);
	add_option(menu_options,"QUIT\n",0.0,0.0,WHITE_,0,TXT_CENTERED,NULL);
	
	menu_options->highlighted_ = 1;
}

void destroy_menu()
{
	delete_options(menu_options);
}