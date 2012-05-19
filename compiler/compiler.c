#include "compiler.h"
#include <unistd.h>
#include <sys/types.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>

extern compiler_t gcc;
extern compiler_t gpp;
extern compiler_t javac;

static int _save_src_to_file(char src[], char save_abspath[])
{
    int fd = 0;
    int srclen = strlen(src);
    if(!access(save_abspath, 0))     /* exsist */
    {
    }
    if((fd = open(save_abspath, O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0)
    {
        __TRACE_LN(__TRACE_KEY, "Internal Error : %s open failed(%s)", save_abspath, strerror(errno));
        return -1;
    }
    if(write(fd, src, srclen) != srclen)
    {
        __TRACE_LN(__TRACE_KEY, "Internal Error : write source code to %s failed", save_abspath);
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

int _run_compiler(compiler_t *pcompiler, char srcfile_dir_abspath[], char srcfile_name[],
                  char exefile_abspath[], char compileinfo_abspath[])
{
    pid_t pid = fork();
    if(pid < 0)
    {
        __TRACE_LN(__TRACE_KEY, "Internal Error : fork compiler process failed");
        return -1;
    }
    else if(pid == 0)
    {
        if(signal(SIGINT, SIG_IGN) == SIG_ERR)
        {
            __TRACE_LN(__TRACE_KEY, "Internal Error : set SIGINT IGN failed");
            return -1;
        }
        if(signal(SIGTSTP, SIG_IGN) == SIG_ERR)
        {
            __TRACE_LN(__TRACE_KEY, "Internal Error : set SIGTSTP IGN failed");
            return -1;
        }
        char cmd[CMD_MAX << 1];
        char *argv[ARG_MAX];
        int argc = 0;
        int space = 1;
        sprintf(cmd, pcompiler->compile_cmd_fmt, srcfile_name, exefile_abspath);
        __TRACE_LN(__TRACE_DBG, "DBG : compile cmd : %s", cmd);
        /* change compiler's work directory */
        if(chdir(srcfile_dir_abspath) < 0)
        {
            __TRACE_LN(__TRACE_KEY, "Internal Error : change compiler's work directory failed");
            return -1;
        }
        char *p = cmd;
        while(*p)
        {
            if(*p == ' ' && !space)
            {
                *p = '\0';
                space = 1;
            }
            else if(*p != ' ' && space)
            {
                argv[argc++] = p;
                space = 0;
            }
            p++;
        }
        argv[argc] = NULL;
        /* trace information */
        int idx = 0;
        while(argv[idx])
        {
            __TRACE_LN(__TRACE_DBG, "DBG : argv[%d] : %s", idx, argv[idx]);
            idx++;
        }

        int fd = 0;
        __TRACE_LN(__TRACE_DBG, "DBG : compile info file abspath : %s", compileinfo_abspath);
        if((fd = open(compileinfo_abspath, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0)
        {
            __TRACE_LN(__TRACE_KEY, "Internal Error : compile info file open failed");
            return -1;
        }
        /* dup2(fd, STDOUT_FILENO); */
        dup2(fd, STDERR_FILENO);
        if(execvp(argv[0], argv) < 0)
        {
            __TRACE_LN(__TRACE_KEY, "Internal Error : execve compiler process failed(%s)", strerror(errno));
            return -1;
        }
    }
    pid_t wait_pid = wait(NULL);
    if(wait_pid < 0)
    {
        __TRACE_LN(__TRACE_KEY, "Internal Error : wait compiler process error(%s)", strerror(errno));
        return -1;
    }
    /* check the exe file and compile error information */
    struct stat s;
    if(access(exefile_abspath, 0))
    {
        if(stat(compileinfo_abspath, &s))
        {
            __TRACE_LN(__TRACE_KEY, "Internal Error : stat() failed");
            return -1;
        }
        if(s.st_size > 0)
        {
            __TRACE_LN(__TRACE_DBG, "DBG : Compile error");
            return -2;      /* compile error */
        }
        __TRACE_LN(__TRACE_KEY, "Internal Error : compile failed(no exe file)");
        return -1;
    }
    return 0;
}

int compile(solution_t *ps, path_info_t *pinfo)
{
    assert(ps);
    int ret = 0;
    if(_save_src_to_file(ps->src, pinfo->srcfile_abspath) < 0)
    {
        __TRACE_LN(__TRACE_KEY, "Internal Error : save source code to file failed");
        return -1;
    }
    int compile_result = 0;
    char exe_dir[PATH_MAX];
    switch(ps->compiler)
    {
    case COMPILER_GCC :
        compile_result = _run_compiler(&gcc, pinfo->srcfile_dir_abspath, pinfo->srcfile_name,
                                                                                    pinfo->exefile_abspath, pinfo->compileinfo_abspath);
        break;
    case COMPILER_GPP:
        compile_result = _run_compiler(&gpp, pinfo->srcfile_dir_abspath, pinfo->srcfile_name,
                                                                                    pinfo->exefile_abspath, pinfo->compileinfo_abspath);
        break;
    case COMPILER_JAVAC:
        strcpy(exe_dir, pinfo->exefile_abspath);
        exe_dir[strlen(exe_dir) - strlen(pinfo->exefile_name)] = '\0';
        compile_result = _run_compiler(&javac, pinfo->srcfile_dir_abspath, pinfo->srcfile_name,
                                                                                    exe_dir, pinfo->compileinfo_abspath);
        break;
    default :
        __TRACE_LN(__TRACE_KEY, "Internal Error : unknow compiler.");
        return -1;
    }
    if(compile_result == -1)
    {
        __TRACE_LN(__TRACE_KEY, "Internal Error : run compiler failed");
        return -1;
    }
    else if(compile_result == -2)
    {
        __TRACE_LN(__TRACE_DBG, "DBG : compile error");
        return -2;
    }
    return 0;
}
