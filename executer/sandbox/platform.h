/*
* platform relative code (proc, ptrace)
*/

#ifndef _PLATFORM_
#define _PLATFORM_

#include <unistd.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/times.h>
#include "../../judgerd.h"

#define SYSCALL_NO(pproc) ((pproc)->regs.orig_eax)

#define SYSCALL_ARG1(pproc) ((pproc)->regs.ebx)
#define SYSCALL_ARG2(pproc) ((pproc)->regs.ecx)
#define SYSCALL_ARG3(pproc) ((pproc)->regs.edx)
#define SYSCALL_ARG4(pproc) ((pproc)->regs.esi)
#define SYSCALL_ARG5(pproc) ((pproc)->regs.edi)
#define SYSCALL_RETVAL(pproc) ((pproc)->regs.eax)

/**
 * @brief Structure for collecting process runtime information.
 */
typedef struct
{
    pid_t pid;                 /* process id */
    pid_t ppid;                /* parent process id */
    char state;                /* state of the process */
    struct tms tm;             /* process time measured in clock */
    unsigned long vsize;       /* virtual memory size (bytes) */
    struct user_regs_struct regs;
    struct user_fpregs_struct fpregs;
	bool is_in_syscall;			/* if now in syscall is_in_syscall = 1 otherwise is_in_syscall = 0 */
} proc_t;

bool proc_probe(pid_t pid, proc_t * const pproc);

bool trace_self(void);

bool trace_next(proc_t * const pproc);

bool trace_kill(const proc_t * const pproc, int signal);

#endif
