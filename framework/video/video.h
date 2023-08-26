#ifndef VIDEO_H
#define VIDEO_H

#include <windows.h>
#include "../glad/glad.h"

enum COLORS {RED_,GREEN_,BLUE_,PURPLE_,YELLOW_,TEAL_,WHITE_,BLACK_,LRED_,LBLUE_,LPURPLE_,PINK_,LTEAL_,GRAY_,NCOLORS_};
enum VIDEO_SETTINGS {SCREEN_W,SCREEN_H,SCREEN_FULL,SCREEN_BUFFER,SCREEN_FPS,SCREEN_MONITOR,VOLUME,MVOLUME,SEVOLUME,OFFSET,SEOFFSET,PLAYSTYLE,KEY1,KEY2,KEY3,KEY4,HITSOUNDS,HESTYLE,HESIZE,HEALPHA,N_VIDEOSETTINGS};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Structs
typedef struct COLOR
{
	float r,g,b,a;
}COLOR;

typedef struct video_settings
{
	char*	name_;
	int		value_,update_;
}video_settings;

typedef struct video_struct
{
	HINSTANCE		hInstance;
	HWND			hWnd;
	HDC 			hDC;
	HGLRC 			hRC;
	DISPLAY_DEVICE	dd;
	DEVMODE			dm;
	video_settings	settings_[N_VIDEOSETTINGS];
	long int		x,y;
	float			aspectRatio_;
	float			frameRate_;
	int				targetFPS_;
	int				shouldReload_;
	int				lastProgram_;
} video_struct;
////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Functions
LONG WINAPI 	WndProc(HWND,UINT,WPARAM,LPARAM);
int 			create_window(int,int,int);
void 			destroy_window();
void			get_current_device();
int 			get_specific_device(int);
int			 	init_video();
void			init_videosettings();
int 			update_settingsfile();
void			update_videosettings(char*,int);
int				update_video();
int 			create_program(const char*,const char*);
void 			destroy_video();

void			start_frametimer();
int 			get_frameready();
void			draw_frametimer(float,float,int);
void 			update_frametimer();
void 			update_fps();
void			reset_fps(int);
void 			cpu_wait();

void			set_cursor(int);

int64_t 		get_globaltime();
////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Globals
extern video_struct* video_;
extern const COLOR COLORS_[];
extern WPARAM KEYS_[];
extern LPARAM KEYSL_[];
extern int KEYMAP_[];
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif //VIDEO_H