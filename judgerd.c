#include "comm.h"
#include "executer/executer.h"
#include "trace/trace.h"
#include "judge_queue/judge_queue.h"
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <assert.h>
#include "db/db.h"
#include "checker/checker.h"

int working = 1;

compiler_t gcc;
compiler_t gpp;
compiler_t javac;
extern char *result_str[];
extern char *srcfile_ext[];
typedef void sigfunc(int);

int get_solution(queue_t *q, solution_t *s);
void * thread_db_fetch(void * arg);
int start_db_fetch(MYSQL *db, queue_t *q, pthread_t *ptid);
int set_path_info(path_info_t *pinfo, solution_t *ps, config_t *pconfig);
int compilers_init(config_t *pconfig);
int clear_tmp_files(path_info_t *pinfo);
sigfunc * _signal(int signo, sigfunc *func);
static void quit(int sig);

typedef  struct thread_arg
{
    MYSQL * db;
    queue_t * q;
}thread_arg_t;
thread_arg_t arg;

int main(int argc, char *argv[])
{
    __TRACE_INIT("judger.log");
    // config data initial
	config_t config;
	if(config_init(&config) < 0)
	{
	    __TRACE_LN(__TRACE_KEY, "oops : Load config file failed");
	    return -1;
	}
	// compilers initial
    if(compilers_init(&config) < 0)
    {
        __TRACE_LN(__TRACE_KEY, "oops : Compilers init failed");
        return -1;
    }
    // pending queue initial
    queue_t q;
    if(queue_init(&q) < 0)
    {
        __TRACE_LN(__TRACE_KEY, "oops : Queue initial failed");
        return -1;
    }
    // start pending db fetch thread
    pthread_t tid = 0;
    MYSQL *ojdb = NULL;
    if(db_open(&ojdb, "10.3.16.157", "myoj1", "root", "101452"))
    {
        __TRACE_LN(__TRACE_KEY, "oops : Database connect failed.");
        return -1;
    }
    if(start_db_fetch(ojdb, &q, &tid) < 0)
    {
        __TRACE_LN(__TRACE_KEY, "oops : start_db_fetch() failed");
        return -1;
    }
    __TRACE_LN(__TRACE_KEY, "LOG : database fetch thread start working...(Thread %uld)", tid);

    solution_t s;
    path_info_t path_info;
    //int internal_error = 0;
    if(_signal(SIGINT, quit) == SIG_ERR)
    {
        __TRACE_LN(__TRACE_KEY, "oops : set SIGINT handler failed");
        return -1;
    }
    if(_signal(SIGTSTP, quit) == SIG_ERR)
    {
        __TRACE_LN(__TRACE_KEY, "oops : set SIGTSTP handler failed");
        return -1;
    }
    while(working)
    {
        memset(&s, 0, sizeof(solution_t));
        if(get_solution(&q, &s) < 0)
        {
            __TRACE_LN(__TRACE_KEY, "oops : get the first solution failed");
            return -1;
        }
        __TRACE_LN(__TRACE_KEY, "LOG : Judge Start-----------------------Run id %d\tProblem id %d", s.run_id, s.problem_id);

        set_path_info(&path_info, &s, &config);
        /* trace information */
        __TRACE_LN(__TRACE_DBG, "DBG : src file name : %s", path_info.srcfile_name);
        __TRACE_LN(__TRACE_DBG, "DBG : src file abspath : %s", path_info.srcfile_abspath);
        __TRACE_LN(__TRACE_DBG, "DBG : exe file name : %s", path_info.exefile_name);
        __TRACE_LN(__TRACE_DBG, "DBG : exe file abspath : %s", path_info.exefile_abspath);
        __TRACE_LN(__TRACE_DBG, "DBG : in data file abspath : %s", path_info.infile_abspath);
        __TRACE_LN(__TRACE_DBG, "DBG : tmpout file abspath : %s", path_info.tmpout_abspath);
        __TRACE_LN(__TRACE_DBG, "DBG : standout file abspath : %s", path_info.ansfile_abspath);

        //result_t result = PENDED;
        /* compile */
        int compile_result = 0;
        judge_result_t result = { PENDED, -1, -1};
        compile_result = compile(&s, &path_info);
        if(compile_result == -1)
        {
            __TRACE_LN(__TRACE_KEY, "Internal Error : compiler exception");
            result.res = INTERNAL_ERROR;
            goto next;
        }
        else if(compile_result == -2)
        {
            __TRACE_LN(__TRACE_DBG, "DBG : Compile error");
            result.res = COMPILE_ERROR;
            char line[4096];
            FILE *fp = fopen(path_info.compileinfo_abspath, "r");
            while(fgets(line, 4096, fp))
            {
                printf("%s", line);
            }
            fclose(fp);
            goto next;
        }
        /* excute the target process */
        if(s.compiler == COMPILER_GCC || s.compiler == COMPILER_GPP)
        {
            /* execute */
            if(normal_execute(s.run_id, s.problem_id,  &path_info, s.quota_wallclock, s.quota_cputime,
                                                            s.quota_memory, s.quota_output, &config, &result) < 0)
            {
                __TRACE_LN(__TRACE_KEY, "Internal Error : normal excuter has errors");
                result.res = INTERNAL_ERROR;
                /* return -1; */
            }
        }
        else if(s.compiler == COMPILER_JAVAC)
        {
            /* execute */
            if(java_execute(s.run_id, s.problem_id,  &path_info, s.quota_wallclock, s.quota_cputime,
                                                            s.quota_memory, s.quota_output, &config, &result) < 0)
            {
                __TRACE_LN(__TRACE_KEY, "Internal Error : java excuter has errors");
                result.res = INTERNAL_ERROR;
                /* return -1; */
            }
        }
        if(result.res != PENDED) goto next;
        /* checker output */
        if(check(path_info.tmpout_abspath, path_info.ansfile_abspath, &result) < 0)
        {
            __TRACE_LN(__TRACE_KEY, "Internal Error : check output failed");
            result.res = INTERNAL_ERROR;
        }
next:
        /* clear tmp files
        *           1) source file; 2) exe file; 3) tmpout file; 4)compile info file
        * if remove() failed, the judger runs continually
        */

        if(clear_tmp_files(&path_info) < 0)
        {
            __TRACE_LN(__TRACE_KEY, "WARNING : some tmp file not removed");
        }

        db_update_result(ojdb, s.run_id, &result);
        __TRACE_LN(__TRACE_KEY, "LOG : Judge Result : %s", result_str[result.res]);
        __TRACE_LN(__TRACE_KEY, "LOG : Judge End-----------------------Run id %d\tProblem id %d", s.run_id, s.problem_id);
    }
    pthread_cancel(tid);
    db_close(&ojdb);
    queue_fini(&q);
    __TRACE_LN(__TRACE_KEY, "Quit. Bye!");
    __TRACE_FINI();
	return 0;
}

int get_solution(queue_t *q, solution_t *s)
{
    if(queue_front_pop(q, s) < 0) return -1;
    return 0;
}

void * thread_db_fetch(void * arg)
{
    MYSQL *ojdb = ((thread_arg_t *)arg)->db;
    queue_t *q = ((thread_arg_t *)arg)->q;
    assert(ojdb);
    assert(q);
    __TRACE_LN(__TRACE_DBG, "ojdb:%p, q:%p", ojdb, q);
    int n = 0, idx = 0;
    solution_t *pbuff;
    while(working)
    {
        if(db_fetch_solutions(ojdb, &pbuff, &n) < 0)
        {
            __TRACE_LN(__TRACE_KEY, "oops : db operate failed");
            break;
        }
        if(!n) sleep(1);
        for(idx = 0; idx < n; idx++)
        {
            queue_enqueue(q, &pbuff[idx]);
        }
    }
    return NULL;
}

int start_db_fetch(MYSQL *db, queue_t *q, pthread_t *ptid)
{
    pthread_t tid = 0;
    arg.db = db;
    arg.q = q;
    if(pthread_create(&tid, NULL, thread_db_fetch, (void *)&arg) < 0)
    {
        __TRACE_LN(__TRACE_KEY, "oops : db fetch thread create failed");
        return -1;
    }
    *ptid = tid;
    return 0;
}

int set_path_info(path_info_t *pinfo, solution_t *ps, config_t *pconfig)
{
    if(ps->compiler == COMPILER_GCC || ps->compiler == COMPILER_GPP)
    {
        sprintf(pinfo->srcfile_name, "%d.%s", ps->run_id, srcfile_ext[ps->compiler]);
        config_get_src_abspath(pconfig, pinfo->srcfile_name, pinfo->srcfile_abspath);
        sprintf(pinfo->exefile_name, "%d", ps->run_id);
        config_get_exe_abspath(pconfig, pinfo->exefile_name, pinfo->exefile_abspath);
    }
    else if(ps->compiler == COMPILER_JAVAC)
    {
        sprintf(pinfo->srcfile_name, "Main.java");
        config_get_src_abspath(pconfig, pinfo->srcfile_name, pinfo->srcfile_abspath);
        sprintf(pinfo->exefile_name, "Main.class");
        config_get_exe_abspath(pconfig, pinfo->exefile_name, pinfo->exefile_abspath);
    }
    else
    {
        __TRACE_LN(__TRACE_KEY, "Internal Error : unknow compiler");
        return -1;
    }
    sprintf(pinfo->srcfile_dir_abspath, "%s%s/", pconfig->tmp_dir_path, pconfig->src_dir_name);     /* =.=! */
    config_get_in_file_abspath(pconfig, ps->problem_id, pinfo->infile_abspath);
    config_get_tmp_out_file_abspath(pconfig, ps->run_id, pinfo->tmpout_abspath);
    config_get_out_file_abspath(pconfig, ps->problem_id, pinfo->ansfile_abspath);
    config_get_compileinfo_file_abspath(pconfig, ps->run_id, pinfo->compileinfo_abspath);
    return 0;
}

int compilers_init(config_t *pconfig)
{
    assert(pconfig);
    gcc.compiler = COMPILER_GCC;
    strcpy(gcc.compile_cmd_fmt,
           "gcc %s -lm -W -Wunused -Wfloat-equal -Wformat -Wparentheses -Wswitch -Wsequence-point -O2 -static -o %s");
    gpp.compiler = COMPILER_GPP;
    strcpy(gpp.compile_cmd_fmt,
           "g++ %s -lm -W -Wunused -Wfloat-equal -Wformat -Wparentheses -Wswitch -Wsequence-point -O2 -static -o %s");
    javac.compiler = COMPILER_JAVAC;
    strcpy(javac.compile_cmd_fmt, "javac -nowarn %s -d %s");
    gcc.pconfig = gpp.pconfig = javac.pconfig = pconfig;
    return 0;
}

int clear_tmp_files(path_info_t *pinfo)
{
    int ret = 0;
    /* remove src file */
    if(!access(pinfo->srcfile_abspath, 0) &&
        remove(pinfo->srcfile_abspath) < 0)
    {
        __TRACE_LN(__TRACE_KEY, "WARNING : source file remove failed");
        ret = -1;
    }
    if(!access(pinfo->exefile_abspath, 0) &&
       remove(pinfo->exefile_abspath) < 0)
    {
        __TRACE_LN(__TRACE_KEY, "WARNING : exe file remove failed");
        ret = -1;
    }
    if(!access(pinfo->tmpout_abspath, 0) &&
       remove(pinfo->tmpout_abspath) < 0)
    {
        __TRACE_LN(__TRACE_KEY, "WARNING : tmpout file remove failed");
        ret = -1;
    }
    if(!access(pinfo->compileinfo_abspath, 0) &&
       remove(pinfo->compileinfo_abspath) < 0)
    {
        __TRACE_LN(__TRACE_KEY, "WARNING : compile info file remove failed");
        ret = -1;
    }

    return ret;
}

sigfunc * _signal(int signo, sigfunc *func)
{
    struct sigaction act, oact;
    act.sa_handler = func;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_flags |= SA_RESTART;
    if (sigaction(signo, &act, &oact) < 0)
        return(SIG_ERR);
    return(oact.sa_handler);
}

static void quit(int sig)
{
    working = 0;
    __TRACE_LN(__TRACE_DBG, "SIGINT");
}
