#include "options.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

options_struct* create_options(char* label,int lCol,float fs,int start,int from,int total,int col,int flags)
{
	options_struct* o_ = (options_struct*)malloc(sizeof(options_struct));
	memset(o_,0,sizeof(options_struct));
	
	options_struct os_ = 
	{
		NULL,		//Options List
		NULL,			//Label Text
		{COLORS_[lCol].r,COLORS_[lCol].g,COLORS_[lCol].b,COLORS_[lCol].a}, 	//Label Color
		0, start,	//Current Option
		from, total,	//From/To
		fs,0.25,		//Font Size
		0,			//Highlighted
		{COLORS_[col].r,COLORS_[col].g,COLORS_[col].b,COLORS_[col].a},			//Color
		NULL,		//Offset
		flags,		//Flags
	};

	memcpy(o_, &os_, sizeof(options_struct));
	
	if(label != NULL)
	{
		o_->label_ = (char*)malloc(sizeof(char)*(strlen(label)+1));
		memcpy(o_->label_,label,strlen(label)+1);
	}
	
	o_->offset_ = (float*)malloc(sizeof(float)*2);
	memset(o_->offset_,0,sizeof(float)*2);
	
	return o_;
}

void add_option(options_struct* o,char* text,float x,float y,int col,int type,int flags,void* data)
{
	o->options_ = (struct option_*)realloc(o->options_, sizeof(struct option_)*(o->nOptions_+1));
	
	struct option_ opt_ = 
	{
		NULL,			//Text
		{x,y},		//Position
		{COLORS_[col].r,COLORS_[col].g,COLORS_[col].b,COLORS_[col].a},	//Color
		type,		//Type
		flags,		//Flags
		data		//Data
	};
	
	memcpy(&(o->options_[o->nOptions_]), &opt_, sizeof(struct option_));
	
	if(text != NULL)
	{	
		o->options_[o->nOptions_].text_ = (char*)malloc(sizeof(char)*(strlen(text)+1));
		memcpy(o->options_[o->nOptions_].text_,text,strlen(text)+1);
	}
	
	o->nOptions_++;
}

void increment_option(options_struct* o, int n)
{
	o->currOption_ += n;
	if(o->currOption_ < 0)
		o->currOption_ = o->nOptions_-1;
	else if(o->currOption_ >= o->nOptions_)
		o->currOption_ = 0;
}

char* get_option_text(options_struct* o)
{
	return o->options_[o->currOption_].text_;
}

void* get_option_data(options_struct* o)
{
	return o->options_[o->currOption_].data_;
}

void draw_options(options_struct* o)
{
	if(o->nOptions_ == 0)
		return;
	
	int from_ = 0, total_ = 0;
	if(o->from_ == -1)
		from_ = 0;
	else
		from_ = o->from_;
	if(o->total_ == -1)
		total_ = o->nOptions_ - from_;
	else
		total_ = o->total_;
	
	memset(o->offset_,0,sizeof(float)*2);
	
	int i = o->currOption_-abs(from_), count = 0;
	if(o->from_ == -1)
		i = 0;
	
	float mod = 1.0;

	while(i < 0)
		i+=o->nOptions_;
	
	while(count < total_)
	{
		if(!(o->flags_ & TXT_NORESET))
			memset(o->offset_,0,sizeof(float)*2);

		if(o->flags_ & TXT_MINIMIZE)
		{
			mod = 1.0/((fabs(from_+count))*0.75+1);
			
		}
		
		if(o->label_ != NULL && count == 0)
			render_text(o->label_,o->options_[i].pos_,o->labelCol_,o->offset_,o->fontSize_,mod,o->bounds_,o->flags_);
		
		if(o->options_[i].type_ == 0)
		{
			if(i == o->currOption_ && mod == 1.0)
			{
				if(o->highlighted_)
					render_text(o->options_[i].text_,o->options_[i].pos_,
						o->col_,o->offset_,o->fontSize_,mod,o->bounds_,o->options_[i].flags_);
				else
					render_text(o->options_[i].text_,o->options_[i].pos_,
						o->options_[i].col_,o->offset_,o->fontSize_,mod,o->bounds_,o->options_[i].flags_);
			}
			else
				render_text(o->options_[i].text_,o->options_[i].pos_,
						o->options_[i].col_,o->offset_,o->fontSize_,mod,o->bounds_,o->options_[i].flags_);
		}
		
		if((o->flags_ & TXT_MINIMIZE) && mod == 1.0)
		{
			o->offset_[1] += o->fontSize_/1.75*TXT_SIZE/5;
		}
		
		i = (i+1)%(o->nOptions_);
		count++;
	}
	/*for(int i = from_; i < from_+to_; i++)
	{
		if(!(o->flags_ & TXT_NORESET))
			memset(o->offset_,0,sizeof(float)*2);
		if(o->label_ != NULL)
			render_text(o->label_,o->options_[i].pos_,o->labelCol_,o->offset_,o->fontSize_,o->flags_);
		
		if(o->options_[i].type_ == 0)
		{
			if(i == o->currOption_)
			{
				if(o->highlighted_)
					render_text(o->options_[i].text_,o->options_[i].pos_,
						o->col_,o->offset_,o->fontSize_,o->options_[i].flags_);
				else
					render_text(o->options_[i].text_,o->options_[i].pos_,
						o->labelCol_,o->offset_,o->fontSize_,o->options_[i].flags_);
			}
			else
				render_text(o->options_[i].text_,o->options_[i].pos_,
						o->options_[i].col_,o->offset_,o->fontSize_,o->options_[i].flags_);
		}
		else
		{
			glDisable(GL_BLEND);
			o->offset_[0] = o->options_[i].pos_[0] + o->offset_[0] + text_->chars[0].width/2+text_->xBearing + 0.0125;
			o->offset_[1] = o->options_[i].pos_[1] + o->offset_[1];
			if(o->highlighted_)
				draw_square(o->offset_,o->col_,0.025,0);
			else
				draw_square(o->offset_,o->labelCol_,0.025,0);
			
			draw_square(o->offset_,(float[]){0.0,0.0,0.0,1.0},0.02,0);
			
			if(o->options_[i].type_ == 2)
			{
				if(o->highlighted_)
					draw_square(o->offset_,o->col_,0.015,0);
				else
					draw_square(o->offset_,(float[]){0.85,0.85,0.85,1.0},0.015,0);
			}

			glEnable(GL_BLEND);
		}
	}*/
}

void clear_options(options_struct* o)
{
	for(int i = 0; i < o->nOptions_; i++)
		free(o->options_[i].text_);
	
	if(o->options_ != NULL)
	{
		free(o->options_);
		o->options_ = NULL;
	}
	
	o->nOptions_ = 0;
}

void delete_options(options_struct* o)
{
	clear_options(o);
	if(o->label_ != NULL)
		free(o->label_);
	free(o->offset_);
	free(o);
}