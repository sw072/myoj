#include "db.h"
#include "../trace/trace.h"
#include <assert.h>

#define sqlstr "select web_submit.id,web_problem.no,web_compiler.sequence,code,timelmt,memlmt from web_submit join web_compiler on web_compiler.id=web_submit.compiler_id join web_problem on web_problem.id=problem_id where resultcode=0 order by web_submit.id limit 10"

#define BUFF_MAX 32

extern char *result_str[];
static solution_t buff[BUFF_MAX];

int db_open(MYSQL ** db, char server[], char db_name[], char db_user[], char db_pwd[])
{
    *db = mysql_init(NULL);
    if(!mysql_real_connect(*db, server, db_user, db_pwd, db_name, 0, NULL, 0))
    {
        __TRACE_LN(__TRACE_KEY, "%p;server:%s;db:%s;user:%s;pwd:%s", *db, server, db_name, db_user, db_pwd);
        __TRACE_LN(__TRACE_KEY, "oops : Database connect failed.");
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
    while((row = mysql_fetch_row(res)))
    {
        buff[idx].run_id = atoi(row[0]);
        buff[idx].problem_id = atoi(row[1]);
        buff[idx].compiler = (compiler_type_t)atoi(row[2]);
        strcpy(buff[idx].src, row[3]);
        buff[idx].quota_cputime = atoi(row[4]);
        buff[idx].quota_wallclock = buff[idx].quota_cputime<<1;
        buff[idx].quota_memory = atoi(row[5]) * 1024;
        buff[idx].quota_output = 32 * 1024 * 1024;
        idx++;
    }
    mysql_free_result(res);
    *pbuff = &buff[0];
    *n = idx;
    __TRACE_LN(__TRACE_DBG, "got %d solutions", idx);
    return 0;
}

int db_update_result(MYSQL *db, int sid, judge_result_t *result)
{
    static char update_sql[1024];
    sprintf(update_sql, "update web_submit set resultcode=%d, resultstring='%s', runtime=%d, runmem=%d where id=%d", result->res, result_str[result->res], result->time, result->memory, sid);
    __TRACE_LN(__TRACE_KEY, "%s", update_sql);
    if(!mysql_query(db, update_sql))
    {
        __TRACE_LN(__TRACE_KEY, "update judge result failed.");
        return -1;
    }
    return 0;
}

int db_close(MYSQL **db)
{
    mysql_close(*db);
    return 0;
}
