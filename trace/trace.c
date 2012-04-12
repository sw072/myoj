#include "trace.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

FILE *fp = NULL;

void trace_init(const char *file_name)
{
	if(!file_name) fp = stdout;
	else fp = fopen(file_name, "a+");
}

void trace_fini()
{
	if (fp != NULL)
		fclose(fp);
}

void trace_ln(int level, const char *format, ...)
{
	if (level < __MIN_TRACE_LEVEL) return;
	va_list args;
	if (fp != NULL)
	{
		va_start(args, format);
		vfprintf(fp, format, args);
		va_end(args);
	}
	if (fp != NULL)
		fprintf(fp, "\n");
	fflush(fp);

#ifdef __TRACE_ECHO
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
#endif
}

void trace(int level, const char *format, ...)
{
	if (level < __MIN_TRACE_LEVEL) return;
	va_list args;

	if (fp != NULL)
	{
		va_start(args, format);
		vfprintf(fp, format, args);
		va_end(args);
	}
#ifdef __TRACE_ECHO
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
#endif
}