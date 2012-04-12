#include "comm.h"
#include "excuter/normal_excuter.h"
#include "trace/trace.h"
#include "judge_queue/judge_queue.h"
#include <signal.h>
#include <pthread.h>
#include <assert.h>
#include "db/db.h"
#include "checker/checker.h"

int working = 1;

int get_solution(queue_t *q, solution_t *s)
{
    if(queue_front_pop(q, s) < 0) return -1;
    return 0;
}

void * thread_db_fetch(void * arg)
{
    queue_t *q = (queue_t *)arg;
    assert(q);
    int n = 0, idx = 0;
    solution_t *pbuff;
    while(1)
    {
        if(db_fetch_solutions(&pbuff, &n) < 0)
        {
            __TRACE_LN(__TRACE_KEY, "oops : db operate failed");
            break;
        }
        for(idx = 0; idx < n; idx++)
        {
            queue_enqueue(q, &pbuff[idx]);
        }
    }
    return NULL;
}

int start_db_fetch(queue_t *q, pthread_t *ptid)
{
    pthread_t tid = 0;
    if(pthread_create(&tid, NULL, thread_db_fetch, (void *)q) < 0)
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

static compiler_t gcc;
static compiler_t gpp;
static compiler_t javac;

int compilers_init(config_t *pconfig)
{
    assert(pconfig);
    gcc.compiler = COMPILER_GCC;
    strcpy(gcc.compile_cmd_fmt,
           "gcc %s -lm -W -Wunused -Wfloat-equal -Wformat -Wparentheses -Wswitch -Wsequence-point -O2 -static -o %s");
    gpp.compiler = COMPILER_GPP;
    strcpy(gpp.compile_cmd_fmt, "g++ %s -o %s");
    javac.compiler = COMPILER_JAVAC;
    strcpy(javac.compile_cmd_fmt, "...........");
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
    /*
    if(!access(pinfo->compileinfo_abspath, 0) &&
       remove(pinfo->compileinfo_abspath) < 0)
    {
        __TRACE_LN(__TRACE_KEY, "WARNING : compile info file remove failed");
        ret = -1;
    }
    */
    return ret;
}

typedef void sigfunc(int);

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

int main(int argc, char *argv[])
{
    __TRACE_INIT("judger.log");
	config_t config;
	if(config_init(&config) < 0)
	{
	    __TRACE_LN(__TRACE_KEY, "oops : Load config file failed");
	    return -1;
	}

    if(compilers_init(&config) < 0)
    {
        __TRACE_LN(__TRACE_KEY, "oops : Compilers init failed");
        return -1;
    }

    queue_t q;
    if(queue_init(&q) < 0)
    {
        __TRACE_LN(__TRACE_KEY, "oops : Queue initial failed");
        return -1;
    }

    pthread_t tid = 0;
    if(start_db_fetch(&q, &tid) < 0)
    {
        __TRACE_LN(__TRACE_KEY, "oops : start_db_fetch() failed");
        return -1;
    }
    __TRACE_LN(__TRACE_KEY, "LOG : database fetch thread start working...(Thread %uld)", tid);

    solution_t s;
    path_info_t path_info;
    int internal_error = 0;
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

        result_t result = PENDED;
        /* compile */
        int compile_result = 0;
        compiler_t *pcompiler = (s.compiler == COMPILER_GCC ? &gcc : (s.compiler == COMPILER_GPP ? &gpp : &javac));
        compile_result = compile(pcompiler, &s, &path_info);
        if(compile_result == -1)
        {
            __TRACE_LN(__TRACE_KEY, "Internal Error : compiler exception");
            result = INTERNAL_ERROR;
            goto next;
        }
        else if(compile_result == -2)
        {
            __TRACE_LN(__TRACE_DBG, "DBG : Compile error");
            result = COMPILE_ERROR;
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
            /* excute */
            if(normal_excute(s.run_id, s.problem_id,  &path_info, s.quota_wallclock, s.quota_cputime,
                                                            s.quota_memory, s.quota_output, &config, &result) < 0)
            {
                __TRACE_LN(__TRACE_KEY, "Internal Error : normal excuter has errors");
                result = INTERNAL_ERROR;
                /* return -1; */
            }
        }
        else if(s.compiler == COMPILER_JAVAC)
        {
        }
        if(result != PENDED) goto next;
        /* checker output */
        if(check(path_info.tmpout_abspath, path_info.ansfile_abspath, &result) < 0)
        {
            __TRACE_LN(__TRACE_KEY, "Internal Error : check output failed");
            result = INTERNAL_ERROR;
        }
next:
        /* clear tmp files
        *           1) source file; 2) exe file; 3) tmpout file; 4)compile info file
        * if remove() failed, the judger runs continually
        */
        /*
        if(clear_tmp_files(&path_info) < 0)
        {
            __TRACE_LN(__TRACE_KEY, "WARNING : some tmp file not removed");
        }
        */
        printf("result : %s\n", result_str[result]);
        __TRACE_LN(__TRACE_KEY, "LOG : Judge End-----------------------Run id %d\tProblem id %d", s.run_id, s.problem_id);
    }
    pthread_cancel(tid);
    queue_fini(&q);
    __TRACE_LN(__TRACE_KEY, "Quit. Bye!");
    __TRACE_FINI();
	return 0;
}
