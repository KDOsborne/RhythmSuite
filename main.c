#include "framework.h"
#include "tools.h"
#include "game.h"

#include <stdio.h>
#include <time.h>

static void delete_temp();
static void update_exe();

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPreviousInst, LPSTR lpszCmdLine, int nCmdShow) {
	CoInitialize(NULL);
	
	ImmDisableIME(0);
	
	delete_temp();
	
	if(init_video() == -1) {
		return -1;
	}
	
	if(init_text() == -1) {
		destroy_video();
		return -1;
	}
	
	if(init_image() == -1) {
		destroy_text();
		destroy_video();
		return -1;
	}

	if(init_shapes() == -1) {
		destroy_image();
		destroy_text();
		destroy_video();
		return -1;
	}
	
	if(init_audio() == -1) {
		destroy_shapes();
		destroy_image();
		destroy_text();
		destroy_video();
		return -1;
	}
	
	init_server();
	
	set_cursor(0);
	
	int retVal_ = 0;
	
	songdb_check();
	start_frametimer();
	srand(time(0));
	
	while(retVal_ != -1 && retVal_ != MENU_EXIT) {
		retVal_ = menu_loop();
		switch(retVal_) {
			case MENU_START:
				songselect();
				break;
			case MENU_EDITOR:
				editor();
				break;
			case MENU_SETTINGS:
				settings_menu();
				break;
			case MENU_STATS:
				playerstats();
				break;
			case MENU_INFO:
				infoscreen();
				break;
			case MENU_PRACTICE:
				practice();
				break;
			case MENU_EXIT:
				break;
			case MENU_ACCOUNT:
				account();
				break;
			case -1:
				//Handle Error
				break;
		}
	}
	
	dbupdate_stat(get_globaltime(),0);
	
	retVal_ = 0;
	if(server_->sdata_[MESSAGE_CHECKUPDATE-1].status == 1) {
		if(server_->sdata_[MESSAGE_CHECKUPDATE-1].result[0] == 2) {
			//UPDATE EXE
			FILE* fp = fopen("rhythmsuite_update.exe","wb");
			if(!fp) {
				write_error(strerror(errno),NULL,0);
			} else {
				fwrite(server_->sdata_[MESSAGE_CHECKUPDATE-1].data,1,server_->sdata_[MESSAGE_CHECKUPDATE-1].bytes,fp);
				fclose(fp);
				
				retVal_ = 1;
			}
		}
	}
	
	destroy_server();
	
	destroy_audio();
	destroy_shapes();
	destroy_image();
	destroy_text();
	destroy_video();
	
	CoUninitialize();
	
	if(retVal_)
		update_exe();
	
	return 0;
}

static void delete_temp() {
	FILE* fp = fopen("_temp_/rhythmsuite.exe","r");
	if(fp) {
		fclose(fp);
		
		remove("_temp_/rhythmsuite.exe");
		RemoveDirectory("_temp_");
	}
}

static void update_exe() {
	if(CreateDirectory("_temp_",NULL)) {
		SetFileAttributes("_temp_",FILE_ATTRIBUTE_HIDDEN);
		rename("rhythmsuite.exe","_temp_/rhythmsuite.exe");
		rename("rhythmsuite_update.exe","rhythmsuite.exe");
	}
}

