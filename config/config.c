#include "config.h"
#include "ini_op.h"
#include "../trace/trace.h"

#define CONFIG_FILE "config.ini"

int config_init(config_t *pconfig)
{
	int ret = 0;
	ini_file_t ini = NULL;
	if(load_ini_file(CONFIG_FILE, &ini) < 0)
	{
	    __TRACE_LN(__TRACE_KEY, "oops : Load ini file failed");
	    return -1;
	}
	if(!ret && get_profile_string(ini, "tmp_dir_path", pconfig->tmp_dir_path) < 0)
	{
	    __TRACE_LN(__TRACE_KEY, "oops : No section : tmp_dir_path");
	    ret = -1;
	}
	if(!ret && get_profile_string(ini, "src_dir_name", pconfig->src_dir_name) < 0)
	{
	    __TRACE_LN(__TRACE_KEY, "oops : No section : src_dir_name");
	    ret = -1;
	}
	if(!ret && get_profile_string(ini, "exe_dir_name", pconfig->exe_dir_name) < 0)
	{
	    __TRACE_LN(__TRACE_KEY, "oops : No section : exe_dir_name");
	    ret = -1;
	}
	if(!ret && get_profile_string(ini, "tmp_out_dir_name", pconfig->tmp_out_dir_name) < 0)
	{
	    __TRACE_LN(__TRACE_KEY, "oops : No section : tmp_out_dir_name");
	    ret = -1;
	}
	if(!ret && get_profile_string(ini, "in_data_dir_path", pconfig->in_data_dir_path) < 0)
	{
	    __TRACE_LN(__TRACE_KEY, "oops : No section : in_data_dir_path");
	    ret = -1;
	}
	if(!ret && get_profile_string(ini, "out_data_dir_path", pconfig->out_data_dir_path) < 0)
	{
	    __TRACE_LN(__TRACE_KEY, "oops : No section : out_data_dir_path");
	    ret = -1;
	}
	if(!ret && get_profile_string(ini, "compileinfo_dir_name", pconfig->compileinfo_dir_name) < 0)
	{
	    __TRACE_LN(__TRACE_KEY, "oops : No section : compileinfo_dir_name");
	    ret = -1;
	}
	if(!ret && get_profile_string(ini, "javalib_dir_path", pconfig->javalib_dir_abspath) < 0)
	{
	    __TRACE_LN(__TRACE_KEY, "oops : No section : javalib_dir_path");
	    ret = -1;
	}
	if(!ret && get_profile_string(ini, "javasandbox_path", pconfig->javasandbox_abspath) < 0)
	{
	    __TRACE_LN(__TRACE_KEY, "oops : No section : javasandbox_path");
	    ret = -1;
	}
	unload_ini_file(ini);
	/* trace infomation */
	__TRACE_LN(__TRACE_KEY, "Configuration:");
	__TRACE_LN(__TRACE_KEY, "\ttmp_dir_path = %s", pconfig->tmp_dir_path);
	__TRACE_LN(__TRACE_KEY, "\tsrc_dir_name = %s", pconfig->src_dir_name);
	__TRACE_LN(__TRACE_KEY, "\texe_dir_name = %s", pconfig->exe_dir_name);
	__TRACE_LN(__TRACE_KEY, "\ttmp_out_dir_name = %s", pconfig->tmp_out_dir_name);
	__TRACE_LN(__TRACE_KEY, "\tcompileinfo_dir_name = %s", pconfig->compileinfo_dir_name);
	__TRACE_LN(__TRACE_KEY, "\tin_data_dir_path = %s", pconfig->in_data_dir_path);
	__TRACE_LN(__TRACE_KEY, "\tout_data_dir_path = %s", pconfig->out_data_dir_path);
	__TRACE_LN(__TRACE_KEY, "\tjavalib_dir_path = %s", pconfig->javalib_dir_abspath);
	__TRACE_LN(__TRACE_KEY, "\tjavasandbox_path = %s", pconfig->javasandbox_abspath);
	return ret;
}

int config_get_in_file_abspath(config_t *pconfig, int problem_id, char abspath[])
{
	abspath[0] = '\0';
	sprintf(abspath, "%s%d/%d", pconfig->in_data_dir_path, problem_id, problem_id);
	return 0;
}

int config_get_out_file_abspath(config_t *pconfig, int problem_id, char abspath[])
{
	abspath[0] = '\0';
	sprintf(abspath, "%s%d/%d", pconfig->out_data_dir_path, problem_id, problem_id);
	return 0;
}

int config_get_src_abspath(config_t *pconfig, char file_name[], char abspath[])
{
	abspath[0] = '\0';
	sprintf(abspath, "%s%s/%s", pconfig->tmp_dir_path, pconfig->src_dir_name, file_name);
	return 0;
}

int config_get_exe_abspath(config_t *pconfig, char file_name[], char abspath[])
{
	abspath[0] = '\0';
	sprintf(abspath, "%s%s/%s", pconfig->tmp_dir_path, pconfig->exe_dir_name, file_name);
	return 0;
}

int config_get_tmp_out_file_abspath(config_t *pconfig, int run_id, char abspath[])
{
	abspath[0] = '\0';
	sprintf(abspath, "%s%s/%d", pconfig->tmp_dir_path, pconfig->tmp_out_dir_name, run_id);
	return 0;
}

int config_get_compileinfo_file_abspath(config_t *pconfig, int run_id, char abspath[])
{
	abspath[0] = '\0';
	sprintf(abspath, "%s%s/%d", pconfig->tmp_dir_path, pconfig->compileinfo_dir_name, run_id);
	return 0;
}
