#ifndef __TRACE_H__
#define __TRACE_H__

#include <stdio.h>

#define __TRACE_ALL

#define __TRACE_ECHO

#define __TRACE_DBG		0
#define __TRACE_INFO	1
#define __TRACE_KEY		2

//define __MIN_TRACE_LEVEL		__TRACE_DBG
#define __MIN_TRACE_LEVEL		__TRACE_INFO
//#define __MIN_TRACE_LEVEL		__TRACE_KEY

#ifdef __TRACE_ALL
#	define __TRACE_LN		trace_ln
#	define __TRACE			trace
#	define __TRACE_INIT		trace_init
#	define __TRACE_FINI		trace_fini
#else
#	define __TRACE_LN		__noop
#	define __TRACE			__noop
#	define __INIT_TRACE		__noop
#	define __FINI_TRACE		__noop
#endif

void trace_ln(int level, const char *format, ...);
void trace(int level, const char *format, ...);
void trace_init(const char *file_name);
void trace_fini();

#endif
