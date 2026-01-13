/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * ez_error.c - _explain_
 *
 * Copyright (C) 2005 ezlibs.com, All Rights Reserved.
 *
 * $Id: ez_error.c 5901 2006-12-12 03:19:09Z wu_junjie $
 *
 *  Explain:
 *     -explain-
 *
 *  Update:
 *     2005-04-02 17:19:24 WuJunjie 10221 Create
 *		wjj 2005-04-02 15:16:48
 *			Create
 * 
 */
/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
#include	"ez_socket.h"

#include	<stdarg.h>		/* ANSI C header file */

#ifdef	HAVE_SYSLOG_H
  #ifndef PSOS
	#include	<syslog.h>		/* for syslog() */
  #endif
#endif

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

int		daemon_proc;		/* set nonzero by daemon_init() */

#if (defined(EZ_ERROR_FUNCTION) && (EZ_ERROR_FUNCTION != 0))

static void	ez_err_doit(int, int, const char *, va_list);

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

/* Nonfatal error related to system call
* Print message and return */

void
ez_err_ret(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	ez_err_doit(1, LOG_INFO, fmt, ap);
	va_end(ap);
	return;
}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/* Fatal error related to system call
* Print message and terminate */

void
ez_err_sys(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	ez_err_doit(1, LOG_ERR, fmt, ap);
	va_end(ap);
#if (defined (EXIT_WHEN_FUNC_ERROR) && (EXIT_WHEN_FUNC_ERROR != 0))
	exit(1);
#endif
}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/* Fatal error related to system call
* Print message, dump core, and terminate */

void
ez_err_dump(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	ez_err_doit(1, LOG_ERR, fmt, ap);
	va_end(ap);

#if (defined (EXIT_WHEN_FUNC_ERROR) && (EXIT_WHEN_FUNC_ERROR != 0))
	abort();		/* dump core and terminate */
	exit(1);		/* shouldn't get here */
#endif

}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/* Nonfatal error unrelated to system call
* Print message and return */

void
ez_err_msg(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	ez_err_doit(0, LOG_INFO, fmt, ap);
	va_end(ap);
	return;
}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/* Fatal error unrelated to system call
* Print message and terminate */

void
ez_err_quit(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	ez_err_doit(0, LOG_ERR, fmt, ap);
	va_end(ap);

#if (defined (EXIT_WHEN_FUNC_ERROR) && (EXIT_WHEN_FUNC_ERROR != 0))
	exit(1);
#endif
}

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/* Print message and return to caller
* Caller specifies "errnoflag" and "level" */

static void
ez_err_doit(int errnoflag, int level, const char *fmt, va_list ap)
{
#ifndef _DEBUG
	return; // do
#endif

	int		errno_save, n;
	char	buf[MAXLINE + 1];

	errno_save = ez_get_errno();		/* value caller might want printed */
#ifdef	HAVE_VSNPRINTF
	vsnprintf(buf, MAXLINE, fmt, ap);	/* safe */
#else
	vsprintf(buf, fmt, ap);				/* not safe */
#endif

	n = strlen(buf);
	if (errnoflag)
	{
		ez_snprintf(buf + n, MAXLINE - n, ": %s", strerror(errno_save));
	}
	
	strcat(buf, "\n");

	if (daemon_proc)
	{
		syslog(level, buf);
	}
	else
	{
		fflush(stdout);		/* in case stdout and stderr are the same */
		fputs(buf, stderr);
		fflush(stderr);
	}

	return;
}

#endif /* EZ_ERROR_FUNCTION */

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
