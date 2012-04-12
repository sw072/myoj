#include "db.h"
#include "../trace/trace.h"
#include <assert.h>

#define BUFF_MAX 8
static solution_t buff[BUFF_MAX];

int id = 1000;

int db_fetch_solutions(solution_t **pbuff, int *n)
{
    while(rand() % 2 != 0)
    {
        sleep(2);
    }
    buff[0].run_id = id;
    buff[0].problem_id = 1000;
    buff[0].compiler = COMPILER_GCC;
    buff[0].quota_wallclock = 5000;
    buff[0].quota_cputime = 1000;
    buff[0].quota_memory = 32 * 1024 * 1024;
    buff[0].quota_output = 32 * 1024 * 1024;
    strcpy(buff[0].src, "#include <stdio.h>\nint main() { printf(\"hello world!\\n\"); return 0; }");
    id++;
    buff[1].run_id = id;
    buff[1].problem_id = 1000;
    buff[1].compiler = COMPILER_GCC;
    buff[1].quota_wallclock = 5000;
    buff[1].quota_cputime = 1000;
    buff[1].quota_memory = 32 * 1024 * 1024;
    buff[1].quota_output = 32 * 1024 * 1024;
    strcpy(buff[1].src, "#include <stdio.h>\nint main() { printf(\"hello world!\\n\"); return 0; }fdsfsdf");
    id++;
    *pbuff = &buff[0];
    *n = 2;
    __TRACE_LN(__TRACE_DBG, "%d", id);
    return 0;
}
