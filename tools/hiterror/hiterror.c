#include "hiterror.h"

static struct hiterror_point* hiterror_points = NULL;
static int warn = 0, warntype = 0, warntime = 0;

static void pop_()
{
	if(hiterror_points != NULL)
	{
		struct hiterror_point* point = hiterror_points;
		hiterror_points = hiterror_points->tail;
		free(point);
	}
}

void add_hiterrorpoint(float value, int32_t time, int color)
{
	if(color == RED_)
		warntype = 3;
	else if(color == YELLOW_)
		warntype = 2;
	else if(color == TEAL_)
		warntype = 1;
	else
		warntype = 0;
	
	if(hiterror_points == NULL)
	{
		hiterror_points = (struct hiterror_point*)malloc(sizeof(struct hiterror_point));
		hiterror_points->value = value;
		hiterror_points->time = time;
		hiterror_points->color = color;
		hiterror_points->tail = NULL;
		return;
	}
	
	struct hiterror_point* point = hiterror_points;
	while(point != NULL)
	{
		if(point->tail == NULL)
		{
			point->tail = (struct hiterror_point*)malloc(sizeof(struct hiterror_point));
			point->tail->value = value;
			point->tail->time = time;
			point->tail->color = color;
			point->tail->tail = NULL;
			return;
		}
		point = point->tail;
	}
}

void draw_hiterror(float x, float y, int32_t time)
{
	struct hiterror_point* point = hiterror_points;
	float w = 1.0, h = 0.0025*(float)video_->settings_[HESIZE].value_, a = video_->settings_[HEALPHA].value_/100.f;
	
	while(point != NULL)
	{
		if(time-point->time >= HITERROR_DURATION)
		{
			pop_();
			point = hiterror_points;
		}
		else
		{
			if(h == 0)
				draw_linea(x,y+point->value,point->color,a-(float)(time-point->time)/HITERROR_DURATION*a,1.0,0);
			else
				draw_rectangle(x,y+point->value,w,h,point->color,a-(float)(time-point->time)/HITERROR_DURATION*a,0);
			//draw_linea(x,point->value,point->color,0.5-(float)(time-point->time)/HITERROR_DURATION*0.5,1.0,0);
			
			point = point->tail;
		}
	}
	
	
	/*glBlendFunc(GL_ONE,GL_ZERO);
	draw_rectangle(x,y,w,h,PURPLE_,1.0,1);
	glBlendFunc(GL_SRC_ALPHA,GL_DST_ALPHA);*/

	
	if(time >= warntime)
	{
		warn = !warn;
		warntime = time + 50;
	}
	
	if(hiterror_points != NULL && (warntype != 0 || hiterror_points->tail != NULL))
	{
		if(warn)
		{
			if(warntype == 1)
				render_simpletext("EARLY",-1/3.f,y-0.1,WHITE_,2,TXT_TOPALIGNED|TXT_CENTERED,NULL);
			else if(warntype == 2)
				render_simpletext("LATE",-1/3.f,y-0.1,WHITE_,2,TXT_TOPALIGNED|TXT_CENTERED,NULL);
			else if(warntype == 3)
				render_simpletext("MISS",-1/3.f,y-0.1,WHITE_,2,TXT_TOPALIGNED|TXT_CENTERED,NULL);
		}
	}
	else
		warntype = 0;
}

void destroy_hiterror()
{
	while(hiterror_points != NULL)
		pop_();
	warntime = 0;
	warntype = 0;
	warn = 0;
}