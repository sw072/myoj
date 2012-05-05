/*
* Static language(C/C++) excuter header file
*/

#ifndef _SANDBOX_
#define _SANDBOX_

#include "../../judgerd.h"
#include "../../config/config.h"
#include <sys/types.h>
#include <sys/resource.h>

typedef enum _quota
{
	QUOTA_WALLCLOCK = 0,
	QUOTA_CPUTIME,
	QUOTA_MEMORY,
	QUOTA_OUTPUT,
	QUOTA_NUM
}quota_type_t;

typedef enum _event_type
{
	EVENT_SYSTEM_CALL,
	EVENT_SYSTEM_CALL_RETURN,
	EVENT_QUOTA,
	EVENT_SIGNAL,
	EVENT_EXIT
}event_type_t;

typedef union _event_data
{
	struct
	{
		long a, b, c, d, e, f;
	}_bitmap_;
	struct
	{
		long sc;
		long arg1, arg2, arg3, arg4, arg5;
	}_sys_call;
	struct
	{
		long sc;
		long retval;
	}_sys_call_ret;
	struct
	{
		int type;
	}_quota;
	struct
	{
		int signal;
	}_signal;
	struct
	{
		int code;
	}_exit;
}event_data_t;

typedef struct _event
{
	event_type_t type;
	event_data_t data;
}event_t;

typedef enum _action_type
{
	ACTION_CONTINUE,
	ACTION_KILL,
	ACTION_EXIT
}action_type_t;

typedef union _action_data
{
	struct
	{
		long a, b;
	}_bitmap_;
	struct
	{
		long reserve;
	}_cont;
	struct 	/* QUOTA EXCEEDED or RUNTIME ERROR or RESTRICT FUNCTION or IE */
	{
		int result;
		long data;	/* signal no. [ or system call no. ] */
	}_kill;
	struct
	{
		int code;
	}_exit;
}action_data_t;

typedef struct _action
{
	action_type_t type;
	action_data_t data;
}action_t;

typedef struct _task
{
	char exe[FILENAME_MAX];	/* the prisoned execute file name */
	char exe_abspath[PATH_MAX + FILENAME_MAX];
	uid_t uid;
	gid_t gid;
	int ifd;
	int ofd;
	int efd;
	rlim_t quota[QUOTA_NUM];
	/* config_t *pconfig; */
}task_t;

typedef struct _stat
{
	struct timeval start_timestamp;		/* start timestamp */
	struct timeval end_timestamp;			/* end timestamp */
	struct rusage ru;							        		/* time */
	struct rusage init_ru;                                /* init rusage */
	rlim_t vsize_peak;								        /* vsize peak */
	union
	{
		int signo;												/* signal no. */
		int rf;														/* restricted system call no. */
		int exitcode;											/* exit code ( exitcode != 0 if abnomal terminate ) */
	} data;
}stat_t;

typedef void (*policy_t)(const event_t *, action_t *);

typedef struct _sandbox
{
	task_t task;
	stat_t stat;
	result_t result;
	policy_t policy;
}sandbox_t;

int sandbox_init(sandbox_t *psbox, task_t *ptask);

int sandbox_excute(sandbox_t *psbox);

int sandbox_fini(sandbox_t *psbox);

#endif
