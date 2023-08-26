#ifndef DATABASE_H
#define DATABASE_H

#include "../../framework.h"
#include <time.h>

#define SONGDB_FILE "rst-tracks.db"

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Structs
typedef struct score_struct
{
	unsigned int 	id,points,miss,result;
	float 			rate,deviation;
	struct tm 		date;
} score_struct;
////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Functions
void 			database_addscore(score_struct*);
void 			database_removescore(score_struct*);
score_struct 	database_bestscore(unsigned int,int);
score_struct* 	database_getscores(unsigned int,int*,score_struct*);
score_struct* 	database_topscores(int);
int 			db_playerrank();
int 			compare_identifier(score_struct*,score_struct*);
void 			dbupdate_stat(int64_t,int);
int64_t 		dbget_stat(int);
int 			songdb_create();
int 			songdb_check();
char* 			songdb_getname(int);
float 			songdb_getdiff(int tid);
int 			songdb_getcount();
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif //DATABASE_H