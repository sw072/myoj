#ifndef _COMM_
#define _COMM_

#define PATH_MAX 2048
#define FILENAME_MAX 512
#define ARG_MAX 32

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

/*
* Result of the solution
*/
typedef enum _result
{
	PENDED = 0,
	ACCEPTED,									/* checker result */
	PRESENTATION_ERROR,				/* checker result */
	COMPILER_ERROR,						/* complier result */
	WRONG_ANSWER,							/* checker result */
	MEMORY_LIMIT_EXCEEDED,		/* sandbox result */
	TIME_LIMIT_EXCEEDED,			/* sandbox result */
	OUTPUT_LIMIT_EXCEEDED,		/* sandbox result */
	RUNTIME_ERROR,						/* sandbox result */
	RESTRICTED_FUNCTION,			/* sandbox result */
	ABNORMAL_TERMINATION,			/* sandbox result */
	INTERNAL_ERROR,
	RESULT_NUM
} result_t;


#endif
