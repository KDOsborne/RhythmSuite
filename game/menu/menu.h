#ifndef MENU_H
#define MENU_H

#include "../../framework.h"
#include "../../tools.h"
#include "../scorescreen/scorescreen.h"

enum MENU_ENUM { MENU_START=1,MENU_ACCOUNT,MENU_PRACTICE,MENU_EDITOR,MENU_INFO,MENU_SETTINGS,MENU_STATS,MENU_EXIT };

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Functions
int menu_loop();
void init_menu();
void destroy_menu();
////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Globals

////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif //MENU_H