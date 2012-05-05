/*
* sanbox for static language(C/C++)
*/
#include "sandbox.h"
#include "platform.h"
#include "restrict_syscall.h"
#include "../../config/config.h"
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/wait.h>
#include "../../trace/trace.h"
#include "symbols.h"

#define SIGML SIGUSR1

#define POST_EVENT(e, type, x...) (e) = (event_t){type, {x}};

void default_tracer_policy(const event_t *pe, action_t *pa);
static int _sandbox_excute(task_t *ptask);

int sandbox_init(sandbox_t *psbox, task_t *ptask)
{
	memset(psbox, 0, sizeof(sandbox_t));
	psbox->task = *ptask;
	psbox->policy = default_tracer_policy;
	psbox->result = PENDED;
	return 0;
}

int sandbox_excute(sandbox_t *psbox)
{
	pid_t child;
	child = fork();
	if(child < 0)
	{
		psbox->result = INTERNAL_ERROR;
		__TRACE_LN(__TRACE_KEY, "Error : fork target process failed");
		return -1;
	}
	else if(child == 0)
	{
		_exit(_sandbox_excute(&psbox->task));
	}
	else
	{
	    __TRACE_LN(__TRACE_DBG, "child pid : %d", child);
		pid_t wait_result = 0;
		int wait_status = 0;
		proc_t proc_data = { 0 };
		int in_syscall = 1;
		event_t e = { 0 };
		action_t action = { 0 };

        /* Have signals kill the prisoner but not self (if possible).  */
        /* In fact, I have deleted the '-' before "child" so this may not be necessary */
        sig_t terminate_signal;
        sig_t interrupt_signal;
        sig_t quit_signal;

        terminate_signal = signal(SIGTERM, SIG_IGN);
        interrupt_signal = signal(SIGINT, SIG_IGN);
        quit_signal = signal(SIGQUIT, SIG_IGN);

        /* Get wallclock start time */
        gettimeofday(&psbox->stat.start_timestamp, NULL);


        /* get initial resource usage */
		wait_result = wait4(child, &wait_status, 0, &psbox->stat.ru);
		memcpy(&psbox->stat.init_ru, &psbox->stat.ru, sizeof(struct rusage));
		/* trace loop */
		__TRACE_LN(__TRACE_DBG, "Enter trace loop");
		bool internal_error = false;
		do{
			if(!wait_result)	/* wait_result == 0 : noting happent */
			{
				goto cont;
			}
			memset(&e, 0, sizeof(event_t));
			memset(&action, 0, sizeof(action_t));
			if(WIFSTOPPED(wait_status))
			{
				switch(WSTOPSIG(wait_status))
				{
				case SIGALRM:
					POST_EVENT(e, EVENT_QUOTA, QUOTA_WALLCLOCK);
					break;
				case SIGXCPU:
				case SIGPROF:
				case SIGVTALRM:
					POST_EVENT(e, EVENT_QUOTA, QUOTA_CPUTIME);
					break;
				case SIGML:
					POST_EVENT(e, EVENT_QUOTA, QUOTA_MEMORY);
					break;
				case SIGXFSZ:
					POST_EVENT(e, EVENT_QUOTA, QUOTA_OUTPUT);
					break;
				case SIGTRAP:
					if(!proc_probe(child, &proc_data))
					{
						/* error */
						internal_error = true;
                        psbox->result = INTERNAL_ERROR;
						kill(child, SIGKILL);
						__TRACE_LN(__TRACE_KEY, "Internal Error : porc_probe() failed");
						break;
					}
					if(psbox->stat.vsize_peak < proc_data.vsize)
					{
						psbox->stat.vsize_peak = proc_data.vsize;
						if(psbox->stat.vsize_peak > psbox->task.quota[QUOTA_MEMORY])
						{
							kill(child, SIGML);
						}
					}
					/* system call or system call return */
					if(in_syscall)
					{
						in_syscall = 0;
						POST_EVENT(e, EVENT_SYSTEM_CALL_RETURN, SYSCALL_NO(&proc_data), SYSCALL_RETVAL(&proc_data));
					}
					else
					{
						in_syscall = 1;
						POST_EVENT(e, EVENT_SYSTEM_CALL, SYSCALL_NO(&proc_data),
									SYSCALL_ARG1(&proc_data), SYSCALL_ARG2(&proc_data),
									SYSCALL_ARG3(&proc_data), SYSCALL_ARG4(&proc_data),
																			SYSCALL_ARG5(&proc_data));
					}
					break;
				default: POST_EVENT(e, EVENT_SIGNAL, WSTOPSIG(wait_status));
				}
			}
			else if(WIFSIGNALED(wait_status))
			{
			    __TRACE_LN(__TRACE_DBG, "WIFSIGNALED");
				POST_EVENT(e, EVENT_SIGNAL, WTERMSIG(wait_status));
			}
			else if(WIFEXITED(wait_status))
			{
				psbox->stat.data.exitcode = WEXITSTATUS(wait_status);
				POST_EVENT(e, EVENT_EXIT, WEXITSTATUS(wait_status));
			}

			/* sink event */
			psbox->policy(&e, &action);
            __TRACE(__TRACE_DBG, "Event type : %s", event_name(e.type));
            __TRACE_LN(__TRACE_DBG, "\tEvent data : %ld %ld %ld %ld %ld %ld",
                                    e.data._bitmap_.a, e.data._bitmap_.b, e.data._bitmap_.c,
                                    e.data._bitmap_.d, e.data._bitmap_.e, e.data._bitmap_.f);
            __TRACE(__TRACE_DBG, "Action type : %s", action_name(action.type));
            __TRACE_LN(__TRACE_DBG, "\tAction data : %ld %ld", action.data._bitmap_.a,
                                                                                                                         action.data._bitmap_.b);
			/* action */
			switch(action.type)
			{
			case ACTION_CONTINUE:
				if(!trace_next(&proc_data))
				{
					internal_error = true;
					psbox->result = INTERNAL_ERROR;
					kill(child, SIGKILL);
					__TRACE_LN(__TRACE_KEY, "Internal Error : trace_next() failed");
					break;
				}
				goto cont;
			case ACTION_KILL:
				trace_kill(&proc_data, 0);
				psbox->result = action.data._kill.result;
				if(psbox->result == RUNTIME_ERROR)
				{
					psbox->stat.data.signo = action.data._kill.data;
				}
				else if(psbox->result == RESTRICTED_FUNCTION)
				{
					psbox->stat.data.rf = action.data._kill.data;
				}
				kill(child, SIGKILL);
				break;
			case ACTION_EXIT:
				psbox->stat.data.exitcode = action.data._exit.code;
				if(psbox->stat.data.exitcode)
				{
				    __TRACE_LN(__TRACE_DBG, "exit code : %ld", psbox->stat.data.exitcode);
					psbox->result = ABNORMAL_TERMINATION;
				}
				/*
				else
				{
				    psbox->result = PENDED;
				}
				*/
				break;
			}
			break;
cont:
            /* trace infomation */
            __TRACE_LN(__TRACE_DBG, "----------------------wait4()----------------------");
		}while(!internal_error && (wait_result = wait4(child, &wait_status, 0, &psbox->stat.ru)) >= 0);
		/* if the sandbox's result is PENDED, the prisoned process is exited nomally */
        /* Get wallclock stop time (call a second time to compensate overhead) */
        gettimeofday(&psbox->stat.end_timestamp, NULL);
        /* Restore signal handlers */
        signal(SIGTERM, interrupt_signal);
        signal(SIGINT, interrupt_signal);
        signal(SIGQUIT, quit_signal);
	}
	return 0;
}

static int _sandbox_excute(task_t *ptask)
{
	/* close the fds */
	/* redirect the input, output and error output */
	if(dup2(ptask->ifd, STDIN_FILENO) < 0)
	{
	    __TRACE_LN(__TRACE_KEY, "Internal Error : redirect stdin failed");
		return -1;
	}
	if(dup2(ptask->ofd, STDOUT_FILENO) < 0)
	{
	    __TRACE_LN(__TRACE_KEY, "Internal Error : redirect stdout failed");
		return -1;
	}
	if(dup2(ptask->efd, STDERR_FILENO) < 0)
	{
	    __TRACE_LN(__TRACE_KEY, "Internal Error : redirect stderr output failed");
		return -1;
	}
	/* Change identity before executing the targeted program */
	if (setgid(ptask->gid) < 0)
	{
	    __TRACE_LN(__TRACE_KEY, "Internal Error : setgid() failed");
        return -1;
	}
	if (setuid(ptask->uid) < 0)
	{
	    __TRACE_LN(__TRACE_KEY, "Internal Error : setuid() failed");
		return -1;
	}

	struct rlimit rlim;
	/* set limitations */
	/* core dump file */
	if(getrlimit(RLIMIT_CORE, &rlim) < 0)
	{
	    __TRACE_LN(__TRACE_KEY, "Internal Error : get RLIMIT_CORE limit failed");
		return -1;
	}
	rlim.rlim_cur = 0;
	if(setrlimit(RLIMIT_CORE, &rlim) < 0)
	{
	    __TRACE_LN(__TRACE_KEY, "Internal Error : set RLIMIT_CORE limit failed");
		return -1;
	}
	/* output quota */
	if(getrlimit(RLIMIT_FSIZE, &rlim) < 0)
	{
	    __TRACE_LN(__TRACE_KEY, "Internal Error : get RLIMIT_FSIZE limit failed");
		return -1;
	}
	rlim.rlim_cur = ptask->quota[QUOTA_OUTPUT];
	if(setrlimit(RLIMIT_FSIZE, &rlim) < 0)
	{
	    __TRACE_LN(__TRACE_KEY, "Internal Error : set RLIMIT_FSIZE limit failed");
		return -1;
	}
	/* memory quota */
	if(getrlimit(RLIMIT_AS, &rlim) < 0)
	{
	    __TRACE_LN(__TRACE_KEY, "Internal Error : get RLIMIT_AS limit failed");
		return -1;
	}
	/* NOTE :  when memory exceed, target process exited with 127, so double the  RLIMIT_AS limit
	 * I don't konow why yet.
	*/
	rlim.rlim_cur = ptask->quota[QUOTA_MEMORY] << 1;
	if(setrlimit(RLIMIT_AS, &rlim) < 0)
	{
	    __TRACE_LN(__TRACE_KEY, "Internal Error : set RLIMIT_AS limit failed");
		return -1;
	}
	/* time quota */
	struct itimerval timerval;
	/* wall clock quota */
	timerval.it_interval.tv_sec = 0;
	timerval.it_interval.tv_usec = 0;
	timerval.it_value.tv_sec = ptask->quota[QUOTA_WALLCLOCK] / 1000;
	timerval.it_value.tv_usec = (ptask->quota[QUOTA_WALLCLOCK] % 1000) * 1000;
	if(setitimer(ITIMER_REAL, &timerval, NULL) < 0)
	{
	    __TRACE_LN(__TRACE_KEY, "Internal Error : set ITIMER_REAL failed");
		return -1;
	}
	/* cpu quota */
	timerval.it_interval.tv_sec = 0;
	timerval.it_interval.tv_usec = 0;
	timerval.it_value.tv_sec = ptask->quota[QUOTA_CPUTIME] / 1000;
	timerval.it_value.tv_usec = (ptask->quota[QUOTA_CPUTIME] % 1000) * 1000;
	if(setitimer(ITIMER_PROF, &timerval, NULL) < 0)
	{
	    __TRACE_LN(__TRACE_KEY, "Internal Error : set ITIMER_PROF failed");
		return -1;
	}
	/* trace me! */
	if(!trace_self())
	{
	    __TRACE_LN(__TRACE_KEY, "Internal Error : trace_self() failed");
		return -1;
	}
	char *argv[ARG_MAX] = { NULL };
	argv[0] = ptask->exe;
	argv[1] = NULL;
	/* ok, let it run! */
	//__TRACE_LN(__TRACE_DBG, "run...");
	if(execve(ptask->exe_abspath, argv, NULL) < 0)
	{
	    __TRACE_LN(__TRACE_KEY, "Internal Error : execve() failed");
		return -1;
	}
	/* NOTE: never reached here */
	return 0;
}

int sandbox_fini(sandbox_t *psbox)
{
	memset(psbox, 0, sizeof(sandbox_t));
	return 0;
}

void default_tracer_policy(const event_t *pe, action_t *pa)
{
	switch(pe->type)
	{
	case EVENT_SYSTEM_CALL:
		if(!restrict_syscall[pe->data._sys_call.sc])
		{
			pa->type = ACTION_CONTINUE;
		}
		else
		{
			pa->type = ACTION_KILL;
			pa->data._kill.result = RESTRICTED_FUNCTION;
			pa->data._kill.data = pe->data._sys_call.sc;
		}
		break;
	case EVENT_SYSTEM_CALL_RETURN:
		pa->type = ACTION_CONTINUE;
		break;
	case EVENT_QUOTA:
		pa->type = ACTION_KILL;
		switch(pe->data._quota.type)
		{
        case QUOTA_WALLCLOCK:
        case QUOTA_CPUTIME:
            pa->data._kill.result = TIME_LIMIT_EXCEEDED;
            break;
        case QUOTA_MEMORY:
            pa->data._kill.result = MEMORY_LIMIT_EXCEEDED;
            break;
        case QUOTA_OUTPUT:
            pa->data._kill.result = OUTPUT_LIMIT_EXCEEDED;
            break;
        default:
            break;
		}
		break;
	case EVENT_SIGNAL:
		pa->type = ACTION_KILL;
		pa->data._kill.result = RUNTIME_ERROR;
		pa->data._kill.data = pe->data._signal.signal;
		break;
	case EVENT_EXIT:
		pa->type = ACTION_EXIT;
		pa->data._exit.code = pe->data._exit.code;
		break;
	}
}
