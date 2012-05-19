#ifndef _EXCUTER_
#define _EXCUTER_

#include "../comm.h"
#include "sandbox/sandbox.h"

int normal_execute(int run_id, int problem_id, path_info_t *pinfo,
								int wallclock/* in ms */, int cputime/* in ms */,
								int memery/* in byte */, int disksize/* in byte */,
								config_t *pconfig, result_t *sandbox_result);

int java_execute(int run_id, int problem_id, path_info_t *pinfo,
								int wallclock/* in ms */, int cputime/* in ms */,
								int memery/* in byte */, int disksize/* in byte */,
								config_t *pconfig, result_t *sandbox_result);

#endif
