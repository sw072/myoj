#ifndef _CONFIG_
#define _CONFIG_
#include "../comm.h"

int config_init(config_t *pconfig);

int config_get_in_file_abspath(config_t *pconfig, int problem_id, char abspath[]);

int config_get_out_file_abspath(config_t *pconfig, int problem_id, char abspath[]);

int config_get_src_abspath(config_t *pconfig, char file_name[], char abspath[]);

int config_get_exe_abspath(config_t *pconfig, char file_name[], char abspath[]);

int config_get_tmp_out_file_abspath(config_t *pconfig, int run_id, char abspath[]);

int config_get_compileinfo_file_abspath(config_t *pconfig, int run_id, char abspath[]);

#endif
