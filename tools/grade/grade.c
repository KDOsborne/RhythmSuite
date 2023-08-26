#include "grade.h"

#define NGRADES 9

static struct grade_bar grade_bar = {
		1000.0,1000.0, 						    //Target/Value
		(TXT_SIZE*5+TXT_BEARING*4)/2,TXT_SIZE,	//Width/Height
		0.0035,									//Border thickness
};

static const char *grades[] = { 
	"SSS+", 
	"SSS",
	"SS",
	"S",
	"AA",
	"A",
	"B",
	"C", 
	"F"
};

static const float grade_points[] = { 
	15.f/3,
	20.f/3,
	25.f/3,
	30.f/3, 
	40.f/3,
	50.f/3,
	65.f/3, 
	80.f/3,
	500.f/6
};

void reset_gradebar()
{
	grade_bar.target = 1000.0;
	grade_bar.value = 1000.0;
}

float update_gradebar(float value,float time)
{
	if(grade_bar.target == 1000.0)
	{
		grade_bar.target = value;
		grade_bar.value = value;
		return value;
	}
	
	grade_bar.target = value;
	if(grade_bar.value < grade_bar.target)
	{
		grade_bar.value += time*(0.0001+(grade_bar.target-grade_bar.value)/100);
		if(grade_bar.value > grade_bar.target)
			grade_bar.value = value;
	}
	else if(grade_bar.value > grade_bar.target)
	{
		grade_bar.value -= time*(0.0001+(grade_bar.value-grade_bar.target)/100);
		if(grade_bar.value < grade_bar.target)
			grade_bar.value = value;
	}
	
	return grade_bar.value;
}

void draw_gradebar(float x, float y, float scale, int flags)
{
	if(flags & 1)
		x -= (grade_bar.width+TXT_BEARING/4)/video_->aspectRatio_*scale;
	if(flags & 2)
		y -= (grade_bar.height/2)*scale;
	
	int grade = get_grade(grade_bar.value);
	
	draw_rectangle(x,y,grade_bar.width*scale,grade_bar.height*scale/2,WHITE_,1.0,1);
	glBlendFunc(GL_ONE,GL_ZERO);
	
	draw_rectangle(x,y,grade_bar.width*scale-grade_bar.border,
					grade_bar.height*scale/2-grade_bar.border,BLACK_,1.0,1);
	
	if(grade < NGRADES)
	{
		float w = 1.0;
		if(grade != 0)
			w = (grade_points[grade]-grade_bar.value)/(grade_points[grade]-grade_points[grade-1]);
		else
			w = (grade_points[grade]-grade_bar.value)/(grade_points[grade]);
		
		if(w < 0)
			w = 0;
		
		draw_rectangle(x-(grade_bar.width*scale-grade_bar.border)/video_->aspectRatio_*(1.0-w),y,
						(grade_bar.width*scale-grade_bar.border)*w,grade_bar.height*scale/2-grade_bar.border,
						WHITE_,1.0,1);
	}
	
	glBlendFunc(GL_SRC_ALPHA,GL_DST_ALPHA);
}

int get_grade(float dev)
{
	for(int i = 0; i < NGRADES; i++)
		if(dev <= grade_points[i])
			return i;
	return NGRADES-1;
}

const char* get_gradetext(float dev)
{
	for(int i = 0; i < NGRADES; i++)
		if(dev <= grade_points[i])
			return grades[i];

	return grades[NGRADES-1];
}

const char* get_gradetext_index(int dev)
{
	if(dev < NGRADES)
		return grades[dev];
	else 
		return NULL;
}