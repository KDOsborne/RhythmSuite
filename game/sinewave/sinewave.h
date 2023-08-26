#ifndef SINEWAVE_H
#define SINEWAVE_H

#include "../../framework.h"
#include "../../tools.h"
#include <time.h>

#define BUFFER_BUFFER 2

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Structs
struct vertex
{
	float x,y,time;
};
typedef struct kpqueue
{
	int 			key;
	int32_t			time;
	struct kpqueue* tail;
}kpqueue;
typedef struct Beat
{
	struct vertex* 		vertices,point;
	struct note_data* 	currNote;
	uint32_t 			nvertices,bufferpos,stride,VAO[BUFFER_BUFFER],VBO[BUFFER_BUFFER];
	int					index,vindex;
} Beat;
////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Functions
int 			sinewave(chart_data*,score_struct*);
Beat** 			init_lines(chart_data*,double,int);

double 			preview_sinewave(Beat**,chart_data*,double,int);
int 			practice_sinewave(float,char*,int,int);
int 			editor_sinewave(chart_data*,int);
void		 	update_line(Beat*,struct sv_data*,struct note_data*,double);
void 			load_line(Beat*,struct sv_data*,struct note_data*);
void 			end_preview();

Beat* 			allocate_line(int,float);
Beat* 			allocate_full(int,float,struct note_data*);
int32_t    		calculate_svtime(struct sv_data*,struct note_data*,int);

void			insert_keypress(int,int32_t);
void 			pop_keypress();
void 			clear_keypresses();

void 			destroy_lines(Beat**);
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif //SINEWAVE_H