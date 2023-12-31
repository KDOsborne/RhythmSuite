#ifndef SHAPES_H
#define SHAPES_H

#include "../glad/glad.h"
#include "../video/video.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Flags

////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Structs
typedef struct Shapes
{
	unsigned int 	shader;
	int 			psu_loc,colu_loc,rotu_loc,upu_loc;
}Shapes;

typedef struct Square
{
	unsigned int VAO,VBO;
}Square;

typedef struct Circle
{
	unsigned int points, VAO, VBO;
}Circle;

typedef struct Line
{
	unsigned int VAO,VBO;
}Line;
////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Functions
int		init_shapes();
void 	init_square();
void 	draw_square(float,float,int,float,int);
void 	draw_square_col(float,float,float*,float,int);
void	draw_rectangle(float,float,float,float,int,float,int);
void 	draw_rectangle_r(float,float,float,float,float,float,int,float,int);
void 	init_circle();
void 	draw_circle(float,float,int,float,int);
void 	init_line();
void 	draw_line(float,float,int,float,int);
void 	draw_linea(float,float,int,float,float,int);
void 	draw_line_r(float,float,float,float,int,float,int);
void    update_shapesvp();
void 	reset_shapes();
void 	destroy_square();
void 	destroy_circle();
void	destroy_line();
void 	destroy_shapes();
////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Globals

////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif //SHAPES_H