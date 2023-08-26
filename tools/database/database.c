#include "database.h"
#include "../chartdata/chartdata.h"
#include "../diffcalc/diffcalc.h"
#include "../grade/grade.h"

#include <windows.h>
#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define SCOREDB_FILE "rst-scores.db"
#define SCOREDB_CLONE "rst-scores_temp.db"

#define DATABASE_VERSION 1.00

struct score_list {
	score_struct score;
	struct score_list* head;
	struct score_list* tail;
};

static void insert_score(struct score_list** list, score_struct* score)
{
	struct score_list* sl = (struct score_list*)malloc(sizeof(struct score_list));
	memcpy(&sl->score,score,sizeof(score_struct));
	sl->head = NULL;
	sl->tail = NULL;
	
	if(*list == NULL)
	{
		*list = sl;
		return;
	}
	
	struct score_list* iter = *list; 
	
	while(iter != NULL)
	{
		if(score->points > iter->score.points)
		{
			if(iter->head == NULL)
			{
				*list = sl;
				sl->head = NULL;
				sl->tail = iter;
				iter->head = sl;
				return;
			}
			else
			{
				iter->head->tail = sl;
				sl->head = iter->head;
				sl->tail = iter;
				iter->head = sl;
				return;
			}  
		}
		if(iter->tail == NULL)
		{
			iter->tail = sl;
			sl->head = iter;
			sl->tail = NULL;
			return;
		}
		iter = iter->tail;
	}
}

static void free_list(struct score_list* list)
{
	if(list != NULL)
	{
		free_list(list->tail);
		free(list);
	}
}

static void database_create()
{
	FILE* fp = fopen(SCOREDB_FILE,"wb");
	if(!fp)
	{
		write_error(strerror(errno),NULL,0);
		return;
	}
	
	float version = DATABASE_VERSION;
	int64_t t = 0, n = 0;
	fwrite(&version,sizeof(float),1,fp);
	fwrite(&t,sizeof(int64_t),1,fp);
	fwrite(&n,sizeof(int64_t),1,fp);
	
	fclose(fp);
}

static struct score_list* get_scorelist()
{
	FILE* fp = fopen(SCOREDB_FILE,"rb");
	float version = 0.f;
	
	if(!fp)
	{
		database_create();
		return NULL;
	}
	
	score_struct* scores = (score_struct*)malloc(sizeof(score_struct));
	score_struct score;
	int count = 0;
	
	memset(scores,0,sizeof(score_struct));
	
	fread(&version,sizeof(float),1,fp);
	fseek(fp,sizeof(int64_t),SEEK_CUR);
	fseek(fp,sizeof(int64_t),SEEK_CUR);
	
	while(fread(&score,sizeof(score_struct),1,fp))
	{
		if(score.deviation > FAIL_ACC)
			continue;
		
		for(int i = 0; i < count+1; i++)
		{
			if(score.id == scores[i].id)
			{
				if(score.points > scores[i].points)
					memcpy(&scores[i],&score,sizeof(score_struct));
				break;
			}
			else if(i == count)
			{
				scores = (score_struct*)realloc(scores,sizeof(score_struct)*(count+2));
				memset(&scores[count+1],0,sizeof(score_struct));
				memcpy(&scores[count],&score,sizeof(score_struct));
				count++;
				break;
			}
		}	
	}
	
	fclose(fp);

	struct score_list* list = NULL;
	for(int i = 0; i < count; i++)
	{
		insert_score(&list,&scores[i]);
	}
	
	free(scores);
	
	return list;
}

void database_addscore(score_struct* score)
{
	FILE* fp = fopen(SCOREDB_FILE,"ab");
	
	if(!fp)
	{
		write_error(strerror(errno),NULL,0);
		return;
	}

	fwrite(score,sizeof(score_struct),1,fp);
	fclose(fp);
}

void database_removescore(score_struct* s)
{
	FILE* fp = fopen(SCOREDB_FILE,"rb");
	
	if(!fp)
	{
		write_error(strerror(errno),NULL,0);
		return;
	}
	
	FILE* wp = fopen(SCOREDB_CLONE,"wb");
	
	score_struct score;
	float version;
	int64_t t = 0, n = 0;
	
	fread(&version,sizeof(float),1,fp);
	fread(&t,sizeof(int64_t),1,fp);
	fread(&n,sizeof(int64_t),1,fp);
	
	fwrite(&version,sizeof(float),1,wp);
	fwrite(&t,sizeof(int64_t),1,wp);
	fwrite(&n,sizeof(int64_t),1,wp);
	
	while(fread(&score,sizeof(score_struct),1,fp) == 1)
	{
		if(compare_identifier(&score,s))
			continue;
		fwrite(&score,sizeof(score_struct),1,wp);
	}

	fclose(wp);
	fclose(fp);
	
	if(remove(SCOREDB_FILE) == -1)
	{
		write_error(strerror(errno),NULL,0);
		remove(SCOREDB_CLONE);
	}
	else
		rename(SCOREDB_CLONE,SCOREDB_FILE);
}

score_struct database_bestscore(unsigned int id, int flag)
{
	FILE* fp = fopen(SCOREDB_FILE,"rb");
	float version = 0.f;
	
	score_struct score;
	memset(&score,0,sizeof(score_struct));
	
	if(!fp)
	{
		database_create();
		return score;
	}
	
	score_struct tempscore;
	memset(&tempscore,0,sizeof(score_struct));
	
	float deviation = 100.0;
	unsigned int points = 0;
	
	fread(&version,sizeof(float),1,fp);
	fseek(fp,sizeof(int64_t),SEEK_CUR);
	fseek(fp,sizeof(int64_t),SEEK_CUR);
	
	while(fread(&tempscore,sizeof(score_struct),1,fp))
	{
		if(tempscore.id == id)
		{
			switch (flag)
			{
				case 0:
					if(tempscore.deviation < deviation)
					{
						deviation = tempscore.deviation;
						score = tempscore;
					}
				break;
				
				case 1:
					if(tempscore.points+1 > points)
					{
						points = tempscore.points+1;
						score = tempscore;
					}
				break;
			}
		}
	}
	
	fclose(fp);
	
	return score;
}

score_struct* database_getscores(unsigned int id, int *n, score_struct* front)
{
	FILE* fp = fopen(SCOREDB_FILE,"rb");
	float version = 0.f;
	
	if(!fp)
	{
		database_create();
		return NULL;
	}
	
	score_struct* scores = NULL;
	score_struct score;
	int nscores = 0;
	
	if(front != NULL)
	{
		scores = (score_struct*)malloc(sizeof(score_struct));
		memcpy(scores,front,sizeof(score_struct));
		nscores = 1;
	}
	
	fread(&version,sizeof(float),1,fp);
	fseek(fp,sizeof(int64_t),SEEK_CUR);
	fseek(fp,sizeof(int64_t),SEEK_CUR);
	
	while(fread(&score,sizeof(score_struct),1,fp))
	{
		if(score.id == id)
		{
			scores = (score_struct*)realloc(scores,sizeof(score_struct)*(nscores+1));
			memcpy(scores+nscores,&score,sizeof(score_struct));
			nscores++;
		}
	}
	
	fclose(fp);
	
	*n = nscores;
	
	for(int i = 0; i < nscores; i++)
	{
		if(front != NULL && i == 0)
			continue;
		
		for(int j = i+1; j < nscores; j++)
		{
			if(scores[j].points > scores[i].points || (scores[j].points == scores[i].points && 
					scores[j].deviation < scores[i].deviation))
			{
				memcpy(&score,scores+j,sizeof(score_struct));
				memcpy(scores+j,scores+i,sizeof(score_struct));
				memcpy(scores+i,&score,sizeof(score_struct));
			}
		}
	}
	
	return scores;
}

score_struct* database_topscores(int nscores)
{
	struct score_list* list = get_scorelist();
	
	score_struct* results = (score_struct*)malloc(sizeof(score_struct)*nscores);
	memset(results,0,sizeof(score_struct)*nscores);
	
	struct score_list* iter = list;
	int n = 0;
	
	while(iter != NULL && n < nscores)
	{
		memcpy(&results[n],&iter->score,sizeof(score_struct));
		iter = iter->tail;
		n++;
	}
	
	free_list(list);

	return results;
}

int db_playerrank()
{
	struct score_list* list = get_scorelist();
	struct score_list* iter = list;
	int rank = 0, n = 0;
	
	while(iter != NULL)
	{
		rank += iter->score.points*pow(0.95,n);
		iter = iter->tail;
		n++;
	}
	
	free_list(list);
	
	return rank;
}

int compare_identifier(score_struct* score1, score_struct* score2)
{
	struct tm t1 = score1->date;
	struct tm t2 = score2->date;
	
	return ((t1.tm_year==t2.tm_year)&&(t1.tm_mon==t2.tm_mon)&&(t1.tm_mday==t2.tm_mday)
				&&(t1.tm_hour==t2.tm_hour)&&(t1.tm_sec==t2.tm_sec)
					&&(score1->points==score2->points)&&(score1->miss==score2->miss)
						&&(score1->result==score2->result));
}

void dbupdate_stat(int64_t n, int flag)
{
	FILE* fp = fopen(SCOREDB_FILE,"rb+");
	float version = 0.f;
	int64_t x = 0;
	
	if(!fp)
	{
		database_create();
		return;
	}
	
	fread(&version,sizeof(float),1,fp);
	if(flag)
		fseek(fp,sizeof(int64_t),SEEK_CUR);
	fread(&x,sizeof(int64_t),1,fp);
	
	x += n;
	
	fseek(fp,sizeof(float),SEEK_SET);
	if(flag)
		fseek(fp,sizeof(int64_t),SEEK_CUR);
	fwrite(&x,sizeof(int64_t),1,fp);
	
	fclose(fp);
}

int64_t dbget_stat(int flag)
{
	FILE* fp = fopen(SCOREDB_FILE,"rb");
	float version = 0.f;
	int64_t x = 0;
	
	if(!fp)
	{
		database_create();
		return x;
	}
	
	fread(&version,sizeof(float),1,fp);
	if(flag)
		fseek(fp,sizeof(int64_t),SEEK_CUR);
	fread(&x,sizeof(int64_t),1,fp);
	
	fclose(fp);
	
	return x;
}

int songdb_create()
{
	DIR *d;
    struct dirent* dir;
	chart_struct* chartStruct_ = create_chartstruct();
	
	FILE* fp = fopen(SONGDB_FILE,"wb");

    d = opendir("./songs");
	
    if (d)
    {
		char songfile[256], directory[256];
		
		memset(songfile,0,sizeof(songfile));
		memset(directory,0,sizeof(directory));
		
        while ((dir = readdir(d)) != NULL)
        {
			if(strstr(dir->d_name, "."))
				continue;
			else
			{
				DIR* songdir;
				
				sprintf(directory,"./songs/%s",dir->d_name);
				
				songdir = opendir(directory);
				
				if(songdir)
				{
					while ((dir = readdir(songdir)) != NULL)
					{
						if(strstr(dir->d_name, ".rst"))
						{
							sprintf(songfile,"%s/%s",directory,dir->d_name);
							
							parse_rst(songfile,chartStruct_,0);
							
							char* title = get_datavalue(chartStruct_->headerdata,"TITLE");
							if(title == NULL)
							{
								write_error("WARNING::COULDNT FIND FILE: ",songfile,0);
								continue;
							}
							
							int id = atoi(get_datavalue(chartStruct_->headerdata,"TRACKID"));
							int tsize = strlen(title)+1;
							int fsize = strlen(dir->d_name)+1;
							int dsize = strlen(directory)+1;
							int strsize = tsize+fsize+dsize+sizeof(int)*3;
							
							float difficulty = run_diffcalc(chartStruct_->charts[0],1.0);
							
							fwrite(&id,sizeof(int),1,fp);
							fwrite(&difficulty,sizeof(float),1,fp);
							fwrite(&strsize,sizeof(int),1,fp);
							
							for(int i = 0; i < tsize-1; i++)
								title[i] = toupper(title[i]);
							
							fwrite(&tsize,sizeof(int),1,fp);
							fwrite(title,sizeof(char)*tsize,1,fp);
							
							for(int i = 0; i < fsize-1; i++)
								dir->d_name[i] = toupper(dir->d_name[i]);
							
							fwrite(&fsize,sizeof(int),1,fp);
							fwrite(dir->d_name,sizeof(char)*fsize,1,fp);
							
							for(int i = 0; i < dsize-1; i++)
								directory[i] = toupper(directory[i]);
							
							fwrite(&dsize,sizeof(int),1,fp);
							fwrite(directory,sizeof(char)*dsize,1,fp);
							
							//sprintf(songfile,"%s",songfile);
							for(int i = 0; i < strlen(songfile); i++)
								songfile[i] = toupper(songfile[i]);
							
							glClear(GL_COLOR_BUFFER_BIT);
							render_simpletext("INITIALIZING DATABASE...",0.0,0.0,WHITE_,5,TXT_CENTERED,NULL);
							render_simpletext(songfile,0.0,-0.1,WHITE_,5,TXT_CENTERED,NULL);
							if(video_->settings_[SCREEN_BUFFER].value_)
								SwapBuffers(video_->hDC);
							else
								glFlush();
						}
					}
				}
				else
				{
					write_error("WARNING::UNABLE TO OPEN DIRECTORY: ",directory,0);
					write_error(strerror(errno),NULL,1);
					continue;
				}
			}
        }
        closedir(d);
    }
	else
	{
		write_error("ERROR::UNABLE TO OPEN SONGS FOLDER",NULL,0);
		write_error(strerror(errno),NULL,1);
		fclose(fp);
		if(chartStruct_ != NULL)
			destroy_chartstruct(chartStruct_);
		
		return -1;
	}
	
	fclose(fp);
	if(chartStruct_ != NULL)
		destroy_chartstruct(chartStruct_);
	
	return 0;
}

int songdb_check()
{
	FILE* fp = fopen(SONGDB_FILE,"rb");
	if(!fp)	
		return songdb_create();
	
	fclose(fp);
	
	return 0;
}

char* songdb_getname(int tid)
{
	FILE* fp = fopen(SONGDB_FILE,"rb");
	if(!fp)	
		return NULL;
	
	int id,skip;
	char* result = NULL;
	
	while(!feof(fp))
	{
		if(fread(&id,sizeof(int),1,fp) != 1)
			break;
		
		fseek(fp,sizeof(float),SEEK_CUR);
		
		if(fread(&skip,sizeof(int),1,fp) != 1)
			break;
		
		if(id == tid)
		{
			int n;
			if(fread(&n,sizeof(int),1,fp) != 1)
				break;
			
			result = (char*)malloc(n);
			fread(result,n,1,fp);
			break;
		}
		else
			fseek(fp,skip,SEEK_CUR);
	}
	
	fclose(fp);
	
	return result;
}

float songdb_getdiff(int tid)
{
	FILE* fp = fopen(SONGDB_FILE,"rb");
	if(!fp)	
		return -1;
	
	int id,skip;
	float diff,result = -1;
	while(!feof(fp))
	{
		if(fread(&id,sizeof(int),1,fp) != 1)
			break;
		if(fread(&diff,sizeof(float),1,fp) != 1)
			break;
		if(fread(&skip,sizeof(int),1,fp) != 1)
			break;
		
		if(id == tid)
		{
			result = diff;
			break;
		}
		else
			fseek(fp,skip,SEEK_CUR);
		
	}
	
	fclose(fp);
	
	return result;
}

int songdb_getcount()
{
	FILE* fp = fopen(SONGDB_FILE,"rb");
	if(!fp)	
		return -1;
	
	int id,skip,count = 0;
	
	while(!feof(fp))
	{
		if(fread(&id,sizeof(int),1,fp) != 1)
			break;
		
		fseek(fp,sizeof(float),SEEK_CUR);
		
		if(fread(&skip,sizeof(int),1,fp) != 1)
			break;
		
		fseek(fp,skip,SEEK_CUR);
		count++;
	}
	
	fclose(fp);
	
	return count;
}