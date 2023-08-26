#include "error.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ERROR_FILE "errorlog.txt"
#define LINE_BREAK "---------------------------------------------------------------------------------\n"

static void write_time(FILE* fp)
{
	time_t time_now = time(NULL);
	struct tm date = *localtime(&time_now);
		
	fprintf(fp,LINE_BREAK);
	fprintf(fp,"%02d/%02d/%d %02d:%02d:%02d\n",date.tm_mon+1,date.tm_mday,
				date.tm_year+1900,date.tm_hour,date.tm_min, date.tm_sec);
	fprintf(fp,LINE_BREAK);
}

void write_error(char* str, char* arg, int flag)
{
	FILE* fp = fopen(ERROR_FILE,"a");
	if(!fp)
		return;
	
	if(flag == 0)
		write_time(fp);
	
	if(arg == NULL)
		fprintf(fp,"%s\n",str);
	else
		fprintf(fp,"%s%s\n",str,arg);
	
	fclose(fp);
}

void write_error_int(char* str, int arg, int flag)
{
	FILE* fp = fopen(ERROR_FILE,"a");
	if(!fp)
		return;
	
	if(flag == 0)
		write_time(fp);
	
	fprintf(fp,"%s%d\n\n",str,arg);
	
	fclose(fp);
}