#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <assert.h>
#include <string.h>
#include <wait.h>
#include "executer.h"
#include "../trace/trace.h"

int java_execute(int run_id, int problem_id, path_info_t *pinfo,
								int wallclock/* in ms */, int cputime/* in ms */,
								int memory/* in byte */, int disksize/* in byte */,
								config_t *pconfig, result_t *sandbox_result)
{
	pid_t wait_result = 0;
	int status = 0;
	char java_class_path[PATH_MAX];
	sprintf(java_class_path, "%s%s", pconfig->tmp_dir_path, pconfig->exe_dir_name);

    pid_t jvm_pid = fork();
    if(jvm_pid < 0)
    {
        __TRACE_LN(__TRACE_KEY, "Internal Error : create jvm process failed.");
        return -1;
    }
    else if(jvm_pid == 0)
    {
        /* run jvm */
        char cmd[CMD_MAX];
        char *argv[ARG_MAX];
        chdir(java_class_path);
        sprintf(cmd, "java -Xms%dk -Xmx%dk -classpath %s -Djava.library.path=%s -jar %s %d %d %d %d %d %d %s %s %s",
                memory / 1024 / 2,
                memory / 1024,
                java_class_path,
                pconfig->javalib_dir_abspath,
                pconfig->javasandbox_abspath,
                run_id,
                cputime,
                memory / 1024,
                disksize / 1024,
                getuid(),
                getgid(),
                pinfo->infile_abspath,
                pinfo->tmpout_abspath,
                pinfo->exefile_abspath);
            __TRACE_LN(__TRACE_DBG, "%s", cmd);
        int idx = 0, cnt = 0;
        argv[cnt++] = cmd;
        while(cmd[idx])
        {
            if(cmd[idx] == ' ')
            {
                cmd[idx] ='\0';
                if(cmd[idx + 1]) argv[cnt++] = &cmd[idx + 1];
            }
            idx++;
        }
        argv[cnt++] = NULL;
        /*
        idx = 0;
        while(argv[idx])
        {
            __TRACE_LN(__TRACE_DBG, "%s", argv[idx]);
            idx++;
        }
        */
        if(execvp(argv[0], argv) < 0)
        {
            __TRACE_LN(__TRACE_KEY, "Internal Error : run jvm failed.");
            return -1;
        }
    }
    wait_result = wait4(jvm_pid, &status, 0, NULL);
    if(WIFSIGNALED(status))
    {
        //信号终止的
        switch(WTERMSIG(status))
        {
            case SIGXCPU:
                *sandbox_result = TIME_LIMIT_EXCEEDED;
                break;
            default:
                __TRACE_LN(__TRACE_INFO,  "Java process was terminated by signal : %d", WTERMSIG(status));
                *sandbox_result = RUNTIME_ERROR;
        }
    }
    else
    {
        *sandbox_result = WEXITSTATUS(status);
        if(*sandbox_result  == 1)
        {
            //子进程返回1，表示出错了
            *sandbox_result = INTERNAL_ERROR;
        }
        /*
        else
        {
            if(this->_result)
                this->_result += 1000;
        }
        */
        if(*sandbox_result  == 0)
        {
           //子进程返回0，正常结束
            *sandbox_result  = PENDED;
        }
    }
    return 0;
}
