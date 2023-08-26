#ifndef GRADE_H
#define GRADE_H

#include "../../framework.h"

#define FAIL_ACC 65/3.f

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Structs
struct grade_bar
{
	float value,target,width,height,border;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Functions
void 			reset_gradebar();
float 			update_gradebar(float,float);
void 			draw_gradebar(float,float,float,int);
int  			get_grade(float);
const char* 	get_gradetext(float);
const char* 	get_gradetext_index(int);
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif //GRADE_H