#ifndef _DB_
#define _DB_

#include "../judgerd.h"
#include "mysql.h"

int db_init(MYSQL ** db, char server[], char db_name[], char db_user[], char db_pwd[]);

int db_fetch_solutions(MYSQL *db, solution_t **pbuff, int *n);

int db_fini(MYSQL **db);

#endif
