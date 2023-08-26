#include "keyboardgraphic.h"

#define SIZE 0.035

static int32_t keytimes[] = {0,0,0,0};

void update_keygraphic(int key,int32_t time)
{
	keytimes[key] = time;
}

void draw_keygraphic(float x, float y, int32_t time)
{
	y += SIZE/2;
	for(int i = 0; i < 4; i++)
	{
		float t = ((float)time-keytimes[i])/175;

		if(time - keytimes[i] < 175)
		{
			float color[] = {(KEYMAP_[i]==0?1.0:t),t,(KEYMAP_[i]==1?1.0:t),1.0};
			draw_square_col(x-(SIZE*1.75)*(1.5-i),y,color,SIZE/(2-t),0);
		}
		else
			draw_square(x-(SIZE*1.75)*(1.5-i),y,WHITE_,SIZE,0);
	}
}

void reset_keygraphic()
{
	memset(keytimes,0,sizeof(keytimes));
}