/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
/*
 * ez_config.h - _explain_
 *
 * Copyright (C) 2005 ezlibs.com, All Rights Reserved.
 *
 * $Id: ez_config.h 8293 2007-04-16 09:43:17Z wu_junjie $
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

#ifndef __EZ_CONFIG_H
#define __EZ_CONFIG_H

/*  programm exit if function error */
#define EXIT_WHEN_FUNC_ERROR 0

#if	defined (ghs) || defined (__GNUC__) || defined (__hpux) || defined (__sgi) || defined (__DECCXX) || defined (__KCC) || defined (__rational__) || defined (__USLC__) || defined (EZ_SOCKET)
#	define EZ_UNUSED_ARG(a) do {/* null */} while (&a == 0)
#else /* ghs || __GNUC__ || ..... */
#	define EZ_UNUSED_ARG(a) (a)
#endif /* ghs || __GNUC__ || ..... */

#if		defined(LINUX) || defined(UCLINUX) || defined(_ANDROID)
	#include "ez_config_linux.h"
#elif	defined(_CYGWIN)
	#include "ez_config_cygwin.h"
#elif	defined(WIN32)
	#include "ez_config_win32.h"
#elif	defined(PSOS)
	#include "ez_config_psos.h"
#elif	defined(MACOSX)
	#include "ez_config_osx.h"
#endif

#define	_D_LISTEN_DEEPTH		6

typedef enum __en_bool
{
	en_false = 0,
	en_true,
}en_bool;

/*-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
#endif /* __EZ_CONFIG_H */
