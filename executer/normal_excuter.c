#include "executer.h"
#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <assert.h>
#include <string.h>
#include "../trace/trace.h"

int normal_execute(int run_id, int problem_id, path_info_t *pinfo,
								int wallclock/* in ms */, int cputime/* in ms */,
								int memory/* in byte */, int disksize/* in byte */,
								config_t *pconfig, result_t *sandbox_result)
{
	/* initial task */
	task_t task;
	int ifd, ofd, efd;
	memset(&task, 0, sizeof(task_t));
	/* set exe file name */
	strcpy(task.exe, pinfo->exefile_name);
	strcpy(task.exe_abspath, pinfo->exefile_abspath);
	/* check the exe file*/
	//...
	/* set uid, gid */
	task.uid = getuid();
	task.gid = getgid();
	/* set in, out, error out fds */
	if((ifd = open(pinfo->infile_abspath, O_RDONLY)) < 0)
	{
        __TRACE_LN(__TRACE_KEY, "Internal Error : Input data file(%s) open failed", pinfo->infile_abspath);
		return -1;
	}
	if((ofd = open(pinfo->tmpout_abspath, O_WRONLY | O_CREAT | O_TRUNC,
                S_IRUSR | S_IWRITE | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) < 0)
	{
	    close(ifd);
        __TRACE_LN(__TRACE_KEY, "Internal Error : Tmp output data file(%s) open failed", pinfo->tmpout_abspath);
		return -1;
	}
	if((efd = open("/dev/null", O_WRONLY)) < 0)
	{
	    close(ifd);
	    close(ofd);
        __TRACE_LN(__TRACE_KEY, "Internal Error : Error output file open failed");
		return -1;
	}
	task.ifd = ifd;
	task.ofd = ofd;
	task.efd = efd;
	/* set quota */
	task.quota[QUOTA_WALLCLOCK] = wallclock;
	task.quota[QUOTA_CPUTIME] = cputime;
	task.quota[QUOTA_MEMORY] = memory;
	task.quota[QUOTA_OUTPUT] = disksize;

	sandbox_t sbox;
	sandbox_init(&sbox, &task);
	__TRACE_LN(__TRACE_DBG, "DBG : sandbox init done");

	if(sandbox_excute(&sbox) < 0) *sandbox_result = INTERNAL_ERROR;
	else *sandbox_result = sbox.result;
	if(sandbox_result != PENDED)
	{
	    float t = sbox.stat.ru.ru_utime.tv_sec * 1000 + sbox.stat.ru.ru_utime.tv_usec / 1000.0;
        t += sbox.stat.ru.ru_stime.tv_sec * 1000 + sbox.stat.ru.ru_stime.tv_sec / 1000.0;
	    __TRACE_LN(__TRACE_DBG, "Time usage : %.2f(ms)", t);
	    __TRACE_LN(__TRACE_DBG, "Memory usage : %d(B)", sbox.stat.vsize_peak);
	}

	sandbox_fini(&sbox);
	close(ifd);
	close(ofd);
	close(efd);

	return 0;
}
