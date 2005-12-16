#include	<stdio.h>
#include	<varargs.h>

void trace(va_alist)
va_dcl 		/* Ne vous affolez pas, il n'y a pas ";"!		*/
{
va_list args;
static FILE	*trace_fp = NULL;
char	*fmt;

if (trace_fp == NULL)
{
	if ((trace_fp = fopen("diana", "w")) == NULL)
	{
		abort();
	}
}

va_start(args);
fmt = va_arg( args, char *);
vfprintf( trace_fp, fmt, args);
fprintf( trace_fp, "\n");
va_end( args);
fflush(trace_fp);
}
