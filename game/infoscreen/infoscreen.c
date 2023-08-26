#include "infoscreen.h"

static char* helptext = "\3GENERAL HOTKEYS:\1\n"
	"F3/F4 - INCREASE/DECREASE FPS LIMIT\n"
	"F5(MAIN MENU) - RELOAD TRACK DATABASE\n"
	"F11(MAIN MENU+SETTINGS) - TOGGLE DOUBLE BUFFERING\n"
	"F12 - SCREENSHOT\n\n"
	"\3GAME HOTKEYS:\1\n"
	"TILDE(X2) - QUICK RETRY\n\n"
	"\3EDITOR HOTKEYS:\1\n"
	"TILDE - VIEW HOTKEYS\n\n"
	"\3GAME INFO:\1\n"
	"RP(RATING POINTS) DETERMINES YOUR RANK\n"
	"RP IS ENTIRELY ACCURACY BASED\n"
	"GRADES: SSS+ > SSS > SS > S > AA > A > B > C > F\n"
	"MINIMUM B NEEDED TO PASS + GAIN RP\n\n"
	"\3TIPS/MISC:\1\n"
	"MAKE SURE TO \4CALIBRATE OFFSET\1 IN THE AUDIO SETTINGS\n"
	"IF YOURE STRUGGLING TO PASS TRACKS TRY OUT THE \5PRACTICE\1 MODE\n"
	"IF A TRACK IS TOO HARD TRY DECREASING THE SPEED\n"
	"FOR GAME CAPTURE TOGGLE DOUBLEBUFFERING ON\n"
	"\2DO NOT FORCE VSYNC\1\n"
	"IF CPU/GPU ON FIRE LOWER FPS LIMIT\n"
	"IF WRISTS ON FIRE TAKE A BREAK\n"
	"DM \4RSTDEV#8392\1 WITH ANY BUGS/INQUIRIES\n"
	"FOR DMCA REQUESTS EMAIL \6I\6.\6C\6.\6W\6E\6E\6N\6E\6R\1 AT GMAIL.COM\n"
	"HAVE FUN!\0";

static int redraw;

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

int infoscreen()
{
	int return_code = 0;
	
	redraw = 2;

	
	while (!return_code)
	{	
		if(get_frameready())
		{
			if(redraw || video_->shouldReload_)
			{
				glClear(GL_COLOR_BUFFER_BIT);

				render_simpletext(helptext,-1.0,0.99,WHITE_,3.25,TXT_TOPALIGNED,NULL);
				
				glBlendFunc(GL_SRC_ALPHA, GL_SRC_ALPHA);
				draw_rectangle(0.f,0.f,1.f,1.f,BLACK_,0.925,0);
				glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
						
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
		
		update_fps();
		cpu_wait();
	}
	
	video_->shouldReload_ = 0;
	
	return return_code;
}