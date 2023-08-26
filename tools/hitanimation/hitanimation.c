#include "hitanimation.h"
#include <math.h>

static struct hitanimation* animations = NULL;
			
static void pop_()
{
	if(animations != NULL)
	{
		struct hitanimation* anim = animations;
		animations = animations->tail;
		free(anim);
	}
}

void add_hitanimation(int32_t time, int color)
{
	if(animations == NULL)
	{
		animations = (struct hitanimation*)malloc(sizeof(struct hitanimation));
		animations->time = time;
		animations->color = color;
		animations->tail = NULL;
		return;
	}
	
	struct hitanimation* anim = animations;
	while(anim != NULL)
	{
		if(anim->time == time)
		{
			anim->color = LPURPLE_;
			return;
		}
		
		if(anim->tail == NULL)
		{
			anim->tail = (struct hitanimation*)malloc(sizeof(struct hitanimation));
			anim->tail->time = time;
			anim->tail->color = color;
			anim->tail->tail = NULL;
			return;
		}
		anim = anim->tail;
	}
}

void draw_hitanimations(int32_t time, float spd)
{
	struct hitanimation* anim = animations;

	while(anim != NULL)
	{
		if(time-anim->time >= spd*4/3)
		{
			pop_();
			anim = animations;
		}
		else
		{
			float radians_x = 0.0, radians_y = 0.0, draw_pos = 0.0;
			for(int j = 0; j < 16; j++)
			{
				radians_x = -sin((22.5*j)*M_PI/180);
				radians_y = cos((22.5*j)*M_PI/180);
				
				draw_pos = (anim->time-time)/(spd*2) - 1/3.f;
				draw_pos -= radians_x*((time-anim->time)/spd/16.0);
				draw_rectangle_r(draw_pos,radians_y*((time-anim->time)/spd/8.0),
					0.05/2,0.05,radians_x,radians_y,anim->color,1-(time-anim->time)/(spd*4/3),1);
			}
			anim = anim->tail;
		}
	}
}

void destroy_hitanimation()
{
	while(animations != NULL)
		pop_();
}