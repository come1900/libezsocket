/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * ez_stdlib.c - ez_socket wrapper
 *
 * Copyright (C) 2005 ezlibs.com, All Rights Reserved.
 *
 * $Id: ez_stdlib.c 5901 2006-12-12 03:19:09Z wu_junjie $
 *
 * ez_stdlib .
 *  Explain:
 *     -explain-
 *
 *  Update:
 *     2005-04-02 17:19:24 WuJunjie 10221 Create
 *		wjj 2005-04-02 15:16:48
 *			Create
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#include	"ez_socket.h"

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#include	<stdarg.h>		/* ANSI C header file */

#ifdef PSOS
#define ez_err_quit myprintf
#endif

int
ez_snprintf(char *buf, size_t size, const char *fmt, ...)
{
	size_t		n;
	va_list		ap;

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);	/* Sigh, some vsprintf's return ptr, not length */
	n = strlen(buf);
	va_end(ap);
	if (n >= size)
		//ez_err_quit("snprintf: '%s' overflowed array", fmt);
		ez_err_sys("snprintf: '%s' overflowed array", fmt);
	return(n);
}
