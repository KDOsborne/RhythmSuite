#include "chartdata.h"
#include <string.h>
#include <stdlib.h>

chart_struct* create_chartstruct()
{
	chart_struct* cs = (chart_struct*)malloc(sizeof(chart_struct));
	memset(cs,0,sizeof(chart_struct));
	
	cs->headerdata = NULL;
	cs->charts = NULL;
	cs->filename = NULL;
	
	add_datafield(&(cs->headerdata),"TITLE");
	add_datafield(&(cs->headerdata),"TRACKID");
	add_datafield(&(cs->headerdata),"AUDIO");
	add_datafield(&(cs->headerdata),"PREVIEW");
	add_datafield(&(cs->headerdata),"IMAGE");
	
	return cs;
}

void clear_chartstruct(chart_struct* cs)
{
	clear_datavalues(cs->headerdata);
	for(int i = 0; i < cs->nCharts; i++)
		destroy_chartdata(cs->charts[i]);
	cs->charts = NULL;
	cs->nCharts = 0;
}

void destroy_chartstruct(chart_struct* cs)
{
	if(cs != NULL)
	{
		destroy_datafields(cs->headerdata);
		for(int i = 0; i < cs->nCharts; i++)
			destroy_chartdata(cs->charts[i]);
		if(cs->filename != NULL)
			free(cs->filename);
		free(cs);
	}
}

chart_data* create_chartdata(char* name)
{
	chart_data* cd = (chart_data*)malloc(sizeof(chart_data));
	memset(cd,0,sizeof(chart_data));
	
	cd->infodata = NULL;
	cd->svs = NULL;
	cd->notes = NULL;
	cd->nNotes = NULL;
	
	cd->name = (char*)malloc(sizeof(char)*(strlen(name)+1));
	memcpy(cd->name,name,sizeof(char)*(strlen(name)+1));
	
	for(int i = 0; i < strlen(cd->name)+1; i++)
		cd->name[i] = toupper(cd->name[i]);
	
	add_datafield(&(cd->infodata),"LINES");
	add_datafield(&(cd->infodata),"CREATOR");
	
	return cd;
}

int get_totalnotes(chart_data* cd)
{
	int total = 0, nLines;
	
	nLines = atoi(get_datavalue(cd->infodata,"LINES"));
	
	for(int i = 0; i < nLines; i++)
		total += cd->nNotes[i];
	
	return total;
}

void destroy_chartdata(chart_data* cd)
{
	int nLines = atoi(get_datavalue(cd->infodata,"LINES"));
	if(nLines)
	{
		for(int i = 0; i < nLines; i++)
			destroy_notes(cd->notes[i]);
		free(cd->nNotes);
	}
	if(cd->nSvs)
		destroy_svs(cd->svs);
	destroy_datafields(cd->infodata);
	
	free(cd->name);
	free(cd);
}

void add_datafield(struct datafield** d,char* name)
{
	struct datafield* field = (struct datafield*)malloc(sizeof(struct datafield));
	memset(field,0,sizeof(struct datafield));
	
	field->name = (char*)malloc(sizeof(char)*(strlen(name)+1));
	memcpy(field->name,name,sizeof(char)*(strlen(name)+1));
	
	field->value = NULL;
	
	if((*d) == NULL)
	{
		*d = field;
		return;
	}
	
	struct datafield* currfield = (*d);
	
	while(currfield != NULL)
	{
		if(currfield->tail == NULL)
		{
			currfield->tail = field;
			return;
		}
		currfield = currfield->tail;
	}
}

void update_datavalue(struct datafield* d,char* name,char* value)
{
	struct datafield* currfield = d;
	
	while(currfield != NULL)
	{
		if(strstr(name,currfield->name))
		{
			currfield->value = (char*)malloc(sizeof(char)*(strlen(value)+1));
			memcpy(currfield->value,value,sizeof(char)*(strlen(value)+1));
			return;
		}
		currfield = currfield->tail;
	}
}

char* get_datavalue(struct datafield* d,char* name)
{
	struct datafield* currfield = d;
	
	while(currfield != NULL)
	{
		if(strstr(name,currfield->name))
			return currfield->value;
		currfield = currfield->tail;
	}
	
	return NULL;
}

void clear_datavalues(struct datafield* d)
{
	struct datafield* currfield = d;
	
	while(currfield != NULL)
	{
		if(currfield->value != NULL)
		{
			free(currfield->value);
			currfield->value = NULL;
		}
		currfield = currfield->tail;
	}
}

void destroy_datafields(struct datafield* d)
{
	if(d != NULL)
	{
		destroy_datafields(d->tail);
		free(d->name);
		free(d->value);
		free(d);
	}
}

void parse_rst(char* filename,chart_struct* cs, int flag)
{
	FILE*	fp;
	char 	buffer[64];
	float 	version;
	
	if(cs->filename != NULL)
		free(cs->filename);
	cs->filename = (char*)malloc(strlen(filename)+1);
	memcpy(cs->filename,filename,strlen(filename)+1);
	
	fp = fopen(filename,"r");
	if(!fp)
	{
		write_error("WARNING::CAN'T OPEN FILE: ",filename,0);
		write_error(strerror(errno),NULL,1);
		return;
	}
	
	if(flag == HEADER_ONLY)
		clear_datavalues(cs->headerdata);
	else
		clear_chartstruct(cs);
	
	fscanf(fp,"v%f",&version);
	
	while(fgets(buffer,sizeof(buffer),fp))
	{
		if(strstr(buffer,"#HEADER"))
		{
			parse_data(fp,cs->headerdata);
			if(flag == HEADER_ONLY)
				break;
		}
		else if(strstr(buffer,"#CHARTS"))
			parse_charts(fp,cs);
	}
	
	fclose(fp);
	
	/*printf("%s\n",get_datavalue(cs->headerdata,"TITLE"));
	for(int i = 0; i < cs->nCharts; i++)
	{
		printf("%s\n",cs->charts[i]->name);
		printf("SVS: %d\n",cs->charts[i]->nSvs-1);
		printf("NOTES: %d\n",get_totalnotes(cs->charts[i]));
	}*/
}

void parse_charts(FILE* fp,chart_struct* cs)
{
	char buffer[64];
	
	chart_data* currchart = NULL;
	
	while(fgets(buffer,sizeof(buffer),fp))
	{
		if(buffer[0] == '\n')
			break;
		if(strstr(buffer,"$"))
		{
			char* newline = strchr(buffer,'\n');
			if(newline)
				newline[0] = '\0';
			
			cs->charts = (chart_data**)realloc(cs->charts,sizeof(chart_data*)*(cs->nCharts+1));
			cs->charts[cs->nCharts] = create_chartdata(buffer+1);
			currchart = cs->charts[cs->nCharts];
			cs->nCharts++;
		}
		else if(strstr(buffer,"@INFO"))
		{
			parse_data(fp,currchart->infodata);
			
			int nLines = atoi(get_datavalue(currchart->infodata,"LINES"));
			if(nLines)
			{
				currchart->notes = (struct note_data**)malloc(sizeof(struct note_data*)*nLines);
				currchart->nNotes = (int*)malloc(sizeof(int)*nLines);
				for(int i = 0; i < nLines; i++)
				{
					currchart->notes[i] = NULL;
					currchart->nNotes[i] = 0;
				}
			}
		}
		else if(strstr(buffer,"@SVS"))
			parse_svs(fp,currchart);
		else if(strstr(buffer,"@NOTES"))
			parse_notes(fp,currchart);
	}
}

void parse_svs(FILE* fp,chart_data* cd)
{
	char buffer[64];
	int32_t t;
	int type;
	float bpm;
	
	while(fgets(buffer,sizeof(buffer),fp))
	{
		if(buffer[0] == '\n')
			break;
		sscanf(buffer,"%d,%d,%f",&t,&type,&bpm);
		add_sv(cd,t,type,bpm);
	}
}

void parse_notes(FILE* fp,chart_data* cd)
{
	char buffer[64];
	int32_t t;
	int line,nLines;
	
	nLines = atoi(get_datavalue(cd->infodata,"LINES"));
	
	while(fgets(buffer,sizeof(buffer),fp))
	{
		if(buffer[0] == '\n')
			break;
		sscanf(buffer,"%d,%u",&line,&t);
		for(int i = 0; i < nLines; i++)
		{
			if((line >> i) & 1)
				add_note(cd,i,t);
		}
		
	}
}

void parse_data(FILE* fp,struct datafield* data)
{
	char buffer[128],name[32],value[64];
	
	while(fgets(buffer,sizeof(buffer),fp))
	{
		if(buffer[0] == '\n')
			break;
		
		char* tok = strtok(buffer,":\n");
		sprintf(name,"%s",tok);
		
		tok = strtok(NULL,":\n");
		sprintf(value,"%s",tok);

		update_datavalue(data,name,value);
	}
}

void add_sv(chart_data* cd, uint32_t t, int type, float bpm)
{
	struct sv_data* sd = malloc(sizeof(struct sv_data));
	memset(sd,0,sizeof(struct sv_data));
	sd->time = t;
	sd->bpm = bpm;
	sd->type = type;
	sd->tail = NULL;
	
	cd->nSvs++;
	
	if(cd->svs == NULL)
	{
		cd->svs = sd;
		return;
	}
	
	struct sv_data* currsv = cd->svs;
	while(currsv != NULL)
	{
		if(currsv->tail == NULL)
		{
			currsv->tail = sd;
			return;
		}
		currsv = currsv->tail;
	}
}

void add_note(chart_data* cd, int line, uint32_t t)
{
	struct note_data* nd = malloc(sizeof(struct note_data));
	memset(nd,0,sizeof(struct note_data));
	nd->time = t;
	nd->hittime = -999999999;
	nd->svtime = -1;
	nd->tail = NULL;
	
	cd->nNotes[line]++;
	
	if(cd->notes[line] == NULL)
	{
		cd->notes[line] = nd;
		return;
	}
	
	struct note_data* currnote = cd->notes[line];
	while(currnote != NULL)
	{
		if(currnote->tail == NULL)
		{
			currnote->tail = nd;
			return;
		}
		currnote = currnote->tail;
	}
}

void destroy_svs(struct sv_data* sd)
{
	while(sd != NULL)
	{
		struct sv_data* sv = sd;
		sd = sd->tail;
		free(sv);
	}
}

void destroy_notes(struct note_data* nd)
{
	while(nd != NULL)
	{
		struct note_data* note = nd;
		nd = nd->tail;
		free(note);
	}
}

chart_struct* copy_chartstruct(chart_struct* copy)
{
	if(copy == NULL)
		return NULL;
	
	chart_struct* cs = create_chartstruct();
	
	cs->filename = (char*)malloc(strlen(copy->filename)+1);
	memcpy(cs->filename,copy->filename,strlen(copy->filename)+1);
	
	char* title = get_datavalue(copy->headerdata,"TITLE");
	char* trackid = get_datavalue(copy->headerdata,"TRACKID");
	char* audio = get_datavalue(copy->headerdata,"AUDIO");
	char* preview = get_datavalue(copy->headerdata,"PREVIEW");
	char* image = get_datavalue(copy->headerdata,"IMAGE");
	
	update_datavalue(cs->headerdata,"TITLE",title);
	update_datavalue(cs->headerdata,"TRACKID",trackid);
	update_datavalue(cs->headerdata,"AUDIO",audio);
	update_datavalue(cs->headerdata,"PREVIEW",preview);
	update_datavalue(cs->headerdata,"IMAGE",image);
	
	for(int i = 0; i < copy->nCharts; i++)
	{
		cs->charts = (chart_data**)realloc(cs->charts,sizeof(chart_data*)*(cs->nCharts+1));
		cs->charts[cs->nCharts] = create_chartdata(copy->charts[i]->name);
		cs->nCharts++;
	}
	
	for(int i = 0; i < cs->nCharts; i++)
	{
		char* creator = get_datavalue(copy->charts[i]->infodata,"CREATOR");
		char* lines = get_datavalue(copy->charts[i]->infodata,"LINES");
		int nLines = atoi(lines);
		
		update_datavalue(cs->charts[i]->infodata,"CREATOR",creator);
		update_datavalue(cs->charts[i]->infodata,"LINES",lines);
		
		cs->charts[i]->notes = (struct note_data**)malloc(sizeof(struct note_data*)*nLines);
		cs->charts[i]->nNotes = (int*)malloc(sizeof(int)*nLines);

		struct sv_data* svs = copy->charts[i]->svs;
		while(svs != NULL)
		{
			add_sv(cs->charts[i],svs->time,svs->type,svs->bpm);
			svs = svs->tail;
		}
		
		for(int j = 0; j < nLines; j++)
		{
			struct note_data* notes = copy->charts[i]->notes[j];
			cs->charts[i]->notes[j] = NULL;
			cs->charts[i]->nNotes[j] = 0;
			
			while(notes != NULL)
			{
				add_note(cs->charts[i],j,notes->time);
				notes = notes->tail;
			}
		}
	}
	
	return cs;
}

int create_rstfile(char* title, int trackid, float bpm)
{
	char mapfile[MAX_PATH];
	
	sprintf(mapfile,"./Songs/%d %s/chart.rst",trackid,title);
	
	FILE *fp = fopen(mapfile,"w");
	if(!fp)
	{
		write_error(strerror(errno),NULL,0);
		return -1;
	}
	
	fprintf(fp,"v1.0\n\n");
	fprintf(fp,"#HEADER\n");
	fprintf(fp,"TITLE:%s\n",title);
	fprintf(fp,"TRACKID:%d\n",trackid);
	fprintf(fp,"AUDIO:audio.mp3\n");
	fprintf(fp,"PREVIEW:0\n");
	fprintf(fp,"IMAGE:bg.jpg\n\n");
	
	fprintf(fp,"#CHARTS\n");
	fprintf(fp,"$1\n");
	fprintf(fp,"@INFO\n");
	fprintf(fp,"LINES:2\n");
	fprintf(fp,"CREATOR:GUEST\n");
	fprintf(fp,"\n@SVS\n");
	fprintf(fp,"-1,1,%.4f\n",bpm);
	
	fprintf(fp,"\n@NOTES\n");
	fprintf(fp,"3,0\n");
	fclose(fp);
	
	return 0;
}

void update_rstfile(chart_struct* cs, char* filename)
{
	if(cs->filename == NULL && filename == NULL)
		return;
	
	char* title = get_datavalue(cs->headerdata,"TITLE");
	char* trackid = get_datavalue(cs->headerdata,"TRACKID");
	char* audio = get_datavalue(cs->headerdata,"AUDIO");
	char* preview = get_datavalue(cs->headerdata,"PREVIEW");
	char* image = get_datavalue(cs->headerdata,"IMAGE");
	
	chart_data* cd = cs->charts[0];
	
	char* lines = get_datavalue(cd->infodata,"LINES");
	char* creator = get_datavalue(cd->infodata,"CREATOR");
	
	if(!title || !trackid || !audio || !preview || !image || !lines || !creator)
		return;
	
	int nLines = atoi(lines);
	
	FILE *fp;
	if(filename == NULL)
		fp = fopen(cs->filename,"w");
	else
		fp = fopen(filename,"w");
	
	fprintf(fp,"v1.0\n\n");
	fprintf(fp,"#HEADER\n");
	fprintf(fp,"TITLE:%s\n",title);
	fprintf(fp,"TRACKID:%s\n",trackid);
	fprintf(fp,"AUDIO:%s\n",audio);
	fprintf(fp,"PREVIEW:%s\n",preview);
	fprintf(fp,"IMAGE:%s\n\n",image);
	
	fprintf(fp,"#CHARTS\n");
	fprintf(fp,"$%s\n",cd->name);
	fprintf(fp,"@INFO\n");
	fprintf(fp,"LINES:%d\n",nLines);
	fprintf(fp,"CREATOR:%s\n",creator);
	fprintf(fp,"\n@SVS\n");
	
	struct sv_data* svs = cd->svs;
	fprintf(fp,"%d,1,%f\n", svs->time, svs->bpm);
	svs = svs->tail;
	while(svs != NULL)
	{
		fprintf(fp,"%d,%d,%f\n", svs->time, svs->type, svs->bpm);
		svs = svs->tail;
	}
	
	fprintf(fp,"\n@NOTES\n");
	
	struct note_data* notes[nLines];
	int nNotes = 0, count = 0;
	
	for(int i = 0; i < nLines; i++)
	{
		nNotes += cd->nNotes[i];
		notes[i] = cd->notes[i];
	}
	
	while(count < nNotes)
	{
		int ind = 0, nextnote = 99999999;
		for(int i = 0; i < nLines; i++)
		{
			if(notes[i] != NULL)
			{
				if(notes[i]->time < nextnote)
				{
					if(ind != 0 && abs(notes[i]->time-nextnote) < 3)
						ind |= (1 << i);
					else
						ind = (1 << i);
					nextnote = notes[i]->time;
				}
				else if(abs(notes[i]->time-nextnote) < 3)
				{
					ind |= (1 << i);
				}
			}
		}
		fprintf(fp,"%d,%d\n",ind,nextnote);
		for(int i = 0; i < nLines; i++)
		{
			if(notes[i] != NULL && ((ind >> i) & 1))
			{
				notes[i] = notes[i]->tail;
				count++;
			}
		}
	}
	
	fclose(fp);
}