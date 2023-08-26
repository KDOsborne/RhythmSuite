#ifndef OPTIONS_H
#define OPTIONS_H

#include "../../framework.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Structs
struct option_
{
	char*		text_;
	float 		pos_[2];
	float 		col_[4];
	int			type_;
	int			flags_;
	void*		data_;
};

typedef struct options_struct
{
	struct option_*		options_;
	char*				label_;
	float 				labelCol_[4];
	int					nOptions_,currOption_;
	int					from_,total_;
	float				fontSize_,bounds_;
	int					highlighted_;
	float				col_[4];
	float*				offset_;
	int					flags_;
} options_struct;
////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Functions
options_struct* 	create_options(char*,int,float,int,int,int,int,int);
void 				add_option(options_struct*,char*,float,float,int,int,int,void*);
void				increment_option(options_struct*,int);
char*				get_option_text(options_struct*);
void*				get_option_data(options_struct*);
void 				draw_options(options_struct*);
void 				clear_options(options_struct*);
void 				delete_options(options_struct*);
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif //OPTIONS_H