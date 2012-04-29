#include "db.h"
#include "../trace/trace.h"
#include <assert.h>

#define sqlstr "select submit.id,problem.pno,cno,scode,ptime,pmem from test.submit \
    join compiler on compiler.id=submit.scompiler join problem on problem.id=submit.pid \
    where sresultcode=0 order by id limit 10"

#define BUFF_MAX 32
static solution_t buff[BUFF_MAX];

int db_init(MYSQL ** db, char server[], char db_name[], char db_user[], char db_pwd[])
{
    *db = mysql_init(NULL);
    if(!mysql_real_connect(*db, server, db_user, db_pwd, db_name, 0, NULL, 0))
    {
        __TRACE_LN(__TRACE_KEY, "%p;server:%s;db:%s;user:%s;pwd:%s", *db, server, db_name, db_user, db_pwd);
        __TRACE_LN(__TRACE_KEY, "oops : Database connect error.");
        return -1;
    }
    return 0;
}

int db_fetch_solutions(MYSQL *db, solution_t **pbuff, int *n)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    mysql_query(db, sqlstr);
    if(!(res = mysql_store_result(db)))
    {
        __TRACE_LN(__TRACE_KEY, "oops : mysql_store_result failed.");
        return -1;
    }
    int idx = 0;
    while(row = mysql_fetch_row(res))
    {
        buff[idx].run_id = atoi(row[0]);
        buff[idx].problem_id = atoi(row[1]);
        buff[idx].compiler = (compiler_type_t)atoi(row[2]);
        strcpy(buff[idx].src, row[3]);
        buff[idx].quota_wallclock = 5000;
        buff[idx].quota_cputime = atoi(row[4]);
        buff[idx].quota_memory = atoi(row[5]);
        buff[idx].quota_output = 32 * 1024 * 1024;
        idx++;
    }
    mysql_free_result(res);
    *pbuff = &buff[0];
    *n = idx;
    __TRACE_LN(__TRACE_DBG, "got %d solutions", idx);
    return 0;
}

int db_fini(MYSQL **db)
{
    mysql_close(*db);
    return 0;
}
