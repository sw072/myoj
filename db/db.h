#ifndef _DB_
#define _DB_

#include "../judgerd.h"
#include "mysql.h"

int db_open(MYSQL ** db, char server[], char db_name[], char db_user[], char db_pwd[]);

int db_fetch_solutions(MYSQL *db, solution_t **pbuff, int *n);

int db_update_result(MYSQL *db, int sid, judge_result_t *result);

int db_close(MYSQL **db);

#endif
