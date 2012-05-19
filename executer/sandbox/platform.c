#include "platform.h"

#include <assert.h>            /* assert() */
#include <fcntl.h>             /* open(), close(), O_RDONLY */
#include <stdio.h>             /* read(), sscanf(), sprintf() */
#include <string.h>            /* memset(), strsep() */
#include <sys/vfs.h>           /* statfs() */
#include <sys/types.h>         /* off_t */
#include <unistd.h>            /* access(), lseek(), {R,F}_OK */
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/syscall.h>

#ifdef __linux__

#ifndef PROCFS
#define PROCFS "/proc"
#endif

#ifdef PROCFS
#ifndef PROC_SUPER_MAGIC
#define PROC_SUPER_MAGIC 0x9fa0
#endif /* !defined PROC_SUPER_MAGIC */
#endif /* PROCFS */

#ifndef CACHE_SIZE
#define CACHE_SIZE (1 << 21)   /* assume 2MB cache */
#endif /* CACHE_SIZE */

static int cache[CACHE_SIZE / sizeof(int)];
volatile int sink;             /* variable used for clearing cache */

void flush_cache(void)
{
    unsigned int i;
    unsigned int sum = 0;
    for (i = 0; i < CACHE_SIZE / sizeof(int); i++) cache[i] = 3;
    for (i = 0; i < CACHE_SIZE / sizeof(int); i++) sum += cache[i];
    sink = sum;
}

bool proc_probe(pid_t pid, proc_t * const pproc)
{
    assert(pproc);

    /* Validating procfs */
    struct statfs sb;
    if ((statfs(PROCFS, &sb) < 0) || (sb.f_type != PROC_SUPER_MAGIC))
    {
		return false;
    }

    /* Validating process stat */
    char buffer[4096];

    sprintf(buffer, PROCFS "/%d/stat", pid);
    if (access(buffer, R_OK | F_OK) < 0)
    {
		return false;
    }

    /* Grab stat information in one read */
    int fd = open(buffer, O_RDONLY);
    int len = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);
    buffer[len] = '\0';

    /* Extract interested information */
    int offset = 0;
    char * token = buffer;
    do
    {
        switch (offset++)
        {
        case  0:           /* pid */
            sscanf(token, "%d", &pproc->pid);
            break;
        case  1:           /* comm */
            break;
        case  2:           /* state */
            sscanf(token, "%c", &pproc->state);
            break;
        case  3:           /* ppid */
            sscanf(token, "%d", &pproc->ppid);
            break;
        case  4:           /* pgrp */
        case  5:           /* session */
        case  6:           /* tty_nr */
        case  7:           /* tty_pgrp */
            break;
        case  8:           /* flags */
            break;
        case  9:           /* min_flt */
        case 10:           /* cmin_flt */
        case 11:           /* maj_flt */
        case 12:           /* cmaj_flt */
            break;
        case 13:           /* utime */
            sscanf(token, "%lu", &pproc->tm.tms_utime);
            break;
        case 14:           /* stime */
            sscanf(token, "%lu", &pproc->tm.tms_stime);
            break;
        case 15:           /* cutime */
            sscanf(token, "%ld", &pproc->tm.tms_cutime);
            break;
        case 16:           /* cstime */
            sscanf(token, "%ld", &pproc->tm.tms_cstime);
            break;
        case 17:           /* priority */
        case 18:           /* nice */
        case 19:           /* 0 */
        case 20:           /* it_real_value */
        case 21:           /* start_time */
            break;
        case 22:           /* vsize */
            sscanf(token, "%lu", &pproc->vsize);
            break;
        case 23:           /* rss */
        case 24:           /* rlim_rss */
        case 25:           /* start_code */
        case 26:           /* end_code */
        case 27:           /* start_stack */
        case 28:           /* esp */
        case 29:           /* eip */
        case 30:           /* pending_signal */
        case 31:           /* blocked_signal */
        case 32:           /* sigign */
        case 33:           /* sigcatch */
        case 34:           /* wchan */
        case 35:           /* nswap */
        case 36:           /* cnswap */
        case 37:           /* exit_signal */
        case 38:           /* processor */
        default:
            break;
        }
    } while (strsep(&token, " ") != NULL);

    /* Must be the parent process in order to probe registers and floating point
       registers; and the status of target process must be 'T' (aka traced) */
    if ((pproc->ppid != getpid()) || (toupper(pproc->state) != 'T'))
    {
		return false;
    }

    /* Inspect process registers */
    /* General purpose registers */
    if (ptrace(PTRACE_GETREGS, pid, NULL, (void *)&pproc->regs) < 0)
    {
		return false;
    }

	/* Inspect floating point registers */
    /* Floating point registers */
    if (ptrace(PTRACE_GETFPREGS, pid, NULL, (void *)&pproc->fpregs) < 0)
    {
		return false;
    }
	return true;
}

bool trace_self(void)
{
    bool res = false;
    res = (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == 0);
	return res;
}

bool trace_next(proc_t * const pproc)
{
    assert(pproc);
    bool res = false;
    res = (ptrace(PTRACE_SYSCALL, pproc->pid, NULL, NULL) == 0);
	return res;
}

bool trace_kill(const proc_t * const pproc, int signal)
{
    assert(pproc);
    ptrace(PTRACE_POKEUSER, pproc->pid, ORIG_EAX * sizeof(int), SYS_pause);
    kill(pproc->pid, signal);
	return true;
}

#else
#error "this platform not supported"
#endif
