#include "text.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define SCROLL_SPEED 0.0001

static text_struct* 		text_ = NULL;
static struct scroll_text* 	scroll_ = NULL;

static const int binary_data[] = { 0, 4329476, 10813440, 11512810,
	0, 0, 0, 4325376,
	17047824, 1116225, 0, 4357252,
	4, 31744, 4, 17043521,
	15521390, 4395140, 16267327, 15249966,
	18415120, 32554511, 31505966, 33038468,
	15252014, 15268367, 131076, 131076,
	17043728, 1016800, 1118273, 15249412,
	0,
	15269425, 16301615, 15238702, 16303663,
	32554047, 32554017, 31520302, 18415153,
	32641183, 25708078, 18128177, 1082431,
	18732593, 18470705, 15255086, 16301089,
	15254838, 16301617, 31504911, 32641156,
	18400814, 18400580, 18405233, 18157905,
	18157700, 32772191, 25436440, 1118480,
	3213379, 4521984, 31, 33554431
};

static const char *vertexShader = "#version 130\n"
	"#extension GL_ARB_explicit_attrib_location : enable\n"
	"layout (location = 0) in vec2 aPos;"
	"uniform float xOffset;"
	"uniform float yOffset;"
	"uniform float xTravel;"
	"uniform float yTravel;"
	"uniform float scale;"
	"uniform vec2 viewPort;"
	"uniform vec4 aColor;"
	"out vec4 ourColor;"
	"void main()"
	"{"
	"	if(viewPort.x > viewPort.y)\n"
	"		gl_Position = vec4(((aPos.x+xTravel)*scale)*(viewPort.y/viewPort.x)+xOffset,(aPos.y+yTravel)*scale+yOffset,0.0,1.0);\n"
	"	else\n"
	"		gl_Position = vec4((aPos.x+xTravel)*scale+xOffset,((aPos.y+yTravel)*scale)*(viewPort.x/viewPort.y)+yOffset,0.0,1.0);"
	"	ourColor = aColor;"
	"}\0";
	
static const char *fragmentShader = "#version 130\n"
	"out vec4 FragColor;"
	"in vec4 ourColor;"
	"void main()"
	"{"
		"FragColor = ourColor;"
	"}\0";

int init_text()
{
	text_ = (text_struct*)malloc(sizeof(text_struct));
	memset(text_, 0, sizeof(text_struct));
	
	int table_size = sizeof(binary_data)/sizeof(int);
	
	text_->shader = create_program(vertexShader, fragmentShader);
	if(text_->shader == -1)
	{
		free(text_);
		return -1;
	}
	
	glUseProgram(text_->shader);
	
	text_->xu_loc = glGetUniformLocation(text_->shader,"xOffset");
	text_->yu_loc = glGetUniformLocation(text_->shader,"yOffset");
	text_->xtu_loc = glGetUniformLocation(text_->shader,"xTravel");
	text_->ytu_loc = glGetUniformLocation(text_->shader,"yTravel");
	text_->fsu_loc = glGetUniformLocation(text_->shader,"scale");
	text_->colu_loc = glGetUniformLocation(text_->shader,"aColor");
	
	glUniform2f(glGetUniformLocation(text_->shader,"viewPort"),video_->settings_[SCREEN_W].value_,video_->settings_[SCREEN_H].value_);
	
	text_->chars = (struct character*)malloc(sizeof(struct character)*table_size);

	float vertices[72];
	for(int i = 0; i < 6; i++)
	{
		for(int j = 0; j < 6; j++)
		{
			vertices[i*12+j*2] = ((float)j-2.5)*(TXT_SIZE/5.f);
			vertices[i*12+j*2+1] =  ((float)i-2.5)*(TXT_SIZE/5.f);
		}
	}
	
	glGenVertexArrays(1, &text_->VAO);
    glGenBuffers(1, &text_->VBO);
	
	glBindVertexArray(text_->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, text_->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	
	for(int i = 0; i < table_size; i++)
	{
		unsigned int *indices = NULL;
		text_->chars[i].num_indices = 0;
		switch (i + ' ')
		{
			case ',':
			case '.':
			case '\'':
			case ':':
			case ';':
			case '!':
				text_->chars[i].width = TXT_SIZE/5.f;
				break;
			default:
				text_->chars[i].width = TXT_SIZE;
		}
		
		glGenBuffers(1, &text_->chars[i].EBO);
		
		for(int j = 0; j < 5; j++)
		{
			for(int k = 0; k < 5; k++)
			{
				if(((binary_data[i] >> (j*5+k)) & 1) == 1)
				{	
					if(indices == NULL)
						indices = (unsigned int*)malloc(sizeof(unsigned int)*6);
					else
						indices =  (unsigned int*)realloc(indices, sizeof(unsigned int)*text_->chars[i].num_indices*6+sizeof(unsigned int)*6);
					
					indices[text_->chars[i].num_indices*6] 	 = 6*(j+1)+k;
					indices[text_->chars[i].num_indices*6+1] = 6*j+k;
					indices[text_->chars[i].num_indices*6+2] = 6*(j+1)+k+1;
					indices[text_->chars[i].num_indices*6+3] = 6*j+k;
					indices[text_->chars[i].num_indices*6+4] = 6*(j+1)+k+1;
					indices[text_->chars[i].num_indices*6+5] = 6*j+k+1;
					
					text_->chars[i].num_indices++;
				}
			}

		}
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text_->chars[i].EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, text_->chars[i].num_indices*sizeof(unsigned int)*6, indices, GL_STATIC_DRAW);
		
		free(indices);
	}
	
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
	
    glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	return 0;
}

void reset_text()
{
	destroy_text();
	init_text();
}

void update_textvp()
{
	glUseProgram(text_->shader);
	glUniform2f(glGetUniformLocation(text_->shader,"viewPort"),video_->settings_[SCREEN_W].value_,video_->settings_[SCREEN_H].value_);
}

void render_text(char *str, float* pos, float* color, float* offset, float fs, float mod, float bounds, int flags)
{
	if(video_->lastProgram_ != text_->shader)
	{
		glUseProgram(text_->shader);
		video_->lastProgram_ = text_->shader;
	}
	
	float xOff = 0, yOff = 0;
	if(offset != NULL)
	{
		xOff = offset[0];
		yOff = offset[1]/mod;
	}
	
	glUniform1f(text_->xu_loc,pos[0]);
	glUniform1f(text_->yu_loc,pos[1]);
	
	if(flags & TXT_TOPALIGNED)
		glUniform1f(text_->ytu_loc,yOff-TXT_SIZE/2);
	else if(flags & TXT_BOTALIGNED)
		glUniform1f(text_->ytu_loc,yOff+TXT_SIZE/2);
	else
		glUniform1f(text_->ytu_loc,yOff);
	
	glUniform1f(text_->fsu_loc,fs*mod);
	glUniform4fv(text_->colu_loc,1,color);
	
	glBindVertexArray(text_->VAO);
	
	float total_width = 0.f, x_ = xOff, y_ = yOff, fs_ = fs;
	float first = 1;
	
	if(flags & TXT_TOPALIGNED)
		y_ -= TXT_SIZE/2;
	else if(flags & TXT_BOTALIGNED)
		y_ += TXT_SIZE/2;

	for(int i = 0; i < strlen(str); i++)
	{
		if(total_width == 0.f)
		{
			total_width -= (TXT_BEARING);
			for(int j = i; j < strlen(str); j++)
			{
				if(str[j] == '\n' || str[j] == '\r')
					break;
				else if(str[j] == ' ')
				{
					total_width += (TXT_BEARING);
					continue;
				}
				else if(str[j] > '`' || str[j] < ' ')
					str[j] = '`';
				
				total_width += (text_->chars[str[j]-' '].width + TXT_BEARING);
			}
			/*if(bounds > 0 && total_width*fs > bounds)
			{
				fs_ = fs*(bounds/(total_width*fs));
				glUniform1f(text_->fsu_loc,fs_);
				total_width = bounds/fs;
			}
			else
				glUniform1f(text_->fsu_loc,fs);*/
			
			if(flags & TXT_CENTERED)
				x_ -= total_width/2-text_->chars[0].width/2;
			else if(flags & TXT_RGHTALIGNED)
				x_ -= total_width-TXT_BEARING;
			else
				x_ += (text_->chars[0].width/2+TXT_BEARING);
			
			first = 1;
		}
		if(str[i] == '\n')
		{
			y_ -= (TXT_SIZE*2);
			x_ = xOff;
			total_width = 0.f;
			
			glUniform1f(text_->ytu_loc,y_);
			continue;
		}
		else if(str[i] == '\r')
		{
			y_ += (TXT_SIZE*2);
			x_ = xOff;
			total_width = 0.f;
			
			glUniform1f(text_->ytu_loc,y_);
			continue;
		}
		else if(str[i] == ' ')
		{
			x_ += (TXT_BEARING);
			continue;
		}
		
		if(!first)
			x_ += (text_->chars[str[i]-' '].width/2);
		first = 0;
		
		glUniform1f(text_->xtu_loc, x_);
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text_->chars[str[i]-' '].EBO);
		glDrawElements(GL_TRIANGLES, text_->chars[str[i]-' '].num_indices*6, GL_UNSIGNED_INT, 0);
	
		x_ += (text_->chars[str[i]-' '].width/2 + TXT_BEARING);
	}
	
	if(offset != NULL)
	{
		offset[0] = x_;
		offset[1] = y_*mod;
	}
}

void render_simpletext(char *str, float x, float y, int color, float fs, int flags, float* ypos)
{
	if(video_->lastProgram_ != text_->shader)
	{
		glUseProgram(text_->shader);
		video_->lastProgram_ = text_->shader;
	}
	
	glUniform1f(text_->xu_loc,x);
	glUniform1f(text_->yu_loc,y);
	
	if(flags & TXT_TOPALIGNED)
		glUniform1f(text_->ytu_loc,-TXT_SIZE/2);
	else if(flags & TXT_BOTALIGNED)
		glUniform1f(text_->ytu_loc,TXT_SIZE/2);
	else
		glUniform1f(text_->ytu_loc,0);
	
	glUniform1f(text_->fsu_loc,fs);
	glUniform4fv(text_->colu_loc,1,(float*)&COLORS_[color]);
	
	glBindVertexArray(text_->VAO);
	
	float total_width = 0.f, x_ = 0, y_ = 0;
	float first = 1;
	if(flags & TXT_TOPALIGNED)
		y_ = -TXT_SIZE/2;
	else if(flags & TXT_BOTALIGNED)
		y_ = TXT_SIZE/2;
	
	for(int i = 0; i < strlen(str); i++)
	{
		if(total_width == 0.f)
		{
			total_width -= (TXT_BEARING);
			for(int j = i; j < strlen(str); j++)
			{
				if(str[j] == '\n' || str[j] == '\r')
					break;
				else if(str[j] >= 1 && str[j] <= 7)
					continue;
				else if(str[j] == ' ')
				{
					total_width += (TXT_BEARING);
					continue;
				}
				else if(str[j] > '`' || str[j] < ' ')
					str[j] = '`';
				
				total_width += (text_->chars[str[j]-' '].width + TXT_BEARING);
			}
			/*if(bounds > 0 && total_width*fs > bounds)
			{
				fs_ = fs*(bounds/(total_width*fs));
				glUniform1f(text_->fsu_loc,fs_);
				total_width = bounds/fs;
			}
			else
				glUniform1f(text_->fsu_loc,fs);*/
			
			if(flags & TXT_CENTERED)
				x_ -= total_width/2-text_->chars[0].width/2;
			else if(flags & TXT_RGHTALIGNED)
				x_ -= total_width-TXT_BEARING;
			else
				x_ += (text_->chars[0].width/2+TXT_BEARING);
			
			first = 1;
		}
		if(str[i] == '\n')
		{
			y_ -= (TXT_SIZE*1.5);
			x_ = 0;
			total_width = 0.f;
			
			glUniform1f(text_->ytu_loc,y_);
			continue;
		}
		else if(str[i] == '\r')
		{
			y_ += (TXT_SIZE*1.5);
			x_ = 0;
			total_width = 0.f;
			
			glUniform1f(text_->ytu_loc,y_);
			continue;
		}
		else if(str[i] >= 1 && str[i] <= 7)
		{
			switch(str[i])
			{
				case '\1':
					glUniform4fv(text_->colu_loc,1,(float*)&COLORS_[WHITE_]);
				break;
				case '\2':
					glUniform4fv(text_->colu_loc,1,(float*)&COLORS_[RED_]);
				break;
				case '\3':
					glUniform4fv(text_->colu_loc,1,(float*)&COLORS_[PINK_]);
				break;
				case '\4':
					glUniform4fv(text_->colu_loc,1,(float*)&COLORS_[TEAL_]);
				break;
				case '\5':
					glUniform4fv(text_->colu_loc,1,(float*)&COLORS_[YELLOW_]);
				break;
				case '\6':
					int rc = rand()%NCOLORS_;
					while(rc == 6 || rc == 7)
						rc = rand()%NCOLORS_;
					glUniform4fv(text_->colu_loc,1,(float*)&COLORS_[rc]);
				break;
				case '\7':
					glUniform4fv(text_->colu_loc,1,(float*)&COLORS_[LBLUE_]);
				break;
			}
			continue;
		}
		else if(str[i] == ' ')
		{
			x_ += (TXT_BEARING);
			continue;
		}
		
		if(!first)
			x_ += (text_->chars[str[i]-' '].width/2);
		first = 0;
		
		glUniform1f(text_->xtu_loc, x_);
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text_->chars[str[i]-' '].EBO);
		glDrawElements(GL_TRIANGLES, text_->chars[str[i]-' '].num_indices*6, GL_UNSIGNED_INT, 0);
	
		x_ += (text_->chars[str[i]-' '].width/2 + TXT_BEARING);
	}
	
	if(ypos != NULL)
		*ypos = y + (y_ - (TXT_SIZE))*fs;
}

void init_scrolltext(char* txt,char* btxt,float x,float y,float x1,float x2)
{
	scroll_ = (struct scroll_text*)malloc(sizeof(struct scroll_text));
	memset(scroll_,0,sizeof(struct scroll_text));
	
	scroll_->text = (char*)malloc(strlen(txt)+1);
	memcpy(scroll_->text,txt,strlen(txt)+1);
	
	if(btxt != NULL)
	{
		scroll_->backtext = (char*)malloc(strlen(btxt)+1);
		memcpy(scroll_->backtext,btxt,strlen(btxt)+1);
	}
	else 
		scroll_->backtext = NULL;
	
	scroll_->xPos = x;
	scroll_->yPos = y;
	scroll_->x1 = x1;
	scroll_->x2 = x2;
}

void render_scrolltext(double time,int col,float fs,int flags)
{
	if(scroll_ == NULL)
		return;
	
	if(video_->lastProgram_ != text_->shader)
	{
		glUseProgram(text_->shader);
		video_->lastProgram_ = text_->shader;
	}
	
	glUniform1f(text_->xu_loc,scroll_->xPos);
	glUniform1f(text_->yu_loc,scroll_->yPos);
	
	if(flags & TXT_TOPALIGNED)
		glUniform1f(text_->ytu_loc,-TXT_SIZE/2);
	else if(flags & TXT_BOTALIGNED)
		glUniform1f(text_->ytu_loc,TXT_SIZE/2);
	else
		glUniform1f(text_->ytu_loc,0);
	
	glUniform1f(text_->fsu_loc,fs);
	glUniform4fv(text_->colu_loc,1,(float*)&COLORS_[col]);
	
	glBindVertexArray(text_->VAO);
	
	float x_ = (text_->chars[0].width/2), y_ = 0, first = 1, drawn = 0;

	for(int i = 0; i < strlen(scroll_->text); i++)
	{
		if(scroll_->text[i] == '\n')
		{
			y_ -= (TXT_SIZE*2);
			x_ = 0;
			
			glUniform1f(text_->ytu_loc,y_);
			continue;
		}
		else if(scroll_->text[i] == '\r')
		{
			y_ += (TXT_SIZE*2);
			x_ = 0;
			
			glUniform1f(text_->ytu_loc,y_);
			continue;
		}
		else if(scroll_->text[i] == ' ')
		{
			x_ += (TXT_BEARING);
			continue;
		}
		
		if(!first)
			x_ += (text_->chars[scroll_->text[i]-' '].width/2);
		else
			first = 0;
		
		if(scroll_->xPos + x_*(fs/video_->aspectRatio_) >= scroll_->x2)
		{
			drawn = 1;
			break;
		}
		
		if(scroll_->xPos + x_*(fs/video_->aspectRatio_) > scroll_->x1)	
		{
			glUniform1f(text_->xtu_loc, x_);
			
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text_->chars[scroll_->text[i]-' '].EBO);
			glDrawElements(GL_TRIANGLES, text_->chars[scroll_->text[i]-' '].num_indices*6, GL_UNSIGNED_INT, 0);
			
			drawn = 1;
		}

		x_ += (text_->chars[scroll_->text[i]-' '].width/2 + TXT_BEARING);
	}
	
	if(!drawn)
	{
		scroll_->xPos = scroll_->x2;
		if(scroll_->backtext != NULL)
		{
			char* temp = scroll_->text;
			scroll_->text = scroll_->backtext;
			scroll_->backtext = temp;
		}
	}
	else
		scroll_->xPos -= time*(SCROLL_SPEED*video_->aspectRatio_);
}

void destroy_scrolltext()
{
	if(scroll_ != NULL)
	{
		free(scroll_->text);
		if(scroll_->backtext != NULL)
			free(scroll_->backtext);
		scroll_ = NULL;
	}
}

void destroy_text()
{
	if(text_ != NULL)
	{
		destroy_scrolltext();
		
		glDeleteVertexArrays(1, &text_->VAO);
		glDeleteBuffers(1, &text_->VBO);
		
		for(int i = 0; i < sizeof(binary_data)/sizeof(int); i++)
			glDeleteBuffers(1, &text_->chars[i].EBO);
		
		glDeleteProgram(text_->shader);
		
		free(text_->chars);
		free(text_);
		text_ = NULL;
	}
}