#ifndef CHARTDATA_H
#define CHARTDATA_H

#include "../../framework.h"
#include <stdio.h>
#include <stdint.h>

#define HEADER_ONLY 1
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Structs
struct sv_data
{
	int32_t 			time;
	double				svtime;
	float				bpm;
	int					type;
	struct sv_data*		tail;
};

struct note_data
{
	int32_t 			time;
	float				hittime;
	double				svtime;
	struct note_data*	tail;
};

struct datafield
{
	char* 				name;
	char* 				value;
	struct datafield* 	tail;
};

typedef struct chart_data
{
	struct datafield*	infodata;
	struct sv_data*		svs;
	struct note_data**	notes;
	char*				name;
	int					nSvs;
	int*				nNotes;
}chart_data;

typedef struct chart_struct
{
	struct datafield*	headerdata;
	chart_data**		charts;
	char*				filename;
	float				version;
	int					nCharts;
	
}chart_struct;
////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Functions
chart_struct* 		create_chartstruct();
chart_struct* 		copy_chartstruct(chart_struct*);
void 				clear_chartstruct(chart_struct* cs);
void 				destroy_chartstruct(chart_struct*);

chart_data*			create_chartdata(char*);
int 				get_totalnotes(chart_data*);
void 				destroy_chartdata(chart_data*);

void 				add_datafield(struct datafield**,char*);
void 				update_datavalue(struct datafield*,char*,char*);
char* 				get_datavalue(struct datafield*,char*);
void 				clear_datavalues(struct datafield*);
void 				destroy_datafields(struct datafield*);

void 				parse_rst(char*,chart_struct*,int);
void 				parse_charts(FILE*,chart_struct*);
void 				parse_svs(FILE*,chart_data*);
void 				parse_notes(FILE*,chart_data*);
void 				parse_data(FILE*,struct datafield*);
int 				create_rstfile(char*,int,float);
void				update_rstfile(struct chart_struct*,char*);

void 				add_sv(chart_data*,uint32_t,int,float);
void 				add_note(chart_data*,int,uint32_t);
void 				destroy_svs(struct sv_data*);
void 				destroy_notes(struct note_data*);
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif //CHARTDATA_H