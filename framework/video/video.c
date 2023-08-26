#include "video.h"
#include "wglext.h"
#include "../error/error.h"
#include "../audio/audio.h"
#include "../text/text.h"
#include "../image/image.h"
#include "../shapes/shapes.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

video_struct* video_;

const COLOR COLORS_[] = {
	{1.0, 0.0, 0.0, 1.0},{0.0, 1.0, 0.0, 1.0},{0.0, 0.0, 1.0, 1.0},
	{1.0, 0.0, 1.0, 1.0},{1.0, 1.0, 0.0, 1.0},{0.0, 1.0, 1.0, 1.0},
	{1.0, 1.0, 1.0, 1.0},{0.0, 0.0, 0.0, 1.0},{1.0, 0.2, 0.2, 1.0},
	{0.2, 0.2, 1.0, 1.0},{1.0, 0.5, 1.0, 1.0},{1.0, 0.5, 1.0, 1.0},
	{0.5, 1.0, 1.0, 1.0},{0.5, 0.5, 0.5, 1.0}
};

WPARAM 	KEYS_[] = { -1, -1, -1, -1 };
LPARAM 	KEYSL_[] = { -1, -1, -1, -1 };
int 	KEYMAP_[] = { -1, -1, -1, -1 };

static HANDLE hTimer;
static LARGE_INTEGER liDueTime,GlobalStart,StartingTime,EndingTime,ElapsedMilliseconds,Frequency;
static long long fps_array[1000];
static int fps_current,fps_end;
static uint32_t FPS,LASTFPS,nFPS;
static char FPSstring[5];
static double avgFPS;

static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = NULL;
static int WGLExtensionSupported(const char *);

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Video Utilities
LONG WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{ 
    switch(uMsg) {
    case WM_PAINT:
		PAINTSTRUCT ps;
		BeginPaint(video_->hWnd, &ps);
		
		EndPaint(video_->hWnd, &ps);
		update_frametimer();
	return 0;
	
	case WM_KEYDOWN:
	switch (wParam) {
		case VK_F3:
			if(video_->settings_[SCREEN_FPS].value_ == 0)
				break;
			
			video_->settings_[SCREEN_FPS].value_--;
			video_->settings_[SCREEN_FPS].update_ = video_->settings_[SCREEN_FPS].value_;
			reset_fps(0);
			break;
		case VK_F4:
			if(video_->settings_[SCREEN_FPS].value_ == 2)
				break;
			
			video_->settings_[SCREEN_FPS].value_++;
			video_->settings_[SCREEN_FPS].update_ = video_->settings_[SCREEN_FPS].value_;
			reset_fps(0);
			break;
		case VK_F12:
			play_soundeffect(SCREENSHOT);
			int w = video_->settings_[SCREEN_W].value_;
			int h = video_->settings_[SCREEN_H].value_;
			void* data = malloc(sizeof(unsigned char)*3*w*h);

			glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,data);
			write_image(w,h,data);
			free(data);
			
			break;
		}
	return 0;
	
    case WM_CLOSE:
		PostQuitMessage(0);
	return 0;
	
	case WM_KILLFOCUS:
		reset_fps(-1);
		if(video_->settings_[SCREEN_FULL].value_)
			ShowWindow(hWnd, SW_MINIMIZE);
	return 0;
	
	case WM_SETFOCUS:
		reset_fps(0);
		if(video_->settings_[SCREEN_FULL].value_)
		{
			video_->shouldReload_ = 1;
			ShowWindow(hWnd, SW_NORMAL);
		}
	return 0;
	
	case WM_SETCURSOR:
	return TRUE;
	
	case WM_MOVE:
		video_->shouldReload_ = 1;
	return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam); 
}

int create_window(int exFlags, int wsFlags, int pfdflags)
{
	WNDCLASS	wc;

    /* only register the window class once - use hInstance as a flag. */
	if(!video_->hInstance)
	{
		video_->hInstance = GetModuleHandle(NULL);
		memset(&wc, 0, sizeof(WNDCLASS));
		wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wc.lpfnWndProc   = (WNDPROC)WndProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = video_->hInstance;
		wc.hIcon         = LoadIcon(NULL, IDI_WINLOGO);
		wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = NULL;
		wc.lpszMenuName  = NULL;
		wc.lpszClassName = "OpenGL";

		if (!RegisterClass(&wc)) {
			MessageBox(NULL, "RegisterClass() failed:  "
				   "Cannot register window class.", "Error", MB_OK);
			return -1;
		}
	}
	
	HWND hWnd;
	memset(&hWnd, 0, sizeof(HWND));
    hWnd = CreateWindowEx(WS_EX_ACCEPTFILES|exFlags, "OpenGL", "Rhythm Suite", WS_CLIPSIBLINGS|WS_CLIPCHILDREN|wsFlags,
			video_->x, video_->y, video_->settings_[SCREEN_W].value_, video_->settings_[SCREEN_H].value_, NULL, NULL, video_->hInstance, NULL);
	
    if (hWnd == NULL) {
	MessageBox(NULL, "CreateWindow() failed:  Cannot create a window.",
		   "Error", MB_OK);
	return -1;
    }
	
	get_current_device();
	
	HDC hDC;
	memset(&hDC, 0, sizeof(HDC));
    hDC = GetDC(hWnd);

    /* there is no guarantee that the contents of the stack that become
       the pfd are zeroed, therefore _make sure_ to clear these bits. */
    PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | pfdflags,  //Flags
		PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
		32,                   // Colordepth of the framebuffer.
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24,                   // Number of bits for the depthbuffer
		8,                    // Number of bits for the stencilbuffer
		0,                    // Number of Aux buffers in the framebuffer.
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

    int pf = ChoosePixelFormat(hDC, &pfd);
    if (pf == 0) {
	MessageBox(NULL, "ChoosePixelFormat() failed:  "
		   "Cannot find a suitable pixel format.", "Error", MB_OK); 
	return -1;
    } 
 
    if (SetPixelFormat(hDC, pf, &pfd) == FALSE) {
	MessageBox(NULL, "SetPixelFormat() failed:  "
		   "Cannot set format specified.", "Error", MB_OK);
	return -1;
    } 

    //SetWindowLongPtr(hWnd, GWL_STYLE, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP);
	
	HGLRC hRC;
	
    hDC = GetDC(hWnd);
    
	hRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC, hRC);
	
	if (WGLExtensionSupported("WGL_EXT_swap_control"))
	{
		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC) wglGetProcAddress("wglSwapIntervalEXT");
		wglSwapIntervalEXT(0);
	}
	
	video_->hWnd = hWnd;
	video_->hDC = hDC;
	video_->hRC = hRC;
	
	return 0;
}
void destroy_window()
{
	wglMakeCurrent(NULL, NULL);
    ReleaseDC(video_->hWnd, video_->hDC);
    wglDeleteContext(video_->hRC);
    DestroyWindow(video_->hWnd);
}

void get_current_device()
{
	HMONITOR monitor_ = MonitorFromWindow(video_->hWnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO minfo_;
	memset(&minfo_,0,sizeof(MONITORINFO));
	minfo_.cbSize = sizeof(MONITORINFO);
	
	GetMonitorInfo(monitor_,&minfo_);
	
	video_->x = minfo_.rcMonitor.left;
	video_->y = minfo_.rcMonitor.top;
	
	int iDevNum = 0;
	DISPLAY_DEVICE dd;
	memset(&dd, 0, sizeof(DISPLAY_DEVICE));
	dd.cb = sizeof(DISPLAY_DEVICE);
	
	DEVMODE dm;
	memset(&dm, 0, sizeof(DEVMODE));
	dm.dmSize = sizeof(DEVMODE);

	while(EnumDisplayDevices(NULL, iDevNum, &dd, 0))
	{
		EnumDisplaySettingsEx(dd.DeviceName,ENUM_CURRENT_SETTINGS,&dm,0);

		if(dm.dmPosition.x == minfo_.rcMonitor.left && dm.dmPosition.y == minfo_.rcMonitor.top)
		{
			video_->dd = dd;
			video_->dm = dm;
			reset_fps(0);
			break;
		}
		iDevNum++;
	}
}

int get_specific_device(int dNum)
{
	int iDevNum = dNum;
	DISPLAY_DEVICE dd;
	memset(&dd, 0, sizeof(DISPLAY_DEVICE));
	dd.cb = sizeof(DISPLAY_DEVICE);
	
	DEVMODE dm;
	memset(&dm, 0, sizeof(DEVMODE));
	dm.dmSize = sizeof(DEVMODE);

	if(!EnumDisplayDevices(NULL, iDevNum, &dd, 0))
		return -1;
	
	if(!EnumDisplaySettingsEx(dd.DeviceName,ENUM_CURRENT_SETTINGS,&dm,0))
		return -1;
	
	video_->dd = dd;
	video_->dm = dm;
	reset_fps(0);
	
	int iModeNum = 0;
	while(EnumDisplaySettingsEx(dd.DeviceName,iModeNum,&dm,0))
	{
		if(dm.dmPelsWidth == video_->settings_[SCREEN_W].update_ &&
			dm.dmPelsHeight == video_->settings_[SCREEN_H].update_)
		{
			return 0;
		}
		iModeNum++;
	}
	
	return -1;
}

int init_video()
{
	video_ = (video_struct*)malloc(sizeof(video_struct));
	memset(video_, 0, sizeof(video_struct));
	video_->hInstance = 0;
	video_->x =	0;
	video_->y =	0;
	
	init_videosettings();
	
	if(create_window(0,(WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX),0))
	{
		free(video_);
		return -1;
	}
	
	if (!gladLoadGL())
    {
		write_error("ERROR::Failed to initialize GLAD",NULL,0);

		destroy_video();
        return -1;
    }
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	
	if(get_specific_device(video_->settings_[SCREEN_MONITOR].update_) == -1)
	{
		video_->settings_[SCREEN_W].update_ = 640;
		video_->settings_[SCREEN_H].update_ = 480;
		video_->settings_[SCREEN_FULL].update_ = 0;
		video_->settings_[SCREEN_BUFFER].update_ = 0;
		video_->settings_[SCREEN_MONITOR].update_ = 0;
		
		get_current_device();
	}
	
	//Set up timer and get clock resolution for fps.
	hTimer = CreateWaitableTimer(NULL, TRUE, NULL);
	QueryPerformanceFrequency(&Frequency);
	
	memset(&GlobalStart,0,sizeof(LARGE_INTEGER));
	QueryPerformanceCounter(&GlobalStart);
	
	return update_video();
}

void init_videosettings()
{
	video_settings vs_[] = { {"WIDTH",640,640}, {"HEIGHT",480,480},
		{"FULLSCREEN",0,0},{"DOUBLEBUFFER",0,0},{"FPSLIMIT",0,0},
		{"MONITOR",0,0},{"VOLUME",50,50},{"MVOLUME",100,100},{"SEVOLUME",100,100},
		{"OFFSET",0,0},{"SEOFFSET",0,0},{"PLAYSTYLE",0,0},{"KEY!",32,32},{"KEY@",33,33},
		{"KEY#",36,36},{"KEY$",37,37},{"HITSOUNDS",0,0},{"HESTYLE",1,1},
		{"HESIZE",1,1},{"HEALPHA",50,50}
	};
	
	memcpy(video_->settings_, vs_, sizeof(video_settings)*N_VIDEOSETTINGS);
	video_->aspectRatio_ = ((float)video_->settings_[SCREEN_W].value_/(float)video_->settings_[SCREEN_H].value_);
	
	FILE* fp = fopen("settings.ini","r");
	if(!fp)
	{
		update_settingsfile();
	}
	else
	{
		char buffer[256], setting[64];
		int value;
		while(fgets(buffer,sizeof(buffer),fp))
		{
			sscanf(buffer,"%s %d",setting,&value);
			update_videosettings(setting,value);
		}
	}
	
	fclose(fp);
	
	KEYS_[0] = MapVirtualKeyEx(video_->settings_[KEY1].update_,MAPVK_VSC_TO_VK_EX,NULL);
	KEYS_[1] = MapVirtualKeyEx(video_->settings_[KEY2].update_,MAPVK_VSC_TO_VK_EX,NULL);
	KEYS_[2] = MapVirtualKeyEx(video_->settings_[KEY3].update_,MAPVK_VSC_TO_VK_EX,NULL);
	KEYS_[3] = MapVirtualKeyEx(video_->settings_[KEY4].update_,MAPVK_VSC_TO_VK_EX,NULL);
	
	KEYSL_[0] = video_->settings_[KEY1].update_;
	KEYSL_[1] = video_->settings_[KEY2].update_;
	KEYSL_[2] = video_->settings_[KEY3].update_;
	KEYSL_[3] = video_->settings_[KEY4].update_;
	
	switch (video_->settings_[PLAYSTYLE].update_)
	{
		case 1:
			KEYMAP_[0] = 0;
			KEYMAP_[1] = 1;
			KEYMAP_[2] = 1;
			KEYMAP_[3] = 0;
			break;
		case 2:
			KEYMAP_[0] = 0;
			KEYMAP_[1] = 0;
			KEYMAP_[2] = 1;
			KEYMAP_[3] = 1;
			break;
		case 3:
			KEYMAP_[0] = 1;
			KEYMAP_[1] = 1;
			KEYMAP_[2] = 0;
			KEYMAP_[3] = 0;
			break;
		case 4:
			KEYMAP_[0] = 0;
			KEYMAP_[1] = 1;
			KEYMAP_[2] = 0;
			KEYMAP_[3] = 1;
			break;
		case 5:
			KEYMAP_[0] = 1;
			KEYMAP_[1] = 0;
			KEYMAP_[2] = 1;
			KEYMAP_[3] = 0;
			break;
		default:
			KEYMAP_[0] = 1;
			KEYMAP_[1] = 0;
			KEYMAP_[2] = 0;
			KEYMAP_[3] = 1;
			break;
	}	
}

int update_settingsfile()
{
	FILE* fp = fopen("settings.ini","w");
	
	if(!fp)
	{
		write_error(strerror(errno),NULL,0);
		return -1;
	}
	
	for(int i = 0; i < N_VIDEOSETTINGS; i++)
		fprintf(fp,"%s %d\n",video_->settings_[i].name_,video_->settings_[i].value_);
	
	fclose(fp);
	
	return 0;
}

void update_videosettings(char* setting, int value)
{
	if(setting == NULL)
		return;
	for(int i = 0; i < N_VIDEOSETTINGS; i++)
	{
		if(strstr(video_->settings_[i].name_,setting))
			video_->settings_[i].update_ = value;
	}
}

int update_video()
{
	int wlStyle = 0, exStyle = 0, pfdflags = 0, swpFlags = 0, cds_ = 0, changed_ = 0, remake_ = 0;
	HWND hWndOrder = NULL;
	
	for(int i = 0; i < N_VIDEOSETTINGS; i++)
	{
		if(video_->settings_[i].value_ != video_->settings_[i].update_)
		{
			video_->settings_[i].value_ = video_->settings_[i].update_;
			changed_ = 1;
			switch(i)
			{
				case SCREEN_FULL:
				case SCREEN_BUFFER:
					remake_ = 1;
			}
		}
	}
	
	video_->aspectRatio_ = ((float)video_->settings_[SCREEN_W].value_/(float)video_->settings_[SCREEN_H].value_);
	
	if(video_->settings_[SCREEN_FULL].value_)
	{
		exStyle |= (WS_EX_APPWINDOW | WS_EX_TOPMOST);
		wlStyle |= WS_POPUP;
		
		cds_ = 1;
	}
	else
	{
		//If borderless fullscreen
		if(video_->settings_[SCREEN_W].value_ == video_->dm.dmPelsWidth
			&& video_->settings_[SCREEN_H].value_ == video_->dm.dmPelsHeight)
		{
			exStyle |= (WS_EX_APPWINDOW | WS_EX_TOPMOST);
			wlStyle |= WS_POPUP;
			hWndOrder = HWND_TOPMOST;
		}
		else
		{
			wlStyle |= (WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX);
			//swpFlags |= SWP_NOMOVE;
			hWndOrder = HWND_NOTOPMOST;
		}
	}
	
	if(video_->settings_[SCREEN_BUFFER].value_)
			pfdflags = PFD_DOUBLEBUFFER;
	
	int centerX = (video_->dm.dmPelsWidth/2)+(video_->dm.dmPosition.x)-(video_->settings_[SCREEN_W].value_/2);
	int centerY = (video_->dm.dmPelsHeight/2)+(video_->dm.dmPosition.y)-(video_->settings_[SCREEN_H].value_/2);
			
	video_->x = centerX;
	video_->y = centerY;
	
	if(remake_)
	{
		destroy_window();
		if(create_window(exStyle,wlStyle,pfdflags))
		{
			write_error("ERROR::Failed to remake window\n",NULL,0);
			return -1;
		}
		if(cds_)
		{
			video_->dm.dmPelsWidth = video_->settings_[SCREEN_W].value_;
			video_->dm.dmPelsHeight = video_->settings_[SCREEN_H].value_;
			
			ChangeDisplaySettingsEx(video_->dd.DeviceName,&video_->dm,NULL,CDS_FULLSCREEN,NULL);
		}
	}
	else
	{
		SetWindowLongPtr(video_->hWnd,GWL_EXSTYLE,exStyle);
		SetWindowLongPtr(video_->hWnd,GWL_STYLE,wlStyle);
		
		if(cds_)
		{
			SetWindowPos(video_->hWnd,hWndOrder,video_->dm.dmPosition.x,video_->dm.dmPosition.y,
				video_->settings_[SCREEN_W].value_,video_->settings_[SCREEN_H].value_,swpFlags);
			
			video_->dm.dmPelsWidth = video_->settings_[SCREEN_W].value_;
			video_->dm.dmPelsHeight = video_->settings_[SCREEN_H].value_;
			
			ChangeDisplaySettingsEx(video_->dd.DeviceName,&video_->dm,NULL,CDS_FULLSCREEN,NULL);
		}
		else
		{
			SetWindowPos(video_->hWnd,hWndOrder,centerX,centerY,
				video_->settings_[SCREEN_W].value_,video_->settings_[SCREEN_H].value_,swpFlags);
		}
	}
		
	ShowWindow(video_->hWnd, SW_SHOW);
	
	RECT lPrect;
	GetClientRect(video_->hWnd,&lPrect);
	glViewport(lPrect.left,lPrect.top,lPrect.right,lPrect.bottom);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
	
	video_->lastProgram_ = -1;
	
	reset_fps(0);
	
	update_settingsfile();
	
	destroy_shapes();
	destroy_image();
	destroy_text();
	
	init_text();
	init_image();
	init_shapes();
	
	return 0;
}

int create_program(const char *vert, const char *frag)
{
	int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	
    glShaderSource(vertexShader, 1, &vert, NULL);
    glCompileShader(vertexShader);
	
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		write_error("ERROR::SHADER::COMPILATION_FAILED\n",infoLog,0);
		return -1;
    }

    int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &frag, NULL);
    glCompileShader(fragmentShader);
	
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        write_error("ERROR::SHADER::COMPILATION_FAILED\n",infoLog,0);
		glDeleteShader(vertexShader);
		return -1;
    }
	
    // link shaders
    int vertexProgram = glCreateProgram();
	
    glAttachShader(vertexProgram, vertexShader);
    glAttachShader(vertexProgram, fragmentShader);
    glLinkProgram(vertexProgram);
	
    // check for linking errors
    glGetProgramiv(vertexProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(vertexProgram, 512, NULL, infoLog);
		write_error("ERROR::SHADER::PROGRAM::LINKING_FAILED\n",infoLog,0);
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		glDeleteProgram(vertexProgram);
		return -1;
    }
	
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
	
	return vertexProgram;
}

void destroy_video()
{
    destroy_window();
	free(video_);
	
	if(hTimer != NULL)
		CloseHandle(hTimer);
}

void start_frametimer()
{
	FPS = 0;
	LASTFPS = 0;
	nFPS = 0;
	avgFPS = 0;
	fps_current = 0;
	fps_end = 0;
	
	memset(fps_array,255,sizeof(fps_array));
	memset(FPSstring,0,sizeof(FPSstring));
	
	memset(&liDueTime,0,sizeof(LARGE_INTEGER));
	memset(&StartingTime,0,sizeof(LARGE_INTEGER));
	memset(&EndingTime,0,sizeof(LARGE_INTEGER));
	memset(&ElapsedMilliseconds,0,sizeof(LARGE_INTEGER));
	
    liDueTime.QuadPart = -1000LL;
	
	QueryPerformanceCounter(&StartingTime);
}

int get_frameready()
{
	QueryPerformanceCounter(&EndingTime);
	ElapsedMilliseconds.QuadPart = (EndingTime.QuadPart - StartingTime.QuadPart)*1000000/Frequency.QuadPart;
		
	return 1;	
}

void draw_frametimer(float x, float y, int flags)
{
	render_simpletext(FPSstring,x,y,WHITE_,3.25,flags,NULL);
}

void update_frametimer()
{
	//QueryPerformanceCounter(&StartingTime);
	StartingTime = EndingTime;
	
	if((fps_end + 1) % 1000 != fps_current)
	{
		fps_array[fps_end] = EndingTime.QuadPart*1000000/Frequency.QuadPart+1000000;
		fps_end = (fps_end + 1) % 1000;
		FPS++;
	}
	
	if(FPS < LASTFPS)
	{
		if(LASTFPS - FPS > 10)
		{
			//printf("DROPPED FRAME %d %lld\n",LASTFPS-FPS,StartingTime.QuadPart/Frequency.QuadPart-GlobalStart.QuadPart/Frequency.QuadPart);
			LASTFPS = FPS;
			return;
		}
	}
	LASTFPS = FPS;
	
	if(nFPS > video_->targetFPS_)
	{
		nFPS = 0;
		avgFPS = 0;
	}
	else
	{	
		avgFPS += FPS;
		nFPS++;
		sprintf(FPSstring,"%4d",(int)avgFPS/nFPS-1);
	}
	
	if(ElapsedMilliseconds.QuadPart < (video_->frameRate_*1000))
	{
		liDueTime.QuadPart -= (video_->frameRate_*1000)-ElapsedMilliseconds.QuadPart;
	}
	else if(ElapsedMilliseconds.QuadPart > (video_->frameRate_*1000))
	{
		liDueTime.QuadPart += ElapsedMilliseconds.QuadPart-(video_->frameRate_*1000);
		if(liDueTime.QuadPart >= 0)
			liDueTime.QuadPart = -100;
	}
}

void update_fps()
{
	while(EndingTime.QuadPart*1000000/Frequency.QuadPart >= fps_array[fps_current] && FPS > 0)
	{
		FPS--;
		fps_current = (fps_current + 1) % 1000;
	}
}

void reset_fps(int value)
{
	if(value == -1)
		video_->targetFPS_ = 30;
	else
	{
		if(video_->settings_[SCREEN_FPS].value_ == 0)
			video_->targetFPS_ = video_->dm.dmDisplayFrequency*2;
		else if(video_->settings_[SCREEN_FPS].value_ == 1)
			video_->targetFPS_ = video_->dm.dmDisplayFrequency*4;
		else if(video_->settings_[SCREEN_FPS].value_ == 2)
			video_->targetFPS_ = video_->dm.dmDisplayFrequency*8;
	}
	
	video_->frameRate_ = 1.f / (video_->targetFPS_) * 1000.f;
}

void cpu_wait()
{
	SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0);
	WaitForSingleObject(hTimer, INFINITE);
}

void set_cursor(int flag)
{
	switch(flag)
	{
		case 0:
			SetCursor(NULL);
			break;
		case 1:
			SetCursor(LoadCursor(NULL,IDC_ARROW));
			break;
		case 2:
			SetCursor(LoadCursor(NULL,IDC_HAND));
			break;
	}
}

int64_t get_globaltime()
{
	LARGE_INTEGER currTime;
	QueryPerformanceCounter(&currTime);
	
	return (currTime.QuadPart-GlobalStart.QuadPart)*1000/Frequency.QuadPart;
}

static int WGLExtensionSupported(const char *extension_name)
{
    PFNWGLGETEXTENSIONSSTRINGEXTPROC _wglGetExtensionsStringEXT = NULL;

    _wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC) wglGetProcAddress("wglGetExtensionsStringEXT");

    if (strstr(_wglGetExtensionsStringEXT(), extension_name) == NULL)
        return 0;
    return 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////