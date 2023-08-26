#include "statistics.h"

static options_struct* top_scores;

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
				switch (msg.wParam) {
					case VK_ESCAPE:		
						play_soundeffect(MENU_BACK);
						result_ = -1;
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

static void init_playerstats()
{
	score_struct* scores = database_topscores(16);
	top_scores = create_options("TOP SCORES\n",LPURPLE_,3.5,-1,-1,-1,PURPLE_,TXT_TOPALIGNED|TXT_NORESET);
	
	if(scores[0].id == 0)
	{
		add_option(top_scores,"N/A",-1.0,0.5,WHITE_,0,TXT_TOPALIGNED,NULL);
		return;
	}
	
	char textbuf[1024];
	memset(textbuf,0,sizeof(textbuf));
	
	for(int i = 0; i < 16; i++)
	{
		if(scores[i].id == 0)
			break;
		
		char* name = songdb_getname(scores[i].id);
		if(name == NULL)
			continue;
		
		int diff = (int)songdb_getdiff(scores[i].id);
		
		for(int j = 0; j < strlen(name); j++)
			name[j] = toupper(name[j]);
		
		sprintf(textbuf,"%dRP %s[LV.%d]<%.2f>\n",scores[i].points,name,diff,scores[i].rate);
		add_option(top_scores,textbuf,-1.0,0.5,WHITE_,0,TXT_TOPALIGNED,NULL);
		free(name);
	}

	free(scores);
}

int playerstats()
{
	int return_code = 0;
	int64_t t = 0, lt = 0L, st = dbget_stat(0);
	char timetext[32], notetext[32], ranktext[64];
	
	memset(timetext,0,sizeof(timetext));
	memset(notetext,0,sizeof(notetext));
	memset(ranktext,0,sizeof(ranktext));
	
	sprintf(notetext,"NOTES SMASHED: %lld",dbget_stat(1));
	sprintf(ranktext,"PLAYER RATING: %d",db_playerrank());
	
	init_playerstats();
		
	while (!return_code)
	{	
		t = get_globaltime();
		if(get_frameready())
		{
			if(t - lt > 1000)
			{
				glClear(GL_COLOR_BUFFER_BIT);
				
				sprintf(timetext,"TIME PLAYED: %03d:%02d:%02d",(int)((t+st)/3600000),(int)(((t+st)%3600000)/60000),(int)(((t+st)%60000)/1000));
				render_simpletext(timetext,-1.0,0.925,WHITE_,3.5,TXT_TOPALIGNED,NULL);
				render_simpletext(notetext,-1.0,0.925-3.5*0.02,WHITE_,3.5,TXT_TOPALIGNED,NULL);
				render_simpletext(ranktext,-1.0,0.925-7*0.02,WHITE_,3.5,TXT_TOPALIGNED,NULL);
				
				draw_options(top_scores);
				
				if(video_->settings_[SCREEN_BUFFER].value_)
					SwapBuffers(video_->hDC);
				else
					glFlush();
				
				lt = t;
			}
			update_frametimer();
		}
		
		return_code = process_messages();
		
		update_fps();
		cpu_wait();
	}
	
	delete_options(top_scores);

	return return_code;
}

