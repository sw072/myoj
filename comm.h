#ifndef _COMM_
#define _COMM_
#include "trace/trace.h"

/* SOURCE FILE MAX LENGTH */
#ifndef SRC_MAX
#define SRC_MAX 4096
#endif

#ifndef PATH_MAX
#define PATH_MAX 2048
#endif

#ifndef FILENAME_MAX
#define FILENAME_MAX 512
#endif

#ifndef ARG_MAX
#define ARG_MAX 32
#endif

#ifndef _HAS_BOOL_
#define _HAS_BOOL_
typedef enum _bool
{
	false = 0,
	true
}bool;
#endif

/*
* Status of the solution
*/
typedef enum _status
{
	PENDING,
	COMPILING,
	PREJUDGING,
	JUDGING,
	FINISH
}status_t;

typedef struct _config
{
	char tmp_dir_path[PATH_MAX];						    		/* tmp data dir */
	char src_dir_name[FILENAME_MAX];		             	/* src file dir name */
	char exe_dir_name[FILENAME_MAX];	        	    	/* exe file dir name */
	char tmp_out_dir_name[FILENAME_MAX];	            /* tmp output file dir name */
	char in_data_dir_path[PATH_MAX];				        		/* input data dir */
	char out_data_dir_path[PATH_MAX];			        		/* output data dir */
	char compileinfo_dir_name[FILENAME_MAX];    /* compile error */
    char javalib_dir_abspath[PATH_MAX];
    char javasandbox_abspath[PATH_MAX];
}config_t;
/* all dir_path end with '/' */
/* src path format : tmp_dir/src_dir_name/{runid.ext} */
/* exe path format : tmp_dir/exe_dir_name/{runid.ext} */
/* tmp output path format : tmp_dir/tmp_out_dir_name/{runid} */
/* input data file path format : in_data_dir/{problem_id}/{problem_id} [only one test file now]*/
/* stand output data file path format : out_data_dir/{problem_id}/{problem_id} */

#ifndef CMD_MAX
#define CMD_MAX 2048
#endif

typedef enum _compiler_type
{
    COMPILER_GCC,
    COMPILER_GPP,
    COMPILER_JAVAC,
    /*
    COMPILER_PASCAL,
    COMPILER_GCJ,
    */
} compiler_type_t;

typedef struct _compiler
{
    compiler_type_t compiler;
    char compile_cmd_fmt[CMD_MAX];
    config_t *pconfig;
} compiler_t;
/*
static const char *srcfile_ext[]={ "c", "cc", "java" };
*/
/*
* Result of the solution
*/
typedef enum _result
{
	PENDED = 0,
	ACCEPTED,									        /* checker result */
	PRESENTATION_ERROR,				    /* checker result */
	COMPILE_ERROR,						        /* complier result */
	WRONG_ANSWER,							/* checker result */
	MEMORY_LIMIT_EXCEEDED,		/* sandbox result */
	TIME_LIMIT_EXCEEDED,			    /* sandbox result */
	OUTPUT_LIMIT_EXCEEDED,		    /* sandbox result */
	RUNTIME_ERROR,						        /* sandbox result */
	RESTRICTED_FUNCTION,			    /* sandbox result */
	ABNORMAL_TERMINATION,		/* sandbox result */
	INTERNAL_ERROR,
	RESULT_NUM
} result_t;
/*
static const char *result_str[] = {
	"PENDED",
	"ACCEPTED",
	"PRESENTATION_ERROR",
	"COMPILE_ERROR",
	"WRONG_ANSWER",
	"MEMORY_LIMIT_EXCEEDED",
	"TIME_LIMIT_EXCEEDED",
	"OUTPUT_LIMIT_EXCEEDED",
	"RUNTIME_ERROR",
	"RESTRICTED_FUNCTION",
	"ABNORMAL_TERMINATION",
	"INTERNAL_ERROR"
};
*/
typedef  struct _solution
{
    int run_id;
    int problem_id;
    compiler_type_t compiler;
    char src[SRC_MAX];
    int quota_wallclock;
    int quota_cputime;
    int quota_memory;
    int quota_output;
} solution_t;

typedef struct _judge_result
{
    result_t res;
    int time;
    int memory;
}judge_result_t;

typedef struct _path_info
{
    char srcfile_name[FILENAME_MAX];
    char srcfile_dir_abspath[PATH_MAX];
    char srcfile_abspath[PATH_MAX];
    char exefile_name[FILENAME_MAX];
    char exefile_abspath[PATH_MAX];
    char infile_abspath[PATH_MAX];
    char tmpout_abspath[PATH_MAX];
    char ansfile_abspath[PATH_MAX];
    char compileinfo_abspath[PATH_MAX];
} path_info_t;

#endif
