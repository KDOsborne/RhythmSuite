#include "shapes.h"
#include <math.h>

static Shapes* 	shapes_ = NULL;
static Square* 	square_ = NULL;
static Circle* 	circle_ = NULL;
static Line*	line_ = NULL;

static const char *vertexShader = "#version 130\n"
	"#extension GL_ARB_explicit_attrib_location : enable\n"
	"layout (location = 0) in vec2 aPos;"
	"uniform float usePort;"
	"uniform vec4 pos_scale;"
	"uniform vec2 viewPort;"
	"uniform vec2 rot;"
	"uniform vec4 aColor;"
	"out vec4 ourColor;"
	"void main()"
	"{"
	"	mat4 m = mat4(rot.y,rot.x,0,0,-rot.x,rot.y,0,0,0,0,1,0,0,0,0,1);"
	"	vec4 p = m*vec4(aPos.x*pos_scale.z,aPos.y*pos_scale.w,0.0,1.0);"
	"	if(usePort == 1)\n"
	"		gl_Position = vec4(p.x*(viewPort.y/viewPort.x)+pos_scale.x,p.y+pos_scale.y,0.0,1.0);\n"
	"	else\n"
	"		gl_Position = vec4(p.x+pos_scale.x,p.y+pos_scale.y,0.0,1.0);"
	"	ourColor = aColor;"
	"}\0";
	
static const char *fragmentShader = "#version 130\n"
	"out vec4 FragColor;"
	"in vec4 ourColor;"
	"void main()"
	"{"
		"FragColor = ourColor;"
	"}\0";
	
int init_shapes()
{
	shapes_ = (Shapes*)malloc(sizeof(Shapes));
	memset(shapes_,0,sizeof(Shapes));
	
	shapes_->shader = create_program(vertexShader, fragmentShader);
	if(shapes_->shader == -1)
		return -1;
	
	glUseProgram(shapes_->shader);
	
	shapes_->psu_loc = glGetUniformLocation(shapes_->shader,"pos_scale");
	shapes_->upu_loc = glGetUniformLocation(shapes_->shader,"usePort");
	shapes_->rotu_loc = glGetUniformLocation(shapes_->shader,"rot");
	shapes_->colu_loc = glGetUniformLocation(shapes_->shader,"aColor");
	
	glUniform2f(glGetUniformLocation(shapes_->shader,"viewPort"),video_->settings_[SCREEN_W].value_,video_->settings_[SCREEN_H].value_);
	glUniform2f(shapes_->rotu_loc,0,1);
	
	init_square();
	init_circle(90);
	init_line();
	
	return 0;
}

void init_square()
{
	float vertices[] = {-1.0,1.0,-1.0,-1.0,1.0,1.0,1.0,-1.0};
	
	square_ = (Square*)malloc(sizeof(Square));
	memset(square_,0,sizeof(Square));
	
	glGenVertexArrays(1, &square_->VAO);
    glGenBuffers(1, &square_->VBO);
	
	glBindVertexArray(square_->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, square_->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
	
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void init_circle(int points)
{
	float vertices[(points+2)*2];
	
	vertices[0] = 0.f;
	vertices[1] = 0.f;
	
	for(int i = 0; i <= points; i++)
	{
		vertices[(i+1)*2] = sin((float)i/points*2*M_PI);
		vertices[(i+1)*2+1] = cos((float)i/points*2*M_PI);
	}
	
	circle_ = (Circle*)malloc(sizeof(Circle));
	memset(circle_,0,sizeof(Circle));
	
	circle_->points = points+2;
	
	glGenVertexArrays(1, &circle_->VAO);
    glGenBuffers(1, &circle_->VBO);
	
	glBindVertexArray(circle_->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, circle_->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*(circle_->points*2), vertices, GL_DYNAMIC_DRAW);
	
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
	
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void init_line()
{
	float vertices[] = {-1.0,0.0,1.0,0.0,0.0,1.0,0.0,-1.0};
	
	line_ = (Line*)malloc(sizeof(Line));
	memset(line_,0,sizeof(Line));
	
	glGenVertexArrays(1, &line_->VAO);
    glGenBuffers(1, &line_->VBO);
	
	glBindVertexArray(line_->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, line_->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
	
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void draw_square(float x, float y, int color, float scale, int flags)
{
	if(video_->lastProgram_ != shapes_->shader)
	{
		glUseProgram(shapes_->shader);
		video_->lastProgram_ = shapes_->shader;
	}
	
	glUniform1f(shapes_->upu_loc,1);
	glUniform4f(shapes_->psu_loc,x,y,scale,scale);
	glUniform4fv(shapes_->colu_loc,1,(float*)&COLORS_[color]);
	
	glBindVertexArray(square_->VAO);
	
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
}

void draw_square_col(float x, float y, float* color, float scale, int flags)
{
	if(video_->lastProgram_ != shapes_->shader)
	{
		glUseProgram(shapes_->shader);
		video_->lastProgram_ = shapes_->shader;
	}
	
	glUniform1f(shapes_->upu_loc,1);
	glUniform4f(shapes_->psu_loc,x,y,scale,scale);
	glUniform4fv(shapes_->colu_loc,1,color);
	
	glBindVertexArray(square_->VAO);
	
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
}

void draw_rectangle(float x, float y, float w, float h, int color, float alpha, int flags)
{
	if(video_->lastProgram_ != shapes_->shader)
	{
		glUseProgram(shapes_->shader);
		video_->lastProgram_ = shapes_->shader;
	}
	
	if(flags == 2)
	{
		y -= h;
	}
	
	glUniform1f(shapes_->upu_loc,flags);
	glUniform4f(shapes_->psu_loc,x,y,w,h);
	glUniform4f(shapes_->colu_loc,COLORS_[color].r,COLORS_[color].g,COLORS_[color].b,alpha);
	
	glBindVertexArray(square_->VAO);
	
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
}

void draw_rectangle_r(float x,float y,float w,float h,float rx,float ry,int color,float alpha,int flags)
{
	if(video_->lastProgram_ != shapes_->shader)
	{
		glUseProgram(shapes_->shader);
		video_->lastProgram_ = shapes_->shader;
	}
	
	glUniform1f(shapes_->upu_loc,flags);
	glUniform4f(shapes_->psu_loc,x,y,w,h);
	glUniform2f(shapes_->rotu_loc,rx,ry);
	glUniform4f(shapes_->colu_loc,COLORS_[color].r,COLORS_[color].g,COLORS_[color].b,alpha);
	
	glBindVertexArray(square_->VAO);
	
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	
	glUniform2f(shapes_->rotu_loc,0,1);
}

void draw_circle(float x, float y, int color, float scale, int flags)
{
	if(video_->lastProgram_ != shapes_->shader)
	{
		glUseProgram(shapes_->shader);
		video_->lastProgram_ = shapes_->shader;
	}
	
	glUniform1f(shapes_->upu_loc,1);
	glUniform4f(shapes_->psu_loc,x,y,scale,scale);
	glUniform4fv(shapes_->colu_loc,1,(float*)&COLORS_[color]);
	
	glBindVertexArray(circle_->VAO);
	
	glDrawArrays(GL_TRIANGLE_FAN,0,circle_->points);
}

void draw_line(float x, float y, int color, float scale, int flags)
{
	if(video_->lastProgram_ != shapes_->shader)
	{
		glUseProgram(shapes_->shader);
		video_->lastProgram_ = shapes_->shader;
	}
	
	glUniform1f(shapes_->upu_loc,0);
	glUniform4f(shapes_->psu_loc,x,y,scale,scale);
	glUniform4fv(shapes_->colu_loc,1,(float*)&COLORS_[color]);
	
	glBindVertexArray(line_->VAO);
	
	glDrawArrays(GL_LINES,flags*2,2);
}

void draw_linea(float x, float y, int color, float alpha, float scale, int flags)
{
	if(video_->lastProgram_ != shapes_->shader)
	{
		glUseProgram(shapes_->shader);
		video_->lastProgram_ = shapes_->shader;
	}
	
	glUniform1f(shapes_->upu_loc,0);
	glUniform4f(shapes_->psu_loc,x,y,scale,scale);
	glUniform4f(shapes_->colu_loc,COLORS_[color].r,COLORS_[color].g,COLORS_[color].b,alpha);
	
	glBindVertexArray(line_->VAO);
	
	glDrawArrays(GL_LINES,flags*2,2);
}

void draw_line_r(float x, float y, float rx, float ry, int color, float scale, int flags)
{
	if(video_->lastProgram_ != shapes_->shader)
	{
		glUseProgram(shapes_->shader);
		video_->lastProgram_ = shapes_->shader;
	}
	
	glUniform1f(shapes_->upu_loc,0);
	glUniform4f(shapes_->psu_loc,x,y,scale,scale);
	glUniform2f(shapes_->rotu_loc,rx,ry);
	glUniform4fv(shapes_->colu_loc,1,(float*)&COLORS_[color]);
	
	glBindVertexArray(line_->VAO);
	
	glDrawArrays(GL_LINES,flags*2,2);
	
	glUniform2f(shapes_->rotu_loc,0,1);
}

void reset_shapes()
{
	destroy_shapes();
	init_shapes();
}

void update_shapesvp()
{
	glUseProgram(shapes_->shader);
	glUniform2f(glGetUniformLocation(shapes_->shader,"viewPort"),video_->settings_[SCREEN_W].value_,video_->settings_[SCREEN_H].value_);
}

void destroy_square()
{
	glDeleteVertexArrays(1, &square_->VAO);
	glDeleteBuffers(1, &square_->VBO);
	
	free(square_);
}

void destroy_circle()
{
	glDeleteVertexArrays(1, &circle_->VAO);
	glDeleteBuffers(1, &circle_->VBO);
	
	free(circle_);
}

void destroy_line()
{
	glDeleteVertexArrays(1, &line_->VAO);
	glDeleteBuffers(1, &line_->VBO);
	
	free(line_);
}

void destroy_shapes()
{
	if(shapes_ != NULL)
	{
		destroy_square();
		destroy_circle();
		destroy_line();
		glDeleteProgram(shapes_->shader);
		free(shapes_);
		shapes_ = NULL;
	}
}