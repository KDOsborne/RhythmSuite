#include "image.h"
#include "../error/error.h"
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image/stb_image_write.h>

static image_struct* image_ = NULL;

static const char *vertexShader = "#version 130\n"
	"#extension GL_ARB_explicit_attrib_location : enable\n"
	"layout (location = 0) in vec3 aPos;"
	"layout (location = 1) in vec3 aColor;"
	"layout (location = 2) in vec2 aTexCoord;"
	"out vec3 ourColor;"
	"out vec2 TexCoord;"
	"uniform vec4 pos_scale;"
	"uniform vec2 viewPort;"
	"void main()"
	"{"
	"	gl_Position = vec4(aPos.x*pos_scale.z*(viewPort.y/viewPort.x)+pos_scale.x,aPos.y*pos_scale.w+pos_scale.y,0.0,1.0);"
	"	ourColor = aColor;"
	"	TexCoord = aTexCoord;"
	"}\0";
	
static const char *fragmentShader = "#version 130\n"
	"out vec4 FragColor;"
	"in vec3 ourColor;"
	"in vec2 TexCoord;"
	"uniform sampler2D texture1;"
	"void main()"
	"{"
	"	FragColor = texture(texture1, TexCoord);"
	"}\0";

int init_image()
{
	image_ = (image_struct*)malloc(sizeof(image_struct));
	memset(image_,0,sizeof(image_struct));
	
	image_->shader = create_program(vertexShader, fragmentShader);
	if(image_->shader == -1)
		return -1;
	
	glUseProgram(image_->shader);
	
	image_->psu_loc = glGetUniformLocation(image_->shader,"pos_scale");

	glUniform2f(glGetUniformLocation(image_->shader,"viewPort"),video_->settings_[SCREEN_W].value_,video_->settings_[SCREEN_H].value_);
	
	float vertices[] = {
    // positions          // colors           // texture coords
     -1.f, 1.f, 0.f,   1.f, 0.f, 0.f,   0.f, 1.f,   // top left
     -1.f, -1.f, 0.f,  0.f, 1.f, 0.f,   0.f, 0.f,   // bottom left
      1.f, 1.f, 0.f,   0.f, 0.f, 1.f,   1.f, 1.f,   // top right
      1.f, -1.f, 0.f,  1.f, 1.f, 0.f,   1.f, 0.f    // bottom right
	};
	
	glGenVertexArrays(1, &image_->VAO);
    glGenBuffers(1, &image_->VBO);
	
	glBindVertexArray(image_->VAO);
	
	glBindBuffer(GL_ARRAY_BUFFER, image_->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	
	// position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
	
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	
	return 0;
}

void load_image(char* filename)
{
	glDeleteTextures(1, &image_->texture);
	
	glGenTextures(1, &image_->texture);
	glBindTexture(GL_TEXTURE_2D, image_->texture);
	
	// set the texture wrapping/filtering options (on the currently bound texture object)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	stbi_set_flip_vertically_on_load(1);
	
	// load and generate the texture
	unsigned char *data = stbi_load(filename,&image_->width,&image_->height,&image_->channels,0);

	if(data)
	{
		if(image_->channels == 3)
			glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,image_->width,image_->height,0,GL_RGB,GL_UNSIGNED_BYTE,data);
		else if(image_->channels == 4)
			glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,image_->width,image_->height,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
		glGenerateMipmap(GL_TEXTURE_2D);
		
		image_->aspectRatio = (float)image_->width/(float)image_->height;
		
		stbi_image_free(data);
	}
	else
		write_error("WARNING::FAILED TO LOAD IMAGE: ",filename,0);
	
	glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, image_->texture);
}

void unload_image()
{
	glDeleteTextures(1, &image_->texture);
}

void draw_image(float x, float y, float scalex, float scaley)
{
	if(video_->lastProgram_ != image_->shader)
	{
		glUseProgram(image_->shader);
		video_->lastProgram_ = image_->shader;
	}
	
	if(scalex/scaley < image_->aspectRatio)
		scaley /= (image_->aspectRatio/(scalex/scaley));
	
	y += scaley;
	
	glUniform4f(image_->psu_loc,x,y,scalex,scaley);
	
	glBindVertexArray(image_->VAO);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
}

void write_image(int w, int h, void* data)
{
	char datetext[MAX_PATH];
	time_t time_now = time(NULL);
	struct tm date = *localtime(&time_now);
	
	sprintf(datetext,"screenshots/Screenshot_%d-%02d-%02d-%02d_%02d_%02d.jpg",date.tm_year+1900,date.tm_mon+1,
				date.tm_mday,date.tm_hour,date.tm_min, date.tm_sec);
	
	CreateDirectory("screenshots",NULL);
	
	stbi_flip_vertically_on_write(1);
	stbi_write_jpg(datetext,w,h,3,data,100);
}

void update_imagevp()
{
	glUseProgram(image_->shader);
	glUniform2f(glGetUniformLocation(image_->shader,"viewPort"),video_->settings_[SCREEN_W].value_,video_->settings_[SCREEN_H].value_);
}

void reset_image()
{
	destroy_image();
	init_image();
}

void destroy_image()
{
	if(image_ != NULL)
	{
		glDeleteTextures(1, &image_->texture);
		glDeleteVertexArrays(1, &image_->VAO);
		glDeleteBuffers(1, &image_->VBO);
		glDeleteProgram(image_->shader);
		
		free(image_);
		image_ = NULL;
	}
}